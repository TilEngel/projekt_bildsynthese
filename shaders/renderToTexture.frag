//renderToTexture.frag (Vereinfacht - ohne Licht-Array)
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
    vec3 normal = normalize(fragWorldNormal);
    vec3 viewDir = normalize(ubo.cameraPos - fragWorldPos);
    
    // Material Properties
    vec3 baseColor = vec3(0.3, 0.6, 0.3);
    float metallic = 0.9;
    float roughness = 0.1;
    
    //Einfaches Ambient
    vec3 ambient = 0.15 * baseColor;
    
    //Reflexion
    vec3 reflectDir = reflect(-viewDir, normal);
    vec3 reflectionColor = texture(cubemapSampler, reflectDir).rgb;
    
    //Fresnel
    float F0 = mix(0.04, 0.95, metallic);
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - max(dot(viewDir, normal), 0.0), 5.0);
    fresnel *= (1.0 - roughness * 0.5);
    
    //Base Color und Reflexion
    vec3 finalColor = mix(
        ambient + baseColor * 0.3,   
        reflectionColor, 
        fresnel
    );
    
    // Metall-Tinting
    if (metallic > 0.5) {
        finalColor *= mix(vec3(1.0), baseColor, metallic * 0.6);
    }
    
    outColor = vec4(finalColor, 1.0);
}