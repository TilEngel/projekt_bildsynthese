//renderToTexture.frag
#version 450

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;

layout(binding = 1) uniform samplerCube cubemapSampler;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;  // Hauptkamera-Position
} ubo;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 normal = normalize(fragNormal);
    
    // Reflexionsvektor berechnen
    vec3 viewDir = normalize(fragWorldPos - ubo.cameraPos);
    vec3 reflectDir = reflect(viewDir, normal);
    
    // Cubemap samplen
    vec3 reflectionColor = texture(cubemapSampler, reflectDir).rgb;
    
    // Optional: Fresnel-Effekt für realistischere Reflexion
    float fresnel = pow(1.0 - max(dot(-viewDir, normal), 0.0), 3.0);
    fresnel = mix(0.2, 1.0, fresnel);  // 20-100% Reflexion
    
    outColor = vec4(reflectionColor * fresnel, 1.0);
}