# Find dependencies:
set(DEPENDENCIES_CONFIGURED fmt GLEW glfw3 glm imgui spdlog Catch2 Microsoft.GSL)

foreach (DEPENDENCY ${DEPENDENCIES_CONFIGURED})
    find_package(${DEPENDENCY} CONFIG REQUIRED)
endforeach ()

add_executable(chip8 main.cpp)
target_link_libraries(chip8 PRIVATE project_options project_warnings)

target_link_system_libraries(
        chip8
        PRIVATE
        $<TARGET_OBJECTS:imgui_opengl>
        fmt::fmt
        spdlog::spdlog
        GLEW::GLEW
        glfw
        imgui::imgui
        glm::glm
        Microsoft.GSL::GSL
        imfilebrowser
)

target_include_directories(chip8 PUBLIC
        ../include
        )

set_target_properties(chip8 PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )

add_subdirectory(utilities)
add_subdirectory(chip8)
add_subdirectory(gui)
