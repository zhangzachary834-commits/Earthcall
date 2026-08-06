import os
import sys
from pathlib import Path

from flask import Flask
from flask_cors import CORS
from flask_socketio import SocketIO
from dotenv import load_dotenv

# The socket handlers live with the rest of the Network modality, beside the
# engine's WebSocketClient/WebSocketServer — they are two halves of one channel,
# not two subsystems (docs/architecture/DIRECTORY_ORDERING.md §3).
_SRC = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(_SRC / "Singularity" / "Network" / "py"))

# Load environment variables from .env
load_dotenv()

# Initialize Flask app
app = Flask(__name__, static_url_path='/static')

# Apply CORS (Cross-Origin Resource Sharing)
CORS(app)

# Initialize SocketIO
socketio = SocketIO(app, cors_allowed_origins=["https://trusted.earthcall.com", "http://localhost:3000", "http://127.0.0.1:3000"])

# Import and register Blueprints
from web.routes import web_bp
from api.routes import api_bp
app.register_blueprint(web_bp)
app.register_blueprint(api_bp)

# Register WebSocket events
from events import register_socket_events
register_socket_events(socketio)

# Import and start the Raw WebSocket Engine Server
from engine_server import start_engine_server

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 5000))
    host = os.environ.get('HOST', '127.0.0.1')
    debug = os.environ.get('DEBUG', 'False').lower() in ('true', '1', 't')
    
    print(f"Starting Earthcall Python Backend on {host}:{port}")
    
    # Start the Raw WebSocket Server for the Engine on port 5001
    engine_port = int(os.environ.get('ENGINE_PORT', 5001))
    engine_server = start_engine_server(host=host, port=engine_port)
    
    # Initialize Robotics Integrations
    sys.path.insert(0, str(_SRC / "Integration" / "py"))
    from robotics.connection_registry import ConnectionRegistry
    from robotics.engine_sync import EngineSync
    
    connection_registry = ConnectionRegistry()
    engine_sync = EngineSync(engine_server)
    
    # Start the Flask-SocketIO Meta-Server
    socketio.run(app, host=host, port=port, debug=debug, allow_unsafe_werkzeug=True, use_reloader=False)