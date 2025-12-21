#version 450

layout(location = 0) in vec3 FragPos;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoord;

layout(location = 0) out vec4 FragColor;

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float radius;       //noch nicht verwendet 
};

layout(set=0, binding=0) uniform LitUBO {
    mat4 view;
    mat4 proj;
    vec3 viewPos;
    int numLights;
    PointLight lights[4];
} ubo;

layout(set=0, binding=1) uniform sampler2D texSampler;

void main() {
    vec3 texColor = texture(texSampler, TexCoord).rgb;
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(ubo.viewPos - FragPos);
    
    vec3 ambient = 0.1 * texColor;
    vec3 result = ambient;
    
    for (int i = 0; i < ubo.numLights; i++) {
        vec3 lightDir = normalize(ubo.lights[i].position - FragPos);
        float distance = length(ubo.lights[i].position - FragPos);
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        
        // Diffuse
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * ubo.lights[i].color * ubo.lights[i].intensity * attenuation;
        
        // Specular
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        vec3 specular = spec * ubo.lights[i].color * 0.5 * attenuation;
        
        result += (diffuse + specular) * texColor;
    }
    
    FragColor = vec4(result, 1.0);
}