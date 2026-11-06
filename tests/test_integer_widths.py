"""Boundary tests for AVR display arithmetic and persisted byte-sized loop counts."""
from pathlib import Path
import re
import unittest
from target_fixture import run_fixture

ROOT = Path(__file__).resolve().parents[1]


def function(path, signature):
    text = (ROOT / path).read_text()
    start = text.index(signature)
    return text[start:text.index("\n}\n", start) + 3]


def fixture_source():
    fixture = (ROOT / "tests/integer_width_fixture.cpp").read_text()
    grid = "src/mcl/MCL/GUI/Pages/Grid/GridPage.cpp"
    text = (ROOT / grid).read_text()
    start = text.index("enum SlotDirtyMask :")
    fixture = fixture.replace("// SLOT_FLAGS", text[start:text.index("};", start) + 2])
    fixture = fixture.replace("// SLOT_UPDATE", function(grid, "bool apply_slot_dirty_fields("))
    page = "src/mcl/MCL/GUI/Pages/Sequencer/SeqExtStepPage.cpp"
    fixture = fixture.replace("// MINIMAP\n", function(page, "void SeqExtStepPage::draw_viewport_minimap()"))
    header = (ROOT / "src/mcl/MCL/GUI/Pages/Sequencer/SeqPage.h").read_text()
    width = re.search(r"static constexpr uint8_t pidx_w = \d+;", header).group(0)
    return fixture.replace("// MINIMAP_WIDTH", width)


class IntegerWidthTests(unittest.TestCase):
    def test_native(self):
        output = run_fixture(fixture_source(), include_dirs=[ROOT / "src/mcl"])
        self.assertIn("PASS: loop counts", output)
        self.assertIn("PASS: minimap", output)

    def test_avr(self):
        output = run_fixture(fixture_source(), avr=True, include_dirs=[ROOT / "src/mcl"])
        self.assertIn("PASS: loop counts", output, output)
        self.assertIn("PASS: minimap", output, output)


if __name__ == "__main__":
    unittest.main()
