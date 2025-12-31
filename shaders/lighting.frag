#version 450

// Input attachments from G-Buffer
layout(input_attachment_index = 0, binding = 0) uniform subpassInput gBufferInput;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput depthInput;

layout(binding = 2) uniform LightingUBO {
    mat4 view;
    mat4 proj;
    vec3 viewPos;
    int numLights;
    
    // Point lights (max 4)
    vec4 lightPositions[4];
    vec4 lightColors[4];
    vec4 lightParams[4]; // x: intensity, y: radius
} lighting;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

vec3 reconstructWorldPos(vec2 uv, float depth) {
    // Reconstruct world position from depth
    vec4 clipSpace = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 viewSpace = inverse(lighting.proj) * clipSpace;
    viewSpace /= viewSpace.w;
    vec4 worldSpace = inverse(lighting.view) * viewSpace;
    return worldSpace.xyz;
}

void main() {
    // Read G-Buffer data
    vec4 gBufferData = subpassLoad(gBufferInput);
    float depth = subpassLoad(depthInput).r;
    
    // Extract data from G-Buffer
    vec3 worldPos = gBufferData.xyz;
    vec3 normal = vec3(gBufferData.w, 0.0, 0.0); // Simplified - need proper unpacking
    
    // Reconstruct position from depth if needed
    if (depth < 1.0) {
        worldPos = reconstructWorldPos(fragTexCoord, depth);
    }
    
    // Base color (would normally come from G-Buffer albedo)
    vec3 albedo = vec3(0.8, 0.8, 0.8);
    
    // Lighting calculation
    vec3 viewDir = normalize(lighting.viewPos - worldPos);
    vec3 finalColor = vec3(0.0);
    
    // Ambient
    vec3 ambient = 0.1 * albedo;
    finalColor += ambient;
    
    // Point lights
    for (int i = 0; i < lighting.numLights && i < 4; i++) {
        vec3 lightPos = lighting.lightPositions[i].xyz;
        vec3 lightColor = lighting.lightColors[i].rgb;
        float intensity = lighting.lightParams[i].x;
        float radius = lighting.lightParams[i].y;
        
        vec3 lightDir = lightPos - worldPos;
        float distance = length(lightDir);
        lightDir = normalize(lightDir);
        
        // Attenuation
        float attenuation = 1.0 / (1.0 + distance * distance / (radius * radius));
        
        // Diffuse
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * lightColor * albedo * intensity * attenuation;
        
        // Specular (Blinn-Phong)
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
        vec3 specular = spec * lightColor * intensity * attenuation;
        
        finalColor += diffuse + specular;
    }
    
    outColor = vec4(finalColor, 1.0);
}