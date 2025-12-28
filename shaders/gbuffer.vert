//gbuffer.vert
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord;

layout(set=0, binding=0) uniform UBO {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

void main() {
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    outWorldPos = worldPos.xyz;
    
    // Normale aus Position ableiten (für einfache Objekte)
    outNormal = mat3(transpose(inverse(push.model))) * normalize(inPosition);
    
    outTexCoord = inTexCoord;
    
    gl_Position = ubo.proj * ubo.view * worldPos;
}