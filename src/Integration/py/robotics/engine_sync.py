class EngineSync:
    """
    Speaks only property writes, ECA events, and Relation assertions over the existing WebSocket path.
    Contains no "robot" specific message shapes. 
    A message for a physical robot arm will be structurally identical to a message for a lamp or door.
    """
    def __init__(self, ws_client):
        self.ws_client = ws_client

    def send_property_write(self, target_slug: str, property_name: str, value: any):
        payload = {
            "type": "PropertyWrite",
            "target": target_slug,
            "property": property_name,
            "value": value
        }
        self.ws_client.send(payload)

    def send_relation_assertion(self, relation_type: str, source_slug: str, target_slug: str):
        payload = {
            "type": "RelationAssertion",
            "relation": relation_type,
            "source": source_slug,
            "target": target_slug
        }
        self.ws_client.send(payload)

    def send_event(self, event_type: str, source_slug: str, details: dict = None):
        payload = {
            "type": "Event",
            "event": event_type,
            "source": source_slug,
            "details": details or {}
        }
        self.ws_client.send(payload)
