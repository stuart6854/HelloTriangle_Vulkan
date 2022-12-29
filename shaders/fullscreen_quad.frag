#version 450

layout (location = 0) in vec2 inTexCoord;
layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform sampler2D tex;

void main()
{
    outFragColor = vec4(texture(tex, inTexCoord).xyz, 1.0);
}