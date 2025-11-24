cmake_minimum_required(VERSION 3.13)
include(FetchContent)

if(CATNIP)
    catnip_package(play_event_exporter DEFAULT release)
    catnip_add_preset(release TOOLSET 3ds BUILD_TYPE Release)
    return()
endif()

# Declare project and use gnu++23 (actually gnu++26) and gnu23
project(play_event_exporter LANGUAGES C CXX ASM)
add_executable(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED True
    CXX_EXTENSIONS ON
    C_STANDARD 23
    C_STANDARD_REQUIRED True
    C_EXTENSIONS ON
)

# Generate symbol list and map information (useful for debugging)
dkp_target_generate_symbol_list(${PROJECT_NAME})

# Targets
ctr_generate_smdh(${PROJECT_NAME}.smdh
    NAME "${PROJECT_NAME}"
    DESCRIPTION "Exports PlayEvent history to a text file"
    AUTHOR "(c) 2025 TuxSH"
)

ctr_create_3dsx(${PROJECT_NAME}
    SMDH ${PROJECT_NAME}.smdh
)

# Dependencies (fmtlib)
FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG        12.1.0
)
FetchContent_MakeAvailable(fmt)

target_link_libraries(${PROJECT_NAME} fmt::fmt)

# Options for code generation
target_compile_options(${PROJECT_NAME} PUBLIC
    # Common C/C++ options (__3DS__ is defined in platform file)
    # Some options already defined in CTR_ARCH_SETTINGS / __dkp_init_platform_settings

    -Wall
    -Wextra
    -Wno-missing-field-initializers
    #-Wno-unused-parameter

    -fdiagnostics-color=always # because of Ninja. Remove as needed
    -ftrivial-auto-var-init=pattern
    #-fshort-enums

    # C++ specific options
    # set(CMAKE_CXX_STANDARD 26) isn't working quite yet
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++26 -fno-exceptions -fno-rtti -fno-threadsafe-statics -fconcepts-diagnostics-depth=3>
)

# Source code files
# $ find source -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.s" \) | sed 's|^|${CMAKE_CURRENT_SOURCE_DIR}/|' | sort
target_sources(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/source/flockfile.c
    ${CMAKE_CURRENT_SOURCE_DIR}/source/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/ptmplays.c
)
