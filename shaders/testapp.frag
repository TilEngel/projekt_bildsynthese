//fragment-shader
#version 450

layout(set = 0, binding = 1) uniform sampler2D tex;

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 texColor = texture(tex, texCoord).rgb;
    outColor = vec4(texColor, 1.0);
}
