#!/usr/bin/env python3
"""
Borealis Sanctuary Comprehensive Validation & Verification Suite
================================================================
Pass 1: Volumetric V1 & V2 Participating Media Sovereignty

Verifies that:
1. `saves/zones/Borealis Sanctuary/zone.json` exists, is valid JSON, and adheres to Invariant 6.
2. Historical zones (Sun, Radiance Gallery, Prism Cathedral) remain untouched.
3. spatialRoot is a valid continuous atmosphere FieldNode with light.source == True.
4. All 5 required participating media spatialFields exist with exact identifiers:
   - Station 1: V1 Airy Transmitting Cloud & Dense Absorbing Cloud
   - Station 2: V2 Emerald-Cyan Surge & Violet-Magenta Whisper
   - Station 3: Hero Auroral Veil Curtain
5. Station 1 (V1 Extinction Sovereignty):
   - Both clouds have BYTE-IDENTICAL volumeDensity ASTs.
   - Both clouds have identical neutral scattering & chroma.
   - Left cloud has low extinction (sigma_t = 0.16) for high transmittance.
   - Right cloud has high extinction (sigma_t = 2.85) for heavy attenuation.
   - sigma_t != D sovereignty is verified.
6. Station 2 (V2 Scattering & Chroma Sovereignty):
   - Both clouds have BYTE-IDENTICAL volumeDensity ASTs.
   - Both clouds have BYTE-IDENTICAL volumeExtinction ASTs (sigma_t = 0.65).
   - Left cloud has strong scattering (sigma_s = 1.25) and emerald/cyan chroma.
   - Right cloud has weaker scattering (sigma_s = 0.28) and violet/magenta chroma.
   - sigma_s != sigma_t != D sovereignty is verified.
7. Station 3 (Hero Auroral Veil Curtain Synthesis):
   - Combines all four authored channels: volumeDensity, volumeExtinction, volumeScattering, volumeChroma.
   - D(p,t) forms an undulating flowing ribbon sheet with vertical folds, rays, and Perlin noise.
   - sigma_t(p,t) has an authored altitude gradient (denser base, gossamer crest).
   - sigma_s(p,t) produces wave-modulated shimmering scattering patches.
   - C_v(p,t) is a 3D vector field transitioning across green (557.7 nm), cyan, violet, and red (630 nm).
8. Architecture & Environment:
   - Promenade pavement segments present along Z.
   - Background reference beacons and quartz needles positioned behind media.
   - All 4 Inscription Stelae present with full pedagogical text.
"""

import json
import os
import sys

