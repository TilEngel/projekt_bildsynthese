//vertex-shader
#version 450

layout(set=0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;
layout(push_constant) uniform PushModel {
    mat4 model;
} pushModel;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 texCoord;

void main() {
    gl_Position = ubo.proj * ubo.view * pushModel.model * vec4(inPosition, 1.0);
    texCoord = inTexCoord;
}
