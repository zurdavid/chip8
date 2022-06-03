#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "gui/GUI.h"

#include <spdlog/spdlog.h>
#include <ranges>
#include <string_view>

#include "utilities/Shader.h"
#include "utilities/Canvas.h"
#include "chip8/Chip8.h"

using Chip8Texture = std::array<uint8_t, chip8::Chip8::SCREEN_WIDTH * chip8::Chip8::SCREEN_HEIGHT>;

GLFWwindow *createWindow(int width, int height);
unsigned int create_texture();
void load_texture(const chip8::Chip8 &chip8, Chip8Texture &texture);
void handleKeyEvents(GLFWwindow *window, chip8::Chip8 &chip8);

static void glfw_error_callback(int error, const char *description)
{
    spdlog::error("Glfw Error {}: {}\n", error, description);
}

static void key_callback(GLFWwindow* window, int key, int , int action, int)
{
    auto* chip8 = static_cast<chip8::Chip8*>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        chip8->toggle_pause();
    }
    static constexpr std::array keybindings{
        GLFW_KEY_M,                             // 0
        GLFW_KEY_7, GLFW_KEY_8, GLFW_KEY_9,     // 1 2 3
        GLFW_KEY_Y, GLFW_KEY_U, GLFW_KEY_I,     // 4 5 6
        GLFW_KEY_H, GLFW_KEY_J, GLFW_KEY_K,     // 7 8 9
        GLFW_KEY_N, GLFW_KEY_COMMA, GLFW_KEY_0, // A B C
        GLFW_KEY_O, GLFW_KEY_L, GLFW_KEY_PERIOD // D E F
    };

    for (size_t keyn = 0; keyn < keybindings.size(); keyn++) {
        if (key == keybindings[keyn]) {
            if (action == GLFW_PRESS)
                chip8->keys[keyn] = true;
            else if (action == GLFW_RELEASE)
                chip8->keys[keyn] = false;
        }
    }
}


int main() {
    // Create window_ with graphics context
    static constexpr auto width = 2 * 640; // 64 * 10;
    static constexpr auto height = 2 * 320; // 32 * 10;
    GLFWwindow *window = createWindow(width, height);
    if(window == nullptr) {return 1;}

    const auto canvas = Canvas{};
    chip8::Chip8 chip8;

    auto chip8_texture = chip8.getScreen();
    create_texture();
    load_texture(chip8, chip8_texture);

    Shader shader("../../res/shaders/vertexShader.glsl", "../../res/shaders/fragmentShader.glsl");

    GUI imgui(window, chip8);

    glfwSetWindowUserPointer(window, static_cast<void*>(&chip8));
    glfwSetKeyCallback(window, key_callback);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Poll and handle events (inputs, window_ resize, etc.)
        glfwPollEvents();
        handleKeyEvents(window, chip8);
        chip8.tick();
        imgui.build_context();

        // TODO clean up
        int display_w{};
        int display_h{};
        glfwGetFramebufferSize(window, &display_w, &display_h);
        auto y = (display_h - 320) / 2;
        auto x = 50;
        auto X = 640;
        auto Y = 320;
        glViewport(x, y, X, Y);

        glClearColor(0.1F, 0.3, 0.3, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);

        if (chip8.draw_flag) {
            load_texture(chip8, chip8_texture);
            chip8.draw_flag = false;
        }
        shader.use();
        canvas.draw();

        imgui.render();

        glfwSwapBuffers(window);
    }

    // Cleanup
    imgui.cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}


GLFWwindow *createWindow(int width, int height) {
    // Setup window_
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) { return nullptr; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(width, height, "Chip8", nullptr, nullptr);

    if (window == nullptr) {
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


void handleKeyEvents(GLFWwindow *window, chip8::Chip8 &chip8){
    // TODO remove
    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
        chip8.exec_op_cycle(); }

//    static constexpr std::array keybindings{
//            GLFW_KEY_M,                             // 0
//            GLFW_KEY_7, GLFW_KEY_8, GLFW_KEY_9,     // 1 2 3
//            GLFW_KEY_Y, GLFW_KEY_U, GLFW_KEY_I,     // 4 5 6
//            GLFW_KEY_H, GLFW_KEY_J, GLFW_KEY_K,     // 7 8 9
//            GLFW_KEY_N, GLFW_KEY_COMMA, GLFW_KEY_0, // A B C
//            GLFW_KEY_O, GLFW_KEY_L, GLFW_KEY_PERIOD // D E F
//    };
//
//    for (size_t key = 0; key < keybindings.size(); key++) {
//        chip8.keys[key] = glfwGetKey(window, keybindings[key]) == GLFW_PRESS;
//    }
}

void load_texture(const chip8::Chip8 &chip8, Chip8Texture &texture) {
    static constexpr int texture_width = chip8::Chip8::SCREEN_WIDTH;
    static constexpr int texture_height = chip8::Chip8::SCREEN_HEIGHT;
    Chip8Texture texture_data = chip8.getScreen();

    // TODO does not work properly
    std::transform(begin(texture), end(texture), begin(texture_data), begin(texture),
      [](const auto v1, const auto v2) { return static_cast<uint8_t>(0.6 * v1 + 0.4 * v2); } );

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
