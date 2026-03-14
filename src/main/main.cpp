// San Francisco Rush: Extreme Racing — N64 Static Recompilation
// Main entry point
// Based on Extreme-G's proven runtime integration pattern

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>

#include <SDL.h>
#include <SDL_syswm.h>

#include "recomp.h"
#include "librecomp/game.hpp"
#include "librecomp/overlays.hpp"
#include "librecomp/rsp.hpp"
#include "ultramodern/ultramodern.hpp"
#include "ultramodern/renderer_context.hpp"
#include "ultramodern/error_handling.hpp"
#include "ultramodern/events.hpp"
#include "ultramodern/input.hpp"
#include "ultramodern/threads.hpp"

// =============================================================================
// External declarations
// =============================================================================

extern SectionTableEntry section_table[];
extern const size_t num_sections_export;
#define num_sections num_sections_export

extern std::unique_ptr<ultramodern::renderer::RendererContext> create_sfrush_render_context(
    uint8_t* rdram, ultramodern::renderer::WindowHandle window_handle, bool developer_mode);

extern "C" void recomp_entrypoint(uint8_t* rdram, recomp_context* ctx);

extern void sfrush_queue_samples(int16_t* samples, size_t num_samples);
extern size_t sfrush_get_frames_remaining();
extern void sfrush_set_frequency(uint32_t freq);

// =============================================================================
// ROM Hash — SF Rush: Extreme Racing (USA)
// CRC1: 0x2A6B1820 / CRC2: 0x6ABCF466
// This hash is for the patched sfrush_recomp.z64 (with decompressed game code)
// =============================================================================
// TODO: compute actual hash of sfrush_recomp.z64 and put it here
constexpr uint64_t SFRUSH_ROM_HASH = 0x0ULL;

// =============================================================================
// Global state
// =============================================================================
static SDL_Window* g_window = nullptr;

// =============================================================================
// RSP Callbacks
// =============================================================================

RspUcodeFunc* get_rsp_microcode(const OSTask* task) {
    return nullptr;
}

// =============================================================================
// Renderer Callbacks
// =============================================================================

std::unique_ptr<ultramodern::renderer::RendererContext> create_render_context_callback(
    uint8_t* rdram, ultramodern::renderer::WindowHandle window_handle, bool developer_mode) {
    return create_sfrush_render_context(rdram, window_handle, developer_mode);
}

// =============================================================================
// GFX Callbacks (window management)
// =============================================================================

ultramodern::gfx_callbacks_t::gfx_data_t create_gfx() {
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "[SFRush] Failed to initialize SDL2: %s\n", SDL_GetError());
        std::exit(1);
    }

    fprintf(stderr, "[SFRush] SDL initialized: %s\n", SDL_GetCurrentVideoDriver());
    return nullptr;
}

ultramodern::renderer::WindowHandle create_window(ultramodern::gfx_callbacks_t::gfx_data_t) {
    g_window = SDL_CreateWindow(
        "San Francisco Rush: Extreme Racing - Recompiled",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 960,
        SDL_WINDOW_RESIZABLE
    );

    if (!g_window) {
        fprintf(stderr, "[SFRush] Failed to create window: %s\n", SDL_GetError());
        std::exit(1);
    }

    fprintf(stderr, "[SFRush] Window created (1280x960)\n");

    SDL_SysWMinfo wminfo;
    SDL_VERSION(&wminfo.version);
    SDL_GetWindowWMInfo(g_window, &wminfo);

    ultramodern::renderer::WindowHandle handle{};
    handle.window = wminfo.info.win.window;
    handle.thread_id = GetCurrentThreadId();
    return handle;
}

void update_gfx(ultramodern::gfx_callbacks_t::gfx_data_t) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                ultramodern::quit();
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    ultramodern::quit();
                }
                break;
        }
    }
}

// =============================================================================
// Input Callbacks
// =============================================================================

void poll_input() {
}

