find_package(imgui CONFIG REQUIRED)

add_library(imfilebrowser INTERFACE)

target_include_directories(imfilebrowser INTERFACE ./)

# Link dependencies:
target_link_system_libraries(
        imfilebrowser
        INTERFACE
        imgui::imgui
)
