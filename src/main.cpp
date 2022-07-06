#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "gui/GUI.h"

#include <spdlog/spdlog.h>
#include <ranges>

#include "utilities/Shader.h"
#include "utilities/Canvas.h"
#include "chip8/Chip8.h"

using Chip8Texture = std::array<uint8_t, chip8::Chip8::screen_size>;

GLFWwindow *createWindow(int width, int height);

unsigned int create_texture();

void load_texture(const chip8::Chip8 &chip8, Chip8Texture &texture);

static void glfw_error_callback(int error, const char *description) {
    spdlog::error("Glfw Error {}: {}\n", error, description);
}

static void key_callback(GLFWwindow *window, int key, int, int action, int mods) { // NOLINT unnamed parameter is not used
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
    static constexpr auto menu_height = 20;
    static constexpr int start_zoom_factor = 20;
    static constexpr auto width = chip8::Chip8::screen_width * start_zoom_factor;
    static constexpr auto height = chip8::Chip8::screen_height * start_zoom_factor + menu_height;
    float zoom_factor = start_zoom_factor;

    GLFWwindow *window = createWindow(width, height);
    if (window == nullptr) {
        return 1;
    }

    const auto canvas = Canvas{};
    chip8::Chip8 chip8;

    auto chip8_texture = chip8.get_screen();
    create_texture();
    load_texture(chip8, chip8_texture);

    Shader shader("res/shaders/vertexShader.glsl", "res/shaders/fragmentShader.glsl");

    {
        GUI imgui(window, chip8, zoom_factor);

        glfwSetWindowUserPointer(window, static_cast<void *>(&chip8));
        glfwSetKeyCallback(window, key_callback);

        // Main loop
        float adder = 0.0F;
        while (!glfwWindowShouldClose(window)) { // NOLINT implicit conversion to bool is more readable
            // Poll and handle events (inputs, window_ resize, etc.)
            glfwPollEvents();

            chip8.tick();
            if (chip8.draw_flag) {
                load_texture(chip8, chip8_texture);
                chip8.draw_flag = false;
            }

            // TODO clean up
            int display_w{};
            int display_h{};
            glfwGetFramebufferSize(window, &display_w, &display_h);
            auto X = static_cast<int>(chip8::Chip8::screen_width * zoom_factor);
            auto Y = static_cast<int>(chip8::Chip8::screen_height * zoom_factor);
            auto x = (display_w - X) / 2;
            auto y = (display_h - Y - menu_height) / 2;
            glViewport(x, y, X, Y);

            // TODO sort out the colors
            glClearColor(0.16F, 0.29F, 0.48F, 1.0F); // NOLINT color
            if (chip8.sound_signal()) {
                adder += 0.1F; // NOLINT
                glClearColor(0.1F + adder, 0.3F + adder, 0.3F, 1.0F); // NOLINT
            }
            glClear(GL_COLOR_BUFFER_BIT);
            shader.use();
            canvas.draw();

            imgui.build_context();
            imgui.render();

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


void load_texture(const chip8::Chip8 &chip8, [[maybe_unused]] Chip8Texture &texture) {
    static constexpr int texture_width = chip8::Chip8::screen_width;
    static constexpr int texture_height = chip8::Chip8::screen_height;
    Chip8Texture texture_data = chip8.get_screen();

    glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RED, texture_width, texture_height,
            0, GL_RED, GL_UNSIGNED_BYTE, texture_data.data());
}


unsigned int create_texture() {
    unsigned int texture{};
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return texture;
}
