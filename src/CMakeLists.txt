# Generic test that uses conan libs
add_executable(chip8 main.cpp $<TARGET_OBJECTS:imgui_opengl>)
target_link_libraries(
        chip8
        PRIVATE project_options
        project_warnings
        CONAN_PKG::docopt.cpp
        CONAN_PKG::fmt
        CONAN_PKG::spdlog
        CONAN_PKG::glew
        CONAN_PKG::glfw
        CONAN_PKG::imgui
        CONAN_PKG::glm
)

target_include_directories(chip8 PUBLIC
        ../include
        )

add_subdirectory(utilities)
add_subdirectory(chip8)
add_subdirectory(gui)