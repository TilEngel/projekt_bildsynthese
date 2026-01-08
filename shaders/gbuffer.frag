//gbuffer.frag
#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outGBuffer;

void main() {
    // Textur-Farbe
    vec3 albedo = texture(texSampler, fragTexCoord).rgb;
    
    // Normale aus Position-Derivaten berechnen
    vec3 dPdx = dFdx(fragWorldPos);
    vec3 dPdy = dFdy(fragWorldPos);
    vec3 normal = normalize(cross(dPdy, dPdx));
    
    //XYZ als RGB, Albedo als Alpha
    outGBuffer = vec4(
        normal * 0.5 + 0.5,          // Normal XYZ: [-1,1] -> [0,1]
        dot(albedo, vec3(0.299, 0.587, 0.114))  // Luminance im Alpha-Kanal
    );
}