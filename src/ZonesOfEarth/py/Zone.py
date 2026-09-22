from typing import Dict
from src.ConstructedBeing.Singular.py.Singular import Singular

class Zone(Singular):
    """
    The container for objects. The 'womb' for spawned beings.
    """
    def __init__(self, identifier: str):
        super().__init__(identifier)
        self.objects: Dict[str, Singular] = {}
        
    def add_object(self, obj: Singular):
        self.objects[obj.get_identifier()] = obj
        
    def remove_object(self, obj_identifier: str):
        if obj_identifier in self.objects:
            del self.objects[obj_identifier]
