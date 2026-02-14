//tessellation.vert
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outTexCoord;

void main() {
    // Nur Passthrough - keine Transformation hier!
    gl_Position = vec4(inPosition, 1.0);
    outNormal = inNormal;
    outTexCoord = inTexCoord;
}