vec3 voxelIndex(vec3 pos) {
    vec3 range = voxelMax - voxelMin;
    pos -= voxelCenter;

    vec3 unit = (pos - voxelMin) / range;
    unit.z = 1 - unit.z;

    if (voxelWarp) {
        unit = smoothstep(0, 1, unit);
    }

    return voxelDim * unit;
}