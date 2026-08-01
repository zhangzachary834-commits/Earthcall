from flask_socketio import emit

def register_socket_events(socketio):
    @socketio.on('connect')
    def handle_connect():
        print("Client connected")
        emit('server_message', {'data': 'Connected to Earthcall WebSockets!'})

    @socketio.on('disconnect')
    def handle_disconnect():
        print("Client disconnected")

    @socketio.on('client_message')
    def handle_client_message(json_data):
        print("Received message: " + str(json_data))
        emit('server_response', {'status': 'success', 'echo': json_data})
