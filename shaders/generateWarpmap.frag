#version 430 core

out vec4 warpedOutput;  // tc.xyz, occupied

in VS_OUT {
    vec2 tc;
} fs_in;

layout(binding = 0, r32ui) uniform readonly uimage3D voxelOccupancy;
layout(binding = 1, rgba32i) uniform readonly iimage3D warpPartials;
layout(binding = 2, r32f) uniform readonly image2D warpWeights;

layout(binding = 0) uniform sampler3D warpmapWeightsLow;
layout(binding = 1) uniform sampler3D warpmapWeightsHigh;

uniform bool toggle = false;
uniform bool warpTextureLinear = false;
uniform bvec3 warpTextureAxes = bvec3(true, true, true);
uniform bool useWarpmapWeightsTexture = false;
uniform float maxWeight = 2.0;

uniform int warpDim = 32;
uniform int layerOffset = 0;

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

vec3 calculateWarpedPosition(vec3 tc) {
    vec3 linearTexcoord = tc * warpDim;   // convert [0, 1] -> [0, warpDim] for indexing into warp texture
    vec3 warpCellIndex;
    vec3 warpCellPosition = modf(linearTexcoord, warpCellIndex);
    int x = int(warpCellIndex.x), y = int(warpCellIndex.y), z = int(warpCellIndex.z);

    bool warpCellOccupied = imageLoad(voxelOccupancy, ivec3(x, y, z)).r > 0;
    if (warpCellOccupied) warpedOutput.w = 1;

    vec3 totalOccupied = vec3(
        imageLoad(warpPartials, ivec3(warpDim - 1, y, z)).x,
        imageLoad(warpPartials, ivec3(x, warpDim - 1, z)).y,
        imageLoad(warpPartials, ivec3(x, y, warpDim - 1)).z
    );
    vec3 partialSum = imageLoad(warpPartials, ivec3(x, y, z)).xyz;

    vec3 warpCellWeightsLow, warpCellWeightsHigh;
    if (useWarpmapWeightsTexture) {
        warpCellWeightsLow = texture(warpmapWeightsLow, tc).xyz; // * maxWeight;
        warpCellWeightsHigh = texture(warpmapWeightsHigh, tc).xyz; // * maxWeight;
    }
    else {
        warpCellWeightsLow = vec3(
        imageLoad(warpWeights, ivec2(totalOccupied.x, 0)).r,
        imageLoad(warpWeights, ivec2(totalOccupied.y, 0)).r,
        imageLoad(warpWeights, ivec2(totalOccupied.z, 0)).r
    );
        warpCellWeightsHigh = vec3(
        imageLoad(warpWeights, ivec2(totalOccupied.x, 1)).r,
        imageLoad(warpWeights, ivec2(totalOccupied.y, 1)).r,
        imageLoad(warpWeights, ivec2(totalOccupied.z, 1)).r
    );
    }
    vec3 warpCellResolution = warpCellOccupied ? warpCellWeightsHigh : warpCellWeightsLow;
    warpedOutput.w = packCellInfo(totalOccupied, warpCellOccupied);

    // if totalOccupied = 0 or warpDim then it doesn't matter which we choose (edge case for all linear cells)
    totalOccupied = mix(vec3(warpDim), totalOccupied, equal(totalOccupied, vec3(0)));
    vec3 previousPartial = warpCellOccupied ? partialSum - vec3(1) : partialSum;
    vec3 warpCellOffset = warpCellWeightsLow * (warpCellIndex - previousPartial) + warpCellWeightsHigh * previousPartial;
    vec3 warpCellInnerOffset = warpCellPosition * warpCellResolution;

    vec3 warpedTexcoord = (warpCellOffset + warpCellInnerOffset) / warpDim;
    return warpedTexcoord;
}

void main() {
    vec3 tc = vec3(fs_in.tc, float(gl_Layer + layerOffset + 0.5) / float(warpDim));
    vec3 warpedTexcoord = calculateWarpedPosition(tc);
    warpedOutput.xyz = mix(tc, warpedTexcoord, warpTextureLinear ? bvec3(false) : warpTextureAxes);

    // Debugging
    // warpedOutput.xyz = mix(tc, warpedTexcoord, toggle);
    // ivec3 index = ivec3(floor(tc * warpDim));
    // warpedOutput.xyz = mix(vec3(0), vec3(1), greaterThan(imageLoad(voxelOccupancy, index).rrr, vec3(0))); // draw occupied cells
    // warpedOutput.xyz = imageLoad(warpPartials, index).zzz / float(warpDim); // draw partials
}
