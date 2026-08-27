import sys
import os

# Add the repository root to the path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../..')))

from src.ConstructedBeing.Singular.py.Singular import Singular
from src.Relation.py.Formation import Formation
from src.ZonesOfEarth.py.Zone import Zone
from src.ZonesOfEarth.AuthorsOfLaw.py.Law import Law

if __name__ == "__main__":
    z = Zone("zone-1")
    f = Formation("formation-1")
    l = Law("law-1")
    
    print(f"Instantiated: {z.get_identifier()}, {f.get_identifier()}, {l.get_identifier()}")
    
    z.set_property("name", "Void")
    print(f"Zone Name Property: {z.get_property('name')}")
    print("Python Earthcall primitives loaded successfully!")
