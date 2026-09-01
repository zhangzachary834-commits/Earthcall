import os
import sys
import importlib
from pathlib import Path

# Add python source path
_REPO_ROOT = Path(__file__).resolve().parents[2]
_FOREIGN_PY = _REPO_ROOT / "src" / "Singularity" / "Foreign" / "py"
sys.path.insert(0, str(_FOREIGN_PY))

def test_cors_default(monkeypatch):
    monkeypatch.delenv("CORS_ALLOWED_ORIGINS", raising=False)
    if "app" in sys.modules:
        del sys.modules["app"]
    import app
    origins = app.socketio.server.eio.cors_allowed_origins
    assert isinstance(origins, list)
    assert "http://127.0.0.1:5005" in origins
    assert "http://localhost:5005" in origins
    assert "http://127.0.0.1:8080" in origins
    assert "http://localhost:8080" in origins

def test_cors_custom_list(monkeypatch):
    monkeypatch.setenv("CORS_ALLOWED_ORIGINS", "https://example.com, https://app.earthcall.io")
    if "app" in sys.modules:
        del sys.modules["app"]
    import app
    origins = app.socketio.server.eio.cors_allowed_origins
    assert origins == ["https://example.com", "https://app.earthcall.io"]

def test_cors_wildcard(monkeypatch):
    monkeypatch.setenv("CORS_ALLOWED_ORIGINS", "*")
    if "app" in sys.modules:
        del sys.modules["app"]
    import app
    origins = app.socketio.server.eio.cors_allowed_origins
    assert origins == "*"
