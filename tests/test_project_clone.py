"""Run the production row copier with native and AVR integer widths.

python3 -m unittest discover -s tests -p test_project_clone.py
The AVR case needs avr-g++ (PATH or PlatformIO) and qemu-system-avr.
"""
from pathlib import Path
import re
import unittest
from target_fixture import run_fixture

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
        self.assertIn("PASS: complete rows and I/O failures", run_fixture(fixture_source()))

    def test_avr_row_copy(self):
        output = run_fixture(fixture_source(), avr=True)
        self.assertIn("PASS: complete rows and I/O failures", output, output)


if __name__ == "__main__":
    unittest.main()
