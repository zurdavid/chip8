#ifndef CHIP8_GUI_H
#define CHIP8_GUI_H

#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "chip8/Chip8.h"
#include <imgui.h>
#include <imfilebrowser.h>

class GUI {
  public:
    GUI(GLFWwindow *window, chip8::Chip8& t_chip8, float &t_zoom_factor);
    void build_context();
    static void render();
    [[nodiscard]] int get_instructions_per_iteration() const { return cycles_per_frame; }

    ~GUI();
    GUI(const GUI&) = delete;
    GUI(GUI&&) = delete;
    GUI& operator=(const GUI&) = delete;
    GUI& operator=(GUI&&) = delete;
  private:
    GLFWwindow *window_;
    chip8::Chip8& chip8;
    int cycles_per_frame = 10;
    float &zoom_factor;

    ImGui::FileBrowser file_dialog;
    ImFont* monospace;

    bool show_demo_window = false;
    bool show_readme_window = false;
    bool show_settings_window = true;
    bool show_control_window = false;
    bool show_memory_window  = false;
    bool shift_implementation_vy = true;

    std::string game_path{};

    std::string help_text{};
    void display_menubar();
    void display_settings_window();
    void display_control_window();
    void display_memory_map();
    void display_readme();
    void load_rom_readme(const std::string &filepath);
};


#endif// CHIP8_GUI_H
