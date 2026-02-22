/**
 * San Francisco Rush: Extreme Racing - Recompiled
 * Main entry point - SDL window, input handling, and game lifecycle
 *
 * Based on patterns from:
 *   - Rampage Recompiled (Midway, shared engine conventions)
 *   - DKR Recompiled (N64Recomp runtime patterns)
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <filesystem>

#include <SDL.h>

// Version
#ifndef SFRUSH_VERSION_MAJOR
#define SFRUSH_VERSION_MAJOR 0
#endif
#ifndef SFRUSH_VERSION_MINOR
#define SFRUSH_VERSION_MINOR 1
#endif
#ifndef SFRUSH_VERSION_PATCH
#define SFRUSH_VERSION_PATCH 0
#endif

static const char* VERSION_STRING = "0.1.0";

// ROM validation
static const uint32_t EXPECTED_CRC1 = 0x2A6B1820;
static const uint32_t EXPECTED_CRC2 = 0x6ABCF466;
static const uint32_t ROM_SIZE = 8 * 1024 * 1024; // 8 MB
static const char* ROM_FILENAME = "baserom.z64";

// Window configuration
static const int WINDOW_WIDTH = 320;
static const int WINDOW_HEIGHT = 240;
static const int WINDOW_SCALE = 3;
static const char* WINDOW_TITLE = "San Francisco Rush: Extreme Racing - Recompiled";

static SDL_Window* g_window = nullptr;
static SDL_Renderer* g_renderer = nullptr;
static bool g_running = true;

// ROM data
static uint8_t* g_rom_data = nullptr;
static uint32_t g_rom_size = 0;

SDL_Window* get_sdl_window() {
    return g_window;
}

/**
 * Read a big-endian 32-bit value from a byte buffer.
 */
static uint32_t read_be32(const uint8_t* data) {
    return ((uint32_t)data[0] << 24) |
           ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] <<  8) |
           ((uint32_t)data[3]);
}

/**
 * Load and validate the ROM file.
 * Returns true on success.
 */
static bool load_rom(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "[ROM] Failed to open: %s\n", path);
        return false;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size != ROM_SIZE) {
        fprintf(stderr, "[ROM] Unexpected size: %ld bytes (expected %u)\n", size, ROM_SIZE);
        fclose(f);
        return false;
    }

    g_rom_data = (uint8_t*)malloc(size);
    if (!g_rom_data) {
        fprintf(stderr, "[ROM] Failed to allocate %ld bytes\n", size);
        fclose(f);
        return false;
    }

    if (fread(g_rom_data, 1, size, f) != (size_t)size) {
        fprintf(stderr, "[ROM] Failed to read ROM data\n");
        free(g_rom_data);
        g_rom_data = nullptr;
        fclose(f);
        return false;
    }

    fclose(f);
    g_rom_size = (uint32_t)size;

    // Validate ROM header
    uint32_t magic = read_be32(g_rom_data);
    if (magic != 0x80371240) {
        fprintf(stderr, "[ROM] Invalid magic: 0x%08X (expected 0x80371240 for .z64 format)\n", magic);
        free(g_rom_data);
        g_rom_data = nullptr;
        return false;
    }

    // Validate CRC
    uint32_t crc1 = read_be32(g_rom_data + 0x10);
    uint32_t crc2 = read_be32(g_rom_data + 0x14);

    if (crc1 != EXPECTED_CRC1 || crc2 != EXPECTED_CRC2) {
        fprintf(stderr, "[ROM] CRC mismatch: got %08X/%08X, expected %08X/%08X\n",
                crc1, crc2, EXPECTED_CRC1, EXPECTED_CRC2);
        fprintf(stderr, "[ROM] This may be the wrong ROM version. Expected: USA v1.0\n");
        free(g_rom_data);
        g_rom_data = nullptr;
        return false;
    }

    // Print ROM info
    char title[21] = {0};
    memcpy(title, g_rom_data + 0x20, 20);
    char code[5] = {0};
    memcpy(code, g_rom_data + 0x3B, 4);

    fprintf(stderr, "[ROM] Loaded: \"%s\" (%s)\n", title, code);
    fprintf(stderr, "[ROM] CRC: %08X / %08X (verified)\n", crc1, crc2);
    fprintf(stderr, "[ROM] Size: %u bytes\n", g_rom_size);

    return true;
}

/**
 * Process SDL input events.
 */
static void process_input() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                g_running = false;
                break;

            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    g_running = false;
                }
                break;
        }
    }
}

/**
 * Main entry point.
 */
int main(int argc, char* argv[]) {
    fprintf(stderr, "==============================================\n");
    fprintf(stderr, " San Francisco Rush: Extreme Racing\n");
    fprintf(stderr, " Static Recompilation v%s\n", VERSION_STRING);
    fprintf(stderr, " Fan-made / Unofficial - Not affiliated with\n");
    fprintf(stderr, " Atari Games, Midway, or any rights holders\n");
    fprintf(stderr, "==============================================\n\n");

    // Find and load ROM
    const char* rom_path = ROM_FILENAME;
    if (argc > 1) {
        rom_path = argv[1];
    }

    // Try default filename, then scan for .z64 files
    if (!load_rom(rom_path)) {
        if (rom_path == ROM_FILENAME) {
            fprintf(stderr, "[ROM] Scanning for .z64 files...\n");
            for (const auto& entry : std::filesystem::directory_iterator(".")) {
                if (entry.path().extension() == ".z64") {
                    fprintf(stderr, "[ROM] Trying: %s\n", entry.path().string().c_str());
                    if (load_rom(entry.path().string().c_str())) {
                        break;
                    }
                }
            }
        }

        if (!g_rom_data) {
            fprintf(stderr, "[ROM] No valid ROM found. Place your ROM as '%s' in the working directory.\n", ROM_FILENAME);
            return 1;
        }
    }

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        fprintf(stderr, "[SDL] Init failed: %s\n", SDL_GetError());
        return 1;
    }

    g_window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH * WINDOW_SCALE, WINDOW_HEIGHT * WINDOW_SCALE,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!g_window) {
        fprintf(stderr, "[SDL] Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    g_renderer = SDL_CreateRenderer(g_window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!g_renderer) {
        fprintf(stderr, "[SDL] Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return 1;
    }

    fprintf(stderr, "[INIT] SDL initialized successfully\n");
    fprintf(stderr, "[INIT] Window: %dx%d (scale %dx)\n",
            WINDOW_WIDTH * WINDOW_SCALE, WINDOW_HEIGHT * WINDOW_SCALE, WINDOW_SCALE);

    // Main loop placeholder - will be replaced with recompiled game loop
    fprintf(stderr, "[INIT] Entering main loop (scaffold mode)\n");
    fprintf(stderr, "[INIT] Press ESC to quit\n");

    while (g_running) {
        process_input();

        // Clear screen with a dark blue (Rush vibes)
        SDL_SetRenderDrawColor(g_renderer, 10, 10, 40, 255);
        SDL_RenderClear(g_renderer);
        SDL_RenderPresent(g_renderer);

        SDL_Delay(16); // ~60fps
    }

    // Cleanup
    fprintf(stderr, "[SHUTDOWN] Cleaning up...\n");

    if (g_rom_data) {
        free(g_rom_data);
        g_rom_data = nullptr;
    }

    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);
    SDL_Quit();

    fprintf(stderr, "[SHUTDOWN] Done.\n");
    return 0;
}
