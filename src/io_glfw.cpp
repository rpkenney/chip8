#include "io_glfw.h"
#include "shaders.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstring>
#include <stdexcept>

GLFWwindow* getWindow(int width, int height, const char* title) {
    glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (window == NULL)
    {
        throw std::runtime_error("Failed to create GLFW window");
    }
    glfwMakeContextCurrent(window);
    return window;
}

void gladLoad() {
     if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        throw std::runtime_error("Failed to initialize GLAD");
    }  
}

unsigned int loadShader(GLenum shaderType, const char* shaderSrc) {
    unsigned int shader;
    shader  = glCreateShader(shaderType);

    glShaderSource(shader, 1, &shaderSrc, NULL);
    glCompileShader(shader);
    
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if(!success)
    {
        throw std::runtime_error("error compiling fragment shader");
    }
    return shader;
}

unsigned int loadShaderProgram() {
    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();

    unsigned int vertexShader = loadShader(GL_VERTEX_SHADER, VERTEX_SHADER_SRC);
    unsigned int fragmentShader = loadShader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER_SRC);

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

    if (!success)
    {
        throw std::runtime_error("error linking shader program");
    }
    glUseProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

Chip8IO_GLFW::Chip8IO_GLFW(int width, int height, const char* title) {
    window = getWindow(width, height, title);

    gladLoad();

    shaderProgram = loadShaderProgram(); 


    glGenBuffers(1, &VBO);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    const float quad[] = {
        // x,    y,    z,    u,   v
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    const GLsizei stride = 5 * static_cast<GLsizei>(sizeof(float));
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    frameBuffer.resize(64 * 32, 0);

    uScreenTexture = glGetUniformLocation(shaderProgram, "screenTexture");
    if (uScreenTexture < 0) {
        throw std::runtime_error("screenTexture uniform not found");
    }

    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 64, 32, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    glUseProgram(shaderProgram);
    glUniform1i(uScreenTexture, 0);
}

Chip8IO_GLFW::~Chip8IO_GLFW() {
    glDeleteTextures(1, &screenTexture);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
}

void Chip8IO_GLFW::clearDisplay() {
    std::memset(frameBuffer.data(), 0, frameBuffer.size());
}

void Chip8IO_GLFW::setPixel(int x, int y, bool active){
    frameBuffer[y * 64 + x] = active ? 255 : 0;
}

bool Chip8IO_GLFW::getPixel(int x, int y) {
    return frameBuffer[y * 64 + x] != 0;
}


bool Chip8IO_GLFW::drawSprite(int x, int y, uint8_t* sprite, int height) {
    bool collision = false;
    for(int i = 0; i < height; i++) {
        uint8_t row = sprite[i];
        for(int j = 0; j < 8; j++) {
            const bool spriteOn = (row & 0x80) != 0;
            row = static_cast<uint8_t>(row << 1);
            const bool active = getPixel(x + j, y + i);
            collision = collision || (active && spriteOn);
            setPixel(x + j, y + i, active ^ spriteOn);
        }
    } 
    return collision;
}

void Chip8IO_GLFW::render() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 32, GL_RED, GL_UNSIGNED_BYTE,
                    frameBuffer.data());

    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glfwSwapBuffers(window);
}


void Chip8IO_GLFW::pollEvents() {
    glfwPollEvents();
}

bool Chip8IO_GLFW::shouldClose() const{
    return glfwWindowShouldClose(window);
}


bool Chip8IO_GLFW::isKeyPressed(int key) const {
    return false;
}