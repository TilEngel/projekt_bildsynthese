//mirror.frag - Fragment Shader für halbtransparenten Spiegel
#version 450

layout(set = 0, binding = 1) uniform sampler2D tex;

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 texColor = texture(tex, texCoord).rgb;
    
    // Leicht bläulicher Tint für Glas-Effekt
    vec3 mirrorTint = vec3(0.9, 0.95, 1.0);
    vec3 finalColor = texColor * mirrorTint;
    
    // Alpha für Transparenz
    float alpha = 0.25;
    
    outColor = vec4(finalColor, alpha);
}
