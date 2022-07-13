#include "gui/GUI.h"

#include <fstream>

#include <fmt/format.h>
#include <gsl/narrow>
#include <spdlog/spdlog.h>
#include <imgui_internal.h>
#include "../res/bindings/imgui_impl_glfw.h"
#include "../res/bindings/imgui_impl_opengl3.h"

#include "chip8/OpcodeToString.h"

namespace {
    using chip8::State;
}

// translate state to possible action name
const char *state_to_action_name(State state) {
    switch (state) {
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
bool CallBackCheckbox(const char *label, bool *v, auto func) {
    bool old_val = *v;
    ImGui::Checkbox(label, v);
    bool new_val = *v;
    if (new_val != old_val) { func(new_val); }
    return new_val;
}


GUI::GUI(GLFWwindow *window, chip8::Chip8 &t_chip8)
        : window_(window), chip8(t_chip8) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = "res/imgui.ini";
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // NOLINT int flags
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // load fonts and merge icons into it
    static constexpr auto fontsize = 18.0F;
    io.Fonts->AddFontFromFileTTF("res/fonts/DroidSans.ttf", fontsize);
    static constexpr std::array<ImWchar, 3> ranges{0xf000, 0xf3ff, 0};
    ImFontConfig config;
    config.MergeMode = true;
    io.Fonts->AddFontFromFileTTF("res/fonts/Font Awesome 6 Free-Regular-400.otf", fontsize, &config, ranges.data());
    monospace = io.Fonts->AddFontFromFileTTF("res/fonts/DroidSansMono.ttf", fontsize);

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    const char *glsl_version = "#version 330";
    ImGui_ImplOpenGL3_Init(glsl_version);

    // file browser settings
    file_dialog.SetTitle("Load Chip8-ROM...");
    file_dialog.SetTypeFilters({".ch8"});
    // TODO hardcoded path
    file_dialog.SetPwd("roms");

    resetWindow();
}


// load window position and size from ini-file
void GUI::resetWindow() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiWindowSettings* settings = ImGui::FindOrCreateWindowSettings("__WINDOW__");
    settings->WantApply = false;
    if (settings->Size.x != 0 && settings->Size.y != 0) {
        glfwSetWindowSize(window_, settings->Size.x, settings->Size.y);
        glfwSetWindowPos(window_, settings->Pos.x, settings->Pos.y);
    }
    ImGui::Render();
}

void GUI::render(uint32_t texture) {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // display different windows
    display_main_window();
    display_chip8_screen(texture);
    display_file_dialog();
    if (show_settings_window) { display_settings_window(); }
    if (show_control_window) { display_control_window(); }
    if (show_readme_window) { display_readme(); }
    if (show_memory_window) { display_memory_map(); }

    if (show_demo_window) {
        ImGui::ShowDemoWindow(&show_demo_window);
        ImGui::ShowStyleEditor();
    }

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


void GUI::display_main_window() {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0F, 0.0F} );
    // NOLINTBEGIN
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    // NOLINTEND

    ImGui::Begin("MainWindow", nullptr, window_flags);

    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0F, 0.0F), 0);
    display_menubar();
    ImGui::End();
}


