vec3 voxelWarpFn(vec3 unit) {
    return smoothstep(0, 1, unit);
}

vec3 voxelIndex(vec3 pos, int voxelDim, vec3 voxelCenter, vec3 voxelMin, vec3 voxelMax, bool voxelWarp) {
    vec3 range = voxelMax - voxelMin;
    pos -= voxelCenter;

    vec3 unit = (pos - voxelMin) / range;
    unit.z = 1 - unit.z;

    if (voxelWarp) {
        unit = voxelWarpFn(unit);
    }

    return voxelDim * unit;
}
