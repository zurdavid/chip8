#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "gui/GUI.h"

#include <spdlog/spdlog.h>

#include "utilities/Shader.h"
#include "utilities/Canvas.h"
#include "utilities/SimpleDisplayTexture.h"
#include "chip8/Chip8.h"
#include "chip8/MazeDemo.h"

GLFWwindow *createWindow(int width, int height);

static void glfw_error_callback(int error, const char *description) {
    spdlog::error("Glfw Error {}: {}\n", error, description);
}

static void
key_callback(GLFWwindow *window, int key, int, int action, int mods) { // NOLINT unnamed parameter is not used
    auto *chip8 = static_cast<chip8::Chip8 *>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        chip8->toggle_pause();
    }
    if (key == GLFW_KEY_R && action == GLFW_PRESS && mods == GLFW_MOD_CONTROL) {
    }

    static constexpr std::array keybindings{
            GLFW_KEY_M,                             // 0
            GLFW_KEY_7, GLFW_KEY_8, GLFW_KEY_9,     // 1 2 3
            GLFW_KEY_Y, GLFW_KEY_U, GLFW_KEY_I,     // 4 5 6
            GLFW_KEY_H, GLFW_KEY_J, GLFW_KEY_K,     // 7 8 9
            GLFW_KEY_N, GLFW_KEY_COMMA, GLFW_KEY_0, // A B C
            GLFW_KEY_O, GLFW_KEY_L, GLFW_KEY_PERIOD // D E F
    };

    for (std::size_t keyn = 0; keyn < keybindings.size(); keyn++) {
        if (key == keybindings[keyn]) {
            if (action == GLFW_PRESS) {
                chip8->keys[keyn] = true;
            } else if (action == GLFW_RELEASE) {
                chip8->keys[keyn] = false;
            }
        }
    }
}


void glfwCleanup(GLFWwindow *window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

int main() {
    // Create window_ with graphics context
    static constexpr int start_zoom_factor = 20; // determines how big the display texture is
    static constexpr auto width = chip8::Chip8::screen_width * start_zoom_factor;
    static constexpr auto height = chip8::Chip8::screen_height * start_zoom_factor;

    GLFWwindow *window = createWindow(width, height);
    if (window == nullptr) {
        return 1;
    }

    chip8::Chip8 chip8;
    chip8.load_rom(maze_data);

    const auto canvas = Canvas{};
    SimpleDisplayTexture display_texture;
    FrameBuffer frame_buffer(width, height);
    Shader shader("res/shaders/vertexShader.glsl", "res/shaders/fragmentShader.glsl");

    {
        GUI imgui(window, chip8);

        glfwSetWindowUserPointer(window, static_cast<void *>(&chip8));
        glfwSetKeyCallback(window, key_callback);

        // Main loop
        while (!glfwWindowShouldClose(window)) { // NOLINT implicit conversion to bool is more readable
            // Poll and handle events (inputs, window_ resize, etc.)
            glfwPollEvents();

            chip8.tick();
            if (chip8.draw_flag) {
                display_texture.load_texture(chip8);
                chip8.draw_flag = false;
            }

            glClearColor(0.0F, 0.0F, 0.0F, 1.0F); // NOLINT color
            glClear(GL_COLOR_BUFFER_BIT);

            // render into FrameBuffer
            frame_buffer.bind_buffer();
            shader.use();
            canvas.draw();
            frame_buffer.unbind_buffer();

            imgui.render(frame_buffer.get_fbo_texture());

            glfwSwapBuffers(window);
        }

    }

    glfwCleanup(window);

    return 0;
}

GLFWwindow *createWindow(int width, int height) {
    // Setup window_
    glfwSetErrorCallback(glfw_error_callback);
    if (glfwInit() == 0) { return nullptr; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(width, height, "Chip8", nullptr, nullptr);

    if (window == nullptr) {
        spdlog::error("Failed to create window");
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = GLFW_TRUE;
    bool err = glewInit() != GLEW_OK;
    if (err) {
        spdlog::error("Failed to initialize GLEW");
        glfwTerminate();
        return nullptr;
    }
    return window;
}
