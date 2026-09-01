import os
import secrets
import sys
from pathlib import Path

from flask import Flask
from flask_cors import CORS
from flask_socketio import SocketIO
from dotenv import load_dotenv

# Setup Python module paths
_CURRENT = Path(__file__).resolve().parent
_SRC = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(_SRC / "Singularity" / "Network" / "py"))
sys.path.insert(0, str(_SRC / "Singularity" / "Physical" / "py"))
sys.path.insert(0, str(_CURRENT))

# Load environment variables from .env
load_dotenv()

# Initialize Flask app
app = Flask(__name__, static_folder='static', template_folder='templates')
app.config['SECRET_KEY'] = os.environ.get('SECRET_KEY') or secrets.token_hex(32)

# Apply CORS (Cross-Origin Resource Sharing)
CORS(app)

# Initialize SocketIO
socketio = SocketIO(app, cors_allowed_origins="*", async_mode='threading')

# Initialize C++ Engine Bridge (WebSocket client to C++ Engine on port 8080)
from bridge import CppBridge
cpp_port = int(os.environ.get('CPP_PORT', 8080))
cpp_host = os.environ.get('CPP_HOST', '127.0.0.1')
bridge = CppBridge(host=cpp_host, port=cpp_port)
bridge.start()
app.config['CPP_BRIDGE'] = bridge

# Import and register Blueprints
from web.routes import web_bp
from api.routes import api_bp
app.register_blueprint(web_bp)
app.register_blueprint(api_bp)

# Register WebSocket events
from events import register_socket_events
register_socket_events(socketio, bridge)

# Start the Raw WebSocket Server for legacy/alternative engine connectivity on port 5001
try:
    from engine_server import start_engine_server
    engine_port = int(os.environ.get('ENGINE_PORT', 5001))
    engine_server = start_engine_server(host="127.0.0.1", port=engine_port)
except Exception as e:
    print(f"[App] Note: EngineServer on 5001 not started: {e}")
    engine_server = None

# Initialize Robotics Integrations
try:
    from robotics.connection_registry import ConnectionRegistry
    from robotics.engine_sync import EngineSync
    connection_registry = ConnectionRegistry()
    engine_sync = EngineSync(bridge)
except Exception as e:
    print(f"[App] Note: Robotics integrations initialized in bridge mode: {e}")

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 5005))
    host = os.environ.get('HOST', '127.0.0.1')
    debug = os.environ.get('DEBUG', 'False').lower() in ('true', '1', 't')
    
    print("=" * 60)
    print(f"🌍 Earthcall First Mover Studio & Engine Console")
    print(f"🚀 Running Web UI on http://{host}:{port}")
    print(f"🔌 Connected to C++ Engine on ws://{cpp_host}:{cpp_port}")
    print("=" * 60)
    
    # Start the Flask-SocketIO Meta-Server
    socketio.run(app, host=host, port=port, debug=debug, allow_unsafe_werkzeug=True, use_reloader=False)
