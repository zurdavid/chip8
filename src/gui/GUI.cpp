#include <fmt/format.h>
#include <ranges>
#include <fstream>
#include <iostream>

#include "gui/GUI.h"
#include "chip8/InstructionSet.h"

#include "../bindings/imgui_impl_glfw.h"
#include "../bindings/imgui_impl_opengl3.h"

GUI::GUI(GLFWwindow *window, chip8::Chip8 &ch8) : window_(window), chip8(ch8) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    const char *glsl_version = "#version 330";
    ImGui_ImplOpenGL3_Init(glsl_version);

    // scale
    static constexpr float scale_factor = 1.3F;
    ImGui::GetStyle().ScaleAllSizes(scale_factor);
    ImGui::GetIO().FontGlobalScale = scale_factor;

    // create a file browser instance
    fileDialog.SetTitle("Load Chip8-ROM...");
    fileDialog.SetTypeFilters({ ".ch8" });
    // TODO hardcoded path
    auto path = fileDialog.GetPwd().parent_path().parent_path().append("roms");
    fileDialog.SetPwd(path);
}


void GUI::render() { ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); }


void GUI::build_context() {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (show_demo_window) { ImGui::ShowDemoWindow(&show_demo_window); }

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
    if (show_control_window) { display_control_window(); }
    //display_memory_map();
    if (show_readme_window) { display_readme(); }
    //display_program_code();

    // Rendering
    ImGui::Render();
}


void GUI::display_menubar() {
    ImGui::BeginMainMenuBar();
    {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open...")) { fileDialog.Open(); }
            if (ImGui::MenuItem("Reload ROM")) { chip8.load_rom(game_path); }
            if (ImGui::MenuItem("Exit")) { glfwSetWindowShouldClose(window_, GLFW_TRUE); }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::Checkbox("Control", &show_control_window );
            if (ImGui::MenuItem("Debug")) { show_debug_window = true; }
            if (ImGui::MenuItem("Debug")) { chip8.load_rom(game_path); }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("About")) {
            if (ImGui::MenuItem("Show ROM-Readme")) { show_readme_window = true; }
            ImGui::Checkbox("Show ImGui Demo Window", &show_demo_window );
            ImGui::EndMenu();
        }
    }
    ImGui::EndMainMenuBar();
}


void GUI::display_control_window() {
    // Begin Window
    ImGui::Begin("Chip8-Emulator!");

    // TODO crashes if no program is loaded
    if (ImGui::Button(chip8.game_running() ? "Pause" : "Resume")) { chip8.pause_unpause(); }

    const auto pc = chip8.getPC();
    ImGui::Text("PC: %X (%d)", pc, pc);
    const uint16_t opcode = (chip8.get_memory()[pc] << 8) + chip8.get_memory()[pc + 1];
    ImGui::Text("Instruction: %04X", opcode);
    auto assembler = chip8::op_to_assembler(opcode);
    ImGui::Text("%s", assembler.data());
    ImGui::Text("I: %X (%d)", chip8.getI(), chip8.getI());

    ImGui::Text("Number of instruction cycles per frame:");
    ImGui::SliderInt("cycles/frame:", &cycles_per_frame, 1, 50);

    ImGui::Text("DT: %d", chip8.getDT());
    ImGui::SameLine();
    if (ImGui::Button("decrease")) { chip8.signal(); }
    ImGui::SameLine();
    auto current_DT = chip8.getDT();
    if (ImGui::Button("set to 0")) {
        for (int i = 0; i < current_DT; i++) { chip8.signal(); }
    }

    if (ImGui::Button("Execute instruction")) { chip8.exec_op_cycle(); }

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
      static_cast<double>(1000.0F / ImGui::GetIO().Framerate),
      static_cast<double>(ImGui::GetIO().Framerate));

    ImGui::End();
}


void GUI::display_memory_map() {
    const auto V = chip8.get_registers();
    if (ImGui::BeginTable("##registers",
          2,
          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
        for (int row = 0; auto v : V) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%1X", row++);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%02X", v);
        }
        ImGui::EndTable();
    }

    // Memory
    const auto mem = chip8.get_memory();
    const auto *itr = mem.begin();
    constexpr auto num_cols = int{ 17 };
    static ImVector<int> selection;

    if (ImGui::BeginTable("##memory",
          num_cols,
          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
        // Table Header
        ImGui::TableHeadersRow();
        for (auto col_idx : std::views::iota(0, 16)) {
            ImGui::TableSetColumnIndex(col_idx + 1);
            ImGui::Text("%2X", col_idx);
        }

        // Table Body
        // TODO whats going on here --> maybe_unused
        for (int cnt = 0, row_idx = 0; [[maybe_unused]] const auto &val : mem) {
            auto col_idx = cnt++ % 16;
            if (col_idx == 0) {
                const auto row_id = row_idx++;
                const bool item_is_selected = selection.contains(row_id);
                ImGui::PushID(row_id);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                const auto label_txt = fmt::format("0x{:03X}_", row_id);
                if (ImGui::Selectable(
                      label_txt.c_str(), item_is_selected, ImGuiSelectableFlags_SpanAllColumns)) {
                    if (ImGui::GetIO().KeyCtrl) {
                        if (item_is_selected) {
                            selection.find_erase_unsorted(row_id);
                        } else {
                            selection.push_back(row_id);
                        }
                    } else {
                        selection.clear();
                        selection.push_back(row_id);
                    }
                }
            }
            ImGui::TableSetColumnIndex(col_idx + 1);
            ImGui::Text("%02X", *(itr++));

            if (col_idx == 0xF) { ImGui::PopID(); }
        }
        ImGui::EndTable();
    }
}


void GUI::cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}


void GUI::load_rom_readme(const std::string &filepath) {
    std::ifstream readme_file(filepath);
    std::stringstream ss;
    if (readme_file.is_open()) {
        ss << readme_file.rdbuf();
        help_text = ss.str();
    } else {
        // spdlog::error("Could not open file: {}", filename);
        std::cerr << "Could not open readme-file: " << filepath << '\n';
        help_text = "No Readme available!";
    }
}


void GUI::display_readme() {
    ImGui::Begin("Readme", &show_readme_window);
    ImGui::TextWrapped("%s", help_text.c_str());
    ImGui::End();
}


void GUI::display_program_code() {
    // if (!chip8.game_running()) return;
    ImGui::Begin("Program");
    for (int pc = chip8::Chip8::PC_START_ADDRESS; pc < chip8::Chip8::PC_START_ADDRESS + chip8.program_size; pc += 2) {
        const uint16_t opcode = (chip8.get_memory()[pc] << 8) + chip8.get_memory()[pc + 1];
        const auto op = chip8::op_to_assembler(opcode);
        ImGui::Text("Instruction: %04X", opcode);
        ImGui::Text("%s", op.data());
    }
    ImGui::End();
}
