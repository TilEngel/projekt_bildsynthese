#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outGBuffer;

void main() {
    vec3 normal = normalize(fragNormal);
    vec4 albedo = texture(texSampler, fragTexCoord);
    
    // Pack G-Buffer data:
    // For simplicity, we pack position.xy in RG, normal.xy in BA
    // This is a simplified approach - real implementations use multiple render targets
    outGBuffer = vec4(fragWorldPos.xyz, normal.x);
}