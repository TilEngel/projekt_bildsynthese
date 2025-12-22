#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

layout(location = 0) out vec3 FragPos;
layout(location = 1) out vec3 Normal;
layout(location = 2) out vec2 TexCoord;

layout(set=0, binding=0) uniform LitUBO {
    mat4 view;
    mat4 proj;
    vec3 viewPos;
    int numLights;
} ubo;

layout(push_constant) uniform PushMod {
    mat4 model;
} pushModel;

void main() {
    FragPos = vec3(pushModel.model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(pushModel.model))) * normalize(aPos); 
    TexCoord = aTexCoord;
    
    gl_Position = ubo.proj * ubo.view * vec4(FragPos, 1.0);
}