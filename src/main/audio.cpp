// Audio stub for SF Rush
// Midway SN64 engine — needs proper implementation later

#include <cstdio>
#include <cstdint>

static bool freq_logged = false;

void sfrush_queue_samples(int16_t* samples, size_t num_samples) {
    // No-op until audio is implemented
}

size_t sfrush_get_frames_remaining() {
    return 2048; // Always report buffer space available
}

void sfrush_set_frequency(uint32_t freq) {
    if (!freq_logged) {
        fprintf(stderr, "[SFRush] Audio frequency set to %u Hz\n", freq);
        freq_logged = true;
    }
}
