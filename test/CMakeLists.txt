# ---- Dependencies ----

find_package(Catch2 CONFIG REQUIRED)

include(Catch)

add_library(catch_main OBJECT catch_main.cpp)
target_link_system_libraries(catch_main PUBLIC Catch2::Catch2)

# ---- Tests ----

macro(TargetDisableClangTidy target)
    set_target_properties(${target}
            PROPERTIES
            CXX_CLANG_TIDY ""
            )
endmacro()

find_package(spdlog CONFIG REQUIRED)
find_package(Microsoft.GSL)

add_executable(tests tests.cpp ../src/chip8/Chip8.cpp)
target_link_libraries(tests
        PRIVATE
        project_warnings
        project_options
        )

target_link_system_libraries(tests
        PRIVATE
        catch_main
        spdlog::spdlog
        Microsoft.GSL::GSL
        )

target_include_directories(tests PUBLIC
        ../include
        )

TargetDisableClangTidy(tests)

add_executable(integration_tests integration_tests.cpp ../src/chip8/Chip8.cpp)
target_link_libraries(tests
        PRIVATE
        project_warnings
        project_options
        )

target_link_system_libraries(integration_tests
        PRIVATE
        catch_main
        spdlog::spdlog
        Microsoft.GSL::GSL
        )

target_include_directories(integration_tests PUBLIC
        ../include
        )

#TargetDisableClangTidy(integration_tests)


# automatically discover tests that are defined in catch based test files you can modify the unittests. TEST_PREFIX to
# whatever you want, or use different for different binaries
catch_discover_tests(
  tests
  TEST_PREFIX
  "unittests."
  EXTRA_ARGS
  -s
  --reporter=xml
  --out=tests.xml)

# ---- Constexpr tests ----

# Add a file containing a set of constexpr tests
add_executable(constexpr_tests constexpr_tests.cpp)
target_link_libraries(constexpr_tests PRIVATE catch_main)

target_include_directories(constexpr_tests PUBLIC
        ../include
        )

catch_discover_tests(
  constexpr_tests
  TEST_PREFIX
  "constexpr."
  EXTRA_ARGS
  -s
  --reporter=xml
  --out=constexpr.xml)