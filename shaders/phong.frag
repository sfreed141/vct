#version 450 core

#define NORMAL_MAP

in VS_OUT {
    vec3 fragPosition;
    vec3 fragNormal;
    vec2 fragTexcoord;

#ifdef NORMAL_MAP
    vec3 tangentLightPos;
    vec3 tangentViewPos;
    vec3 tangentFragPos;
#endif
} fs_in;

// Diffuse color
uniform sampler2D texture0;

#ifdef NORMAL_MAP
layout(binding = 5) uniform sampler2D normalMap;
#endif

uniform sampler2D shadowmap;

uniform sampler3D voxelColor;
uniform sampler3D voxelNormal;
uniform sampler3D voxelRadiance;

uniform bool voxelize = false;
uniform bool normals = false;
uniform bool dominant_axis = false;
uniform bool radiance = false;

uniform bool enableShadows = true;
uniform bool enableNormalMap = true;
uniform bool enableIndirect = false;
uniform bool enableDiffuse = true;
uniform bool enableSpecular = true;
uniform float ambientScale = 0.2;

uniform float shine;

uniform vec3 eye;
uniform vec3 lightPos;
uniform vec3 lightInt;
uniform mat4 ls;

uniform int miplevel = 0;
uniform int voxelDim;

uniform int vctSteps;
uniform float vctConeAngle;
uniform float vctBias;
uniform float vctConeInitialHeight;
uniform float vctLodOffset;

out vec4 color;

// based on https://github.com/godotengine/godot/blob/master/drivers/gles3/shaders/scene.glsl
// experiment with cone aperture, lod scaling, steps vs distance vs alpha
vec3 traceCone(sampler3D voxelTexture, vec3 position, vec3 direction, int steps) {
	// const float bias = 1.0;
	float bias = vctBias;

	direction /= voxelDim;
	vec3 start = position + bias * direction;
	
	float coneAngle = vctConeAngle;
	// float coneTanHalfAngle = tan(coneAngle / 2.0);

	float coneHeight = vctConeInitialHeight;
	float coneRadius;

	vec3 color = vec3(0);
	float alpha = 0;

	for (int i = 0; i < steps && alpha < 0.95; i++) {
		coneRadius = coneHeight * tan(coneAngle / 2.0);
		float lod = log2(max(1.0, 2 * coneRadius));
		vec4 sampleColor = textureLod(voxelTexture, start + coneHeight * direction, lod + vctLodOffset);
		// float a = 1 - alpha;
		// color += sampleColor.rgb * a;
		// alpha += a * sampleColor.a;
		color += sampleColor.rgb * sampleColor.a;
		coneHeight += coneRadius;
	}

	return color;
}

ivec3 voxelIndex(vec3 pos) {
    const float minx = -20, maxx = 20,
        miny = -20, maxy = 20,
        minz = -20, maxz = 20;

    float rangex = maxx - minx;
    float rangey = maxy - miny;
    float rangez = maxz - minz;

    float x = voxelDim * ((pos.x - minx) / rangex);
    float y = voxelDim * ((pos.y - miny) / rangey);
    float z = voxelDim * (1 - (pos.z - minz) / rangez);

    return ivec3(x, y, z);
}

float calcShadowFactor(vec3 lsPosition) {
	vec3 shifted = (lsPosition + 1.0) * 0.5;

	float shadowFactor = 0;
	float bias = 0.01;
	float fragDepth = shifted.z + bias;

	if (fragDepth > 1.0) {
		return 0;
	}

	vec2 sampleOffset = 1.0 / textureSize(shadowmap, 0);

	if (fragDepth > texture(shadowmap, shifted.xy).r)
		shadowFactor += 0.2;
	if (fragDepth > texture(shadowmap, shifted.xy + vec2(sampleOffset.x, sampleOffset.y)).r)
		shadowFactor += 0.2;
	if (fragDepth > texture(shadowmap, shifted.xy + vec2(sampleOffset.x, -sampleOffset.y)).r)
		shadowFactor += 0.2;
	if (fragDepth > texture(shadowmap, shifted.xy + vec2(-sampleOffset.x, sampleOffset.y)).r)
		shadowFactor += 0.2;
	if (fragDepth > texture(shadowmap, shifted.xy + vec2(-sampleOffset.x, -sampleOffset.y)).r)
		shadowFactor += 0.2;

    return shadowFactor;
}

