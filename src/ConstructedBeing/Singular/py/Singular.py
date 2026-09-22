from typing import Any, Dict
from src.ConstructedBeing.Singular.py.Property import PropertyPath

class Singular:
    """
    The foundational being in Earthcall.
    It has a stable identifier and a legible property registry.
    There are NO black boxes and NO hidden domain variables.
    """
    def __init__(self, identifier: str):
        self._identifier = identifier
        # The public surface for all governable state
        self._property_registry: Dict[str, Any] = {}
        
    def get_identifier(self) -> str:
        return self._identifier
        
    def get_property(self, path: str) -> Any:
        return PropertyPath(path).resolve(self)
        
    def set_property(self, path: str, value: Any) -> bool:
        segments = path.split(".")
        current = self
        
        # Navigate to the second-to-last segment
        for segment in segments[:-1]:
            if not hasattr(current, '_property_registry'):
                return False
            current = current._property_registry.get(segment)
            if not current:
                return False
        
        # Set the final segment
        last_segment = segments[-1]
        if hasattr(current, '_property_registry'):
            current._property_registry[last_segment] = value
            return True
        return False
