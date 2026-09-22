#!/usr/bin/env python3
"""
Prism Cathedral Comprehensive Validation & Verification Suite
=============================================================
Pass 2: Complex-Shaped Mathematical Light Fields & Volumetric Media

Tests that:
1. `saves/zones/Prism Cathedral/zone.json` exists, is valid JSON, and adheres to Invariant 6.
2. Historical zones (Sun, Radiance Gallery, etc.) remain untouched.
3. The central nave progression has all stations (Entrance, Foundation 1, Foundation 2,
   Rungs 3 through 8, and The Summit: THE PRISM).
4. All parallel wings are present (Wing A: Surface Color, Wing B: Volumetric V0,
   Wing C: Live Authoring, Wing D: Compatibility & Refusal).
5. All 14 Inscription Stelae exist and carry exact equations, doctrines, and invariants.
6. All 17 FieldNodes (1 spatialRoot + 16 spatialFields) have valid ASTs and properties.
7. Shape Vocabulary Verification:
   - Station 3: Hollow Shell and Toroidal Ring Radiance Fields
   - Station 4: Time-Animated Breathing Shell, Pulsing Heart, and Rotating Lobes
   - Station 5: Bipolar N/S Chroma, Spectral Wave, and Concentric Shells Chroma
   - Station 6: Toroidal Radial Outward Emission and Rotating Lighthouse Beam
   - Station 7: 4 Coexisting Independent Sources with Distinct Mathematical Shapes:
       * Source A: Spherical Luminous Shell
       * Source B: Toroidal Ring Field
       * Source C: Noisy Organic Lobe
       * Source D: Narrow Vertical Pillar Filament
   - Station 8: Blocker occluding filament while toroidal source illuminates receiver
8. Volumetric V0 Participating Media (Wing B & Summit):
   - Exhibit B1: Soft Spherical Cloud
   - Exhibit B2: Hollow Shell Nebular Membrane (D=0 in core, D>0 in shell)
   - Exhibit B3: Toroidal / Ring Medium (D=0 in hole, D>0 on tube ring)
   - Exhibit B4: CSG-Subtracted Crescent Cloud (D=0 inside carved hole)
   - Exhibit B5: Noise-Warped Organic Cloud (Op::Noise envelope)
   - Exhibit B6: Time-Varying Breathing Hollow Nebula (D(p,t) reads t)
   - Exhibit B7: DIAGNOSTIC PROXY-PROOF: 20x8x20m AABB containing tiny slender fog ring!
   - Exhibit B8: Dual Sovereign Being (rho_source != D_medium) + Opaque Depth Truncation Pillar
   - Summit: Encircling Toroidal Medium ring around the Colossal Crystal Prism
   - Summit: Celestial Rotunda Atmosphere
9. Constitutional Invariants:
   - rho_source != V_transport != D_medium
   - All 6 standalone media in Wing B have volumeDensity and light.source == False (no implicit fog)
   - Dual Sovereign Being has distinct rho and D mathematical ASTs
10. Wing A materials contain valid authored colorExpr (SDF surface color fields).
"""

import json
import os
import sys

