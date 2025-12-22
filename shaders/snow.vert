//snow.vert
#version 450

struct Particle {
   vec3 position;
   vec3 velocity;
};


layout(set = 0, binding = 0) uniform UniformBufferObject {
  
    mat4 view;
    mat4 proj;
} ubo;

layout(set = 0, binding = 1) readonly buffer particles {
   Particle part[];
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 texCoord;

void main() {
   // Position des Partikels holen
    vec3 particlePos = part[gl_InstanceIndex].position;
    
    // Billboard: Quad soll immer zur Kamera zeigen
    // Vereinfachte Version: Quad bleibt im Weltkoordinatensystem
    vec3 worldPos = inPosition + particlePos;
    
    gl_Position = ubo.proj * ubo.view * vec4(worldPos, 1.0);
    texCoord = inTexCoord;
}
