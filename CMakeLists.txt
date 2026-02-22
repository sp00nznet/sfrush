cmake_minimum_required(VERSION 3.20)
project(SFRushRecompiled
    VERSION 0.1.0
    DESCRIPTION "San Francisco Rush: Extreme Racing - Static Recompilation"
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Version defines
add_compile_definitions(
    SFRUSH_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    SFRUSH_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    SFRUSH_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# ──────────────────────────────────────────────
# Dependencies
# ──────────────────────────────────────────────

# SDL2
include(FetchContent)
FetchContent_Declare(
    SDL2
    URL https://github.com/libsdl-org/SDL/releases/download/release-2.30.3/SDL2-2.30.3.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(SDL2)

# N64ModernRuntime (provides ultramodern + librecomp)
if(EXISTS "${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/CMakeLists.txt")
    add_subdirectory(lib/N64ModernRuntime)
endif()

# RT64 - N64 RDP GPU emulation
if(EXISTS "${CMAKE_SOURCE_DIR}/lib/rt64/CMakeLists.txt")
    add_subdirectory(lib/rt64)
endif()

# ──────────────────────────────────────────────
# Recompiled Functions Library
# ──────────────────────────────────────────────

file(GLOB RECOMP_SOURCES "RecompiledFuncs/*.c")
if(RECOMP_SOURCES)
    add_library(RecompiledFuncs STATIC ${RECOMP_SOURCES})
    target_include_directories(RecompiledFuncs PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    # Suppress warnings in generated code
    if(MSVC)
        target_compile_options(RecompiledFuncs PRIVATE /w)
    else()
        target_compile_options(RecompiledFuncs PRIVATE
            -fno-strict-aliasing
            -Wno-unused-variable
            -Wno-implicit-function-declaration
            -Wno-unused-but-set-variable
            -Wno-sometimes-uninitialized
        )
    endif()
    if(TARGET librecomp)
        target_link_libraries(RecompiledFuncs PRIVATE librecomp)
    endif()
endif()

# ──────────────────────────────────────────────
# Recompiled Patches Library
# ──────────────────────────────────────────────

file(GLOB PATCH_SOURCES "RecompiledPatches/*.c")
if(PATCH_SOURCES)
    add_library(PatchesLib STATIC ${PATCH_SOURCES})
    target_include_directories(PatchesLib PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/patches
    )
    if(MSVC)
        target_compile_options(PatchesLib PRIVATE /w)
    else()
        target_compile_options(PatchesLib PRIVATE -fno-strict-aliasing)
    endif()
endif()

# ──────────────────────────────────────────────
# Main Executable
# ──────────────────────────────────────────────

set(SFRUSH_SOURCES
    src/main/main.cpp
    src/main/rt64_render_context.cpp
    src/main/support.cpp
    src/main/register_overlays.cpp
    src/main/register_patches.cpp
    src/game/input.cpp
    src/game/stubs.cpp
    src/game/recomp_api.cpp
    src/game/rom_decompression.cpp
    rsp/aspMain.cpp
)

add_executable(SFRushRecompiled ${SFRUSH_SOURCES})

target_include_directories(SFRushRecompiled PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(SFRushRecompiled PRIVATE SDL2::SDL2 SDL2::SDL2main)

if(TARGET librecomp)
    target_link_libraries(SFRushRecompiled PRIVATE librecomp)
endif()
if(TARGET ultramodern)
    target_link_libraries(SFRushRecompiled PRIVATE ultramodern)
endif()
if(TARGET rt64)
    target_link_libraries(SFRushRecompiled PRIVATE rt64)
endif()
if(TARGET RecompiledFuncs)
    target_link_libraries(SFRushRecompiled PRIVATE RecompiledFuncs)
endif()
if(TARGET PatchesLib)
    target_link_libraries(SFRushRecompiled PRIVATE PatchesLib)
endif()

# Platform-specific
if(WIN32)
    target_link_libraries(SFRushRecompiled PRIVATE Winmm.lib Dbghelp.lib)
endif()

# Generate PDB and MAP for debugging in Release
if(MSVC)
    set_target_properties(SFRushRecompiled PROPERTIES
        LINK_FLAGS "/DEBUG /MAP"
    )
endif()

# Copy SDL2 DLL on Windows
if(WIN32)
    add_custom_command(TARGET SFRushRecompiled POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL2::SDL2>
            $<TARGET_FILE_DIR:SFRushRecompiled>
        COMMENT "Copying SDL2.dll"
    )
endif()
