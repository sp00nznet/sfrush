// Custom implementations for game functions that can't be auto-recompiled
// These replace N64Recomp "ignored" functions with PC-appropriate behavior

#include <cstdio>
#include <cstring>
#include "recomp.h"

// func_800BD440 is the game's main initialization entry point
// Called by the original func_800005D8 after LZSS decompression
extern "C" void func_800BD440(uint8_t* rdram, recomp_context* ctx);

// =============================================================================
// func_800005D8 — Main initialization thread (Thread 6)
//
// Original behavior:
//   1. __osSetFpcCsr(0x01000E00)  — set FPU rounding mode
//   2. osCreateMesgQueue x4       — create message queues
//   3. Check osTvType, set framerate (2=PAL, 16=MPAL, 30=NTSC)
//   4. Create scheduler thread (func_800007F0)
//   5. Set up more message queues (func_8000096C)
//   6. osInvalDCache on 0x8005BB10-0x800D8060
//   7. LZSS decompress from ROM 0x7A7930 → 0x8005BB10
//   8. bzero BSS 0x800D8060-0x8016A9B0
//   9. Call func_800BD440(arg) — enter main game code
//
// PC replacement:
//   Steps 6-8 are handled by on_game_init callback (data already in RDRAM).
//   Steps 1-5 set up OS state that ultramodern handles.
//   We just need step 9: call func_800BD440.
// =============================================================================
extern "C" void func_800005D8(uint8_t* rdram, recomp_context* ctx) {
    fprintf(stderr, "[SFRush] func_800005D8: Skipping LZSS decompress (data pre-loaded)\n");
    fprintf(stderr, "[SFRush] func_800005D8: Calling func_800BD440 (game init)...\n");

    // Call the game's main init function
    // In the original, this is called with the bootproc argument passed through
    func_800BD440(rdram, ctx);

    fprintf(stderr, "[SFRush] func_800005D8: func_800BD440 returned\n");
}
