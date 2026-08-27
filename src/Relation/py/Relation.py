from src.ConstructedBeing.Singular.py.Singular import Singular

class Relation(Singular):
    """
    A first-class, weighted connection with an event timeline.
    Connections are beings, not arrays or implicit logic.
    """
    def __init__(self, identifier: str, source_id: str, target_id: str, relation_type: str, weight: float = 1.0):
        super().__init__(identifier)
        
        # Expose the relation state legibly to the world
        self._property_registry.update({
            "source_id": source_id,
            "target_id": target_id,
            "type": relation_type,
            "weight": weight
        })
