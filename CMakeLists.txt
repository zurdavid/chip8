cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)

# Add project_options v0.22.4
# https://github.com/aminya/project_options
include(FetchContent)
FetchContent_Declare(_project_options URL https://github.com/aminya/project_options/archive/refs/tags/v0.22.4.zip)
FetchContent_MakeAvailable(_project_options)
include(${_project_options_SOURCE_DIR}/Index.cmake)


set(ENABLE_DEVELOPER_MODE
        TRUE
        CACHE BOOL "Enable 'developer mode'")

# Change this to false if you want to disable warnings_as_errors in developer mode
set(OPT_WARNINGS_AS_ERRORS_DEVELOPER_DEFAULT TRUE)

# install vcpkg dependencies: - should be called before defining project()
# run_vcpkg()

set(ENABLE_CACHE ON)

# Set the project name and language
project(
        chip8
        VERSION 0.0.1
        DESCRIPTION "A Chip8 emulator"
        LANGUAGES CXX)

include(${_project_options_SOURCE_DIR}/src/DynamicProjectOptions.cmake)
dynamic_project_options(
        # Note: PCH is disabled by default in developer mode because these headers become
        # globally included and they can mask other errors
        PCH_HEADERS
        <vector>
        <array>
        <string>
        <spdlog/spdlog.h>
        ENABLE_CONAN
        CPPCHECK_OPTIONS
        --enable=style,performance,warning,portability
        --inline-suppr
        # We cannot act on a bug/missing feature of cppcheck
        --suppress=cppcheckError
        --suppress=internalAstError
        # if a file does not have an internalAstError, we get an unmatchedSuppression error
        --suppress=unmatchedSuppression
        --suppress=passedByValue
        --suppress=syntaxError
        --suppress=clang-diagnostic-keyword-macro
        --inconclusive
)

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(test)
