#include "glfw_frontend.h"

#include "breakpoints_loader.h"
#include "cpu.h"
#include "debug.h"
#include "debug_frame.h"
#include "debugger.h"
#include "format_debug_frame.h"
#include "framebuffer.h"
#include "keypad_state.h"
#include "memory.h"
#include "rom_loader.h"
#include "runner.h"
#include "shaders.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

namespace {

GLFWwindow* createGlfwWindow(int width, int height, const char* title) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (window == nullptr) {
        throw std::runtime_error("Failed to create GLFW window");
    }
    glfwMakeContextCurrent(window);
    return window;
}

void gladLoad() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }
}

unsigned int loadShader(GLenum shaderType, const char* shaderSrc) {
    unsigned int shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderSrc, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        throw std::runtime_error("error compiling shader");
    }
    return shader;
}

unsigned int loadShaderProgram() {
    unsigned int program = glCreateProgram();
    unsigned int vs = loadShader(GL_VERTEX_SHADER, VERTEX_SHADER_SRC);
    unsigned int fs = loadShader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER_SRC);
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    int success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        throw std::runtime_error("error linking shader program");
    }
    glUseProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

constexpr int kChip8KeyGlfwMap[16] = {
    GLFW_KEY_X,  // 0
    GLFW_KEY_1,  // 1
    GLFW_KEY_2,  // 2
    GLFW_KEY_3,  // 3
    GLFW_KEY_Q,  // 4
    GLFW_KEY_W,  // 5
    GLFW_KEY_E,  // 6
    GLFW_KEY_A,  // 7
    GLFW_KEY_S,  // 8
    GLFW_KEY_D,  // 9
    GLFW_KEY_Z,  // A
    GLFW_KEY_C,  // B
    GLFW_KEY_4,  // C
    GLFW_KEY_R,  // D
    GLFW_KEY_F,  // E
    GLFW_KEY_V,  // F
};

/// Owns the GLFW window, OpenGL program, VAO/VBO, and the screen texture.
/// Knows how to upload a `Chip8FrameBuffer` and present a frame.
class GlfwWindow {
public:
    GlfwWindow(int width, int height, const char* title) {
        window_ = createGlfwWindow(width, height, title);
        gladLoad();
        program_ = loadShaderProgram();

        glGenBuffers(1, &vbo_);
        glGenVertexArrays(1, &vao_);
        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);

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

        const int u_screen = glGetUniformLocation(program_, "screenTexture");
        if (u_screen < 0) {
            throw std::runtime_error("screenTexture uniform not found");
        }

        glGenTextures(1, &texture_);
        glBindTexture(GL_TEXTURE_2D, texture_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, Chip8FrameBuffer::WIDTH,
                     Chip8FrameBuffer::HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

        glUseProgram(program_);
        glUniform1i(u_screen, 0);
    }

    ~GlfwWindow() {
        glDeleteTextures(1, &texture_);
        glDeleteVertexArrays(1, &vao_);
        glDeleteBuffers(1, &vbo_);
        glDeleteProgram(program_);
        glfwTerminate();
    }

    GlfwWindow(const GlfwWindow&) = delete;
    GlfwWindow& operator=(const GlfwWindow&) = delete;

    GLFWwindow* handle() const { return window_; }
    bool shouldClose() const { return glfwWindowShouldClose(window_); }
    void pollEvents() { glfwPollEvents(); }

    void render(const Chip8FrameBuffer& fb) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindTexture(GL_TEXTURE_2D, texture_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Chip8FrameBuffer::WIDTH,
                        Chip8FrameBuffer::HEIGHT, GL_RED, GL_UNSIGNED_BYTE, fb.pixels());

        glUseProgram(program_);
        glBindVertexArray(vao_);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window_);
    }

private:
    GLFWwindow* window_ = nullptr;
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    unsigned int program_ = 0;
    unsigned int texture_ = 0;
};

void syncKeypadFromGlfw(GLFWwindow* win, Chip8KeypadState& kp) {
    for (int i = 0; i < 16; ++i) {
        kp.setKey(i, glfwGetKey(win, kChip8KeyGlfwMap[i]) == GLFW_PRESS);
    }
}

}  // namespace

namespace glfw_frontend {

int run(const Options& opts) {
    Chip8Memory memory;
    {
        std::string err;
        if (!loadRomFromFile(memory, opts.rom_path, err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
    }

    std::unordered_set<std::uint16_t> breakpoints;
    if (opts.breakpoints_path != nullptr) {
        std::string err;
        if (!loadBreakpointsFile(opts.breakpoints_path, breakpoints, err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
    }

    Chip8FrameBuffer fb;
    Chip8KeypadState kp;
    Chip8CPU cpu(memory, fb, kp);

    TerminalDebugObserver terminal_observer;
    Chip8Debugger debugger;
    debugger.setObserver(&terminal_observer);
    debugger.setStartPaused(opts.step);
    if (opts.trace) {
        debugger.setTraceLevel(TraceLevel::Instructions);
    }
    debugger.setBreakpoints(std::move(breakpoints));

    Chip8Runner runner(cpu, memory, debugger);

    GlfwWindow window(640, 320, "CHIP-8");
    GLFWwindow* const win = window.handle();

    bool prev_space = false;
    bool prev_enter = false;
    bool prev_p = false;
    bool prev_n = false;

    while (!window.shouldClose()) {
        window.pollEvents();
        syncKeypadFromGlfw(win, kp);

        const bool space = glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS;
        const bool enter = glfwGetKey(win, GLFW_KEY_ENTER) == GLFW_PRESS ||
                           glfwGetKey(win, GLFW_KEY_KP_ENTER) == GLFW_PRESS;
        const bool p = glfwGetKey(win, GLFW_KEY_P) == GLFW_PRESS;
        const bool n = glfwGetKey(win, GLFW_KEY_N) == GLFW_PRESS;

        if (space && !prev_space) debugger.requestStep();
        if (enter && !prev_enter) debugger.requestResume();
        if (n && !prev_n) debugger.requestStepOver();
        if (p && !prev_p && debugger.pacing() == PausePacing::Manual) {
            formatDebugFrame(debugger.captureFrame(cpu, memory), stderr);
        }
        prev_space = space;
        prev_enter = enter;
        prev_p = p;
        prev_n = n;

        if (runner.tick()) {
            window.render(fb);
        }
    }

    return 0;
}

}  // namespace glfw_frontend