void GUI::display_chip8_screen(uint32_t texture) const {
    static constexpr ImVec4 bg_color{0.16F, 0.29F, 0.48F, 0.0F};
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_color);

    ImGuiWindowClass window_class;
    window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar; // ImGuiDockNodeFlags_NoTabBar is an internal flag
    ImGui::SetNextWindowClass(&window_class);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove;
    ImGui::Begin("Chip8 display", nullptr, window_flags);
        auto screen_pos = ImGui::GetCursorScreenPos();
        const auto win_size = ImGui::GetWindowSize();
        const auto win_width = win_size.x - 10;
        const auto win_height = win_size.y - 10;
        static constexpr float padding = 10;
        float width = win_width;
        float height = win_height;
        if (fixed_aspect_ratio) {
            auto x_ratio = win_width / chip8::Chip8::screen_width;
            auto y_ratio = win_height / chip8::Chip8::screen_height;
            if (x_ratio < y_ratio) {
                height = win_height / y_ratio * x_ratio;
                screen_pos.y +=  (win_height - height) / 2 - padding;
                height += screen_pos.y - padding;
            } else {
                // TODO somethings wrong here
                width = win_width / x_ratio * y_ratio;
                screen_pos.x +=  (win_width - width) / 2 - padding;
                width += screen_pos.x;
            }
        }
        width -= padding;
        height -= padding;
        // pass the display texture of the FBO
        ImGui::GetWindowDrawList()->AddImage(
            reinterpret_cast<ImTextureID>(texture), // NOLINT cast to void* (OpenGL texture)
            {screen_pos.x, screen_pos.y},
            ImVec2(ImGui::GetCursorScreenPos().x + width,
                   ImGui::GetCursorScreenPos().y + height),
            ImVec2(0, 1),
            ImVec2(1, 0));
    ImGui::End();
    ImGui::PopStyleColor();
}


void GUI::display_menubar() {
    const auto state = chip8.get_state();

    ImGui::BeginMenuBar();
    {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open...", "Ctrl+O")) { file_dialog.Open(); }
            if (ImGui::MenuItem("Exit")) { glfwSetWindowShouldClose(window_, GLFW_TRUE); }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Chip8")) {
            ImGui::BeginDisabled(state == State::Empty);
            if (ImGui::MenuItem(state_to_action_name(state), "Space")) { chip8.toggle_pause(); }
            ImGui::EndDisabled();

            ImGui::BeginDisabled(chip8.get_state() == State::Empty);
            if (ImGui::MenuItem("Reset ROM", "Ctrl+R")) { chip8.reset_rom(); }//chip8.load_rom_from_file(game_path); }
            ImGui::EndDisabled();
            ImGui::EndMenu();

        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Settings", nullptr, &show_settings_window);
            ImGui::MenuItem("Control", nullptr, &show_control_window);
            ImGui::MenuItem("Memory Map", nullptr, &show_memory_window);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("About")) {
            ImGui::MenuItem("Show ROM-Readme", nullptr, &show_readme_window);
            ImGui::MenuItem("Show ImGui Demo Window", nullptr, &show_demo_window);
            ImGui::EndMenu();
        }
    }
    ImGui::EndMenuBar();
}


void GUI::display_file_dialog() {
    file_dialog.Display();
    if (file_dialog.HasSelected()) {
        // set window_ title
        const auto title = fmt::format("Chip8 - {}", file_dialog.GetSelected().filename().string());
        glfwSetWindowTitle(window_, title.c_str());
        // load game
        game_path = file_dialog.GetSelected().string();
        chip8.load_rom_from_file(game_path);
        auto readme_file = file_dialog.GetSelected().replace_extension(".txt").string();
        load_rom_readme(readme_file);

        file_dialog.ClearSelected();
    }
}


