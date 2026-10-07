import os
import re

def check_links():
    docs_dir = 'docs/architecture/interrelations'
    md_files = [os.path.join(docs_dir, f) for f in os.listdir(docs_dir) if f.endswith('.md')]

    broken_links = []

    for md_file in md_files:
        with open(md_file, 'r', encoding='utf-8') as f:
            content = f.read()

            # Find links like [text](path) or `path` if it's used directly in the Connected Documents section
            # The files use format like: `../ontology/NO_BLACK_BOX.md`
            links = re.findall(r'`([^`]+\.md)`', content)

            for link in links:
                if link.startswith('../'):
                    # Resolve relative path
                    target = os.path.normpath(os.path.join(os.path.dirname(md_file), link))
                    if not os.path.exists(target):
                        broken_links.append((md_file, link, target))

    if broken_links:
        print("Broken links found:")
        for md_file, link, target in broken_links:
            print(f"File: {md_file}")
            print(f"  Link: {link}")
            print(f"  Resolves to missing: {target}")
            print()
        return False
    else:
        print("No broken links found!")
        return True

if __name__ == '__main__':
    if not check_links():
        exit(1)
