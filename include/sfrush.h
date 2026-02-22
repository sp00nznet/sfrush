/**
 * San Francisco Rush: Extreme Racing - Recompiled
 * Main project header
 */

#ifndef SFRUSH_H
#define SFRUSH_H

#include <cstdint>
#include <string>

// Forward declarations
struct SDL_Window;

// ── Global accessors ──
SDL_Window* get_sdl_window();

// ── Render context ──
namespace sfrush {
    bool render_init();
    void render_shutdown();
    void render_set_vi_registers(uint32_t origin, uint32_t width, uint32_t status);
}

// ── Input ──
namespace sfrush {
    void input_init();
    void input_update();
    void input_shutdown();
}

// ── ROM access ──
namespace sfrush {
    bool rom_read(uint32_t offset, void* dst, uint32_t size);
    const uint8_t* rom_ptr(uint32_t offset);
    uint32_t rom_get_size();
}

// ── MIO0 decompression ──
namespace sfrush {
    bool mio0_is_compressed(const uint8_t* data);
    uint8_t* mio0_decompress(const uint8_t* src, uint32_t* out_size);
}

// ── RSP Audio ──
namespace sfrush {
    void rsp_audio_process(uint32_t task_addr, uint32_t task_size);
    uint32_t rsp_audio_get_task_count();
}

// ── Platform support ──
namespace sfrush {
    std::string get_asset_path(const std::string& filename);
    bool file_exists(const std::string& path);
}

#endif // SFRUSH_H
