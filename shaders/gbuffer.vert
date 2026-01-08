//gbuffer.vert
#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    vec4 worldPos = push.model * vec4(inPosition, 1.0);

    fragWorldPos = worldPos.xyz;
    fragTexCoord = inTexCoord;

    gl_Position = ubo.proj * ubo.view * worldPos;
}
