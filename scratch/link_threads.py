import os
from pathlib import Path

base_dir = Path("/Users/zacharyzhang/Documents/GitHub/Earthcall/agent intercom/communication-threads")

def get_cluster(filepath):
    path_str = str(filepath)
    name = filepath.name
    if "rete-and-law" in path_str:
        return "Rete and Law Engine"
    elif "performance-and-lag" in path_str:
        return "Performance and Lag"
    elif "saves-and-zones" in path_str:
        return "Saves and Zones"
    elif "ontomath-light-and-image" in path_str:
        if "SUN_UPDATE_V4" in name: return "Volumetric V4 Updates (PR #339)"
        elif "SUN_UPDATE_V5" in name: return "Volumetric V5 Updates (PR #343)"
        elif "SUN_UPDATE_POST_V5" in name: return "Post-V5 Tribunal Updates"
        elif "SUN_UPDATE_RUNG9" in name: return "Rung 9 Updates"
        elif "SUN_HANDOFF_OntoMath_Radiance" in name: return "OntoMath Radiance Rungs"
        elif "SUN_HANDOFF_VOLUMETRIC" in name: return "Volumetric Handoffs"
        elif "VISIBILITY_SUN" in name or "PR320" in name: return "Visibility & PR #320"
        elif "CODEX_TO_SOL" in name: return "Codex-Sol Syncs"
        else: return "OntoMath Light & Image (Misc)"
    else:
        return "General Intercom"

files = list(base_dir.rglob("*.md"))

clusters = {}
for f in files:
    if f.name == "00_THREAD_INDEX.md": continue
    c = get_cluster(f)
    if c not in clusters: clusters[c] = []
    clusters[c].append(f)

for c in clusters:
    clusters[c].sort(key=lambda x: x.name)

# 1. Create a central index
index_content = "# Agent Intercom - Thread Index\n\nThis index links the fragmented 'chaos threads' across different domains.\n\n"
for c, flist in sorted(clusters.items()):
    index_content += f"## {c}\n"
    for f in flist:
        rel_path = f.relative_to(base_dir)
        # url encode space
        url_path = str(rel_path).replace(" ", "%20")
        index_content += f"- [{f.name}](./{url_path})\n"
    index_content += "\n"

index_file = base_dir / "00_THREAD_INDEX.md"
index_file.write_text(index_content)

# 2. Inject navigation blocks into files
nav_marker = "<!-- NAV_BLOCK_START -->"
for c, flist in clusters.items():
    for f in flist:
        content = f.read_text()
        if nav_marker in content: continue # Already injected
        
        # Build block
        rel_to_base = os.path.relpath(index_file, f.parent)
        block = f"{nav_marker}\n> [!NOTE]\n> **Thread Navigation: {c}**\n"
        block += f"> [View Full Thread Index]({rel_to_base})\n>\n> **Related in this thread:**\n"
        for other in flist:
            if other == f: continue
            rel_other = os.path.relpath(other, f.parent)
            block += f"> - [{other.name}]({rel_other})\n"
        block += "<!-- NAV_BLOCK_END -->\n\n"
        
        # Inject right after the first heading, or at the top
        lines = content.split('\n')
        injected = False
        for i, line in enumerate(lines):
            if line.startswith("# "):
                lines.insert(i+1, "\n" + block)
                injected = True
                break
        if not injected:
            lines.insert(0, block)
            
        f.write_text('\n'.join(lines))

print("Done grouping and linking.")
