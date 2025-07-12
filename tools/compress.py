import sys
import os
import math
from typing import Union
from datetime import datetime, timedelta
import multiprocessing
from concurrent.futures import ProcessPoolExecutor, as_completed
import itertools

# Using a class for Context to mirror the F# record
class Context:
    """Holds the state of the compression process."""
    def __init__(self, stream, reg, pending, data, data_head, data_len, lookback, max_seg):
        self.stream = stream        # list[int]: The output byte stream (built in reverse)
        self.reg = reg              # list[bool]: A bit register (up to 8 bits, built in reverse)
        self.pending = pending      # list[int]: Bytes waiting for the register to flush (built in reverse)
        self.data = data            # bytes: The input data
        self.data_head = data_head  # int: Current position in the input data
        self.data_len = data_len    # int: Total length of the input data
        self.lookback = lookback    # int: The offset of the last matched pattern
        self.max_seg = max_seg      # int: Tracks the maximum length of a copied segment

    def replace(self, **kwargs):
        """Creates a new Context with updated attributes, similar to F#'s `with`."""
        attrs = self.__dict__.copy()
        attrs.update(kwargs)
        return Context(**attrs)

# Monadic-style Result class and operators to mimic F# for chaining operations
class Result:
    def __init__(self, is_ok, value):
        self.is_ok = is_ok
        self.value = value

def Ok(val):
    return Result(True, val)

def Error(val):
    return Result(False, val)

def bind(x, f):
    """Equivalent to F# >>= operator. Chains operations, propagating errors."""
    if x.is_ok:
        return f(x.value)
    else:
        return x

def pipe_through(x, f):
    """Equivalent to F# >=> operator. Applies f to the inner value, ignoring Ok/Error state."""
    return f(x.value)

def reg_full(ctx):
    return len(ctx.reg) == 8

_cnt = 0

def flush(ctx: Context) -> Context:
    """If the bit register is full, packs it into a byte and adds it to the stream."""
    if not reg_full(ctx):
        return ctx

    global _cnt
    _cnt += 1 + len(ctx.pending)
    
    b = 0
    # F# reverses the register before processing; self.reg is already reversed.
    for bit in reversed(ctx.reg):
        b = (b << 1) | (1 if bit else 0)

    # The F# code builds the stream backwards. We prepend to Python lists to mimic this.
    # new_stream = pending_list @ (b :: old_stream_list)
    new_stream = ctx.pending + [b] + ctx.stream
    
    return ctx.replace(reg=[], pending=[], stream=new_stream)

def push_bit(bit: bool, ctx: Context) -> Result:
    """Adds a single bit to the register, flushing if necessary."""
    ctx = flush(ctx)
    # Prepend to list to mimic F# `::` operator for reverse construction
    new_ctx = ctx.replace(reg=[bit] + ctx.reg)
    return Ok(new_ctx)

def push_num(num: int, ctx: Context) -> Result:
    """Encodes a number using a variable-length scheme."""
    bits = []
    if num > 0:
        temp_num = num
        while temp_num != 0:
            bit = (temp_num & 1) != 0
            bits.insert(0, bit)
            temp_num >>= 1
    
    # Skip the MSB (always 1 for this encoding)
    bits_to_push = bits[1:]
    
    mctx = Ok(ctx)
    for b in bits_to_push:
        mctx = bind(mctx, lambda current_ctx: push_bit(False, current_ctx))
        mctx = bind(mctx, lambda current_ctx: push_bit(b, current_ctx))
        
    mctx = bind(mctx, lambda current_ctx: push_bit(True, current_ctx)) # Push a stop bit
    return mctx

def push_plain(ctx: Context) -> Context:
    """Pushes a literal byte to the stream."""
    if ctx.data_head >= ctx.data_len:
        return ctx

    # F#: push_bit true ctx >=> fun ctx -> ...
    result = push_bit(True, ctx)
    
    def update_pending(current_ctx):
        new_head = current_ctx.data_head + 1
        # Prepend to list to mimic F# `::`
        new_pending = [current_ctx.data[current_ctx.data_head]] + current_ctx.pending
        return current_ctx.replace(data_head=new_head, pending=new_pending)

    return pipe_through(result, update_pending)

def compare(data: bytes, i: int, j: int, length: int) -> bool:
    if length <= 0:
        return True
    return data[i : i + length] == data[j : j + length]

def strstr_rev(data: bytes, head: int, i: int, length: int) -> Union[int, None]:
    """Searches backwards for a string."""
    while i >= 0:
        if compare(data, i, head, length):
            return i
        i -= 1
    return None

