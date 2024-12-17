open System
open System.IO
open System.Threading
open System.Diagnostics

type Context =
  {
    stream: byte list
    reg: bool list
    pending: byte list
    data: byte[]
    data_head: int
    data_len: int
    lookback: int
    max_seg: int
  }

let (>=>) (x: Result<Context, Context>) f =
  match x with
  | Ok ctx -> f ctx
  | Error ctx -> f ctx

let (>>=) (x: Result<'a, Context>) (f: 'a -> Result<'b, Context>) =
  match x with
  | Ok t -> f t
  | Error ctx -> Error ctx


let reg_full ctx =
  ctx.reg.Length = 8

let mutable _cnt = 0

// requires a full reg
let flush (ctx: Context) =
  if not (reg_full ctx) then ctx
  else

  _cnt <- _cnt + 1 + ctx.pending.Length
  (*printfn "flush, data head = %d" ctx.data_head*)
  let mutable b = 0uy
  let reg = List.rev ctx.reg
  for bit in reg do
    b <- (b <<< 1) ||| if bit then 1uy else 0uy
  (*printfn "shift   %02X"  b*)
  (*for p in List.rev ctx.pending do*)
    (*printfn "byte    %02X" p*)
  { ctx with reg = []; pending = []; stream = ctx.pending @ b :: ctx.stream }

let push_bit bit ctx =
  ctx
  |> flush
  |> fun ctx -> {ctx with reg = bit::ctx.reg; }
  |> Ok

let push_num num ctx =
  (*printfn "push_num %d" num*)
  let mutable bits = []
  let mutable num = num

  while num <> 0 do
    let bit = num &&& 1 <> 0
    bits <- bit :: bits
    num <- num >>> 1
  // skip the MSB (always 1)
  bits |> List.skip 1 |> List.fold (fun mctx b ->
    mctx
    >>= push_bit false
    >>= push_bit b) (Ok ctx)
  >>= push_bit true // push a stop bit

let push_plain ctx =
  if ctx.data_head >= ctx.data_len then ctx
  else
  push_bit true ctx >=> fun ctx ->
  { ctx with data_head = ctx.data_head + 1; pending = ctx.data.[ctx.data_head] :: ctx.pending }

let rec compare (data: byte[]) i j len =
  if len <= 0 then Some i
  elif data.[i] = data.[j] then compare data (i+1) (j+1) (len-1)
  else None

let rec strstr_rev (data: byte[]) head i len =
  if i < 0 then None
  else match compare data i head len with
       | Some x -> Some i
       | None -> strstr_rev data head (i-1) len

let commit_copy (log) (ctx, i, len) =
  // Shift idx, update lookback
  (*printfn "pattern [%d] %d %d" len i ctx.data_head*)
  let new_lookback = ctx.data_head - i
  if log then
    printfn "%08X <- %08X \t [%d]" ctx.data_head i len
  let ctx =
    if new_lookback = ctx.lookback then
      // no change, push "2"
      (*printfn "no change lookback push 2"*)
      push_num 2 ctx
    else
      let offset = new_lookback - 1 + 0x300
      let idx = offset / 0x100
      let adj = byte(offset - idx * 0x100)
      (*printfn "push lookback idx = %d adj = %d" idx adj*)
      push_num idx ctx
      >>= fun ctx ->
      // place adj in pending but don't alter the register
      Ok { ctx with pending = adj :: ctx.pending; lookback = new_lookback }
  ctx
// lookback len
>>= fun ctx ->
  let adj_len = len - 1 - (if new_lookback > 0xd00 then 1 else 0)
  let ctx =
    if adj_len < 4 then
      // 2bit encoding
      push_bit (adj_len &&& 0x2 <> 0) ctx
      >>= push_bit (adj_len &&& 0x1 <> 0)
    else
      // 2bit 0, then shift len
      // with 2nd 0-bit shared with initial len 0 prefix
      (*printfn "push 2 0s"*)
      push_bit false ctx
      >>= push_num (adj_len - 2)
  ctx
// Advance head
>>= fun ctx ->
  Ok { ctx with data_head = ctx.data_head + len; max_seg = max ctx.max_seg len }

let find_pattern (ctx: Context) (max_len) =
  if max_len < 2 then Error ctx
  else
  let max_len = min ctx.data_head (min (ctx.data_len - ctx.data_head) max_len)
  let rec find_impl l =
    if l > max_len then None
    else

    match strstr_rev ctx.data ctx.data_head (ctx.data_head - 1) l with
    | Some p ->
      match find_impl (l+1) with
      | Some q -> Some q
      | _ -> Some(p, l)
    | None -> None

  match find_impl 2 with
  | Some(i,j) -> Ok(ctx, i, j)
  | _ -> Error ctx

// A: Action.
// P: Longest Pattern Index
// L: Pattern Length
// C: Action Cost
let scan_patterns (A: _[]) (P: _[]) (L: _[]) (C: _[]) n ctx =
  printfn "Scanning for patterns..."

  let start_time        = DateTime.Now
  let mutable prev_time = start_time
  let mutable progress  = 0
  let mutable prev_progress = progress;


  [| 0 .. n-1 |] |>
  Array.Parallel.iter (fun i ->
    let ctx = {ctx with data_head = i}
    let p, l =
      match find_pattern ctx 2048 with
      | Ok (_, idx, len) -> (idx, len)
      | Error _ -> (0, 1)
    A.[i] <- i-1
    P.[i] <- p
    L.[i] <- l
    C.[i] <- 0.0

    let pg = Interlocked.Increment(&progress)
    let mutable avg_speed_inv = TimeSpan.FromSeconds(1.0) / 10000.0
    if pg % 10000 = 0 then
      let now            =  DateTime.Now
      let time_elapsed   =  now - prev_time
      let prog_made      =  pg - prev_progress
      let speed_inv      =  time_elapsed / float(prog_made)
      avg_speed_inv <- avg_speed_inv * 0.8 + speed_inv * 0.2
      let time_remain    =  avg_speed_inv * float(n - pg)
      prev_progress      <- pg
      prev_time          <- now
      Console.CursorLeft <- 0
      printf "%d bytes processed (%2f%%), time left = %O         " pg (float pg / float n * 100.0) time_remain
  )

  printfn ""
  printfn "Saving patterns."

  use fp = new StreamWriter("patterns.txt")
  fprintfn fp "%d" n
  for i = 0 to n-1 do
    fprintfn fp "%d" A.[i]
    fprintfn fp "%d" P.[i]
    fprintfn fp "%d" L.[i]
    fprintfn fp "%f" C.[i]

let load_patterns (A: _[]) (P: _[]) (L: _[]) (C: _[]) n =
  (*printfn "Loading patterns..."*)
  let lines =
    File.ReadAllLines("patterns.txt")
  let fsize = int lines.[0]
  if fsize <> n then failwith "file size mismatch"
  let mutable idx = 1
  for i = 0 to n-1 do
    A.[i] <- int lines.[idx]
    P.[i] <- int lines.[idx+1]
    L.[i] <- int lines.[idx+2]
    C.[i] <- float lines.[idx+3]
    idx <- idx + 4

let compress ((padj: float, pshift: int), plen: float) (ctx: Context) =

  _cnt <- 0

  let n = ctx.data_len
  let A = Array.create n 0
  let P = Array.create n 0
  let L = Array.create n 0
  let C = Array.create (n+1) 0.0

  if File.Exists("patterns.txt") then
    load_patterns A P L C n
  else
    scan_patterns A P L C n ctx

  for i = 0 to n-1 do
    A.[i] <- i + 1
    C.[i] <- 100000000.0

  C.[n] <- 0.0

  let shift_bits x =
    let x = max x 2.0
    (*printfn "sh (%f) = %f" x (floor(log(x + 0.2)/log(2.0) - 1.0) / 4.0)*)
    floor(log(x)/log(2.0)) / 4.0

  for i = n - 1 downto 0 do
    for j = 1 to L.[i] do
      let cost =
        if j = 1 then C.[i+j] + 1.125
        else
          // idx, 2bit len, len, adj
          let idx_cost = shift_bits(float(i - P.[i] + pshift) / 256.0)
          let adj_cost = padj
          let len_cost =
            if j < 4 then
              // only 2-bit
              0.25
            else
              // 2-bit + shift len
              0.25 + shift_bits(float j + plen)

          let total_cost = idx_cost + adj_cost + len_cost

          (*printfn "%08X %f %f %f" i idx_cost adj_cost len_cost*)

          C.[i+j] + total_cost
      if C.[i] > cost then
        C.[i] <- cost
        A.[i] <- i + j
    (*if i % 10000 = 0 then*)
      (*printfn "%08X %f %d (%f%%)" i C.[i] (n - i) (C.[i] / float(n-i) * 100.0)*)

  let steps = ResizeArray()

  let rec v i =
    if i < n then
      steps.Add((P.[i], A.[i] - i))
      v A.[i]

  v 0

  (*printfn "Compressing."*)
  let ctx =
    steps |>
    Seq.fold (fun c (idx, len) ->
      if len = 1 then
        push_plain c
      else
        commit_copy false (c, idx, len) >=> id
    ) ctx

  printfn "a = %f, s = %d, l = %f, L = %08X" padj pshift plen _cnt
  if ctx.data_head <> ctx.data_len then failwith "???"

  // insert stop condition, adj = 0xFF, idx = 0x01000002
  let stopped =
    {ctx with lookback = 0}
    |> push_num 0x01000002
    >=> (fun x -> {x with pending = 0xFFuy :: x.pending})
  let ins_0 = 9 - stopped.reg.Length
  printfn "inserting %d trailing bits to flush" ins_0
  let stopped = [1..ins_0] |> List.fold (fun x i -> push_bit true x >=> id) stopped

  { stopped with stream = List.rev stopped.stream; }


[<EntryPoint>]
let main argv =
  let proc = Process.GetCurrentProcess()
  proc.PriorityClass <- ProcessPriorityClass.BelowNormal
  printfn "Packing file: %s" argv.[0]
  let file = File.ReadAllBytes argv.[0]

  let ctx = { stream = []; reg = []; pending = []; data = file; data_head = 0; data_len = file.Length; lookback = 1; max_seg = 0 }
  let padj,pshift,plen = 
    if argv.Length < 3 then 1.2,0x300,0.0
    else Double.Parse(argv.[2]),Int32.Parse(argv.[3]),Double.Parse(argv.[4])

  let ctx =
    (*([| 1.2 .. 0.01 .. 1.2 |], [|0x300 .. 0x10 .. 0x300|])*)
    (*([| 1.2, 0x300 |], [| -0.15 .. 0.01 .. -0.05|])*)
    ([| padj, pshift |], [| plen |])
    ||> Array.allPairs
    |> Array.map compress
    |> Array.map (fun f -> f ctx)
    |> Array.minBy (fun ctx -> ctx.stream.Length)
  let outdata = Array.ofList ctx.stream

  File.WriteAllBytes(argv.[1], outdata)
  printfn "Compression complete."
  printfn "In file size = %08X" file.Length
  printfn "Out file size = %08X" outdata.Length
  printfn "Maximum segment length = %08X" ctx.max_seg

  0
