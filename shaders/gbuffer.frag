#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragWorldNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragViewPos;

layout(location = 0) out vec4 outGBuffer;

void main() {
    vec4 albedo = texture(texSampler, fragTexCoord);
    
    // Normale berechnen (im Fragment Shader für bessere Qualität)
    vec3 normal = normalize(cross(dFdx(fragWorldPos), dFdy(fragWorldPos)));
    
    // Pack: RGB = Albedo, A = encoded normal
    // (Vereinfacht - für echtes Deferred bräuchtest du mehrere Attachments)
    float normalEncoded = dot(normal, vec3(0.333));
    
    outGBuffer = vec4(albedo.rgb, normalEncoded);
}