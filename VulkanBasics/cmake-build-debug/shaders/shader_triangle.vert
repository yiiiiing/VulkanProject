#version 450
// input
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

// uniform (same to each vertex)
layout(binding = 0) uniform UniformBufferObject {
    mat4 modelMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 transformMatrix;
} ubo;


// output
layout(location = 0) out vec3 fragColor;


void main() {
    gl_Position = ubo.transformMatrix * vec4(inPosition, 1.0);
    fragColor = inColor;
}
