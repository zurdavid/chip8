# build ImGUI
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW CONFIG REQUIRED)

macro(TargetDisableClangTidy target)
    set_target_properties(${target}
            PROPERTIES
            CXX_CLANG_TIDY ""
            )
endmacro()

macro(TargetDisableCppCheck target)
    set_target_properties(${target} PROPERTIES C_CPPCHECK "")
    set_target_properties(${target} PROPERTIES C_CLANG_TIDY "")
    set_target_properties(${target} PROPERTIES CXX_CPPCHECK "")
    set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY "")
endmacro()

add_library(imgui_opengl OBJECT
        imgui_impl_glfw.cpp
        imgui_impl_opengl3.cpp
        )

target_compile_options(imgui_opengl PRIVATE -w)
TargetDisableClangTidy(imgui_opengl)
TargetDisableCppCheck(imgui_opengl)

target_link_libraries(imgui_opengl PRIVATE
        imgui::imgui
        glfw::glfw
        GLEW::GLEW
        glm::glm
        )
