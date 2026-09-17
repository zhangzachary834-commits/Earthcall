import json

node = {
    "op": 3,
    "children": [
        {"op": 1, "var": "v"},
        {"op": 0, "scalarForm": {"terms": [{"c": 0.5, "factors": {}}]}}
    ]
}

selector = {
    "input": "v",
    "pieces": [
        {
            "where": node,
            "call": None,
            "fold": None
        },
        {
            "guard": {"kind": 3, "children": []}
        }
    ]
}

print(json.dumps(selector))
