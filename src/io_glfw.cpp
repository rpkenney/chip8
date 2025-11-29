#include "io_glfw.h"
#include "shaders.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

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

std::vector<unsigned int> coordinateToIndices(int x, int y) {
    
    std::vector<unsigned int> indices;
    unsigned int topLeft = x * (32 + 1) + y;
    unsigned int bottomLeft = topLeft + 1;

    unsigned int topRight = topLeft + (32 + 1);
    unsigned int bottomRight = topRight + 1; 
    
    indices.insert(indices.end(), {topLeft, bottomLeft, topRight, topRight, bottomRight, bottomLeft});

    return indices; 
    
}
    
Chip8IO_GLFW::Chip8IO_GLFW(int width, int height, const char* title) {
    window = getWindow(width, height, title);

    gladLoad();

    shaderProgram = loadShaderProgram(); 

    
    std::vector<unsigned int> topSquare = coordinateToIndices(0, 0);

    indices.insert(indices.end(), topSquare.begin(), topSquare.end());  


    glGenBuffers(1, &VBO);

    glGenBuffers(1, &EBO);    

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    initVertices(width, height);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);


    frameBuffer.resize(64 * 32, 0);

    setPixel(1, 2, true);
}

Chip8IO_GLFW::~Chip8IO_GLFW() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
}

void Chip8IO_GLFW::clearDisplay() {
    indices.clear();
}

void Chip8IO_GLFW::setPixel(int x, int y, bool active){
    if (active) {
        frameBuffer[y * 64 + x] = 1;
    } else {
        frameBuffer[y * 64 + x] = 0;
    }
}


void Chip8IO_GLFW::drawSprite(int x, int y, uint8_t* sprite, int height) {
    
}

void Chip8IO_GLFW::render() {
    clearDisplay();

    for(int x = 0; x < 64; x++){
        for(int y = 0; y < 32; y++){
            if (frameBuffer[y * 64 + x]) {
                std::vector<unsigned int> pixel = coordinateToIndices(x, y);
                indices.insert(indices.end(), pixel.begin(), pixel.end()); 
            }
        }
    }    

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0); 

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


void Chip8IO_GLFW::initVertices(int width, int height) {
    if (width % 64 > 0) {
        throw std::runtime_error("Window width must be multiple of 64");
    }
    if (height % 32 > 0) {
        throw std::runtime_error("Window height must be multiple of 32");
    }
    
    int i = 0;
    for (int x = 0; x <= 64; x++) {
        for (int y = 0; y <= 32; y++) {
            float ndc_x = ((x / 64.0f) * 2) - 1;
            float ndc_y = ((y / 32.0f) * -2) + 1;

            vertices.insert(vertices.end(), {ndc_x, ndc_y, 0.0f});
        }
    }

}


