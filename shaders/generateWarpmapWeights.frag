#version 430 core

layout(location = 0) out vec4 warpWeightsLow;
layout(location = 1) out vec4 warpWeightsHigh;

in VS_OUT {
    vec2 tc;
} fs_in;

layout(binding = 0, r32ui) uniform readonly uimage3D voxelOccupancy;
layout(binding = 1, rgba32i) uniform readonly iimage3D warpPartials;
layout(binding = 2, r32f) uniform readonly image2D warpWeights;

uniform int warpDim = 32;
uniform int layerOffset = 0;
uniform float maxWeight = 2.0;

float packCellInfo(vec3 totalOccupied, bool warpCellOccupied) {
    uint bits = (uint(totalOccupied.x) & 0x001F)
        | (uint(totalOccupied.y) & 0x001F) << 5
        | (uint(totalOccupied.z) & 0x001F) << 10
        | (warpCellOccupied ? 1 : 0) << 15;

    return float(bits) / 65535.0;
}

vec4 unpackCellInfo(float bits) {
    uint data = uint(bits * 65535.0);
    return vec4(
        float((data & 0x001F)),
        float((data & 0x03D0) >> 5),
        float((data & 0x7C00) >> 10),
        float((data & 0x8000) >> 15)
    );
}

vec3 calculateWarpWeights(vec3 tc) {
    vec3 linearTexcoord = tc * warpDim;   // convert [0, 1] -> [0, warpDim] for indexing into warp texture
    vec3 warpCellIndex;
    vec3 warpCellPosition = modf(linearTexcoord, warpCellIndex);
    int x = int(warpCellIndex.x), y = int(warpCellIndex.y), z = int(warpCellIndex.z);

    bool warpCellOccupied = imageLoad(voxelOccupancy, ivec3(x, y, z)).r > 0;

    vec3 totalOccupied = vec3(
        imageLoad(warpPartials, ivec3(warpDim - 1, y, z)).x,
        imageLoad(warpPartials, ivec3(x, warpDim - 1, z)).y,
        imageLoad(warpPartials, ivec3(x, y, warpDim - 1)).z
    );

    vec3 warpCellWeightsLow = vec3(
        imageLoad(warpWeights, ivec2(totalOccupied.x, 0)).r,
        imageLoad(warpWeights, ivec2(totalOccupied.y, 0)).r,
        imageLoad(warpWeights, ivec2(totalOccupied.z, 0)).r
    );
    vec3 warpCellWeightsHigh = vec3(
        imageLoad(warpWeights, ivec2(totalOccupied.x, 1)).r,
        imageLoad(warpWeights, ivec2(totalOccupied.y, 1)).r,
        imageLoad(warpWeights, ivec2(totalOccupied.z, 1)).r
    );
    vec3 warpCellResolution = warpCellOccupied ? warpCellWeightsHigh : warpCellWeightsLow;
    // warpWeightsLow.w = warpWeightsHigh.w = packCellInfo(totalOccupied, warpCellOccupied);
    warpWeightsLow.w = warpCellOccupied ? 0 : 1;
    warpWeightsHigh.w = warpCellOccupied ? 1 : 0;
    warpWeightsLow.xyz = warpCellWeightsLow; // / maxWeight;
    warpWeightsHigh.xyz = warpCellWeightsHigh; // / maxWeight;

    return warpCellResolution;
}

void main() {
    vec3 tc = vec3(fs_in.tc, float(gl_Layer + layerOffset + 0.5) / float(warpDim));
    calculateWarpWeights(tc);
}