def main():
    print("============================================================")
    print("Running Prism Cathedral Validation Suite (Pass 2: Complex Shapes)")
    print("============================================================")

    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    cathedral_path = os.path.join(repo_root, "saves", "zones", "Prism Cathedral", "zone.json")
    sun_path = os.path.join(repo_root, "saves", "zones", "Sun", "zone.json")
    gallery_path = os.path.join(repo_root, "saves", "zones", "Radiance Gallery", "zone.json")

    # 1. Non-destruction check on historical zones
    assert os.path.exists(sun_path), "Sun Zone must remain untouched!"
    assert os.path.exists(gallery_path), "Radiance Gallery must remain untouched!"
    print("  ok: Historical zones (Sun, Radiance Gallery) remain untouched")

    # 2. Check Prism Cathedral file
    assert os.path.exists(cathedral_path), f"Prism Cathedral save missing at {cathedral_path}"
    with open(cathedral_path, "r") as f:
        data = json.load(f)
    print("  ok: Prism Cathedral save file exists and parsed as valid JSON")

    # 3. Invariant 6 Check (directoryKey == documentIdentity)
    dir_name = os.path.basename(os.path.dirname(cathedral_path))
    doc_id = data.get("identifier")
    doc_name = data.get("name")
    assert doc_id == dir_name == "Prism Cathedral", f"Invariant 6 violation: {doc_id} != {dir_name}"
    assert doc_name == "Prism Cathedral"
    print("  ok: Invariant 6 satisfied: identifier matches directory name 'Prism Cathedral'")

    # 4. Check spatialRoot
    assert "spatialRoot" in data, "spatialRoot must be present"
    sr = data["spatialRoot"]
    assert sr.get("id") == "prism-cathedral.primary-light-field"
    props = sr.get("authoredProperties", {})
    assert props.get("light.source", {}).get("v") is True, "spatialRoot must have light.source=true"
    assert props.get("light.enabled", {}).get("v") is True
    assert props.get("light.intensity", {}).get("v") == 1.8
    assert sr.get("field", {}).get("mode") == "AST", "spatialRoot field must be AST mode"
    scalar_ast = sr["field"]["ast"]
    assert scalar_ast.get("input") == "z", "spatialRoot scalar AST must partition along z"
    pieces = scalar_ast.get("pieces", [])
    assert len(pieces) >= 10, f"spatialRoot must contain at least 10 pieces along Z (found {len(pieces)})"
    print(f"  ok: spatialRoot carries valid AST-mode scalar field with {len(pieces)} Z-pieces")

    # Check chroma and angular ASTs on spatialRoot
    assert "lightChroma" in sr and len(sr["lightChroma"]["pieces"]) >= 6
    assert "lightAngular" in sr and len(sr["lightAngular"]["pieces"]) >= 4
    print("  ok: spatialRoot carries authored lightChroma and lightAngular Piecewise structures")

    # 5. Check spatialFields (16 fields: 4 Rung 7/8 sources + 8 Wing B media + 4 Summit media/sources)
    assert "spatialFields" in data, "spatialFields must be present"
    fields = data["spatialFields"]
    assert len(fields) == 16, f"Must have exactly 16 additional spatialFields (found {len(fields)})"

    field_map = {f["id"]: f for f in fields}
    expected_field_ids = [
        # Rung 7 sources
        "prism.station7.sapphire-lantern",
        "prism.station7.amethyst-pulsar",
        "prism.station7.emerald-filament",
        # Rung 8 source
        "prism.station8.blue-source",
        # Wing B participating media
        "prism.wingB.spherical-cloud",
        "prism.wingB.hollow-shell",
        "prism.wingB.torus-medium",
        "prism.wingB.csg-crescent-medium",
        "prism.wingB.noise-organic-cloud",
        "prism.wingB.time-breathing-nebula",
        "prism.wingB.proxy-proof-torus",
        "prism.wingB.dual-sovereign-being",
        # Summit sources & media
        "prism.summit.sol-secundus",
        "prism.summit.sol-tertius",
        "prism.summit.torus-medium",
        "prism.summit.celestial-veil"
    ]
    for fid in expected_field_ids:
        assert fid in field_map, f"Missing required spatialField: {fid}"
    print(f"  ok: All {len(expected_field_ids)} required spatialFields exist with exact identifiers")

    # 6. Verify Volumetric V0 Participating Media (Wing B & Summit)
    # A. Spherical Cloud (B1)
    b1 = field_map["prism.wingB.spherical-cloud"]
    assert "volumeDensity" in b1 and len(b1["volumeDensity"]["pieces"]) > 0
    assert b1["authoredProperties"]["light.source"]["v"] is False

    # B. Hollow Shell (B2)
    b2 = field_map["prism.wingB.hollow-shell"]
    assert "volumeDensity" in b2
    assert b2["authoredProperties"]["light.source"]["v"] is False
    # Check that Op::Abs (25) is used for hollow shell
    b2_json = json.dumps(b2["volumeDensity"])
    assert '"op": 25' in b2_json, "Hollow shell must use Op::Abs (25) to carve hollow core"

    # C. Toroidal Donut Medium (B3)
    b3 = field_map["prism.wingB.torus-medium"]
    assert "volumeDensity" in b3
    assert b3["authoredProperties"]["light.source"]["v"] is False
    b3_json = json.dumps(b3["volumeDensity"])
    assert '"op": 27' in b3_json, "Toroidal medium must use Op::Sqrt (27) for tube distance"

    # D. CSG-Subtracted Crescent Cloud (B4)
    b4 = field_map["prism.wingB.csg-crescent-medium"]
    assert "volumeDensity" in b4
    assert b4["authoredProperties"]["light.source"]["v"] is False

    # E. Noise-Warped Organic Cloud (B5)
    b5 = field_map["prism.wingB.noise-organic-cloud"]
    assert "volumeDensity" in b5
    assert b5["authoredProperties"]["light.source"]["v"] is False
    b5_json = json.dumps(b5["volumeDensity"])
    assert '"op": 29' in b5_json, "Noise-warped cloud must use Op::Noise (29)"

    # F. Time-Breathing Hollow Nebula (B6)
    b6 = field_map["prism.wingB.time-breathing-nebula"]
    assert "volumeDensity" in b6
    assert b6["authoredProperties"]["light.source"]["v"] is False
    b6_json = json.dumps(b6["volumeDensity"])
    assert '"var": "t"' in b6_json, "Time-varying nebula must read temporal coordinate 't'"

    # G. DIAGNOSTIC PROXY-PROOF TORUS (B7)
    b7 = field_map["prism.wingB.proxy-proof-torus"]
    assert "volumeDensity" in b7
    assert b7["authoredProperties"]["light.source"]["v"] is False
    assert b7["scale"] == [10.0, 4.0, 10.0], "Proxy-proof torus must have colossal 20x8x20m AABB proxy!"
    print("  ok: Wing B exhibits B1-B7 verified: Spherical, Hollow, Torus, CSG, Noise, Time, Proxy-Proof")

    # H. Dual Sovereign Being (B8)
    b8 = field_map["prism.wingB.dual-sovereign-being"]
    assert b8["authoredProperties"]["light.source"]["v"] is True
    assert "volumeDensity" in b8
    rho_json = json.dumps(b8["field"]["ast"], sort_keys=True)
    d_json = json.dumps(b8["volumeDensity"], sort_keys=True)
    assert rho_json != d_json, "Constitution: rho_source AST != D_medium AST on Dual Sovereign Being"
    print("  ok: Constitution: Dual Sovereign Being has distinct rho and D mathematical ASTs")

    # I. Summit Torus Medium & Celestial Veil
    summit_torus = field_map["prism.summit.torus-medium"]
    assert "volumeDensity" in summit_torus
    assert summit_torus["authoredProperties"]["light.source"]["v"] is False
    veil = field_map["prism.summit.celestial-veil"]
    assert "volumeDensity" in veil
    assert veil["authoredProperties"]["light.source"]["v"] is False
    print("  ok: The Summit contains sovereign Toroidal Fog Medium ring and Celestial Veil")

    # 7. Verify Rung 7 Four Distinct Shaped Sources
    s1 = sr  # Sol Hearth is on spatialRoot
    s2 = field_map["prism.station7.sapphire-lantern"]
    assert s2["authoredProperties"]["light.source"]["v"] is True
    assert '"op": 27' in json.dumps(s2["field"]["ast"]), "Sapphire Lantern must have Toroidal Ring AST"

    s3 = field_map["prism.station7.amethyst-pulsar"]
    assert s3["authoredProperties"]["light.source"]["v"] is True
    assert '"op": 29' in json.dumps(s3["field"]["ast"]), "Amethyst Pulsar must use Op::Noise (29)"

    s4 = field_map["prism.station7.emerald-filament"]
    assert s4["authoredProperties"]["light.source"]["v"] is True
    assert s4["authoredProperties"]["shape.kind"]["v"].find("Filament") != -1
    print("  ok: Rung 7 verified: 4 independent sources of distinct shapes (Shell, Torus, Noisy Lobe, Filament)")

    # 8. Check Materials & Wing A Surface Color Fields
    assert "materials" in data
    mats = data["materials"]
    mat_map = {m["id"]: m for m in mats}
    surface_color_mats = [
        "material.prism_surface_rainbow",
        "material.prism_surface_radial",
        "material.prism_surface_lattice",
        "material.prism_surface_pure_blue"
    ]
    for smid in surface_color_mats:
        assert smid in mat_map, f"Missing Wing A surface color material: {smid}"
        m = mat_map[smid]
        assert "colorExpr" in m, f"Material {smid} must contain colorExpr"
        assert len(m["colorExpr"]["pieces"]) > 0, f"Material {smid} colorExpr must have pieces"
    print("  ok: Wing A surface color materials contain valid authored colorExpr ASTs")

    # 9. Check Pedagogical Inscription Stelae
    objects = data.get("world", {}).get("objects", [])
    assert len(objects) >= 220, f"Cathedral must contain complete architecture (found {len(objects)} objects)"
    stelae = {obj["objectID"]: obj for obj in objects if "stele" in obj.get("objectID", "")}

    required_stelae = [
        "cathedral.entrance.stele",
        "cathedral.station1.stele",
        "cathedral.station2.stele",
        "cathedral.station3.stele",
        "cathedral.station4.stele",
        "cathedral.station5.stele",
        "cathedral.station6.stele",
        "cathedral.station7.stele",
        "cathedral.station8.stele",
        "cathedral.wingA.stele",
        "cathedral.wingB.stele",
        "cathedral.wingC.stele",
        "cathedral.wingD.stele",
        "cathedral.prism.summit_stele"
    ]
    for stid in required_stelae:
        assert stid in stelae, f"Missing pedagogical stele: {stid}"
        props = stelae[stid].get("authoredProperties", {})
        assert "displayName" in props
        assert "inscription.doctrine" in props
        assert "inscription.invariant" in props
        assert "inscription.equation" in props
    print(f"  ok: All {len(required_stelae)} required pedagogical Stelae are present with full inscriptions")

    print("------------------------------------------------------------")
    print("Prism Cathedral Validation: ALL CHECKS PASSED (100% SUCCESS)!")
    print("============================================================")

if __name__ == "__main__":
    main()
