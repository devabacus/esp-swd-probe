"""Тесты сторожа порта S3: исходники ветки зелёные, мутации краснят нужную проверку."""
import shutil
import tempfile
import unittest
from pathlib import Path

import check_s3_port as c

ROOT = Path(__file__).resolve().parent.parent
FILES = ["components/DAP/source/spi_switch.c", "components/DAP/config/DAP_config.h", "main/usb_dap.c"]


class S3Port(unittest.TestCase):
    def tree(self, **edits):
        d = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, d)
        for f in FILES:
            (d / f).parent.mkdir(parents=True, exist_ok=True)
            t = (ROOT / f).read_text(encoding="utf-8")
            for old, new in edits.get(f, []):
                self.assertIn(old, t, f"мутация не нашла место в {f}")
                t = t.replace(old, new)
            (d / f).write_text(t, encoding="utf-8")
        return d

    def test_current_tree_green(self):
        self.assertEqual(c.run(ROOT), [])

    def test_no_dedicated_clock(self):
        d = self.tree(**{FILES[0]: [("_dedic_gpio_ll_enable_bus_clock(true);", "")]})
        self.assertTrue(any(e.startswith("(а)") for e in c.run(d)))

    def test_clock_only_in_comment_is_red(self):
        d = self.tree(**{FILES[0]: [("_dedic_gpio_ll_enable_bus_clock(true);", "// _dedic_gpio_ll_enable_bus_clock(true);")]})
        self.assertTrue(any(e.startswith("(а)") for e in c.run(d)))

    def test_oen_enable_empty(self):
        d = self.tree(**{FILES[1]: [("gpio_ll_output_enable(&GPIO, PIN_SWDIO_MOSI);", "")]})
        errs = c.run(d)
        self.assertTrue(any("OUT_ENABLE" in e for e in errs))
        self.assertFalse(any("OUT_DISABLE" in e for e in errs))

    def test_oen_disable_empty(self):
        d = self.tree(**{FILES[1]: [("gpio_ll_output_disable(&GPIO, PIN_SWDIO_MOSI);", "")]})
        self.assertTrue(any("OUT_DISABLE" in e for e in c.run(d)))

    def test_packet_count_stream(self):
        d = self.tree(**{FILES[1]: [("#define DAP_PACKET_COUNT 1\n", "#define DAP_PACKET_COUNT 255\n")]})
        self.assertTrue(any(e.startswith("(в)") for e in c.run(d)))

    def test_packet_count_ok_with_split_marker(self):
        d = self.tree(**{FILES[1]: [("#define DAP_PACKET_COUNT 1\n", "#define DAP_PACKET_COUNT 255\n")],
                         FILES[2]: [("tud_vendor_read(", "/* S3_PORT_PER_TRANSFER_READ */ tud_vendor_read(")]})
        self.assertEqual(c.run(d), [])


if __name__ == "__main__":
    unittest.main()
