import json

def collect_vars_from_function(func):
    vars_found = set()
    if 'pieces' in func:
        for p in func['pieces']:
            if 'expr' in p and 'sum' in p['expr']:
                for term in p['expr']['sum']:
                    if 'factors' in term:
                        if isinstance(term['factors'], dict):
                            for k in term['factors'].keys():
                                vars_found.add(k)
                        elif isinstance(term['factors'], list):
                            for factor in term['factors']:
                                if isinstance(factor, dict) and factor.get('type') == 'var':
                                    vars_found.add(factor['name'])
            # if 'value' is used instead of 'expr'
            if 'value' in p and 'sum' in p['value']:
                for term in p['value']['sum']:
                    if 'factors' in term:
                        if isinstance(term['factors'], dict):
                            for k in term['factors'].keys():
                                vars_found.add(k)
                        elif isinstance(term['factors'], list):
                            for factor in term['factors']:
                                if isinstance(factor, dict) and factor.get('type') == 'var':
                                    vars_found.add(factor['name'])
    return vars_found

def walk(node):
    if isinstance(node, dict):
        if 'function' in node and 'kind' in node and node['kind'] in [6, 8]:
            # It's a Map action or Zone condition
            vars_found = collect_vars_from_function(node['function'])
            if 'input' in node['function']:
                vars_found.add(node['function']['input'])
                
            bindings = node.get('bindings', {})
            for v in vars_found:
                if v not in bindings:
                    bindings[v] = v
            if bindings:
                node['bindings'] = bindings
        
        for k, v in node.items():
            walk(v)
    elif isinstance(node, list):
        for item in node:
            walk(item)

def main():
    with open('saves/worlds/chess.json', 'r') as f:
        data = json.load(f)
        
    walk(data)
    
    with open('saves/worlds/chess.json', 'w') as f:
        json.dump(data, f, indent=2)

if __name__ == '__main__':
    main()
