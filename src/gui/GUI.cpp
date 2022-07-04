#include <fmt/format.h>
#include <fstream>
#include <spdlog/spdlog.h>
#include <gsl/narrow>

#include "gui/GUI.h"
#include "chip8/InstructionSet.h"

#include "../res/bindings/imgui_impl_glfw.h"
#include "../res/bindings/imgui_impl_opengl3.h"


using chip8::State, State::Paused, chip8::State::Running, chip8::State::Empty, chip8::State::Reset;

// translate state to possible action name
const char* state_to_action_name(State t){
    switch(t){
        case State::Running:
            return "Pause";
        case State::Paused:
            return "Resume";
        case State::Reset:  // NOLINT intentional
            return "Start";
        case State::Empty:
            return "Start"; // Corresponding Button should be disabled
    }
    return "";
}


// ImGui::Checkbox wrapper function, that calls a function when the checkbox is clicked.
bool CallBackCheckbox(const char* label, bool* v, auto func) {
    bool old_val = *v;
    ImGui::Checkbox(label, v);
    bool new_val = *v;
    if (new_val != old_val) { func(new_val); }
    return new_val;
}


GUI::GUI(GLFWwindow *window, chip8::Chip8 &t_chip8) : window_(window), chip8(t_chip8) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();

    // Style
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // load font and merge icons into it
    static constexpr auto fontsize = 18.0F;
    [[maybe_unused]] ImFont* font = io.Fonts->AddFontFromFileTTF("res/fonts/DroidSans.ttf", fontsize);
    static constexpr std::array<ImWchar,3> ranges{ 0xf000, 0xf3ff, 0 };
    ImFontConfig config;
    config.MergeMode = true;
    io.Fonts->AddFontFromFileTTF("res/fonts/Font Awesome 6 Free-Regular-400.otf", fontsize, &config, ranges.data());

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    const char *glsl_version = "#version 330";
    ImGui_ImplOpenGL3_Init(glsl_version);

    // file browser settings
    fileDialog.SetTitle("Load Chip8-ROM...");
    fileDialog.SetTypeFilters({ ".ch8" });
    // TODO hardcoded path
    fileDialog.SetPwd("roms");
}


void GUI::render() { ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); }


void GUI::build_context() {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

//    if (IsKeyDown(ImGuiKey key)) {
//
//    }

    if (show_demo_window) { ImGui::ShowDemoWindow(&show_demo_window); }
    ImGui::ShowStyleEditor();
    ImGui::ShowFontSelector("change font");

    fileDialog.Display();
    // TODO this is handling input - needs to be somewhere else
    if (fileDialog.HasSelected()) {
        // set window_ title
        const auto title = fmt::format("Chip8 - {}", fileDialog.GetSelected().filename().string());
        glfwSetWindowTitle(window_, title.c_str());
        // load game
        game_path = fileDialog.GetSelected().string();
        chip8.load_rom(game_path);
        auto readme_file = fileDialog.GetSelected().replace_extension(".txt").string();
        load_rom_readme(readme_file);

        fileDialog.ClearSelected();
    }

    display_menubar();
    if (show_settings_window) { display_settings_window(); }
    if (show_control_window) { display_control_window(); }
    if (show_readme_window) { display_readme(); }

    // Rendering
    ImGui::Render();
}



void GUI::display_menubar() {
    const auto state = chip8.get_state();

    ImGui::BeginMainMenuBar();
    {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open...", "Ctrl+O")) { fileDialog.Open(); }
            if (ImGui::MenuItem("Exit")) { glfwSetWindowShouldClose(window_, GLFW_TRUE); }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Chip8")) {
            ImGui::BeginDisabled(state == Empty);
            if (ImGui::MenuItem(state_to_action_name(state), "Space")) { chip8.toggle_pause(); }
            ImGui::EndDisabled();

            ImGui::BeginDisabled(chip8.get_state() == State::Empty);
            if (ImGui::MenuItem("Reset ROM", "Ctrl+R")) { chip8.load_rom(game_path); }
            ImGui::EndDisabled();
            ImGui::EndMenu();

        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Settings", nullptr,  &show_settings_window );
            ImGui::MenuItem("Control", nullptr, &show_control_window );
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("About")) {
            ImGui::MenuItem("Show ROM-Readme", nullptr, &show_readme_window);
            ImGui::MenuItem("Show ImGui Demo Window", nullptr, &show_demo_window );
            ImGui::EndMenu();
        }
    }
    ImGui::EndMainMenuBar();
}


