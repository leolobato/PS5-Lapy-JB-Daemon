import importlib.util
from pathlib import Path
import unittest

SPEC = importlib.util.spec_from_file_location(
    "owned_daemon_build", Path(__file__).resolve().parents[1] / "tools/build_owned_daemon.py")
build = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(build)


class OwnedDaemonTitleTests(unittest.TestCase):
    def test_accepts_explicit_native_playden_title(self):
        self.assertTrue(build.valid_target_title("FAKE77031"))
        self.assertTrue(build.valid_target_title("PPSA99995"))

    def test_rejects_partial_wildcard_or_unsafe_title(self):
        for title in ("FAKE*", "FAKE77031/", "FAKE7703", "FAKE770310", "fake77031",
                      "FAKE77\n31", "../77031", "", "PLDM00001"):
            self.assertFalse(build.valid_target_title(title), title)

    def test_service_wildcard_retains_existing_scope(self):
        self.assertTrue(build.valid_target_title("*"))


if __name__ == "__main__":
    unittest.main()
