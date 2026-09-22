class ConnectionRegistry:
    """
    Maintains the session registry (sockets, retries, connection health).
    Must not store poses, joint states, or telemetry history—the Earthcall world graph holds the state.
    """
    def __init__(self):
        self._connections = {}

    def register_connection(self, device_id, connection):
        self._connections[device_id] = connection

    def unregister_connection(self, device_id):
        if device_id in self._connections:
            del self._connections[device_id]

    def get_connection(self, device_id):
        return self._connections.get(device_id)

    def get_all_active_devices(self):
        return list(self._connections.keys())