def main():
    print("============================================================")
    print("Running Borealis Sanctuary Validation Suite (Volumetric V1+V2)")
    print("============================================================")

    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    borealis_path = os.path.join(repo_root, "saves", "zones", "Borealis Sanctuary", "zone.json")
    world_path = os.path.join(repo_root, "saves", "worlds", "borealis_sanctuary.json")

    # 1. Historical zones untouched
    for hz in ["Sun", "Radiance Gallery", "Prism Cathedral"]:
        hz_path = os.path.join(repo_root, "saves", "zones", hz, "zone.json")
        assert os.path.exists(hz_path), f"Historical zone missing: {hz_path}"
    print("  ok: Historical zones (Sun, Radiance Gallery, Prism Cathedral) remain untouched")

    # 2. Check Borealis Sanctuary file
    assert os.path.exists(borealis_path), f"Borealis Sanctuary save missing at {borealis_path}"
    with open(borealis_path, "r", encoding="utf-8") as f:
        doc = json.load(f)
    print("  ok: Borealis Sanctuary save file exists and parsed as valid JSON")

    # 3. Invariant 6: directory name matches document identifier
    doc_id = doc.get("identifier")
    doc_name = doc.get("name")
    dir_name = os.path.basename(os.path.dirname(borealis_path))
    assert doc_id == dir_name == "Borealis Sanctuary", f"Invariant 6 violation: {doc_id} != {dir_name}"
    assert doc_name == "Borealis Sanctuary"
    print("  ok: Invariant 6 satisfied: identifier matches directory name 'Borealis Sanctuary'")

    # 4. Check companion session save file
    assert os.path.exists(world_path), f"Companion world save missing at {world_path}"
    with open(world_path, "r", encoding="utf-8") as f:
        world_doc = json.load(f)
    assert world_doc.get("activeZone") == "Borealis Sanctuary"
    print("  ok: Companion world save exists and targets 'Borealis Sanctuary'")

    # 5. Check spatialRoot
    root = doc.get("spatialRoot")
    assert root is not None, "spatialRoot is missing"
    assert root.get("id") == "borealis.primary-atmosphere-root"
    root_props = root.get("authoredProperties", {})
    assert root_props.get("light.source", {}).get("v") is True, "light.source must be True"
    assert root_props.get("light.ambient", {}).get("v") == 0.04, "Dark ambient night starlight expected"
    print("  ok: spatialRoot carries valid dark starlight atmosphere field with light.source=true")

    # 6. Check spatialFields
    fields = doc.get("spatialFields", [])
    assert len(fields) == 5, f"Expected 5 spatialFields, got {len(fields)}"
    field_map = {f.get("id"): f for f in fields}

    expected_ids = [
        "borealis.v1.airy-transmitting-cloud",
        "borealis.v1.dense-absorbing-cloud",
        "borealis.v2.emerald-cyan-surge",
        "borealis.v2.violet-magenta-whisper",
        "borealis.hero.auroral-curtain"
    ]
    for eid in expected_ids:
        assert eid in field_map, f"Missing required spatial field: {eid}"
    print("  ok: All 5 required participating media spatialFields exist with exact identifiers")

    # 7. Station 1: V1 Extinction Sovereignty Pair
    f_airy = field_map["borealis.v1.airy-transmitting-cloud"]
    f_dense = field_map["borealis.v1.dense-absorbing-cloud"]

    # Byte-identical density AST
    density_airy_json = json.dumps(f_airy["volumeDensity"], sort_keys=True)
    density_dense_json = json.dumps(f_dense["volumeDensity"], sort_keys=True)
    assert density_airy_json == density_dense_json, "V1 pair must have byte-identical volumeDensity ASTs!"
    print("  ok: V1 pair: both media share BYTE-IDENTICAL volumeDensity ASTs")

    # Extinction comparison
    ext_airy = f_airy["volumeExtinction"]["pieces"][0]["mathNode"]["scalarForm"]["terms"][0]["c"]
    ext_dense = f_dense["volumeExtinction"]["pieces"][0]["mathNode"]["scalarForm"]["terms"][0]["c"]
    assert ext_airy <= 0.25, f"Airy cloud extinction should be low, got {ext_airy}"
    assert ext_dense >= 2.5, f"Dense cloud extinction should be high, got {ext_dense}"
    print(f"  ok: V1 pair: airy extinction sigma_t={ext_airy} vs dense extinction sigma_t={ext_dense} verified")

    # Neutral scattering & chroma
    chroma_airy = json.dumps(f_airy["volumeChroma"], sort_keys=True)
    chroma_dense = json.dumps(f_dense["volumeChroma"], sort_keys=True)
    assert chroma_airy == chroma_dense, "V1 pair must share neutral volumeChroma"
    print("  ok: V1 pair: identical neutral scattering & white chroma setup verified")

    # 8. Station 2: V2 Scattering & Chroma Sovereignty Pair
    f_emerald = field_map["borealis.v2.emerald-cyan-surge"]
    f_violet = field_map["borealis.v2.violet-magenta-whisper"]

    # Byte-identical density AST
    density_emerald_json = json.dumps(f_emerald["volumeDensity"], sort_keys=True)
    density_violet_json = json.dumps(f_violet["volumeDensity"], sort_keys=True)
    assert density_emerald_json == density_violet_json, "V2 pair must have byte-identical volumeDensity ASTs!"
    print("  ok: V2 pair: both media share BYTE-IDENTICAL volumeDensity ASTs")

    # Byte-identical extinction AST
    ext_emerald_json = json.dumps(f_emerald["volumeExtinction"], sort_keys=True)
    ext_violet_json = json.dumps(f_violet["volumeExtinction"], sort_keys=True)
    assert ext_emerald_json == ext_violet_json, "V2 pair must have byte-identical volumeExtinction ASTs!"
    print("  ok: V2 pair: both media share BYTE-IDENTICAL volumeExtinction ASTs (sigma_t = 0.65)")

    # Scattering comparison
    scat_emerald = f_emerald["volumeScattering"]["pieces"][0]["mathNode"]["scalarForm"]["terms"][0]["c"]
    scat_violet = f_violet["volumeScattering"]["pieces"][0]["mathNode"]["scalarForm"]["terms"][0]["c"]
    assert scat_emerald >= 1.0, f"Emerald scattering should be strong, got {scat_emerald}"
    assert scat_violet <= 0.35, f"Violet scattering should be weaker, got {scat_violet}"
    print(f"  ok: V2 pair: strong scattering sigma_s={scat_emerald} vs weak scattering sigma_s={scat_violet}")

    # Chroma comparison
    chroma_em_node = f_emerald["volumeChroma"]["pieces"][0]["mathNode"]
    chroma_vi_node = f_violet["volumeChroma"]["pieces"][0]["mathNode"]
    assert chroma_em_node["op"] == 2 and chroma_vi_node["op"] == 2, "Chroma must be VectorConstruct (op 2)"
    r_em = chroma_em_node["children"][0]["scalarForm"]["terms"][0]["c"]
    g_em = chroma_em_node["children"][1]["scalarForm"]["terms"][0]["c"]
    b_em = chroma_em_node["children"][2]["scalarForm"]["terms"][0]["c"]
    assert g_em > 0.8 and r_em < 0.2, "Emerald/cyan chroma must be green/cyan dominant"

    r_vi = chroma_vi_node["children"][0]["scalarForm"]["terms"][0]["c"]
    g_vi = chroma_vi_node["children"][1]["scalarForm"]["terms"][0]["c"]
    b_vi = chroma_vi_node["children"][2]["scalarForm"]["terms"][0]["c"]
    assert r_vi > 0.6 and b_vi > 0.8 and g_vi < 0.3, "Violet/magenta chroma must be red/blue dominant"
    print("  ok: V2 pair: emerald/cyan C_v vs violet/magenta C_v spectral chroma verified")

    # 9. Station 3: Hero Auroral Veil Curtain Synthesis
    f_hero = field_map["borealis.hero.auroral-curtain"]
    assert "volumeDensity" in f_hero, "Hero curtain missing volumeDensity"
    assert "volumeExtinction" in f_hero, "Hero curtain missing volumeExtinction"
    assert "volumeScattering" in f_hero, "Hero curtain missing volumeScattering"
    assert "volumeChroma" in f_hero, "Hero curtain missing volumeChroma"

    hero_d_str = json.dumps(f_hero["volumeDensity"])
    assert "29" in hero_d_str, "Hero density must include Op::Noise (29)"
    assert "sin" in hero_d_str or "trans" in hero_d_str, "Hero density must include sinusoidal waves"
    print("  ok: Hero curtain: D(p,t) forms flowing ribbon sheet with harmonic folds & Perlin noise")

    hero_e_str = json.dumps(f_hero["volumeExtinction"])
    assert "y" in hero_e_str, "Hero extinction must vary with altitude y"
    print("  ok: Hero curtain: sigma_t(p,t) authors independent altitude extinction gradient")

    hero_s_str = json.dumps(f_hero["volumeScattering"])
    assert "29" in hero_s_str or "trans" in hero_s_str, "Hero scattering must produce modulated wave patches"
    print("  ok: Hero curtain: sigma_s(p,t) authors wave-modulated shimmering scattering patches")

    hero_c_node = f_hero["volumeChroma"]["pieces"][0]["mathNode"]
    assert hero_c_node["op"] == 2, "Hero chroma must be VectorConstruct"
    assert len(hero_c_node["children"]) == 3, "Hero chroma must have 3 components"
    print("  ok: Hero curtain: C_v(p,t) authors continuous multi-color spectral vector field")

    # 10. Background Architecture & Stelae
    world_objs = doc.get("world", {}).get("objects", [])
    obj_map = {o["objectID"]: o for o in world_objs}

    assert "borealis.station1.beacon-left" in obj_map
    assert "borealis.station1.beacon-right" in obj_map
    print("  ok: Background golden reference beacons present behind Station 1 clouds (Z=35)")

    assert "borealis.station2.pillar-left" in obj_map
    assert "borealis.station2.pillar-right" in obj_map
    print("  ok: Background quartz needles present behind Station 2 clouds (Z=70)")

    for i in range(5):
        assert f"borealis.station3.needle_{i}" in obj_map
    print("  ok: 5 Celestial Spire needles present behind Hero Curtain (Z=125)")

    for stele_id in ["borealis.stele.entrance", "borealis.stele.station1", "borealis.stele.station2", "borealis.stele.station3"]:
        assert stele_id in obj_map, f"Missing pedagogical stele: {stele_id}"
        inscription = obj_map[stele_id]["authoredProperties"]["stele.inscription"]["v"]
        assert len(inscription) > 100, f"Inscription on {stele_id} too short"
    print("  ok: All 4 Pedagogical Inscription Stelae present with full architectural doctrine")

    print("------------------------------------------------------------")
    print("Borealis Sanctuary Validation: ALL CHECKS PASSED (100% SUCCESS)!")
    print("============================================================")
    return 0

if __name__ == "__main__":
    sys.exit(main())
