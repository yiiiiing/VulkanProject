#version 450

// input
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 lightDirection;


const float DIFFUSE_INTENSITY = 1.0;
const float AMBIENT_INTENSITY = 0.02;

// uniform
layout(binding = 1) uniform sampler2D texSampler;

// output
layout(location = 0) out vec4 outColor;

// calculate diffuse intensity
float diffuse(vec3 surfaceNormal, vec3 lightDirection){
    return max(DIFFUSE_INTENSITY * dot(surfaceNormal, lightDirection), 0);
}

void main() {
    float intensity = AMBIENT_INTENSITY + diffuse(normal, lightDirection);
    outColor = texture(texSampler, fragTexCoord);
}
