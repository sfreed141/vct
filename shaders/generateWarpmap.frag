#version 430 core

out vec4 warpedOutput;  // tc.xyz, occupied

in VS_OUT {
    vec2 tc;
} fs_in;

const int warpDim = 32;

layout(binding = 0, r32ui) uniform readonly uimage3D warpTexture;
layout(binding = 1, rgba32i) uniform readonly iimage3D warpPartials;
layout(binding = 2, r32f) uniform readonly image2D warpWeights;

uniform bool toggle = false;

vec3 calculateWarpedPosition(vec3 tc) {
    vec3 linearTexcoord = tc * warpDim;   // convert [0, 1] -> [0, warpDim] for indexing into warp texture
    vec3 warpCellIndex;
    vec3 warpCellPosition = modf(linearTexcoord, warpCellIndex);
    int x = int(warpCellIndex.x), y = int(warpCellIndex.y), z = int(warpCellIndex.z);

    bool warpCellOccupied = imageLoad(warpTexture, ivec3(x, y, z)).r > 0;
    if (warpCellOccupied) warpedOutput.w = 1;

    vec3 totalOccupied = vec3(
        imageLoad(warpPartials, ivec3(warpDim - 1, y, z)).x,
        imageLoad(warpPartials, ivec3(x, warpDim - 1, z)).y,
        imageLoad(warpPartials, ivec3(x, y, warpDim - 1)).z
    );
    vec3 partialSum = imageLoad(warpPartials, ivec3(x, y, z)).xyz;

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

    // if totalOccupied = 0 or warpDim then it doesn't matter which we choose (edge case for all linear cells)
    // TODO replace with mix?
    if (totalOccupied.x == 0) totalOccupied.x = warpDim;
    if (totalOccupied.y == 0) totalOccupied.y = warpDim;
    if (totalOccupied.z == 0) totalOccupied.z = warpDim;
    vec3 previousPartial = warpCellOccupied ? partialSum - vec3(1) : partialSum;
    vec3 warpCellOffset = warpCellWeightsLow * (warpCellIndex - previousPartial) + warpCellWeightsHigh * previousPartial;
    vec3 warpCellInnerOffset = warpCellPosition * warpCellResolution;

    vec3 warpedTexcoord = (warpCellOffset + warpCellInnerOffset) / warpDim;
    return warpedTexcoord;
}

void main() {
    vec3 tc = vec3(fs_in.tc, float(gl_Layer + 0.5) / float(warpDim));
    vec3 warpedTexcoord = calculateWarpedPosition(tc);
    warpedOutput.xyz = warpedTexcoord;

    // Debugging
    // warpedOutput.xyz = mix(tc, warpedTexcoord, toggle);
    // ivec3 index = ivec3(floor(tc * warpDim));
    // warpedOutput.xyz = mix(vec3(0), vec3(1), greaterThan(imageLoad(warpTexture, index).rrr, vec3(0))); // draw occupied cells
    // warpedOutput.xyz = imageLoad(warpPartials, index).zzz / float(warpDim); // draw partials
}
