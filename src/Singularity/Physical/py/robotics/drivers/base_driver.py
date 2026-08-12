class BaseDriver:
    """
    Abstract base class for pure protocol translation.
    Reads raw data from the socket and forwards it to engine_sync.py
    to become property writes. Decides nothing.
    """
    def __init__(self, connection_id, engine_sync):
        self.connection_id = connection_id
        self.engine_sync = engine_sync

    def connect(self, address):
        raise NotImplementedError

    def disconnect(self):
        raise NotImplementedError

    def handle_incoming_data(self, data):
        """
        Subclasses should implement parsing protocol data here,
        and call self.engine_sync methods.
        """
        raise NotImplementedError

    def send_command(self, command_data):
        """
        Subclasses should implement sending protocol-specific commands.
        """
        raise NotImplementedError
