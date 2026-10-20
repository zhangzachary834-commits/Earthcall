"""Independently decode native captures; never generate replacement images.

Codex / GPT-6.1 Sol / 01a10a2b-a247-7c11-9d5f-7a8b89df6cfc /
2026-10-17T16:27:11-07:00. Zach requested property-only Screen sources.
Run with the bundled workspace Python (Pillow and NumPy), after the native probe.
"""
from pathlib import Path
import json
import sys
from PIL import Image
import numpy as np

root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[1] / "verification/singular-screen-properties-2026-10-17"
report = {}
for bearer in ("singular", "object"):
    for phase in ("bands", "edited", "pixel"):
        path = root / f"property-screen-{bearer}-{phase}.png"
        actual = np.asarray(Image.open(path).convert("RGB"))
        height, width, _ = actual.shape
        expected = np.zeros((height, width, 3), dtype=np.uint8)
        expected[:, :, 2] = 255
        if phase == "pixel":
            expected[9, 7] = [255, 0, 0]
        else:
            # Integer intervals are independent of the authored polynomial AST.
            assert width % 8 == 0
            for start, stop in ((width // 8, width // 4), (5 * width // 8, 3 * width // 4)):
                expected[:, start:stop, 0] = np.rint(255 * (np.arange(start, stop) + .5) / width).astype(np.uint8)[None, :]
                expected[:, start:stop, 1] = np.rint(255 * (np.arange(height) + .5) / height).astype(np.uint8)[:, None]
                expected[:, start:stop, 2] = 64 if phase == "bands" else 191
        error = int(np.abs(actual.astype(np.int16) - expected.astype(np.int16)).max())
        red = int(np.all(actual == [255, 0, 0], axis=-1).sum())
        assert error == 0, (path, error)
        if phase == "pixel":
            assert red == 1
        report[path.name] = dict(width=width, height=height, pixelsChecked=width * height,
                                 maxRgbByteError=error, redPixels=red)
(root / "independent-png-check.json").write_text(json.dumps(report, indent=2) + "\n")
print(f"PASS: {len(report)} native captures; {sum(item['pixelsChecked'] for item in report.values())} RGB pixels; zero byte error.")
