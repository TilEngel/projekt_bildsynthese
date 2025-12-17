#version 450
layout(location = 0) in vec3 inPos;

layout(push_constant) uniform Push {
    mat4 model;
} push;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} cam;

void main() {
    gl_Position = cam.proj * cam.view * push.model * vec4(inPos, 1.0);
}
