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
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;

void main() {
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;
    
    // Normal transformation (inverse transpose for non-uniform scaling)
    mat3 normalMatrix = transpose(inverse(mat3(push.model)));
    // For now, calculate normal from geometry (simple approach)
    fragNormal = normalMatrix * vec3(0.0, 1.0, 0.0); // Placeholder
    
    fragTexCoord = inTexCoord;
    
    gl_Position = ubo.proj * ubo.view * worldPos;
}