def commit_copy(log: bool, ctx: Context, i: int, length: int) -> Result:
    """Encodes a copy (pattern) operation."""
    new_lookback = ctx.data_head - i
    if log:
        print(f"{ctx.data_head:08X} <- {i:08X} \t [{length}]")
    
    if new_lookback == ctx.lookback:
        mctx = push_num(2, ctx)
    else:
        offset = new_lookback - 1 + 0x300
        idx = offset // 0x100
        adj = offset - idx * 0x100
        
        mctx = push_num(idx, ctx)
        
        def add_pending_adj(current_ctx):
             new_pending = [adj] + current_ctx.pending
             return Ok(current_ctx.replace(pending=new_pending, lookback=new_lookback))
        
        mctx = bind(mctx, add_pending_adj)

    def encode_len(current_ctx):
        adj_len = length - 1 - (1 if new_lookback > 0xd00 else 0)
        
        if adj_len < 4:
            mctx_len = push_bit((adj_len & 0x2) != 0, current_ctx)
            mctx_len = bind(mctx_len, lambda c: push_bit((adj_len & 0x1) != 0, c))
            return mctx_len
        else:
            mctx_len = push_bit(False, current_ctx)
            mctx_len = bind(mctx_len, lambda c: push_num(adj_len - 2, c))
            return mctx_len

    mctx = bind(mctx, encode_len)

    def advance_head(current_ctx):
        new_head = current_ctx.data_head + length
        new_max_seg = max(current_ctx.max_seg, length)
        return Ok(current_ctx.replace(data_head=new_head, max_seg=new_max_seg))

    mctx = bind(mctx, advance_head)
    return mctx

def find_pattern(ctx: Context, max_len: int) -> Result:
    """Finds the longest matching pattern for the data at the current head."""
    max_len = min(ctx.data_head, min(ctx.data_len - ctx.data_head, max_len))
    
    best_match = None
    l = 2
    while l <= max_len:
        p = strstr_rev(ctx.data, ctx.data_head, ctx.data_head - 1, l)
        if p is not None:
            best_match = (p, l)
            l += 1
        else:
            break
    
    if best_match:
        p, l = best_match
        return Ok((ctx, p, l))
    else:
        return Error(ctx)

# Worker function for parallel processing. Must be top-level for pickling.
def _scan_worker(data: bytes, i: int) -> tuple[int, int, int]:
    ctx = Context(None, None, None, data, i, len(data), None, None)
    result = find_pattern(ctx, 2048)
    if result.is_ok:
        _, p, l = result.value
        return (i, p, l)
    else:
        return (i, 0, 1)

def scan_patterns(A, P, L, C, n, ctx):
    """Pre-computes all possible patterns in parallel."""
    print("Scanning for patterns...")
    start_time = datetime.now()
    prev_time = start_time
    progress = 0
    prev_progress = 0
    avg_speed_inv = timedelta(seconds=1.0) / 10000.0

    with ProcessPoolExecutor() as executor:
        futures = {executor.submit(_scan_worker, ctx.data, i): i for i in range(n)}
        
        for future in as_completed(futures):
            i, p, l = future.result()
            A[i] = i - 1
            P[i] = p
            L[i] = l
            C[i] = 0.0
            progress += 1

            if progress % 10000 == 0:
                now = datetime.now()
                time_elapsed = now - prev_time
                prog_made = progress - prev_progress
                if prog_made > 0:
                    speed_inv = time_elapsed / float(prog_made)
                    avg_speed_inv = avg_speed_inv * 0.8 + speed_inv * 0.2
                    time_remain = avg_speed_inv * float(n - progress)
                    prev_progress = progress
                    prev_time = now
                    
                    progress_percent = float(progress) / float(n) * 100.0
                    print(f"\r{progress} bytes processed ({progress_percent:.2f}%), time left = {time_remain}         ", end="")
    
    print("\nSaving patterns.")
    with open("patterns.txt", "w") as fp:
        fp.write(f"{n}\n")
        for i in range(n):
            fp.write(f"{A[i]}\n{P[i]}\n{L[i]}\n{C[i]}\n")

def load_patterns(A, P, L, C, n):
    """Loads pre-computed patterns from a file."""
    print("Loading patterns...")
    with open("patterns.txt", "r") as fp:
        lines = [line.strip() for line in fp.readlines()]
    
    fsize = int(lines[0])
    if fsize != n:
        raise ValueError("File size mismatch in patterns.txt")
    
    idx = 1
    for i in range(n):
        A[i], P[i], L[i], C[i] = int(lines[idx]), int(lines[idx+1]), int(lines[idx+2]), float(lines[idx+3])
        idx += 4