bool get_input(int controller_num, uint16_t* buttons, float* x, float* y) {
    if (controller_num != 0) return false;

    *buttons = 0;
    *x = 0.0f;
    *y = 0.0f;

    const uint8_t* keys = SDL_GetKeyboardState(nullptr);

    // N64 button mapping
    if (keys[SDL_SCANCODE_RETURN]) *buttons |= 0x8000; // A
    if (keys[SDL_SCANCODE_LSHIFT]) *buttons |= 0x4000; // B
    if (keys[SDL_SCANCODE_Z])      *buttons |= 0x2000; // Z (gas in Rush)
    if (keys[SDL_SCANCODE_SPACE])  *buttons |= 0x1000; // Start
    if (keys[SDL_SCANCODE_UP])     *buttons |= 0x0800; // D-Up
    if (keys[SDL_SCANCODE_DOWN])   *buttons |= 0x0400; // D-Down
    if (keys[SDL_SCANCODE_LEFT])   *buttons |= 0x0200; // D-Left
    if (keys[SDL_SCANCODE_RIGHT])  *buttons |= 0x0100; // D-Right

    // Analog stick
    if (keys[SDL_SCANCODE_W]) *y += 1.0f;
    if (keys[SDL_SCANCODE_S]) *y -= 1.0f;
    if (keys[SDL_SCANCODE_A]) *x -= 1.0f;
    if (keys[SDL_SCANCODE_D]) *x += 1.0f;

    return true;
}

void set_rumble(int controller_num, bool rumble) {
}

ultramodern::input::connected_device_info_t get_connected_device_info(int controller_num) {
    if (controller_num == 0) {
        return { ultramodern::input::Device::Controller, ultramodern::input::Pak::None };
    }
    return { ultramodern::input::Device::None, ultramodern::input::Pak::None };
}

// =============================================================================
// Event Callbacks
// =============================================================================

void vi_callback() {
}

void gfx_init_callback() {
    fprintf(stderr, "[SFRush] Graphics initialized, starting game...\n");
    std::u8string game_id = u8"sfrush";
    recomp::start_game(game_id);
    fprintf(stderr, "[SFRush] Game started\n");
}

// =============================================================================
// Error Handling
// =============================================================================

void error_message_box(const char* msg) {
    fprintf(stderr, "[SFRush ERROR] %s\n", msg);
    if (g_window) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SF Rush Error", msg, g_window);
    }
}

// =============================================================================
// Thread Naming
// =============================================================================

std::string get_game_thread_name(const OSThread* t) {
    return "SFRush-Thread";
}

// =============================================================================
// Crash Handler (Windows SEH with stack trace)
// =============================================================================

