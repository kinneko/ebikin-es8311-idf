# original implementation by ebikin
# SPDX-License-Identifier: GPL-3.0-only
"""Host regression guard for the exact LifeLog codec register recipe (no hardware)."""
import pathlib
import re
import unittest

SOURCE = pathlib.Path(__file__).resolve().parents[1] / "components/ebikin_es8311/ebikin_es8311.c"
EXPECTED = [
    (0x00, 0x1f), (0x00, 0), (0x00, 0x80), (0x01, 0xbf),
    (0x02, 0x10), (0x03, 0x10), (0x04, 0x10), (0x05, 0),
    (0x06, 3), (0x07, 0), (0x08, 0xff), (0x09, 0x10),
    (0x0a, 0x10), (0x0d, 1), (0x0e, 2), (0x12, 0),
    (0x13, 0x10), (0x1c, 0x6a), (0x37, 8), (0x17, 0xff),
    (0x14, 0x1a), (0x16, 0), (0x31, 0x60),
]


class CodecSequence(unittest.TestCase):
    def test_exact_register_order_and_delay(self):
        text = SOURCE.read_text()
        body = text.split("esp_err_t ebikin_es8311_configure(", 1)[1].split(
            "esp_err_t ebikin_es8311_stop_mute(", 1
        )[0]
        self.assertIsNotNone(re.search(
            r"write_register\(codec, 0x00, 0x1f\);.*?vTaskDelay\(pdMS_TO_TICKS\(20\)\)",
            body, re.S
        ))
        table = body.split("settings[][2] = {", 1)[1].split("};", 1)[0]
        pairs = [(int(a, 0), int(b, 0)) for a, b in re.findall(
            r"\{\s*(0x[0-9a-f]+|\d+)\s*,\s*(0x[0-9a-f]+|\d+)\s*\}", table
        )]
        self.assertEqual([(0x00, 0x1f)] + pairs, EXPECTED)
        self.assertIn("if (err != ESP_OK) return err;", body)

    def test_no_application_or_legacy_bus_driver(self):
        text = SOURCE.read_text()
        self.assertNotIn("i2c_param_config", text)
        self.assertNotIn("i2c_driver_install", text)
        self.assertNotIn("i2cdev_done", text)
        self.assertNotIn("i2s_channel_", text)


if __name__ == "__main__":
    unittest.main()
