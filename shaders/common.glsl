const float PI = 3.1415982;

// Returns position of a voxel in texture coordinates. worldPosition assumed inside the voxel volume.
vec3 voxelLinearPosition(vec3 worldPosition, vec3 voxelCenter, vec3 voxelMin, vec3 voxelMax) {
    vec3 tc = (worldPosition - voxelCenter - voxelMin) / (voxelMax - voxelMin);
    return tc;
}

vec3 voxelWarpFn(vec3 x) {
    // if (any(notEqual(unit, clamp(unit, 0, 1)))) return unit;

    const float alpha = 0.25;
    x = alpha * x + (3 - 3 * alpha) * x * x + (2 * alpha - 2) * x * x * x;

    return clamp(x, 0, 1);
}

vec3 voxelWarp(vec3 p_tc, vec3 c_tc) {
    vec3 offset = p_tc - c_tc;
    offset = 0.5 * offset + 0.5;
    offset = voxelWarpFn(offset);
    offset = 2 * offset - 1;

    return c_tc + offset;
}

// position and camera are in world space
vec3 voxelWarpedPosition(vec3 position, vec3 camera, vec3 voxelCenter, vec3 voxelMin, vec3 voxelMax) {
    vec3 p_tc = voxelLinearPosition(position, voxelCenter, voxelMin, voxelMax);
    vec3 c_tc = voxelLinearPosition(camera, voxelCenter, voxelMin, voxelMax);

    return voxelWarp(p_tc, c_tc);
}

vec3 voxelIndex(vec3 pos, int voxelDim, vec3 voxelCenter, vec3 voxelMin, vec3 voxelMax, bool warpVoxels) {
    if (warpVoxels) {
        pos = voxelWarpedPosition(pos, eye, voxelCenter, voxelMin, voxelMax);
    }
    else if (warpTexture) {
        vec3 tc = voxelLinearPosition(pos, voxelCenter, voxelMin, voxelMax);
        pos = texture(warpmap, tc).xyz;
    }
    else {
        pos = voxelLinearPosition(pos, voxelCenter, voxelMin, voxelMax);
    }

    return voxelDim * pos;
}

vec3 linearVoxelSize(int voxelDim, vec3 voxelMin, vec3 voxelMax) {
    return (voxelMax - voxelMin) / float(voxelDim);
}

vec3 voxelWarpFnGradient(vec3 p_tc, vec3 c_tc) {
    const float h = 0.01;
    const vec3 offsets[] = vec3[](
        vec3(h, 0, 0),
        vec3(0, h, 0),
        vec3(0, 0, h)
    );

    vec3 warpedPosition = voxelWarp(p_tc, c_tc);
    vec3 gradient;
    for (int i = 0; i < 3; i++) {
        vec3 partial = (voxelWarp(p_tc + offsets[i], c_tc) - warpedPosition) / h;
        gradient[i] = partial[i];
    }

    return gradient;
}

vec3 voxelWarpFnGradient(vec3 position, vec3 camera, vec3 voxelCenter, vec3 voxelMin, vec3 voxelMax) {
    vec3 p_tc = voxelLinearPosition(position, voxelCenter, voxelMin, voxelMax);
    vec3 c_tc = voxelLinearPosition(camera, voxelCenter, voxelMin, voxelMax);

    return voxelWarpFnGradient(p_tc, c_tc);
}

vec3 voxelWarpTextureGradient(vec3 p_tc) {
    vec3 warpedPosition = texture(warpmap, p_tc).xyz;
    const ivec3 offsets[] = ivec3[](
        ivec3(1, 0, 0),
        ivec3(0, 1, 0),
        ivec3(0, 0, 1)
    );
    vec3 texelSize = 1.0 / textureSize(warpmap, 0);
    vec3 gradient;
    for (int i = 0; i < 3; i++) {
        vec3 partial = (textureOffset(warpmap, p_tc, offsets[i]).xyz - warpedPosition) / texelSize;
        gradient[i] = partial[i];
    }

    return gradient;
}

// Approximate voxel size at a given position (derived from gradient)
// vec3 warpedVoxelSize(vec3 unit, int voxelDim, vec3 voxelMin, vec3 voxelMax) {
//     return voxelWarpFnGradient(unit) * linearVoxelSize(voxelDim, voxelMin, voxelMax);
// }
