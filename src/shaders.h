#ifndef CHIP8_IO_GLFW_SHADERS_H
#define CHIP8_IO_GLFW_SHADERS_H


constexpr const char VERTEX_SHADER_SRC[] = R"delim(
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    TexCoord = aTexCoord;
}
)delim";

constexpr const char FRAGMENT_SHADER_SRC[] = R"delim(
#version 330 core

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D screenTexture;

void main() {
    vec2 uv = vec2(TexCoord.x, 0 - TexCoord.y);
    float on = texture(screenTexture, uv).r;
    FragColor = vec4(on, on, on, 1.0);
}
)delim";


#endif

