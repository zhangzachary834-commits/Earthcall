"""Surveyor: isolated source patch that removes the 2304-tile / 32784-ray pin from the 6em clean harness.

Claude / Sonnet 5.5 (claude-sonnet-5-5) / session 06f17252-810f-48ee-ad18-95ed12c6ff27 / 2026-10-27.
Pure text transform of the generated C++ (no GPU, no edit of any shared file).  The patched harness
  1. reads tile_offsets / tile_ray_indices from the JSON named by EC_SURVEYOR_LAYOUT (built and validated in
     sonnet_surveyor_layout.py) instead of recomputing `px/10/4` and demanding exactly 2304 tiles;
  2. re-validates them in C++ (framing, monotone offsets, exact permutation, <=16 rays/tile) so a corrupted
     file fails loudly instead of silently dropping rays the way `rayInTile >= rayCount` would;
  3. checks the real schedule buffers against wgpuDeviceGetLimits() before any allocation.
GPU arithmetic, shaders and routing are untouched.  EC_GPU_SCALE>1 remains the legacy exact-replication mode
and is refused when a surveyor layout is active (distinct rays are never replicated).
"""
import hashlib, sys

PIN_BEGIN = '    std::map<uint32_t, std::vector<uint32_t>> groups4x4;\n'
PIN_END = '    baseTileOffsets[2304] = uint32_t(baseTileRayIndices.size());\n'
TILECOUNT_OLD = '    uint32_t baseTileCount = 2304;\n'
GUARD_ANCHOR = '    WGPUBuffer uBuf = makeBuffer(WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst, sizeof(udata), &udata);\n'   # first schedule allocation, after the device exists

LAYOUT_CPP = r'''    // SURVEYOR: data-driven tile layout (sonnet_surveyor_layout.py); replaces the 2304-tile pin.
    std::vector<uint32_t> baseTileOffsets;
    std::vector<uint32_t> baseTileRayIndices;
    {
        const char* layoutPath = std::getenv("EC_SURVEYOR_LAYOUT");
        if (!layoutPath) throw std::runtime_error("EC_SURVEYOR_LAYOUT required");
        J layout; { std::ifstream lin(layoutPath); lin >> layout; }
        baseTileOffsets = layout.at("tile_offsets").get<std::vector<uint32_t>>();
        baseTileRayIndices = layout.at("tile_ray_indices").get<std::vector<uint32_t>>();
        if (baseTileOffsets.size() < 2 || baseTileOffsets.size() - 1 != layout.at("tile_count").get<size_t>())
            throw std::runtime_error("surveyor layout: tile_count framing");
        if (baseTileOffsets.front() != 0 || baseTileOffsets.back() != baseTileRayIndices.size() || baseTileRayIndices.size() != count)
            throw std::runtime_error("surveyor layout: offsets do not cover exactly the captured rays");
        std::vector<uint8_t> seen(count, 0);
        for (size_t t = 0; t + 1 < baseTileOffsets.size(); ++t) {
            if (baseTileOffsets[t] > baseTileOffsets[t + 1]) throw std::runtime_error("surveyor layout: offsets not monotone");
            if (baseTileOffsets[t + 1] - baseTileOffsets[t] > 16u) throw std::runtime_error("surveyor layout: tile over 16-ray capacity");
        }
        for (uint32_t r : baseTileRayIndices) {
            if (r >= count || seen[r]) throw std::runtime_error("surveyor layout: index out of range or repeated");
            seen[r] = 1;
        }
    }
'''

GUARD_CPP = r'''    {   // SURVEYOR: validated capacities against the actual device limits (device was created with adapter limits).
        WGPULimits lim{};
        if (wgpuDeviceGetLimits(gpu.device, &lim) != WGPUStatus_Success) throw std::runtime_error("surveyor: device limits unavailable");
        const uint64_t perBuffer[] = {uint64_t(rayBytes), uint64_t(subpanelBytes), uint64_t(overlapBytes),
                                      uint64_t(count) * sizeof(GpuResult), uint64_t(count) * 4u};
        for (uint64_t b : perBuffer)
            if (b > lim.maxStorageBufferBindingSize || b > lim.maxBufferSize)
                throw std::runtime_error("surveyor: buffer of " + std::to_string(b) + " B exceeds device limits");
        if ((uint64_t(count)+63u)/64u > lim.maxComputeWorkgroupsPerDimension ||
            (uint64_t(numTiles)+63u)/64u > lim.maxComputeWorkgroupsPerDimension ||
            (uint64_t(numTiles)+3u)/4u > lim.maxComputeWorkgroupsPerDimension)
            throw std::runtime_error("surveyor: dispatch workgroups exceed device limits");
        if (uint64_t(count) * std::max(maxSubPerRay, maxOverPerRay) > 0xFFFFFFFFull) throw std::runtime_error("surveyor: u32 ray-slot index overflow");
        std::cerr << "SURVEYOR budget ok: rays=" << count << " tiles=" << numTiles << " subpanelBytes=" << subpanelBytes
                  << " maxStorageBufferBindingSize=" << lim.maxStorageBufferBindingSize << std::endl;
    }
'''


def patch(src):
    """Return patched source; every anchor must match exactly once or the transform refuses."""
    for anchor in (PIN_BEGIN, PIN_END, TILECOUNT_OLD, GUARD_ANCHOR):
        if src.count(anchor) != 1:
            raise ValueError('anchor mismatch (generated harness changed?): ' + anchor.strip()[:60])
    b, e = src.index(PIN_BEGIN), src.index(PIN_END) + len(PIN_END)
    if not b < e:
        raise ValueError('anchor order')
    src = src[:b] + LAYOUT_CPP + src[e:]
    src = src.replace(TILECOUNT_OLD, '    uint32_t baseTileCount = uint32_t(baseTileOffsets.size() - 1);\n'
                      '    if (scaleK != 1) throw std::runtime_error("surveyor: distinct native rays are never replicated (EC_GPU_SCALE must be 1)");\n')
    # guard needs numTiles/maxSubPerRay/rayBytes (defined earlier in main) and gpu (created just before the anchor)
    src = src.replace(GUARD_ANCHOR, GUARD_CPP + GUARD_ANCHOR)
    return src


def selftest(src):
    out = patch(src)
    assert 'requires exactly 2304' not in out and 'groups4x4' not in out and 'baseTileOffsets[2304]' not in out
    assert out.count('getenv("EC_SURVEYOR_LAYOUT")') == 1 and out.count('wgpuDeviceGetLimits') == 1
    return hashlib.sha256(out.encode()).hexdigest()


if __name__ == '__main__':
    print('library module; use sonnet_surveyor_capture.py', file=sys.stderr)
