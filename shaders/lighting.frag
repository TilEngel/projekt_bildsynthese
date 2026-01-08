//lighting.frag- letzte Phase von deferred Shading
#version 450

layout(input_attachment_index = 0, binding = 0) uniform subpassInput gBufferInput;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput depthInput;

struct Light {
    vec3 position;
    float intensity;
    vec3 color;
    float radius;
};

// Licht-Daten aus der Scene
layout(binding = 2) uniform LightingUBO {
    mat4 invView;
    mat4 invProj;
    vec3 viewPos;
    int numLights;
    vec2 screenSize; 
    vec2 _padding;
    Light lights[4];
} ubo;

layout(location = 0) out vec4 outColor;

// World Position aus Depth rekonstruieren
vec3 reconstructWorldPosition(vec2 screenUV, float depth) {
    // NDC space: [-1, 1]
    vec4 ndc = vec4(screenUV * 2.0 - 1.0, depth, 1.0);
    
    //Clip space -> View space
    vec4 viewPos = ubo.invProj * ndc;
    viewPos /= viewPos.w;
    
    //View space -> World space
    vec4 worldPos = ubo.invView * viewPos;
    return worldPos.xyz;
}

void main() {
    //Screen UVs aus gl_FragCoord und screenSize
    vec2 screenUV = gl_FragCoord.xy / ubo.screenSize;
    
    vec4 gBufferData = subpassLoad(gBufferInput);
    float depth = subpassLoad(depthInput).r;
    
    // Normale bestimmen
    vec3 normal = normalize(gBufferData.rgb * 2.0 - 1.0);
    float albedoLuminance = gBufferData.a;
    
    //ungültige Pixel filtern
    if (length(gBufferData.rgb) < 0.1) {
        outColor = vec4(0.2, 0.3, 0.4, 1.0);
        return;
    }
    
    // Rekonstruiere World Position
    vec3 worldPos = reconstructWorldPosition(screenUV, depth);
    vec3 viewDir = normalize(ubo.viewPos - worldPos);
    
    // Albedo
    vec3 albedo = vec3(albedoLuminance);
    
    
    // Ambient
    vec3 ambient = 0.2 * albedo;
    vec3 totalLight = ambient;
    
    //für jede Lichtquelle
    for (int i = 0; i < ubo.numLights && i < 4; i++) {
        vec3 lightPos = ubo.lights[i].position;
        vec3 lightColor = ubo.lights[i].color;
        float lightIntensity = ubo.lights[i].intensity;
        float lightRadius = ubo.lights[i].radius;
        
        //Licht-Richtung und Distanz
        vec3 lightDir = lightPos - worldPos;
        float distance = length(lightDir);
        lightDir = normalize(lightDir);
        
        // Attenuation (quadratisch mit Radius cutoff)
        float attenuation = lightIntensity / (1.0 + distance * distance / lightRadius);
        attenuation = max(attenuation, 0.0);
        
        // Diffuse
        float ndotl = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = ndotl * lightColor * albedo * attenuation;
        
        // Specular (Blinn-Phong)
        vec3 halfVec = normalize(lightDir + viewDir);
        float ndoth = max(dot(normal, halfVec), 0.0);
        float specular = pow(ndoth, 32.0) * attenuation;
        vec3 specularContrib = specular * lightColor * 0.3;  // 30% specular
        
        totalLight += diffuse + specularContrib;
    }
    
    outColor = vec4(totalLight, 1.0);
}