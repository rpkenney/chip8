#ifndef CHIP8_IO_GLFW_H
#define CHIP8_IO_GLFW_H


#include "io.h"

#include <vector>

struct GLFWwindow;

class Chip8IO_GLFW : public Chip8IO {
public:
    Chip8IO_GLFW(int width, int height, const char* title); 
    ~Chip8IO_GLFW() override;
    
    void clearDisplay() override;
    bool drawSprite(int x, int y, uint8_t* sprite, int height) override;
    void render() override;
    void pollEvents() override;

    bool shouldClose() const override;
    bool isKeyPressed(int key) const override;

private:
    GLFWwindow* window;
    unsigned int VAO, VBO, EBO, shaderProgram;

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    void initVertices(int width, int height);
    void setPixel(int x, int y, bool active);
    bool getPixel(int x, int y);

    std::vector<uint8_t> frameBuffer;
};

#endif
