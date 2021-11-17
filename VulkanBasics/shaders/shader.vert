#version 450

// input
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

// uniform (same to each vertex)
layout(binding = 0) uniform UniformBufferObject {
    mat4 modelMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 transformMatrix;
} ubo;


// output
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 lightDirection;

vec3 lightPos = vec3(2.f, 2.f, 2.f);

void main() {
    gl_Position = ubo.transformMatrix * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    
    vec3 normal = normalize((ubo.modelMatrix * vec4(inNormal, 0.f)).xyz);
    fragNormal = normal;
    lightDirection = normalize(gl_Position.xyz - lightPos);
}
