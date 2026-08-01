import os
from flask import Flask
from flask_cors import CORS
from flask_socketio import SocketIO
from dotenv import load_dotenv

# Load environment variables from .env
load_dotenv()

# Initialize Flask app
app = Flask(__name__, static_url_path='/static')

# Apply CORS (Cross-Origin Resource Sharing)
CORS(app)

# Initialize SocketIO
socketio = SocketIO(app, cors_allowed_origins="*")

# Import and register Blueprints
from web.routes import web_bp
from api.routes import api_bp
app.register_blueprint(web_bp)
app.register_blueprint(api_bp)

# Register WebSocket events
from sockets.events import register_socket_events
register_socket_events(socketio)

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 5000))
    host = os.environ.get('HOST', '127.0.0.1')
    debug = os.environ.get('DEBUG', 'True').lower() in ('true', '1', 't')
    
    print(f"Starting Earthcall Python Backend on {host}:{port}")
    socketio.run(app, host=host, port=port, debug=debug)