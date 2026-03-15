// Custom implementation of func_800005D8 (Thread 6 - main init thread)
//
// Simplified approach: create the message queues the game expects,
// then call func_800BD440 (game init entry).

#include <cstdio>
#include <cstring>
#include "recomp.h"

extern "C" void func_800BD440(uint8_t* rdram, recomp_context* ctx);
extern "C" void osCreateMesgQueue_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void __osSetFpcCsr_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osViSetEvent_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osViSetMode_recomp(uint8_t* rdram, recomp_context* ctx);
extern "C" void osCreatePiManager_recomp(uint8_t* rdram, recomp_context* ctx);

extern "C" void func_800005D8(uint8_t* rdram, recomp_context* ctx) {
    fprintf(stderr, "[SFRush] func_800005D8: Initializing game...\n");

    uint64_t saved_a0 = ctx->r4;

    // Set FPU control
    ctx->r4 = 0x01000E00;
    __osSetFpcCsr_recomp(rdram, ctx);

    // Create the 4 message queues that func_800BD440 expects.
    // osCreateMesgQueue(mq, msgBuf, count)
    // a0 = mq ptr, a1 = msg buffer ptr, a2 = count

    // Queue 1: mq=0x80020EB8, buf=0x80020ED0, count=8
    ctx->r4 = (uint64_t)(int64_t)(int32_t)0x80020EB8;
    ctx->r5 = (uint64_t)(int64_t)(int32_t)0x80020ED0;
    ctx->r6 = 8;
    osCreateMesgQueue_recomp(rdram, ctx);

    // Queue 2: mq=0x80020EF0, buf=0x80020F08, count=8
    ctx->r4 = (uint64_t)(int64_t)(int32_t)0x80020EF0;
    ctx->r5 = (uint64_t)(int64_t)(int32_t)0x80020F08;
    ctx->r6 = 8;
    osCreateMesgQueue_recomp(rdram, ctx);

    // Queue 3: mq=0x80021470, buf=0x80021488, count=0x3C
    ctx->r4 = (uint64_t)(int64_t)(int32_t)0x80021470;
    ctx->r5 = (uint64_t)(int64_t)(int32_t)0x80021488;
    ctx->r6 = 0x3C;
    osCreateMesgQueue_recomp(rdram, ctx);

    // Queue 4: mq=0x80021438, buf=0x80021450, count=8
    ctx->r4 = (uint64_t)(int64_t)(int32_t)0x80021438;
    ctx->r5 = (uint64_t)(int64_t)(int32_t)0x80021450;
    ctx->r6 = 8;
    osCreateMesgQueue_recomp(rdram, ctx);

    fprintf(stderr, "[SFRush]   Message queues created\n");

    // Store 1 to 0x80021664 (flag set by original after queue creation)
    *(uint32_t*)(rdram + (0x80021664 - 0x80000000)) = 1;

    // Store 90.0f to 0x80021590 (FPU constant set by original)
    float val = 90.0f;
    memcpy(rdram + (0x80021590 - 0x80000000), &val, 4);

    // Set up VI event: osViSetEvent(mq, msg, retraceCount)
    // The game waits on queue 0x80020EB8 for VI retrace messages
    fprintf(stderr, "[SFRush]   Setting VI event: mq=0x80020EB8...\n");
    ctx->r4 = (uint64_t)(int64_t)(int32_t)0x80020EB8; // mq
    ctx->r5 = 0; // msg (NULL is standard for VI events)
    ctx->r6 = 1; // retrace count (every frame)
    osViSetEvent_recomp(rdram, ctx);
    fprintf(stderr, "[SFRush]   VI event set OK\n");

    // Set a default VI mode so the VI thread doesn't skip processing.
    // Without a mode set, update_vi() returns early (null guard), the state
    // never swaps, and events configured in next_state never become cur_state.
    // We need to call osViSetMode with a valid OSViMode pointer in RDRAM.
    //
    // Write an NTSC LAN1 mode to a scratch area in RDRAM.
    // OSViMode is ~80 bytes. We'll use the BSS area at a known safe location.
    {
        // NTSC LAN1 mode values (from N64 SDK osViModeFpalLan1 equivalent)
        // This is OSViMode for NTSC 320x240 16-bit
        uint32_t mode_addr_vram = 0x80058000; // safe scratch area in boot BSS
        uint32_t mode_offset = mode_addr_vram - 0x80000000;
        uint32_t* mode = (uint32_t*)(rdram + mode_offset);

        // OSViMode structure (simplified — comRegs and fldRegs):
        // type, comRegs (ctrl, width, burst, vSync, hSync, leap, hStart, xScale, vCurrent)
        // Then 2 field register sets
        mode[0] = 1; // type = OS_VI_NTSC_LAN1
        // comRegs.ctrl = 0x3016 (16-bit, AA+resamp+dedither)
        mode[1] = 0x00003016;
        // comRegs.width = 320
        mode[2] = 320;
        // comRegs.burst = standard NTSC
        mode[3] = 0x03E52239;
        // comRegs.vSync = 0x20D
        mode[4] = 0x0000020D;
        // comRegs.hSync = 0x00000C15
        mode[5] = 0x00000C15;
        // comRegs.leap = 0x0C150C15
        mode[6] = 0x0C150C15;
        // comRegs.hStart = 0x006C02EC
        mode[7] = 0x006C02EC;
        // comRegs.xScale = 0x200 (1:1 for 320px)
        mode[8] = 0x00000200;
        // comRegs.vCurrent = 0
        mode[9] = 0;
        // Field 0: origin=0, yScale=0x2000800, vStart=0x00230239, vBurst=0x000E0204
        mode[10] = 0; // fldRegs[0].origin
        mode[11] = 0x02000800; // fldRegs[0].yScale
        mode[12] = 0x00250239; // fldRegs[0].vStart
        mode[13] = 0x000E0204; // fldRegs[0].vBurst
        // Field 1: (same for non-interlaced)
        mode[14] = 0; // fldRegs[1].origin
        mode[15] = 0x02000800;
        mode[16] = 0x00250239;
        mode[17] = 0x000E0204;

        ctx->r4 = (uint64_t)(int64_t)(int32_t)mode_addr_vram;
        osViSetMode_recomp(rdram, ctx);
        fprintf(stderr, "[SFRush]   VI mode set (NTSC 320x240 at 0x%08X)\n", mode_addr_vram);
    }

    // Set up PI manager (needed for ROM DMA operations)
    // osCreatePiManager(pri, cmdQueue, cmdBuf, cmdMsgCnt)
    ctx->r4 = 150; // priority (OS_PRIORITY_PIMGR)
    ctx->r5 = (uint64_t)(int64_t)(int32_t)0x80021470; // cmdQueue
    ctx->r6 = (uint64_t)(int64_t)(int32_t)0x80021488; // cmdBuf
    ctx->r7 = 0x3C; // cmdMsgCnt
    osCreatePiManager_recomp(rdram, ctx);
    fprintf(stderr, "[SFRush]   PI Manager created\n");

    // Call game init
    ctx->r4 = saved_a0;
    fprintf(stderr, "[SFRush]   Calling func_800BD440 (game init)...\n");
    func_800BD440(rdram, ctx);
    fprintf(stderr, "[SFRush]   func_800BD440 returned\n");
}
