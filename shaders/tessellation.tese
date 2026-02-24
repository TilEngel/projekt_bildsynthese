//tessellation.tese
#version 450

layout(triangles, equal_spacing, ccw) in;

layout(location = 1) in vec3 inNormal[];
layout(location = 0) in vec2 inTexCoord[];

layout(location = 1) out vec3 fragNormal;
layout(location = 0) out vec2 fragTexCoord;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} ubo;

void main() {
    // Barycentric interpolation
    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;
    
    vec3 pos = gl_TessCoord.x * p0 + 
               gl_TessCoord.y * p1 + 
               gl_TessCoord.z * p2;
    
    // Displacement (optional - für Bump/Height mapping)
    vec3 normal = normalize(gl_TessCoord.x * inNormal[0] +
                           gl_TessCoord.y * inNormal[1] +
                           gl_TessCoord.z * inNormal[2]);
    
    // Hier könntest du Height-Map basiertes Displacement hinzufügen:
    // pos += normal * texture(heightMap, texCoord).r * displacementScale;
    
    gl_Position = ubo.proj * ubo.view * pc.model * vec4(pos, 1.0);
    
    fragNormal = normalize(mat3(pc.model) * normal);
    fragTexCoord = gl_TessCoord.x * inTexCoord[0] +
                   gl_TessCoord.y * inTexCoord[1] +
                   gl_TessCoord.z * inTexCoord[2];
}