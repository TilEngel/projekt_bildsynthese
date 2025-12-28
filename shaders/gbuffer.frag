//gbuffer.frag
#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outPosition;

layout(set=0, binding=1) uniform sampler2D texSampler;

void main() {
    // Albedo (diffuse color)
    outAlbedo = texture(texSampler, inTexCoord);
    
    // Normal (world space, encoded in [0,1])
    outNormal = vec4(normalize(inNormal) * 0.5 + 0.5, 1.0);
    
    // Position (world space)
    outPosition = vec4(inWorldPos, 1.0);
}