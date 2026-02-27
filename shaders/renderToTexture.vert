//Vertex Shader für Render-To-Texture
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragWorldNormal; 
layout(location = 2) out vec2 fragTexCoord; 

void main() {
    //World Position
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;
    
    //Normale in World Space transformieren
    mat3 normalMatrix = transpose(inverse(mat3(push.model)));
    fragWorldNormal = normalize(normalMatrix * inNormal);
    
    // Texturkoordinaten durchreichen
    fragTexCoord = inTexCoord; 
    
    gl_Position = ubo.proj * ubo.view * worldPos;
}