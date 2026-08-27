from typing import Any

class PropertyPath:
    """
    The address of a variable in Earthcall.
    Resolves string paths (e.g., 'position.y') against a Singular's property registry.
    """
    def __init__(self, path: str):
        self.segments = path.split(".")
    
    def resolve(self, root: Any) -> Any:
        current = root
        for i, segment in enumerate(self.segments):
            if not hasattr(current, '_property_registry'):
                return None
            
            val = current._property_registry.get(segment)
            if i == len(self.segments) - 1:
                return val
                
            current = val
        return current

    def __str__(self):
        return ".".join(self.segments)
