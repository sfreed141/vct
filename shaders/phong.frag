#version 450 core

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexcoord;

// Diffuse color
uniform sampler2D texture0;

uniform sampler2D shadowmap;

//layout(binding = 1, rgba16f) uniform image3D voxelColor;
uniform sampler3D voxelColor;

uniform bool voxelize = false;
uniform bool normals = false;
uniform bool dominant_axis = false;
uniform bool enableShadows = true;

uniform float shine;

uniform vec3 eye;
uniform vec3 lightPos;
uniform vec3 lightInt;
uniform mat4 ls;

uniform int miplevel = 0;

out vec4 color;

// based on https://github.com/godotengine/godot/blob/master/drivers/gles3/shaders/scene.glsl
// experiment with cone aperture, lod scaling, steps vs distance vs alpha
vec3 traceCone(vec3 position, vec3 direction, int steps) {
	const float bias = 1.0;
	vec3 start = position + bias * direction;
	
	const float coneAngle = 0.785398163; // pi/4
	float coneTanHalfAngle = tan(coneAngle / 2.0);

	float coneHeight = 1;
	float coneRadius;

	vec3 color = vec3(0);
	float alpha = 0;

	for (int i = 0; i < steps && alpha < 0.95; i++) {
		coneRadius = coneHeight * coneTanHalfAngle;
		float lod = log2(max(1.0, 2 * coneRadius));
		vec4 sampleColor = textureLod(voxelColor, start + coneHeight * direction, lod);
		float a = 1 - alpha;
		color += sampleColor.rgb * a;
		alpha += a * sampleColor.a;
		coneHeight += coneRadius;
	}

	return color;
}

ivec3 voxelIndex(vec3 pos) {
    const float minx = -20, maxx = 20,
        miny = -20, maxy = 20,
        minz = -20, maxz = 20;
    const int voxelDim = 64;

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
    color = texture(texture0, fragTexcoord);

    vec3 norm = normalize(fragNormal);
    vec3 light = normalize(lightPos - fragPosition);
    vec3 view = normalize(eye - fragPosition);

    vec3 h = normalize(0.5 * (view + light));
    float vdotr = max(dot(norm, h), 0);

    float ambient = 0.2;
    float diffuse = max(dot(norm, light), 0);
    float specular = pow(vdotr, shine);

	float shadowFactor = 1.0;
	if (enableShadows) {
		shadowFactor = 1.0 - calcShadowFactor((ls * vec4(fragPosition, 1.0)).xyz);
	}

    if (voxelize) {
        ivec3 i = voxelIndex(fragPosition);
        //color = imageLoad(voxelColor, i).rgba;
		color = textureLod(voxelColor, vec3(i) / 64.0, miplevel).rgba;

		// TODO: hacky stuff to play around with
		//color.rgb = color.rgb / color.a;
		//color.a = 1;
		if (color.a > 1) color.a = 1;
		if (any(greaterThan(color.rgb, vec3(1)))) color.rgb = vec3(1, 0, 1);
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
            color = vec4((ambient + shadowFactor * (diffuse + specular)) * lightInt * color.rgb, 1);

			#if 0
			vec3 voxelPosition = vec3(voxelIndex(fragPosition)) / 64.0;
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
