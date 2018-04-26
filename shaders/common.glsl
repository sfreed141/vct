float sumGeometricSeries(float a, float r, float n) {
	return a * (1 - pow(r, n + 1)) / (1 - r);
}

float calculateBaseVoxelSize(float origin, float voxelMax, int voxelDim) {
	return (voxelMax - origin) / sumGeometricSeries(1, exp2(1 / (voxelDim - 1)), voxelDim - 1);
}

vec3 calculateVoxelPosition(vec3 d, int voxelDim, float baseSize) {
	float r = exp2(1 / (voxelDim - 1));
	return 3 * log2(1 - d / baseSize * (1 - r));
}

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