void main() {
    color = texture(texture0, fs_in.fragTexcoord);

	vec3 norm, light, view;
#ifdef NORMAL_MAP
	if (enableNormalMap) {
		norm = texture(normalMap, fs_in.fragTexcoord).rgb;
		norm = normalize(norm * 2.0 - 1.0);
		light = normalize(fs_in.tangentLightPos - fs_in.tangentFragPos);
		view = normalize(fs_in.tangentViewPos - fs_in.tangentFragPos);
	}
	else
#endif
	{
		norm = normalize(fs_in.fragNormal);
		light = normalize(lightPos - fs_in.fragPosition);
		view = normalize(eye - fs_in.fragPosition);
	}

    vec3 h = normalize(view + light);
    float vdotr = max(dot(norm, h), 0);

    float ambient = ambientScale;
    float diffuse = max(dot(norm, light), 0);
    float specular = pow(vdotr, shine);

	float shadowFactor = 1.0;
	if (enableShadows) {
		shadowFactor = 1.0 - calcShadowFactor((ls * vec4(fs_in.fragPosition, 1.0)).xyz);
	}

    if (voxelize) {
		vec3 i = vec3(voxelIndex(fs_in.fragPosition)) / voxelDim;
		
		if (normals) {
			vec3 normal = normalize(textureLod(voxelNormal, i, miplevel).rgb);
			color = vec4(normal, 1);
		}
		else if (radiance) {
			color = textureLod(voxelRadiance, i, miplevel).rgba;
		}
		else {
			color = textureLod(voxelColor, i, miplevel).rgba;			
		}
    }
    else {
        if (normals) {
			color = vec4(norm, 1);
        }
        else if (dominant_axis) {
            norm = abs(norm);
            color = vec4(step(vec3(max(max(norm.x, norm.y), norm.z)), norm.xyz), 1);
        }
        else {
			float directLighting = 0.0;
			directLighting += enableDiffuse ? diffuse : 0.0;
			directLighting += enableSpecular ? specular : 0.0;
			
			if (enableIndirect) {
				vec3 voxelPosition = vec3(voxelIndex(fs_in.fragPosition)) / voxelDim;
				vec3 indirect = vec3(0);
				indirect += traceCone(radiance ? voxelRadiance : voxelColor, voxelPosition, norm, vctSteps);
				// indirect += textureLod(radiance ? voxelRadiance : voxelColor, voxelPosition, miplevel).rgb;
				indirect *= ambientScale;
				color = vec4(indirect + shadowFactor * directLighting * lightInt * color.rgb, 1);
			}
			else {
				color = vec4((ambient + shadowFactor * directLighting) * lightInt * color.rgb, 1);
			}

			#if 0
			vec3 voxelPosition = vec3(voxelIndex(fs_in.fragPosition)) / voxelDim;
			vec3 indirect = vec3(0);
			indirect += traceCone(voxelPosition, norm, 8);

			vec3 coneDirs[4] = vec3[] (
				vec3(0.707, 0.707, 0),
				vec3(0, 0.707, 0.707),
				vec3(-0.707, 0.707, 0),
				vec3(0, 0.707, -0.707)
			);
			float coneWeights[4] = float[](0.25, 0.25, 0.25, 0.25);
			for (int i = 0; i < 4; i++) {
				vec3 dir = normalize(norm * coneDirs[i]);
				indirect += coneWeights[i] * traceCone(voxelPosition, dir, 8);
			}

            color = vec4(indirect + shadowFactor * (diffuse + specular) * lightInt * color.rgb, 1);
			clamp(color.rgb, vec3(0), vec3(1));
			#endif
        }
    }
}
