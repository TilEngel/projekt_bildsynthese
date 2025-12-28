#version 450
layout(location = 0) in vec2 inTexCoord;
layout(location = 0) out vec4 outColor;

// Input attachments (G-Buffer)
layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputAlbedo;
layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput inputNormal;
layout(input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput inputPosition;

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float radius;
};

layout(set = 0, binding = 3) uniform LightingUBO {
    vec3 viewPos;
    int numLights;
    PointLight lights[4];
} ubo;

void main() {
    // Read from G-Buffer
    vec3 albedo = subpassLoad(inputAlbedo).rgb;
    vec3 normal = subpassLoad(inputNormal).rgb * 2.0 - 1.0;
    vec3 worldPos = subpassLoad(inputPosition).rgb;
    
    // Debug: Prüfe auf ungültige Werte
    if (length(normal) < 0.01) {
        normal = vec3(0.0, 1.0, 0.0); // Fallback
    } else {
        normal = normalize(normal);
    }
    
    vec3 viewDir = normalize(ubo.viewPos - worldPos);
    
    // Ambient (heller für bessere Sichtbarkeit)
    vec3 ambient = 0.3 * albedo;
    vec3 result = ambient;
    
    // Lighting
    for (int i = 0; i < min(ubo.numLights, 4); i++) {
        vec3 lightDir = normalize(ubo.lights[i].position - worldPos);
        float distance = length(ubo.lights[i].position - worldPos);
        
        // Attenuation
        float attenuation = 0.0;
        if (distance < ubo.lights[i].radius) {
            float distanceRatio = clamp(distance / ubo.lights[i].radius, 0.0, 1.0);
            float attenSquared = 1.0 - (distanceRatio * distanceRatio);
            attenuation = ubo.lights[i].intensity * (attenSquared * attenSquared);
        }
        
        // Diffuse
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * ubo.lights[i].color * attenuation;
        
        // Specular
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
        vec3 specular = spec * ubo.lights[i].color * 0.3 * attenuation;
        
        result += (diffuse + specular) * albedo;
    }
    
    outColor = vec4(result, 1.0);
}