/**
 * San Francisco Rush: Extreme Racing - Recompiled
 * Recompilation API
 *
 * Provides game-specific API functions for the recompilation runtime.
 * Handles ROM data access, game-specific function exports, and
 * runtime hooks.
 */

#include <cstdio>
#include <cstdint>
#include <cstring>

namespace sfrush {

// ROM data access (defined in main.cpp)
extern uint8_t* g_rom_data;
extern uint32_t g_rom_size;

/**
 * Read bytes from the ROM at a given offset.
 */
bool rom_read(uint32_t offset, void* dst, uint32_t size) {
    if (!g_rom_data || offset + size > g_rom_size) {
        fprintf(stderr, "[API] ROM read out of bounds: offset=0x%08X size=0x%X\n", offset, size);
        return false;
    }
    memcpy(dst, g_rom_data + offset, size);
    return true;
}

/**
 * Get a pointer to ROM data at a given offset.
 */
const uint8_t* rom_ptr(uint32_t offset) {
    if (!g_rom_data || offset >= g_rom_size) {
        fprintf(stderr, "[API] ROM ptr out of bounds: offset=0x%08X\n", offset);
        return nullptr;
    }
    return g_rom_data + offset;
}

/**
 * Get the loaded ROM size.
 */
uint32_t rom_get_size() {
    return g_rom_size;
}

} // namespace sfrush
