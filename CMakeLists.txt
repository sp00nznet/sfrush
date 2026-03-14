cmake_minimum_required(VERSION 3.20)

project(SFRushRecompiled VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)

# =============================================================================
# Options
# =============================================================================
set(N64_MODERN_RUNTIME_DIR "${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime" CACHE PATH
    "Path to N64ModernRuntime")
set(RT64_DIR "${CMAKE_SOURCE_DIR}/lib/rt64" CACHE PATH
    "Path to RT64 renderer")
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE PATH
    "vcpkg toolchain file")

# =============================================================================
# SDL2 (via vcpkg)
# =============================================================================
find_package(SDL2 CONFIG REQUIRED)

# =============================================================================
# RT64 Renderer
# =============================================================================
link_directories("C:/vcpkg/installed/x64-windows/lib")
link_directories("C:/vcpkg/installed/x64-windows/lib/manual-link")
set(RT64_STATIC ON CACHE BOOL "Build RT64 as static library" FORCE)
add_subdirectory(${RT64_DIR} ${CMAKE_BINARY_DIR}/rt64)

# =============================================================================
# N64ModernRuntime (librecomp + ultramodern)
# =============================================================================
add_subdirectory(${N64_MODERN_RUNTIME_DIR} ${CMAKE_BINARY_DIR}/N64ModernRuntime)

# =============================================================================
# Recompiled Functions Library
# =============================================================================
file(GLOB RECOMP_SOURCES "RecompiledFuncs/*.c")

add_library(RecompiledFuncs STATIC ${RECOMP_SOURCES})

target_include_directories(RecompiledFuncs PRIVATE
    ${N64_MODERN_RUNTIME_DIR}/N64Recomp/include
    ${CMAKE_SOURCE_DIR}/RecompiledFuncs
)

if(MSVC)
    target_compile_options(RecompiledFuncs PRIVATE /w)
else()
    target_compile_options(RecompiledFuncs PRIVATE -fno-strict-aliasing)
endif()

# =============================================================================
# Main Executable
# =============================================================================
add_executable(SFRushRecompiled
    src/main/main.cpp
    src/main/section_table.cpp
    src/main/rt64_render_context.cpp
    src/main/audio.cpp
    src/main/os_stubs.cpp
)

target_include_directories(SFRushRecompiled PRIVATE
    ${N64_MODERN_RUNTIME_DIR}/N64Recomp/include
    ${N64_MODERN_RUNTIME_DIR}/librecomp/include
    ${N64_MODERN_RUNTIME_DIR}/ultramodern/include
    ${RT64_DIR}/src
    ${RT64_DIR}/src/contrib
    ${RT64_DIR}/src/contrib/hlslpp/include
    ${RT64_DIR}/src/contrib/plume
    ${RT64_DIR}/src/contrib/imgui
    ${RT64_DIR}/src/contrib/mupen64plus-core/src/api
    ${CMAKE_SOURCE_DIR}/RecompiledFuncs
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(SFRushRecompiled PRIVATE NOMINMAX HLSL_CPU FFX_GCC)

target_link_libraries(SFRushRecompiled PRIVATE
    RecompiledFuncs
    librecomp
    ultramodern
    rt64
    SDL2::SDL2
    SDL2::SDL2main
)

if(WIN32)
    target_link_libraries(SFRushRecompiled PRIVATE dxgi d3d12 d3dcompiler dbghelp)
    target_compile_options(SFRushRecompiled PRIVATE /Zi)
    target_link_options(SFRushRecompiled PRIVATE /DEBUG /OPT:REF /OPT:ICF)
endif()
