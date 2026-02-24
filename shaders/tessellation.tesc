#version 450
layout(vertices = 3) out;

layout(location = 1) in vec3 inNormal[];
layout(location = 0) in vec2 inTexCoord[];

layout(location = 1) out vec3 outNormal[];
layout(location = 0) out vec2 outTexCoord[];

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} ubo;

void main() {
    if (gl_InvocationID == 0) {
        gl_TessLevelOuter[0] = 8.0;
        gl_TessLevelOuter[1] = 8.0;
        gl_TessLevelOuter[2] = 8.0;
        gl_TessLevelInner[0] = 8.0;
    }
    
    // Pass-through
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    outNormal[gl_InvocationID] = inNormal[gl_InvocationID];
    outTexCoord[gl_InvocationID] = inTexCoord[gl_InvocationID];
}