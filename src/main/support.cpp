/**
 * San Francisco Rush: Extreme Racing - Recompiled
 * Platform support utilities
 */

#include <cstdio>
#include <string>
#include <filesystem>

#include <SDL.h>

extern SDL_Window* get_sdl_window();

namespace sfrush {

/**
 * Get the path to the assets directory.
 */
std::string get_asset_path(const std::string& filename) {
    std::filesystem::path assets_dir = "assets";
    return (assets_dir / filename).string();
}

/**
 * Check if a file exists.
 */
bool file_exists(const std::string& path) {
    return std::filesystem::exists(path);
}

} // namespace sfrush