def compress(params: tuple, initial_ctx: Context) -> Context:
    """Main compression function using dynamic programming."""
    (padj, pshift), plen = params
    global _cnt
    _cnt = 0

    n = initial_ctx.data_len
    A = [0] * n
    P = [0] * n
    L = [0] * n
    C = [0.0] * (n + 1)

    if os.path.exists("patterns.txt"):
        try:
            load_patterns(A, P, L, C, n)
        except (ValueError, IndexError) as e:
            print(f"patterns.txt is invalid ({e}). Rescanning...", file=sys.stderr)
            scan_patterns(A, P, L, C, n, initial_ctx)
    else:
        scan_patterns(A, P, L, C, n, initial_ctx)

    for i in range(n):
        A[i] = i + 1
        C[i] = 10_000_000.0
    C[n] = 0.0

    def shift_bits(x):
        x = max(x, 2.0)
        return math.floor(math.log2(x)) / 4.0

    print(f"Optimizing path with params: a={padj}, s={hex(pshift)}, l={plen}")
    for i in range(n - 1, -1, -1):
        # Cost of pushing a literal byte
        cost_plain = C[i + 1] + 1.125
        if C[i] > cost_plain:
            C[i] = cost_plain
            A[i] = i + 1

        # Cost of copying a pattern
        for j in range(2, L[i] + 1):
            if i + j > n: continue
            
            idx_cost = shift_bits(float(i - P[i] + pshift) / 256.0)
            adj_cost = padj
            
            if j < 4:
                len_cost = 0.25 # 2-bit encoding
            else:
                len_cost = 0.25 + shift_bits(float(j) + plen)
            
            total_cost = idx_cost + adj_cost + len_cost
            cost_copy = C[i + j] + total_cost

            if C[i] > cost_copy:
                C[i] = cost_copy
                A[i] = i + j

    steps = []
    curr = 0
    while curr < n:
        next_pos = A[curr]
        length = next_pos - curr
        steps.append((P[curr], length))
        curr = next_pos

    print("Compressing...")
    work_ctx = initial_ctx
    for idx, length in steps:
        if length == 1:
            work_ctx = push_plain(work_ctx)
        else:
            res = commit_copy(False, work_ctx, idx, length)
            # The `>=> id` in F# just unwraps the Result
            work_ctx = res.value

    print(f"a = {padj}, s = {hex(pshift)}, l = {plen}, L = {_cnt:08X}")
    if work_ctx.data_head != work_ctx.data_len:
        raise Exception("Compression did not consume all data.")

    # Insert stop condition
    stopped_ctx = work_ctx.replace(lookback=0)
    res = push_num(0x01000002, stopped_ctx)
    res = bind(res, lambda c: Ok(c.replace(pending=[0xFF] + c.pending)))
    
    # Insert trailing bits to flush the final byte
    def pad_and_flush(c):
        ins_0 = 9 - len(c.reg)
        mctx = Ok(c)
        for _ in range(ins_0):
            mctx = bind(mctx, lambda current_ctx: push_bit(True, current_ctx))
        # The last flush is implicit in the final `push_bit` calls
        return mctx.value

    final_ctx = pipe_through(res, pad_and_flush)
    
    # The stream was built backwards, so reverse it for the final output
    return final_ctx.replace(stream=final_ctx.stream[::-1])


def main():
    if len(sys.argv) < 3:
        print(f"Usage: python {sys.argv[0]} <infile> <outfile> [adj_cost] [shift_offset] [len_cost_adj]")
        sys.exit(1)

    try:
        import psutil
        psutil.Process().nice(psutil.BELOW_NORMAL_PRIORITY_CLASS)
    except (ImportError, AttributeError):
        try:
            os.nice(10)
        except AttributeError:
            print("Warning: Could not set process priority.", file=sys.stderr)

    infile, outfile = sys.argv[1], sys.argv[2]
    print(f"Packing file: {infile}")
    try:
        file_data = open(infile, "rb").read()
    except FileNotFoundError:
        print(f"Error: Input file not found: {infile}", file=sys.stderr)
        sys.exit(1)
        
    initial_ctx = Context(
        stream=[], reg=[], pending=[], data=file_data, 
        data_head=0, data_len=len(file_data), lookback=1, max_seg=0
    )

    if len(sys.argv) < 6:
        padj, pshift, plen = 1.2, 0x300, 0.0
    else:
        padj = float(sys.argv[3])
        pshift = int(sys.argv[4], 0) # Allow hex input e.g. 0x300
        plen = float(sys.argv[5])
    
    # The F# code was structured to allow searching over multiple parameters.
    # We run with the single specified (or default) set of parameters.
    param_sets = [((padj, pshift), plen)]
    results = [compress(params, initial_ctx) for params in param_sets]
    best_ctx = min(results, key=lambda ctx: len(ctx.stream))
    
    outdata = bytes(best_ctx.stream)
    with open(outfile, "wb") as f:
        f.write(outdata)
        
    print("Compression complete.")
    print(f"In file size  = {len(file_data):08X} ({len(file_data)})")
    print(f"Out file size = {len(outdata):08X} ({len(outdata)})")
    print(f"Maximum segment length = {best_ctx.max_seg:08X} ({best_ctx.max_seg})")

if __name__ == "__main__":
    # This is necessary for multiprocessing on some platforms (Windows, macOS)
    multiprocessing.freeze_support()
    main()
