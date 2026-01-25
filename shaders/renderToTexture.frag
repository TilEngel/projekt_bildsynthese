//renderToTexture.frag
#version 450

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragWorldNormal;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} ubo;

layout(binding = 1) uniform samplerCube cubemapSampler;

layout(location = 0) out vec4 outColor;

void main() {
    // Interpolierte Normale normalisieren
    vec3 normal = normalize(fragWorldNormal);
    
    // View Direction von Fragment zur Kamera
    vec3 viewDir = normalize(ubo.cameraPos - fragWorldPos);
    
    // Reflektierte Richtung berechnen
    vec3 reflectDir = reflect(-viewDir, normal);
    
    // Sample aus Cubemap
    vec3 reflectionColor = texture(cubemapSampler, reflectDir).rgb;
    
    // Fresnel-Effekt (stärker an Grenz-Winkeln)
    float fresnel = pow(1.0 - max(dot(viewDir, normal), 0.0), 3.0);
    fresnel = mix(0.04, 1.0, fresnel);  // Basis-Reflektivität 4%
    
    // Optional: Mix mit einer Base Color für Nicht-Perfekte-Spiegel
    // vec3 baseColor = vec3(0.8, 0.8, 0.9);
    // vec3 finalColor = mix(baseColor, reflectionColor, fresnel);
    
    outColor = vec4(reflectionColor * fresnel, 1.0);
}