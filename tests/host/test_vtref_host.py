import os, sys, unittest
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "tools"))
import vtref


class ParseTest(unittest.TestCase):
    def test_ok(self):
        self.assertEqual(vtref.parse_response(bytes([0x80, 0xE6, 0x0C, 2])), (3302, 2))

    def test_none(self):
        self.assertEqual(vtref.parse_response(bytes([0x80, 0, 0, 0])), (0, 0))

    def test_hex_text_with_openocd_noise(self):
        out = "Info : CMSIS-DAP: FW Version = 2.0.0\n80 08 07 01 00 00\n"
        self.assertEqual(vtref.extract_response(out), bytes.fromhex("8008070100 00".replace(" ", "")))
        self.assertEqual(vtref.parse_response(vtref.extract_response(out)), (1800, 1))

    def test_short_answer_means_not_supported(self):
        with self.assertRaises(vtref.NotSupported):
            vtref.parse_response(bytes([0x80]))

    def test_wrong_id(self):
        with self.assertRaises(vtref.NotSupported):
            vtref.parse_response(bytes([0x81, 0, 0, 0]))

    def test_no_hex_in_output(self):
        with self.assertRaises(vtref.NotSupported):
            vtref.extract_response("Error: unable to open CMSIS-DAP device\n")


class TextTest(unittest.TestCase):
    def test_verdicts(self):
        self.assertIn("напрямую", vtref.verdict(2))
        self.assertIn("сдвиг", vtref.verdict(1))
        self.assertIn("нет питания", vtref.verdict(0))
        self.assertIn("не подключать", vtref.verdict(3))

    def test_format_mv_comma(self):
        self.assertEqual(vtref.format_line(3302, 2), "3302 мВ (3,30 В): 3,3 В — напрямую")


if __name__ == "__main__":
    unittest.main()
