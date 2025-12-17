#version 450

layout(location = 0) out vec4 outColor;

void main() {
    // leicht dunkles Glas / Metall
    vec3 mirrorColor = vec3(0.08, 0.08, 0.1);

    // Alpha < 1.0 → Reflektion scheint durch
    float alpha = 0.4;

    outColor = vec4(mirrorColor, alpha);
}