static LONG WINAPI crash_handler(EXCEPTION_POINTERS* ep) {
    fprintf(stderr, "\n[CRASH] Exception 0x%08lX at address 0x%p\n",
            ep->ExceptionRecord->ExceptionCode,
            ep->ExceptionRecord->ExceptionAddress);
    if (ep->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
        fprintf(stderr, "[CRASH] Access violation %s address 0x%p\n",
                ep->ExceptionRecord->ExceptionInformation[0] ? "writing" : "reading",
                (void*)ep->ExceptionRecord->ExceptionInformation[1]);
    }
    HMODULE hMod = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                       (LPCSTR)ep->ExceptionRecord->ExceptionAddress, &hMod);
    if (hMod) {
        char modName[MAX_PATH];
        GetModuleFileNameA(hMod, modName, MAX_PATH);
        uintptr_t offset = (uintptr_t)ep->ExceptionRecord->ExceptionAddress - (uintptr_t)hMod;
        fprintf(stderr, "[CRASH] Module: %s + 0x%llX\n", modName, (unsigned long long)offset);
    }
    SymInitialize(GetCurrentProcess(), NULL, TRUE);
    STACKFRAME64 frame = {};
    CONTEXT ctx = *ep->ContextRecord;
    frame.AddrPC.Offset = ctx.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrStack.Offset = ctx.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = ctx.Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
    fprintf(stderr, "[CRASH] Stack trace:\n");
    for (int i = 0; i < 20; i++) {
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, GetCurrentProcess(),
                         GetCurrentThread(), &frame, &ctx, NULL,
                         SymFunctionTableAccess64, SymGetModuleBase64, NULL))
            break;
        char buf[sizeof(SYMBOL_INFO) + 256];
        SYMBOL_INFO* sym = (SYMBOL_INFO*)buf;
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 255;
        DWORD64 disp = 0;
        if (SymFromAddr(GetCurrentProcess(), frame.AddrPC.Offset, &disp, sym)) {
            fprintf(stderr, "  [%d] %s + 0x%llX\n", i, sym->Name, (unsigned long long)disp);
        } else {
            fprintf(stderr, "  [%d] 0x%llX\n", i, (unsigned long long)frame.AddrPC.Offset);
        }
    }
    fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[]) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    SetUnhandledExceptionFilter(crash_handler);

    fprintf(stderr, "==============================================\n");
    fprintf(stderr, " San Francisco Rush: Extreme Racing\n");
    fprintf(stderr, " Static Recompilation v0.2.0\n");
    fprintf(stderr, " Fan-made / Unofficial\n");
    fprintf(stderr, "==============================================\n\n");

    // Register game entry
    recomp::GameEntry game_entry{};
    game_entry.rom_hash = SFRUSH_ROM_HASH;
    game_entry.internal_name = "S.F. RUSH";
    game_entry.game_id = u8"sfrush";
    game_entry.mod_game_id = "sfrush";
    game_entry.save_type = recomp::SaveType::Eep4k;
    game_entry.is_enabled = true;
    game_entry.entrypoint_address = (gpr)(int32_t)0x80000400;
    game_entry.entrypoint = recomp_entrypoint;

    // Register sections
    recomp::overlays::overlay_section_table_data_t sections_data{};
    sections_data.code_sections = section_table;
    sections_data.num_code_sections = num_sections;
    sections_data.total_num_sections = num_sections;

    recomp::overlays::overlays_by_index_t overlays_data{};
    overlays_data.table = nullptr;
    overlays_data.len = 0;

    recomp::overlays::register_overlays(sections_data, overlays_data);

    if (!recomp::register_game(game_entry)) {
        fprintf(stderr, "[SFRush] Failed to register game!\n");
        return 1;
    }

    fprintf(stderr, "[SFRush] Game registered: %zu sections, %zu + %zu functions\n",
           num_sections, section_table[0].num_funcs,
           num_sections > 1 ? section_table[1].num_funcs : 0);

    recomp::register_config_path(std::filesystem::current_path());

    // Load ROM
    recomp::check_all_stored_roms();
    std::u8string game_id_check = u8"sfrush";
    if (!recomp::is_rom_valid(game_id_check)) {
        fprintf(stderr, "[SFRush] No stored ROM found, loading sfrush_recomp.z64...\n");
        auto rom_error = recomp::select_rom("sfrush_recomp.z64", game_id_check);
        if (rom_error != recomp::RomValidationError::Good) {
            fprintf(stderr, "[SFRush] ROM validation failed (error %d)!\n", (int)rom_error);
            fprintf(stderr, "  Place sfrush_recomp.z64 in the working directory.\n");
            fprintf(stderr, "  Generate it with: node tools/build_recomp_rom.js\n");
            return 1;
        }
        fprintf(stderr, "[SFRush] ROM validated and stored.\n");
    } else {
        fprintf(stderr, "[SFRush] Stored ROM found.\n");
    }

    // Configure runtime
    recomp::Configuration cfg{};
    cfg.project_version = { 0, 2, 0, "-alpha" };

    recomp::rsp::callbacks_t rsp_callbacks{};
    rsp_callbacks.get_rsp_microcode = get_rsp_microcode;
    cfg.rsp_callbacks = rsp_callbacks;

    ultramodern::renderer::callbacks_t renderer_callbacks{};
    renderer_callbacks.create_render_context = create_render_context_callback;
    cfg.renderer_callbacks = renderer_callbacks;

    ultramodern::audio_callbacks_t audio_callbacks{};
    audio_callbacks.queue_samples = sfrush_queue_samples;
    audio_callbacks.get_frames_remaining = sfrush_get_frames_remaining;
    audio_callbacks.set_frequency = sfrush_set_frequency;
    cfg.audio_callbacks = audio_callbacks;

    ultramodern::input::callbacks_t input_callbacks{};
    input_callbacks.poll_input = poll_input;
    input_callbacks.get_input = get_input;
    input_callbacks.set_rumble = set_rumble;
    input_callbacks.get_connected_device_info = get_connected_device_info;
    cfg.input_callbacks = input_callbacks;

    ultramodern::gfx_callbacks_t gfx_callbacks{};
    gfx_callbacks.create_gfx = create_gfx;
    gfx_callbacks.create_window = create_window;
    gfx_callbacks.update_gfx = update_gfx;
    cfg.gfx_callbacks = gfx_callbacks;

    ultramodern::events::callbacks_t events_callbacks{};
    events_callbacks.vi_callback = vi_callback;
    events_callbacks.gfx_init_callback = gfx_init_callback;
    cfg.events_callbacks = events_callbacks;

    ultramodern::error_handling::callbacks_t error_callbacks{};
    error_callbacks.message_box = error_message_box;
    cfg.error_handling_callbacks = error_callbacks;

    ultramodern::threads::callbacks_t threads_callbacks{};
    threads_callbacks.get_game_thread_name = get_game_thread_name;
    cfg.threads_callbacks = threads_callbacks;

    fprintf(stderr, "[SFRush] Starting runtime...\n");

    recomp::start(cfg);

    // Cleanup
    if (g_window) {
        SDL_DestroyWindow(g_window);
    }
    SDL_Quit();

    fprintf(stderr, "\n[SFRush] Shutdown complete.\n");
    return 0;
}
