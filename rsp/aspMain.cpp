/**
 * San Francisco Rush: Extreme Racing - Recompiled
 * RSP Audio Microcode Stub
 *
 * San Francisco Rush is a Midway title and uses Midway's proprietary
 * SN64 sound engine, the same audio system used in Rampage: World Tour.
 *
 * The SN64 engine handles:
 *   - ADPCM decode (N64 compressed audio)
 *   - Reverb/effects processing
 *   - Multichannel mixing
 *   - Audio DMA from ROM to RDRAM
 *
 * Audio implementation is a shared research effort with the Rampage
 * recompilation project. Breakthroughs in either project benefit both.
 *
 * For now, this stub logs audio task information for analysis.
 */

#include <cstdio>
#include <cstdint>

namespace sfrush {

static uint32_t audio_task_count = 0;

/**
 * Process an RSP audio task.
 * Called when the game submits an audio task to the RSP.
 */
void rsp_audio_process(uint32_t task_addr, uint32_t task_size) {
    audio_task_count++;

    // Log first 10 tasks, then sample every 100th
    if (audio_task_count <= 10 || audio_task_count % 100 == 0) {
        fprintf(stderr, "[AUDIO] RSP task #%u: addr=0x%08X size=0x%X\n",
                audio_task_count, task_addr, task_size);
    }

    // TODO: Implement SN64 audio processing
    // See Rampage Recompiled for SN64 research notes
    // Audio bank location needs to be identified in the SF Rush ROM
}

uint32_t rsp_audio_get_task_count() {
    return audio_task_count;
}

} // namespace sfrush
