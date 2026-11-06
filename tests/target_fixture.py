"""Compile small production-code fixtures natively or on an emulated ATmega2560."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


def run_fixture(source, avr=False, include_dirs=()):
    compiler = shutil.which("avr-g++" if avr else "c++")
    if avr and not compiler:
        bundled = Path.home() / ".platformio/packages/toolchain-atmelavr/bin/avr-g++"
        if bundled.is_file():
            compiler = str(bundled)
    emulator = shutil.which("qemu-system-avr") if avr else None
    if not compiler or (avr and not emulator):
        raise unittest.SkipTest("avr-g++ and qemu-system-avr required" if avr
                                else "c++ unavailable")
    with tempfile.TemporaryDirectory(prefix="mcl-target-fixture-") as folder:
        path = Path(folder)
        (path / "test.cpp").write_text(source)
        program = path / "test.elf"
        command = [compiler, "-std=c++11", "-Os" if avr else "-O2"]
        if avr:
            command += ["-mmcu=atmega2560"]
        command += ["-I" + str(directory) for directory in include_dirs]
        subprocess.run(command + [str(path / "test.cpp"), "-o", str(program)], check=True)
        if not avr:
            result = subprocess.run([str(program)], stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, timeout=10)
            if result.returncode != 0:
                raise AssertionError(result.stdout.decode(errors="replace"))
            return result.stdout.decode()
        # AVR fixtures print a result and halt; there is no process-exit ABI.
        try:
            result = subprocess.run(
                [emulator, "-M", "mega2560", "-display", "none", "-monitor", "none",
                 "-serial", "stdio", "-bios", str(program)],
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=5)
            output = result.stdout
        except subprocess.TimeoutExpired as error:
            output = error.stdout or b""
        return output.decode(errors="replace")