void GUI::display_settings_window() {
    ImGui::Begin("Settings", &show_settings_window); // , nullptr, ImGuiWindowFlags_NoMove);

    ImGui::Text("Number of instruction cycles per frame:");
    static constexpr auto max_cycles_per_frame = 500;
    ImGui::SliderInt("cycles/frame:", &chip8.cycles_per_frame, 1, max_cycles_per_frame);

    ImGui::Checkbox("Chip8-Display: Fixed Aspect Ratio", &fixed_aspect_ratio);

    CallBackCheckbox(
        "Shift operations: shift value of register Vy",
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
    ImGui::Begin("call stack", &show_control_window);
    const auto state = chip8.get_state();

    ImGui::BeginDisabled(state != State::Paused && state != State::Reset);
    if (ImGui::Button("Execute instruction")) { chip8.exec_op_cycle(); }
    ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::BeginDisabled(state == State::Empty);
    if (ImGui::Button(state_to_action_name(state))) { chip8.toggle_pause(); }
    ImGui::EndDisabled();

    ImGui::Text("Tick count: %zu", chip8.get_tick_count());
    ImGui::Separator();

    const auto pc = chip8.get_pc();
    ImGui::BeginChild("stack", ImVec2(ImGui::GetContentRegionAvail().x * 0.5F, 0), true); // NOLINT no magic number
    const uint16_t op = gsl::narrow_cast<uint16_t>(chip8.get_memory()[pc] << 8) | chip8.get_memory()[pc + 1]; // NOLINT signed because of int promotion
    auto op_text = chip8::opcode_to_assembler(op);
    ImGui::Text("%04X \t %s", op, op_text.data());

    ImGui::Separator();

    auto call_stack = chip8.get_call_stack();
    for (const auto opcode: call_stack) {
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
    for (int n = 0; const auto reg: registers) {
        ImGui::Text("V%X = 0x%02X", n++, reg);
    }
    ImGui::Text("DelayTimer: %d", chip8.get_delay_timer());
    ImGui::Text("Sound Timer: %d", chip8.get_sound_timer());
    ImGui::EndChild();

    ImGui::End();
}

// On hover show the opcode in assembly and a textual description
static void MemText(const uint16_t word) {
    ImGui::TextUnformatted(fmt::format("{:04x}", word).c_str());
    if (ImGui::IsItemHovered()) {

        static constexpr auto max_tooltip_width = 20.0F;
        auto assembler = chip8::opcode_to_assembler(word);
        auto assembler2 = chip8::opcode_to_assembler_formatted(word);
        auto help_text = chip8::opcode_to_assembler_help_text(word);
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * max_tooltip_width);
        ImGui::Text("%s\n%s\n\n%s", assembler.data(), assembler2.data(), help_text.data());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void GUI::display_memory_map() {
    const auto &mem = chip8.get_memory();
    static constexpr auto words_per_row = 8;
    static constexpr auto rows = chip8::Chip8::mem_size / 16;

    auto flags = ImGuiTableFlags_Borders // NOLINT enum
                 | ImGuiTableFlags_RowBg
                 | ImGuiTableFlags_SizingFixedFit;
    ImGui::Begin("memory map", &show_memory_window);
    ImGui::PushFont(monospace);
    if (ImGui::BeginTable("test", words_per_row + 1, flags)) {
        ImGui::TableSetupColumn("");
        for (int i = 0; i < words_per_row; i++) {
            ImGui::TableSetupColumn(fmt::format("{:04x}", i * 2).c_str());
        }
        ImGui::TableHeadersRow();

        for (unsigned int row = 0; row < rows; row++) {
            if (row << 4U == chip8::Chip8::pc_start_address) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("prog:");
                ImGui::TableNextRow();
            }
            ImGui::TableNextColumn();
            ImGui::Text("0x%04x", row << 4U);
            for (unsigned int col = 0; col < words_per_row; col++) {
                const auto idx = row * 16 + 2 * col;
                const auto byte1 = mem[idx];
                const auto byte2 = mem[idx + 1];
                const auto word = gsl::narrow<uint16_t>((byte1 << 8U) | byte2); // NOLINT
                ImGui::TableNextColumn();
                MemText(word);
                if (idx == chip8.get_pc()) {
                    const ImU32 cell_bg_color = ImGui::GetColorU32(ImVec4(0.3F, 0.3F, 0.7F, 0.65F));
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, cell_bg_color);
                }
            }
        }
        ImGui::EndTable();
    }
    ImGui::PopFont();
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
    ImGuiWindowSettings* settings = ImGui::FindOrCreateWindowSettings("__WINDOW__");
    settings->WantApply = false;
    int x{};
    int y{};
    glfwGetWindowSize(window_, &x, &y);
    settings->Size = ImVec2ih(gsl::narrow_cast<short>(x), gsl::narrow_cast<short>(y));
    glfwGetWindowPos(window_, &x, &y);
    settings->Pos = ImVec2ih(gsl::narrow_cast<short>(x), gsl::narrow_cast<short>(y));

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
