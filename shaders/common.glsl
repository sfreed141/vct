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
    if (any(greaterThan(unit, vec3(1))) || any(lessThan(unit, vec3(0)))) return unit;

    // return smoothstep(0, 1, unit);
    const float alpha = 0.25;
    // float x = unit.x;
    // unit.x = alpha * x + 3 * (1 - alpha) * x * x - x * x * x;
    unit = alpha * unit + (3 - 3 * alpha) * unit * unit - unit * unit * unit;
    return clamp(unit, 0, 1);
}

vec3 voxelWarpFnGradient(vec3 unit) {
    const float h = 0.01;
    const vec3 offsets[] = vec3[](
        vec3(h, 0, 0),
        vec3(0, h, 0),
        vec3(0, 0, h)
    );

    vec3 gradient;
    for (int i = 0; i < 3; i++) {
        vec3 partial = (voxelWarpFn(unit + offsets[i]) - voxelWarpFn(unit)) / h;
        gradient[i] = partial[i];
    }

    return gradient;
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

vec3 linearVoxelSize(int voxelDim, vec3 voxelMin, vec3 voxelMax) {
    return (voxelMax - voxelMin) / float(voxelDim);
}

// Approximate voxel size at a given position (derived from gradient)
vec3 warpedVoxelSize(vec3 unit, int voxelDim, vec3 voxelMin, vec3 voxelMax) {
    return voxelWarpFnGradient(unit) * linearVoxelSize(voxelDim, voxelMin, voxelMax);
}
