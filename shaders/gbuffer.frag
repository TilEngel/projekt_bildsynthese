#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outGBuffer;   // Normal + Metallic
layout(location = 1) out vec4 outAlbedo;    // Albedo RGB + Roughness

void main() {
    // Textur-Farbe
    vec3 albedo = texture(texSampler, fragTexCoord).rgb;
    
    // Normale aus Position-Derivaten berechnen
    vec3 dPdx = dFdx(fragWorldPos);
    vec3 dPdy = dFdy(fragWorldPos);
    vec3 normal = normalize(cross(dPdy, dPdx));
    
    //Prüfe auf ungültige Normalen
    if (any(isnan(normal)) || any(isinf(normal)) || length(normal) < 0.1) {
        // Fallback: nach oben
        normal = vec3(0.0, 1.0, 0.0);
    }
    
    // G-Buffer:Normal RGB + Metallic A
    outGBuffer = vec4(
        normal * 0.5 + 0.5,  // Normal xyz: [-1,1]-> [0,1]
        0.0                   //metallic
    );
    
    //Albedo: Farbe RGB + roughness A
    outAlbedo = vec4(
        albedo,    //RGB-Farbe
        0.5        // Roughness
    );
}