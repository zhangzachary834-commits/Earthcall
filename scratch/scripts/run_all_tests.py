#!/usr/bin/env python3
import os
import subprocess
import sys

BUILD_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '../../build'))

TESTS = [
    "property_bridge_test",
    "law_model_test",
    "law_audit_test",
    "law_loop_test",
    "change_recorder_test",
    "object_concept_test",
    "law_graph_test",
    "metalaw_test",
    "ontomath_test",
    "ontomath_sounding_test",
    "infrasound_floor_test",
    "continuous_law_test",
    "time_flow_test",
    "law_persistence_test",
    "geometry_cache_test",
    "glu_compat_test",
    "zone_being_test",
    "formation_merge_test",
    "material_being_test",
    "material_render_test",
    "primitive_render_test",
    "action_spawn_test",
    "binary_packing_test",
    "basic_cube_law_test",
    "bezier_patch_law_test",
    "shape_generator_law_test",
    "singular_set_to_set_test",
    "formation_topology_test",
    "first_mover_test",
    "identity_test",
    "person_migration_test",
    "channel_paths_test",
    "constitution_test",
    "ground_plane_test",
    "law_creation_test",
    "logos_modality_test",
    "paint_test",
    "rete_compile_test",
    "test_field",
    "webgpu_object_test",
    "webgpu_sdf_parity_test",
    "object_roundtrip_test",
    "foreign_integration_test",
    "frontier_test",
    "serialization_compat_test",
    "law_reversal_test"
]

def main():
    passed = 0
    failed = 0
    missing = 0
    failed_names = []

    print(f"Running tests from {BUILD_DIR}...", flush=True)
    for t in TESTS:
        test_path = os.path.join(BUILD_DIR, t)
        if not os.path.exists(test_path):
            print(f"  [MISSING] {t}", flush=True)
            missing += 1
            continue
        try:
            res = subprocess.run([test_path], capture_output=True, text=True, timeout=5)
            if res.returncode == 0:
                print(f"  [PASS] {t}", flush=True)
                passed += 1
            else:
                print(f"  [FAIL] {t} (exit {res.returncode})", flush=True)
                failed += 1
                failed_names.append(t)
        except subprocess.TimeoutExpired:
            print(f"  [TIMEOUT] {t}", flush=True)
            failed += 1
            failed_names.append(f"{t}(timeout)")
        except Exception as e:
            print(f"  [ERROR] {t}: {e}", flush=True)
            failed += 1
            failed_names.append(f"{t}(error)")

    print("-" * 50, flush=True)
    print(f"Summary: {passed} passed, {failed} failed, {missing} missing out of {len(TESTS)}", flush=True)
    if failed_names:
        print(f"Failed tests: {', '.join(failed_names)}", flush=True)
    print("-" * 50, flush=True)
    return 0 if failed == 0 else 1

if __name__ == '__main__':
    sys.exit(main())
