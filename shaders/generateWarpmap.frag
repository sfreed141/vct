#version 430 core

out vec4 warpedOutput;  // tc.x, tc.y, cell.x, cell.y
                        // storing the weights might be helpful for filtering?

in VS_OUT {
    vec2 tc;
} fs_in;

const int warpDim = 4;
uniform float warpTexture[warpDim * warpDim];
uniform float occupancyTexture[(warpDim * 2) * (warpDim * 2)];
uniform int warpPartialsX[warpDim * warpDim], warpPartialsY[warpDim * warpDim];
uniform float warpWeights[2 * (warpDim+1)];

uniform vec2 click;
uniform bool toggle;

vec2 calculateWarpedPosition(vec2 tc) {
    vec2 linearTexcoord = tc * warpDim;   // convert [0, 1] -> [0, warpDim] for indexing into warp texture
    vec2 warpCellIndex;
    vec2 warpCellPosition = modf(linearTexcoord, warpCellIndex);
    int x = int(warpCellIndex.x), y = int(warpCellIndex.y);
    // if (warpCellOccupied) color.g = 1;

    bool warpCellOccupied = warpTexture[y*warpDim + x] > 0.5;
    vec2 totalOccupied = vec2(warpPartialsX[y*warpDim + warpDim - 1], warpPartialsY[(warpDim - 1)*warpDim + x]);
    vec2 partialSum = vec2(warpPartialsX[y*warpDim + x], warpPartialsY[y*warpDim + x]);
    // color.r = totalOccupied.y / warpDim;

    vec2 warpCellWeightsLow = vec2(warpWeights[0*(warpDim+1)+int(totalOccupied.x)], warpWeights[0*(warpDim+1)+int(totalOccupied.y)]);
    vec2 warpCellWeightsHigh = vec2(warpWeights[1*(warpDim+1)+int(totalOccupied.x)], warpWeights[1*(warpDim+1)+int(totalOccupied.y)]);
    vec2 warpCellResolution = warpCellOccupied ? warpCellWeightsHigh : warpCellWeightsLow;

    warpedOutput.zw = warpCellResolution / 2.5;   // need to map [0, 1]. max weight right now is 2.5

    // if totalOccupied = 0 or warpDim then it doesn't matter which we choose (edge case for all linear cells)
    if (totalOccupied.x == 0) totalOccupied.x = warpDim;
    if (totalOccupied.y == 0) totalOccupied.y = warpDim;
    vec2 previousPartial = warpCellOccupied ? partialSum - vec2(1) : partialSum;
    vec2 warpCellOffset = warpCellWeightsLow * (warpCellIndex - previousPartial) + warpCellWeightsHigh * previousPartial;
    vec2 warpCellInnerOffset = warpCellPosition * warpCellResolution;
    // color.rb = warpCellOffset / warpDim;

    vec2 warpedTexcoord = (warpCellOffset + warpCellInnerOffset) / warpDim;
    return warpedTexcoord;
}

void main() {
    vec2 tc = fs_in.tc;
    vec2 warpedTexcoord = calculateWarpedPosition(tc);
    warpedOutput.xy = warpedTexcoord;
}