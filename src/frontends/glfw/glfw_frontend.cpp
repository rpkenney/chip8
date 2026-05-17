#include <chip8/frontends/glfw/glfw_frontend.h>

#include <chip8/machine/cpu.h>
#include <chip8/debug/debug_frame.h>
#include <chip8/debug/debugger.h>
#include <chip8/app/emulator.h>
#include <chip8/debug_map/debug_map.h>
#include <chip8/machine/framebuffer.h>
#include <chip8/frontends/glfw/imgui_panels.h>
#include <chip8/machine/keypad_state.h>
#include <chip8/machine/memory.h>
#include <chip8/app/runner.h>
#include <chip8/frontends/glfw/shaders.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <stdexcept>

namespace {

void centerWindowOnPrimaryMonitor(GLFWwindow* window, int width, int height) {
    GLFWmonitor* const mon = glfwGetPrimaryMonitor();
    if (mon == nullptr) {
        return;
    }
    int mx = 0;
    int my = 0;
    glfwGetMonitorPos(mon, &mx, &my);
    const GLFWvidmode* const vm = glfwGetVideoMode(mon);
    if (vm == nullptr) {
        return;
    }
    const int x = mx + (vm->width - width) / 2;
    const int y = my + (vm->height - height) / 2;
    glfwSetWindowPos(window, x, y);
}

GLFWwindow* createGlfwWindow(int width, int height, const char* title) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (window == nullptr) {
        throw std::runtime_error("Failed to create GLFW window");
    }
    centerWindowOnPrimaryMonitor(window, width, height);
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

/// Owns the GLFW window, GL program, VAO/VBO, screen texture, and the ImGui
/// context. Provides a small set of primitives the loop composes
/// (`uploadFramebuffer`, `drawFullscreenQuad`, `swap`) so the loop can choose
/// between the legacy quad path and an ImGui frame.
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

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window_, /*install_callbacks=*/true);
        ImGui_ImplOpenGL3_Init("#version 330");
    }

    ~GlfwWindow() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

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
    unsigned int texture() const { return texture_; }

    void framebufferSize(int& w, int& h) const {
        glfwGetFramebufferSize(window_, &w, &h);
    }

    void uploadFramebuffer(const Chip8FrameBuffer& fb) {
        glBindTexture(GL_TEXTURE_2D, texture_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Chip8FrameBuffer::WIDTH,
                        Chip8FrameBuffer::HEIGHT, GL_RED, GL_UNSIGNED_BYTE, fb.pixels());
    }

    void drawFullscreenQuad() {
        glUseProgram(program_);
        glBindVertexArray(vao_);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    void swap() { glfwSwapBuffers(window_); }

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

int run(Chip8Emulator& emu) {
    Chip8Memory& memory = emu.memory();
    Chip8FrameBuffer& fb = emu.framebuffer();
    Chip8KeypadState& kp = emu.keypad();
    Chip8CPU& cpu = emu.cpu();
    Chip8Debugger& debugger = emu.debugger();
    Chip8Runner& runner = emu.runner();

    const auto& dm_opt = emu.debugMap();
    const chip8::debug_map::DebugMap* const dm_ptr =
        dm_opt.has_value() ? &*dm_opt : nullptr;
    GlfwWindow window(1280, 720, "CHIP-8");
    GLFWwindow* const win = window.handle();

    bool panels_visible = false;
    bool prev_f1 = false;
    bool prev_space = false;
    bool prev_enter = false;

    while (!window.shouldClose()) {
        window.pollEvents();

        // F1 always toggles panels, even when an ImGui widget has focus, so
        // there is always a way to dismiss the UI.
        const bool f1 = glfwGetKey(win, GLFW_KEY_F1) == GLFW_PRESS;
        if (f1 && !prev_f1) {
            panels_visible = !panels_visible;
        }
        prev_f1 = f1;

        // ImGui swallows keys when a text widget has focus.
        const bool ui_eats_input =
            panels_visible && ImGui::GetIO().WantCaptureKeyboard;

        if (ui_eats_input) {
            kp.clear();
            prev_space = prev_enter = false;
        } else {
            syncKeypadFromGlfw(win, kp);

            const bool space = glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS;
            const bool enter = glfwGetKey(win, GLFW_KEY_ENTER) == GLFW_PRESS ||
                               glfwGetKey(win, GLFW_KEY_KP_ENTER) == GLFW_PRESS;

            if (space && !prev_space) debugger.requestStep();
            if (enter && !prev_enter) debugger.requestResume();
            prev_space = space;
            prev_enter = enter;
        }

        // Repaint when ~60 Hz timers tick or an instruction ran (see Chip8Runner::tick).
        if (runner.tick()) {
            int fb_w = 0, fb_h = 0;
            window.framebufferSize(fb_w, fb_h);
            glViewport(0, 0, fb_w, fb_h);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            window.uploadFramebuffer(fb);

            if (panels_visible) {
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();
                const imgui_panels::CentralRect central =
                    imgui_panels::build(debugger, cpu, memory, dm_ptr);
                ImGui::Render();

                // Draw the CHIP-8 quad sized to the dockspace's central node.
                // The quad is aspect-fit (2:1) and centered inside that rect.
                if (central.valid) {
                    const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
                    const float px = central.x * scale.x;
                    const float py = central.y * scale.y;
                    const float pw = central.w * scale.x;
                    const float ph = central.h * scale.y;

                    constexpr float aspect =
                        static_cast<float>(Chip8FrameBuffer::WIDTH) /
                        static_cast<float>(Chip8FrameBuffer::HEIGHT);
                    float qw = pw;
                    float qh = qw / aspect;
                    if (qh > ph) {
                        qh = ph;
                        qw = qh * aspect;
                    }
                    const float pad_x = (pw - qw) * 0.5f;
                    const float pad_y = (ph - qh) * 0.5f;

                    const int gx = static_cast<int>(px + pad_x);
                    // GL viewport is bottom-up; ImGui rect is top-down.
                    const int gy = fb_h - static_cast<int>(py + pad_y) -
                                   static_cast<int>(qh);
                    glViewport(gx, gy, static_cast<int>(qw),
                               static_cast<int>(qh));
                    window.drawFullscreenQuad();
                    glViewport(0, 0, fb_w, fb_h);
                }

                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            } else {
                window.drawFullscreenQuad();
            }

            window.swap();
        }
    }

    return 0;
}

}  // namespace glfw_frontend
