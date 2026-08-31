from flask_socketio import emit
from flask import current_app

def register_socket_events(socketio, bridge):
    """
    Registers Socket.IO real-time event handlers for the Earthcall web UI.
    """
    
    # Hook bridge state changes and engine events directly into Socket.IO broadcasts
    def on_engine_state_sync(state):
        socketio.emit('state_sync', state)
        
    def on_engine_event(event_data):
        socketio.emit('engine_event', event_data)
        
    bridge.on_state_sync_callbacks.append(on_engine_state_sync)
    bridge.on_event_callbacks.append(on_engine_event)

    @socketio.on('connect')
    def handle_connect():
        print("[SocketIO] Web Client Connected")
        # Send current state and status immediately
        emit('state_sync', bridge.get_state())
        emit('engine_status', bridge.get_status())

    @socketio.on('disconnect')
    def handle_disconnect():
        print("[SocketIO] Web Client Disconnected")

    @socketio.on('get_state')
    def handle_get_state():
        emit('state_sync', bridge.get_state())

    @socketio.on('utterance')
    def handle_utterance(data):
        text = data.get('text', data.get('payload', ''))
        source = data.get('source', 'web_ui')
        target = data.get('target', '')
        if text:
            bridge.emit_utterance(text, source, target)

    @socketio.on('spawn_object')
    def handle_spawn_object(data):
        shape = data.get('shape', 'Cube')
        position = data.get('position', [0.0, 1.0, 0.0])
        color = data.get('color', [0.2, 0.7, 1.0])
        name = data.get('name', '')
        dimensions = float(data.get('dimensions', 1.0))
        material_id = data.get('materialId', 'material.default')
        bridge.spawn_object(shape, position, color, name, dimensions, material_id)

    @socketio.on('update_object')
    def handle_update_object(data):
        obj_id = data.get('id', '')
        if obj_id:
            bridge.update_object(obj_id, data)

    @socketio.on('delete_object')
    def handle_delete_object(data):
        obj_id = data.get('id', '')
        if obj_id:
            bridge.delete_object(obj_id)

    @socketio.on('write_property')
    def handle_write_property(data):
        target = data.get('target', '')
        prop = data.get('property', '')
        val = data.get('value')
        if target and prop:
            bridge.write_property(target, prop, val)

    @socketio.on('toggle_law')
    def handle_toggle_law(data):
        law_id = data.get('identifier', '')
        enabled = data.get('enabled', True)
        if law_id:
            bridge.toggle_law(law_id, enabled)

    @socketio.on('switch_zone')
    def handle_switch_zone(data):
        index = data.get('index')
        name = data.get('name')
        bridge.switch_zone(index=index, name=name)

    @socketio.on('teleport')
    def handle_teleport(data):
        pos = data.get('position', [0.0, 1.8, 0.0])
        bridge.teleport_player(pos)

    @socketio.on('set_physics')
    def handle_set_physics(data):
        bridge.set_physics(flying=data.get('flying'), gravity_viz=data.get('gravity_viz'))

    @socketio.on('command')
    def handle_command(data):
        bridge.send_command(data)
