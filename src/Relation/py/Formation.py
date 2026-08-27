from src.Relation.py.Relation import Relation

class Formation(Relation):
    """
    An aggregate or whole that has an identity of its own.
    A structure among members.
    """
    def __init__(self, identifier: str):
        # A Formation is itself a Relation and a Singular
        super().__init__(identifier, source_id="", target_id="", relation_type="formation")
        self._members = set()
        
    def add_member(self, member_id: str):
        self._members.add(member_id)
        
    def remove_member(self, member_id: str):
        self._members.discard(member_id)
