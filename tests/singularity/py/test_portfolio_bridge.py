import os
import sys
import unittest
from unittest.mock import patch
from pathlib import Path

_SRC = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(_SRC))
sys.path.insert(0, str(_SRC / "Singularity" / "Foreign" / "py"))

from src.Singularity.Foreign.py.bridge import CppBridge
from src.Singularity.Foreign.py.api.routes import (
    PORTFOLIO_ALLOWED_ORIGINS,
    _portfolio_origin_allowed,
)


class TestPortfolioBridgeProjection(unittest.TestCase):
    def test_projection_is_read_only_and_bounded(self):
        bridge = CppBridge()
        bridge.connected = True
        bridge.has_engine_snapshot = True
        bridge.current_state = {
            "timestamp": 123.0,
            "active_zone_index": 2,
            "active_zone_name": "Law Garden",
            "active_zone_id": "zone-law-garden",
            "objects": [
                {
                    "id": "obj-a",
                    "name": "Object A",
                    "type": "ConstructedBeing",
                    "position": [1.0, 2.0, 3.0],
                    "secret_internal_field": "must-not-leak",
                }
            ],
            "laws": [
                {
                    "identifier": "law-a",
                    "name": "Threshold Law",
                    "enabled": True,
                    "activation": 2,
                    "scope": 1,
                    "conditionDescription": "signal.level >= 0.6",
                    "actionDescription": "publish signal-awakened",
                    "private_runtime_state": "must-not-leak",
                }
            ],
        }
        bridge.recent_events.append({
            "type": "engine_event",
            "event": "law-applied",
            "private": "event payload is intentionally forwarded as emitted",
        })

        projection = bridge.get_portfolio_state()

        self.assertEqual(projection["schema"], "earthcall.portfolio.v1")
        self.assertTrue(projection["connected"])
        self.assertEqual(projection["active_zone"]["name"], "Law Garden")
        self.assertNotIn("secret_internal_field", projection["objects"][0])
        self.assertNotIn("private_runtime_state", projection["laws"][0])
        self.assertEqual(len(projection["recent_events"]), 1)

    def test_synthetic_state_is_not_exposed_as_live_engine_truth(self):
        bridge = CppBridge()
        bridge.connected = True
        # A connection without a real engine snapshot must not expose Studio fallback objects.
        projection = bridge.get_portfolio_state()
        self.assertFalse(projection["has_engine_snapshot"])
        self.assertEqual(projection["objects"], [])
        self.assertEqual(projection["laws"], [])
        self.assertEqual(projection["active_zone"]["name"], "")

    def test_default_origins_are_immutable(self):
        self.assertIsInstance(PORTFOLIO_ALLOWED_ORIGINS, frozenset)
        self.assertIn("https://zhangzachary834-commits.github.io", PORTFOLIO_ALLOWED_ORIGINS)

    def test_environment_origins_override_defaults(self):
        with patch.dict(os.environ, {"PORTFOLIO_ALLOWED_ORIGINS": "https://custom.example, http://localhost:4200"}):
            self.assertTrue(_portfolio_origin_allowed("https://custom.example"))
            self.assertTrue(_portfolio_origin_allowed("http://localhost:4200"))
            self.assertFalse(_portfolio_origin_allowed("https://zhangzachary834-commits.github.io"))
            self.assertFalse(_portfolio_origin_allowed("https://evil.example"))

    def test_public_portfolio_origin_is_explicitly_allowed(self):
        origin = "https://zhangzachary834-commits.github.io"
        self.assertIn(origin, PORTFOLIO_ALLOWED_ORIGINS)
        self.assertTrue(_portfolio_origin_allowed(origin))
        self.assertFalse(_portfolio_origin_allowed("https://evil.example"))


if __name__ == "__main__":
    unittest.main()