void GUI::display_settings_window() {
    ImGui::Begin("Settings");

    ImGui::Text("Number of instruction cycles per frame:");
    static constexpr auto max_cycles_per_frame = 500;
    ImGui::SliderInt("cycles/frame:", &chip8.cycles_per_frame, 1, max_cycles_per_frame);

    CallBackCheckbox(
        "Shift operations: shift value of register VY",
        &shift_implementation_vy,
        [this] (bool v) { this->chip8.set_shift_implementation(v);  }
    );

    ImGui::Separator(); ImGui::Separator();


    ImGui::Separator();

    static constexpr auto milis_in_second = 1000.0F;
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
      static_cast<double>(milis_in_second / ImGui::GetIO().Framerate),
      static_cast<double>(ImGui::GetIO().Framerate));

    ImGui::End();
}

void GUI::display_control_window() {
    ImGui::Begin("call stack");
    const auto state = chip8.get_state();

    ImGui::BeginDisabled( state != Paused && state != Reset);
    if (ImGui::Button("Execute instruction")) { chip8.exec_op_cycle(); }
    ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::BeginDisabled(state == Empty);
    if (ImGui::Button(state_to_action_name(state))) { chip8.toggle_pause(); }
    ImGui::EndDisabled();

    ImGui::Text("Tick count: %zu", chip8.get_tick_count());
    ImGui::Separator();

    const auto pc = chip8.get_pc();
    ImGui::BeginChild("stack", ImVec2(ImGui::GetContentRegionAvail().x * 0.5F, 0), true); // NOLINT no magic number
        const uint16_t op = gsl::narrow_cast<uint16_t>(chip8.get_memory()[pc] << 8) + chip8.get_memory()[pc + 1]; // NOLINT signed because of int promotion
        auto op_text = chip8::opcode_to_assembler(op);
        ImGui::Text("%04X \t %s", op, op_text.data());

        ImGui::Separator();

        auto call_stack = chip8.get_call_stack();
        for (const auto opcode : call_stack) {
            auto assembler = chip8::opcode_to_assembler(opcode);
            ImGui::Text("%04X \t %s", opcode, assembler.data());
        }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("registers", ImVec2(0, 0), true);
        ImGui::Text("PC: 0x%2X (%d)", pc, pc);
        ImGui::Separator();
        const auto I = chip8.get_i();
        ImGui::Text("I: %X (%d)", I, I);

    auto registers = chip8.get_registers();
        for (int n = 0; const auto reg : registers) {
            ImGui::Text("V%X = 0x%02X", n++, reg);
    }
    ImGui::Text("DelayTimer: %d", chip8.get_delay_timer());
    ImGui::Text("Sound Timer: %d", chip8.get_sound_timer());
    ImGui::EndChild();

    ImGui::End();
}


void GUI::load_rom_readme(const std::string &filepath) {
    std::ifstream readme_file(filepath);
    std::stringstream ss;
    if (readme_file.is_open()) {
        ss << readme_file.rdbuf();
        help_text = ss.str();
    } else {
        spdlog::info("Could not open readme-file: {}", filepath);
        help_text = "No Readme available!";
    }
}


void GUI::display_readme() {
    ImGui::Begin("Readme", &show_readme_window);
    ImGui::TextWrapped("%s", help_text.c_str());
    ImGui::End();
}


GUI::~GUI() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
