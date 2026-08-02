from flask_socketio import emit

def register_socket_events(socketio):
    @socketio.on('connect')
    def handle_connect():
        print("Client connected (Wasm Engine)")
        # Send an initial state sync snapshot (dummy for now)
        emit('state_sync', {'snapshot': 'hello_from_server'})

    @socketio.on('disconnect')
    def handle_disconnect():
        print("Client disconnected")

    @socketio.on('law_action')
    def handle_law_action(json_data):
        print(f"Server received Law Action: {json_data}")
        # Broadcast the law action to all other clients so they can simulate it
        emit('state_sync', {'event': 'law_executed', 'data': json_data}, broadcast=True, include_self=False)

    @socketio.on('cpp_message')
    def handle_cpp_message(json_data):
        print(f"Received C++ message: {json_data}")
        emit('server_response', {'status': 'success', 'echo': json_data})
