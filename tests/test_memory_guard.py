import os
import pathlib
import subprocess
import tempfile
import unittest

GUARD = pathlib.Path(__file__).resolve().parents[1] / "targets/core/n16r8_memory_guard.h"
REQUIRED = (
    "CONFIG_IDF_TARGET_ESP32S3",
    "CONFIG_SPIRAM_MODE_OCT",
    "CONFIG_SPIRAM_ECC_ENABLE",
    "CONFIG_ESPTOOLPY_FLASHSIZE_16MB",
)


class MemoryProfileGuard(unittest.TestCase):
    def preprocess(self, disabled=None):
        with tempfile.TemporaryDirectory() as folder:
            pathlib.Path(folder, "sdkconfig.h").write_text(
                "".join(f"#define {name} {int(name != disabled)}\n" for name in REQUIRED)
            )
            return subprocess.run(
                [os.environ.get("CXX", "g++"), "-E", "-x", "c++", "-I", folder, str(GUARD)],
                capture_output=True,
                text=True,
                check=False,
            )

    def test_expected_module_profile_is_accepted(self):
        result = self.preprocess()
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_unsafe_memory_profiles_are_rejected(self):
        for disabled in REQUIRED:
            with self.subTest(disabled=disabled):
                self.assertNotEqual(self.preprocess(disabled).returncode, 0)


if __name__ == "__main__":
    unittest.main()
