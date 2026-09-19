import sys
import unittest
from pathlib import Path

_SRC = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(_SRC))
sys.path.insert(0, str(_SRC / "Singularity" / "Foreign" / "py"))

from flask import Flask

from src.Singularity.Foreign.py.bridge import CppBridge
from src.Singularity.Foreign.py.api.routes import api_bp


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
            "objects": [{
                "id": "obj-a",
                "name": "Object A",
                "type": "ConstructedBeing",
                "position": [1, 2, 3],
                "private_internal_field": "do-not-project",
            }],
            "laws": [{
                "identifier": "law-a",
                "name": "Law A",
                "enabled": True,
                "activation": 1,
                "scope": 1,
                "conditionDescription": "signal.level >= 0.6",
                "actionDescription": "Map emission",
                "private_internal_field": "do-not-project",
            }],
        }
        for i in range(30):
            bridge.recent_events.append({"type": "engine_event", "index": i})

        projected = bridge.get_portfolio_state()

        self.assertEqual(projected["schema"], "earthcall.portfolio.v1")
        self.assertTrue(projected["connected"])
        self.assertTrue(projected["has_engine_snapshot"])
        self.assertEqual(projected["active_zone"]["id"], "zone-law-garden")
        self.assertEqual(len(projected["recent_events"]), 20)
        self.assertEqual(projected["recent_events"][0]["index"], 10)
        self.assertNotIn("private_internal_field", projected["objects"][0])
        self.assertNotIn("private_internal_field", projected["laws"][0])


class _FakeBridge:
    def get_portfolio_state(self):
        return {
            "schema": "earthcall.portfolio.v1",
            "connected": True,
            "active_zone": {"name": "Sanctum of Beginnings"},
            "objects": [],
            "laws": [],
            "recent_events": [],
        }


class TestPortfolioLiveRoute(unittest.TestCase):
    def setUp(self):
        self.app = Flask(__name__)
        self.app.config["TESTING"] = True
        self.app.config["CPP_BRIDGE"] = _FakeBridge()
        self.app.register_blueprint(api_bp)
        self.client = self.app.test_client()

    def test_allowed_portfolio_origin_receives_cors(self):
        origin = "https://zhangzachary834-commits.github.io"
        response = self.client.get("/api/portfolio/live", headers={"Origin": origin})

        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.json["schema"], "earthcall.portfolio.v1")
        self.assertEqual(response.headers["Access-Control-Allow-Origin"], origin)

    def test_disallowed_origin_is_refused(self):
        response = self.client.get(
            "/api/portfolio/live",
            headers={"Origin": "https://example-attacker.invalid"},
        )

        self.assertEqual(response.status_code, 403)
        self.assertNotIn("Access-Control-Allow-Origin", response.headers)

    def test_private_network_preflight_is_explicitly_allowed_for_portfolio(self):
        origin = "https://zhangzachary834-commits.github.io"
        response = self.client.options(
            "/api/portfolio/live",
            headers={
                "Origin": origin,
                "Access-Control-Request-Private-Network": "true",
            },
        )

        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.headers["Access-Control-Allow-Origin"], origin)
        self.assertEqual(response.headers["Access-Control-Allow-Private-Network"], "true")


if __name__ == "__main__":
    unittest.main()
