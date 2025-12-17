// mirror.frag
#version 450
layout(location = 0) out vec4 outColor;

void main() {
    // Leicht getöntes Glas mit hoher Transparenz
    vec3 mirrorTint = vec3(0.9, 0.95, 1.0); // Leichter Blau-Stich
    
    // Sehr transparent, damit Reflektion durchscheint
    float alpha = 0.15; // Weniger deckkraft = mehr Reflektion sichtbar
    
    outColor = vec4(mirrorTint, alpha);
}
