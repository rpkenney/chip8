#ifndef CHIP8_IO_GLFW_SHADERS_H
#define CHIP8_IO_GLFW_SHADERS_H


constexpr const char VERTEX_SHADER_SRC[] = R"delim(
#version 330 core

layout (location = 0) in vec3 aPos; 

void main (){ 
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)delim";

constexpr const char FRAGMENT_SHADER_SRC[] = R"delim(
#version 330 core

out vec4 FragColor; 

void main() { 
    FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f); 
}
)delim";


#endif

