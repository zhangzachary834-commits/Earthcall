"""
Witness Documentation:

OLD TEST PROVED:
    Key-filtering on a synthetic, manually assigned `current_state` dictionary
    assigned directly to CppBridge.

OLD TEST COULD NOT PROVE:
    1. Module import resolution: `routes.py` omitted top-level `PORTFOLIO_ALLOWED_ORIGINS`,
       causing `test_portfolio_bridge.py` to fail during unittest discovery.
    2. Real runtime state sync path: `CppBridge.get_portfolio_state()` gates world
       projection on `self.has_engine_snapshot = True`, which is set only when
       `CppBridge._process_message()` receives and parses an actual 'state_sync'
       WebSocket message from the C++ Engine. The old test manually assigned
       `current_state` without exercising `_process_message()`, leaving the
       real message processing path unverified.

REAL PATH TEST ADDED:
    `test_real_state_sync_message_processing_and_portfolio_projection()`
    passes a raw 'state_sync' JSON string message into `bridge._process_message()`,
    verifying that the real message handling pipeline sets `has_engine_snapshot = True`,
    updates `current_state`, and populates `get_portfolio_state()` cleanly.
"""

import json
import sys
import unittest
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

    def test_public_portfolio_origin_is_explicitly_allowed(self):
        origin = "https://zhangzachary834-commits.github.io"
        self.assertIn(origin, PORTFOLIO_ALLOWED_ORIGINS)
        self.assertTrue(_portfolio_origin_allowed(origin))
        self.assertFalse(_portfolio_origin_allowed("https://evil.example"))

    def test_real_state_sync_message_processing_and_portfolio_projection(self):
        bridge = CppBridge()
        bridge.connected = True

        raw_state_sync = json.dumps({
            "type": "state_sync",
            "timestamp": 456.0,
            "active_zone_index": 1,
            "active_zone_name": "Temple of Echoes",
            "active_zone_id": "zone-temple",
            "objects": [
                {
                    "id": "obj-b",
                    "name": "Echo Pedestal",
                    "type": "ConstructedBeing",
                    "position": [0.0, 1.0, 0.0],
                    "secret_internal_field": "must-not-leak",
                }
            ],
            "laws": [
                {
                    "identifier": "law-b",
                    "name": "Acoustic Reflection",
                    "enabled": True,
                    "private_runtime_state": "must-not-leak",
                }
            ],
        })

        # Process message via the exact runtime method CppBridge uses on receiving WebSocket frames
        self.assertFalse(bridge.has_engine_snapshot)
        self.assertEqual(bridge.get_portfolio_state()["objects"], [])
        bridge._process_message(raw_state_sync)

        self.assertTrue(bridge.has_engine_snapshot)
        self.assertEqual(bridge.messages_received, 1)
        self.assertEqual(bridge.current_state["active_zone_id"], "zone-temple")
        projection = bridge.get_portfolio_state()

        self.assertEqual(projection["schema"], "earthcall.portfolio.v1")
        self.assertTrue(projection["has_engine_snapshot"])
        self.assertEqual(projection["active_zone"]["name"], "Temple of Echoes")
        self.assertEqual(projection["active_zone"]["id"], "zone-temple")
        self.assertEqual(len(projection["objects"]), 1)
        self.assertEqual(projection["objects"][0]["id"], "obj-b")
        self.assertNotIn("secret_internal_field", projection["objects"][0])
        self.assertEqual(len(projection["laws"]), 1)
        self.assertEqual(projection["laws"][0]["identifier"], "law-b")
        self.assertNotIn("private_runtime_state", projection["laws"][0])


if __name__ == "__main__":
    unittest.main()
