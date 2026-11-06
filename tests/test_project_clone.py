"""Run the production row copier with native and AVR integer widths.

python3 -m unittest discover -s tests -p test_project_clone.py
The AVR case needs avr-g++ (PATH or PlatformIO) and qemu-system-avr.
"""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def fixture_source():
    source = (ROOT / "src/mcl/MCL/Project.cpp").read_text()
    start = source.index("bool copy_grid_row_slots_raw(")
    end = source.index("\n}\n", start) + 3
    memory = (ROOT / "src/mcl/MCL/MCLMemory.h").read_text()
    constants = "\n".join(re.findall(
        r"^constexpr size_t (?:GRID_WIDTH|GRID_SLOT_BYTES) = .*?;", memory, re.M))
    return (ROOT / "tests/project_clone_fixture.cpp").read_text().replace(
        "// GRID_CONSTANTS", constants).replace("// ROW_COPIER", source[start:end])


class ProjectCloneTests(unittest.TestCase):
    def test_native_row_copy(self):
        compiler = shutil.which("c++")
        if not compiler:
            self.skipTest("c++ unavailable")
        with tempfile.TemporaryDirectory(prefix="mcl-clone-native-") as folder:
            path = Path(folder)
            (path / "test.cpp").write_text(fixture_source())
            subprocess.run([compiler, "-std=c++11", "-O2", str(path / "test.cpp"),
                            "-o", str(path / "test")], check=True)
            result = subprocess.run([str(path / "test")], capture_output=True,
                                    text=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("PASS: complete rows and I/O failures", result.stdout)

    def test_avr_row_copy(self):
        compiler = shutil.which("avr-g++")
        bundled = Path.home() / ".platformio/packages/toolchain-atmelavr/bin/avr-g++"
        if not compiler and bundled.is_file():
            compiler = str(bundled)
        emulator = shutil.which("qemu-system-avr")
        if not compiler or not emulator:
            self.skipTest("avr-g++ and qemu-system-avr required")
        with tempfile.TemporaryDirectory(prefix="mcl-clone-avr-") as folder:
            path = Path(folder)
            (path / "test.cpp").write_text(fixture_source())
            subprocess.run([compiler, "-std=c++11", "-Os", "-mmcu=atmega2560",
                            str(path / "test.cpp"), "-o", str(path / "test.elf")],
                           check=True)
            # The fixture prints once and halts; QEMU has no AVR process-exit ABI.
            try:
                result = subprocess.run(
                    [emulator, "-M", "mega2560", "-display", "none", "-monitor",
                     "none", "-serial", "stdio", "-bios", str(path / "test.elf")],
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=5)
                output = result.stdout
            except subprocess.TimeoutExpired as error:
                output = error.stdout or b""
            self.assertIn(b"PASS: complete rows and I/O failures", output,
                          output.decode(errors="replace"))


if __name__ == "__main__":
    unittest.main()
