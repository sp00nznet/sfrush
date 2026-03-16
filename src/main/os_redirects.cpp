// OS Function Redirects for SF Rush
//
// The boot segment contains recompiled libultra OS functions that must be
// redirected to ultramodern/librecomp's native reimplementations.
// Functions listed as "ignored" in the recomp TOML generate no code,
// so we provide wrappers here that call the _recomp versions from ultramodern.
//
// The _recomp functions have identical signatures: (uint8_t* rdram, recomp_context* ctx)
// and read arguments from ctx->r4, r5, r6, r7 (matching MIPS calling convention).

#include <cstdio>
#include "recomp.h"
#include "librecomp/game.hpp"

// ============================================================================
// Declarations for ultramodern/librecomp reimplemented functions
// ============================================================================

extern "C" {
    // Thread management (ultra_translation.cpp)
    void osCreateThread_recomp(uint8_t* rdram, recomp_context* ctx);
    void osStartThread_recomp(uint8_t* rdram, recomp_context* ctx);
    void osStopThread_recomp(uint8_t* rdram, recomp_context* ctx);
    void osDestroyThread_recomp(uint8_t* rdram, recomp_context* ctx);
    void osSetThreadPri_recomp(uint8_t* rdram, recomp_context* ctx);
    void osGetThreadPri_recomp(uint8_t* rdram, recomp_context* ctx);
    void osGetThreadId_recomp(uint8_t* rdram, recomp_context* ctx);
    void osYieldThread_recomp(uint8_t* rdram, recomp_context* ctx);

    // Message queues (ultra_translation.cpp)
    void osCreateMesgQueue_recomp(uint8_t* rdram, recomp_context* ctx);
    void osRecvMesg_recomp(uint8_t* rdram, recomp_context* ctx);
    void osSendMesg_recomp(uint8_t* rdram, recomp_context* ctx);
    void osJamMesg_recomp(uint8_t* rdram, recomp_context* ctx);
    void osSetEventMesg_recomp(uint8_t* rdram, recomp_context* ctx);

    // VI (ultra_translation.cpp)
    void osCreateViManager_recomp(uint8_t* rdram, recomp_context* ctx);
    void osViSetMode_recomp(uint8_t* rdram, recomp_context* ctx);
    void osViSetEvent_recomp(uint8_t* rdram, recomp_context* ctx);
    void osViSwapBuffer_recomp(uint8_t* rdram, recomp_context* ctx);
    void osViBlack_recomp(uint8_t* rdram, recomp_context* ctx);
    void osViSetSpecialFeatures_recomp(uint8_t* rdram, recomp_context* ctx);
    void osViGetCurrentFramebuffer_recomp(uint8_t* rdram, recomp_context* ctx);
    void osViGetNextFramebuffer_recomp(uint8_t* rdram, recomp_context* ctx);
    void osViSetXScale_recomp(uint8_t* rdram, recomp_context* ctx);
    void osViSetYScale_recomp(uint8_t* rdram, recomp_context* ctx);
    void osViRepeatLine_recomp(uint8_t* rdram, recomp_context* ctx);

    // Timer (ultra_translation.cpp)
    void osSetTimer_recomp(uint8_t* rdram, recomp_context* ctx);
    void osStopTimer_recomp(uint8_t* rdram, recomp_context* ctx);
    void osGetTime_recomp(uint8_t* rdram, recomp_context* ctx);
    void osSetTime_recomp(uint8_t* rdram, recomp_context* ctx);
    void osGetCount_recomp(uint8_t* rdram, recomp_context* ctx);
    void osSetCount_recomp(uint8_t* rdram, recomp_context* ctx);

    // Memory (ultra_translation.cpp + recomp.cpp)
    void osVirtualToPhysical_recomp(uint8_t* rdram, recomp_context* ctx);
    void osGetMemSize_recomp(uint8_t* rdram, recomp_context* ctx);
    void osInvalDCache_recomp(uint8_t* rdram, recomp_context* ctx);
    void osInvalICache_recomp(uint8_t* rdram, recomp_context* ctx);
    void osWritebackDCache_recomp(uint8_t* rdram, recomp_context* ctx);
    void osWritebackDCacheAll_recomp(uint8_t* rdram, recomp_context* ctx);

    // Interrupt (ultra_translation.cpp)
    void osSetIntMask_recomp(uint8_t* rdram, recomp_context* ctx);
    void __osDisableInt_recomp(uint8_t* rdram, recomp_context* ctx);
    void __osRestoreInt_recomp(uint8_t* rdram, recomp_context* ctx);

    // PI (pi.cpp)
    void osPiStartDma_recomp(uint8_t* rdram, recomp_context* ctx);
    void osEPiStartDma_recomp(uint8_t* rdram, recomp_context* ctx);
    void osEPiReadIo_recomp(uint8_t* rdram, recomp_context* ctx);
    void osPiGetStatus_recomp(uint8_t* rdram, recomp_context* ctx);
    void osCartRomInit_recomp(uint8_t* rdram, recomp_context* ctx);
    void osCreatePiManager_recomp(uint8_t* rdram, recomp_context* ctx);
    void __osPiGetAccess_recomp(uint8_t* rdram, recomp_context* ctx);
    void __osPiRelAccess_recomp(uint8_t* rdram, recomp_context* ctx);

    // SP (sp.cpp)
    void osSpTaskLoad_recomp(uint8_t* rdram, recomp_context* ctx);
    void osSpTaskStartGo_recomp(uint8_t* rdram, recomp_context* ctx);
    void osSpTaskYield_recomp(uint8_t* rdram, recomp_context* ctx);

    // AI (ai.cpp)
    void osAiSetFrequency_recomp(uint8_t* rdram, recomp_context* ctx);
    void osAiSetNextBuffer_recomp(uint8_t* rdram, recomp_context* ctx);
    void osAiGetLength_recomp(uint8_t* rdram, recomp_context* ctx);
    void osAiGetStatus_recomp(uint8_t* rdram, recomp_context* ctx);

    // Controller (cont.cpp)
    void osContInit_recomp(uint8_t* rdram, recomp_context* ctx);
    void osContStartReadData_recomp(uint8_t* rdram, recomp_context* ctx);
    void osContGetReadData_recomp(uint8_t* rdram, recomp_context* ctx);
    void osContStartQuery_recomp(uint8_t* rdram, recomp_context* ctx);
    void osContGetQuery_recomp(uint8_t* rdram, recomp_context* ctx);
    void osContSetCh_recomp(uint8_t* rdram, recomp_context* ctx);

    // Init (ultra_translation.cpp)
    void osInitialize_recomp(uint8_t* rdram, recomp_context* ctx);
    void __osInitialize_common_recomp(uint8_t* rdram, recomp_context* ctx);

    // COP0 / FPU
    void __osSetFpcCsr_recomp(uint8_t* rdram, recomp_context* ctx);

    // DP (dp.cpp)
    void osDpGetStatus_recomp(uint8_t* rdram, recomp_context* ctx);
    void osDpSetStatus_recomp(uint8_t* rdram, recomp_context* ctx);

    // Eeprom (eep.cpp)
    void osEepromProbe_recomp(uint8_t* rdram, recomp_context* ctx);
    void osEepromRead_recomp(uint8_t* rdram, recomp_context* ctx);
    void osEepromWrite_recomp(uint8_t* rdram, recomp_context* ctx);

    // TLB
    void osUnmapTLBAll_recomp(uint8_t* rdram, recomp_context* ctx);
}

// ============================================================================
// Redirect macros — map func_XXXXXXXX to the ultramodern _recomp version
// ============================================================================

static int g_os_call_count = 0;

#define REDIRECT(vram_func, ultra_func) \
    extern "C" void vram_func(uint8_t* rdram, recomp_context* ctx) { \
        if (g_os_call_count < 500) { \
            g_os_call_count++; \
            fprintf(stderr, "[OS] " #ultra_func " (via " #vram_func ")\n"); \
        } \
        ultra_func(rdram, ctx); \
    }

// ============================================================================
// Boot segment OS function redirects
// These map the VRAM-addressed function names to ultramodern reimplementations.
// The VRAM addresses were identified by analyzing the boot code.
//
// To identify which func_XXXXXXXX maps to which OS function:
// 1. Check instruction patterns (MFC0, LUI 0xA4xx, etc.)
// 2. Cross-reference with other N64 games' symbol maps
// 3. Match function sizes and call patterns to known libultra functions
// ============================================================================

// --- Thread management ---
REDIRECT(func_80004110, osCreateThread_recomp)    // osCreateThread
REDIRECT(func_80004260, osStartThread_recomp)     // osStartThread
REDIRECT(func_80005210, osSetThreadPri_recomp)    // osSetThreadPri

// --- Message queues ---
REDIRECT(func_8000D7B0, osCreateMesgQueue_recomp) // osCreateMesgQueue

extern "C" void func_80004930(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mq = (uint32_t)ctx->r4;
    uint32_t flag = (uint32_t)ctx->r6;
    if (g_os_call_count < 500) {
        g_os_call_count++;
        fprintf(stderr, "[OS] osRecvMesg(mq=0x%08X, flag=%d)\n", mq, flag);
    }
    osRecvMesg_recomp(rdram, ctx);
    if (g_os_call_count < 500) {
        fprintf(stderr, "[OS] osRecvMesg returned\n");
    }
}

extern "C" void func_80005470(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mq = (uint32_t)ctx->r4;
    uint32_t msg = (uint32_t)ctx->r5;
    uint32_t flag = (uint32_t)ctx->r6;
    if (g_os_call_count < 500) { g_os_call_count++; fprintf(stderr, "[OS] osSendMesg(mq=0x%08X, msg=0x%08X, flag=%d)\n", mq, msg, flag); }
    osSendMesg_recomp(rdram, ctx);
}

// func_800050C0: internal osSendMesg used by the DMA system
extern "C" void func_800050C0(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mq = (uint32_t)ctx->r4;
    uint32_t msg = (uint32_t)ctx->r5;
    uint32_t flag = (uint32_t)ctx->r6;
    if (g_os_call_count < 500) { g_os_call_count++; fprintf(stderr, "[OS] osSendMesg_internal(mq=0x%08X, msg=0x%08X, flag=%d)\n", mq, msg, flag); }
    osSendMesg_recomp(rdram, ctx);
}

extern "C" void func_80005430(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mq = (uint32_t)ctx->r4;
    uint32_t msg = (uint32_t)ctx->r5;
    fprintf(stderr, "[OS] osJamMesg(mq=0x%08X, msg=0x%08X)\n", mq, msg);
    osJamMesg_recomp(rdram, ctx);
}

// --- Event messages ---
// func_80004DD0, func_80004E40, func_80004EB0, func_80004F20
// These are complex internal OS functions that manipulate RDRAM event tables.
// They run as recompiled code (not redirected to ultramodern).

// --- VI (Video Interface) ---
REDIRECT(func_800043B0, osCreateViManager_recomp)  // osCreateViManager
// func_80004540: 2-arg VI mode wrapper. arg0=VI state struct, arg1=mode pointer.
// When a0=0, loads default VI state from global 0x8001AB60.
// When a1=0, loads default mode from a0->mode_offset (4).
// We redirect to osViSetMode with the resolved mode pointer.
extern "C" void func_80004540(uint8_t* rdram, recomp_context* ctx) {
    uint32_t vi_state = (uint32_t)ctx->r4;
    uint32_t vi_mode = (uint32_t)ctx->r5;

    // Write a default NTSC LAN1 mode to RDRAM at 0x80058000 if not already set
    static bool mode_initialized = false;
    if (!mode_initialized) {
        uint32_t mode_offset = 0x58000; // RDRAM offset for our scratch mode
        uint32_t* mode = (uint32_t*)(rdram + mode_offset);
        mode[0] = 1;           // type = NTSC
        mode[1] = 0x00003016;  // ctrl
        mode[2] = 320;         // width
        mode[3] = 0x03E52239;  // burst
        mode[4] = 0x0000020D;  // vSync
        mode[5] = 0x00000C15;  // hSync
        mode[6] = 0x0C150C15;  // leap
        mode[7] = 0x006C02EC;  // hStart
        mode[8] = 0x00000200;  // xScale
        mode[9] = 0;           // vCurrent
        mode[10] = 0;          // fldRegs[0].origin
        mode[11] = 0x02000800; // fldRegs[0].yScale
        mode[12] = 0x00250239; // fldRegs[0].vStart
        mode[13] = 0x000E0204; // fldRegs[0].vBurst
        mode[14] = 0;          // fldRegs[1].origin
        mode[15] = 0x02000800;
        mode[16] = 0x00250239;
        mode[17] = 0x000E0204;
        mode_initialized = true;
    }

    // Install a monitoring write at 0x80162AE8 to track when it changes
    // (will be overwritten by game code when it writes there)
    *(uint32_t*)(rdram + 0x162AE8) = 0xDEADBEEF; // sentinel value

    // Always call osViSetMode with our NTSC mode
    ctx->r4 = (uint64_t)(int64_t)(int32_t)0x80058000;
    osViSetMode_recomp(rdram, ctx);
    fprintf(stderr, "[SFRush] func_80004540: osViSetMode(NTSC 320x240) called\n");

    // CRITICAL: Set up VI event delivery.
    // The scheduler thread waits on queue 0x800216B0, and the game thread
    // waits on queue 0x80020EB8. We set the VI event on the game's queue
    // since the scheduler doesn't forward events via osSendMesg.
    // The scheduler still gets retraces via its own RDRAM-based event setup.
    {
        uint64_t saved_r4 = ctx->r4;
        uint64_t saved_r5 = ctx->r5;
        uint64_t saved_r6 = ctx->r6;
        // Set VI event on the GAME's retrace queue
        ctx->r4 = (uint64_t)(int64_t)(int32_t)0x80020EB8; // game retrace queue
        ctx->r5 = 0;
        ctx->r6 = 1;
        osViSetEvent_recomp(rdram, ctx);
        fprintf(stderr, "[SFRush] func_80004540: osViSetEvent(mq=0x80020EB8) — game retrace\n");
        ctx->r4 = saved_r4;
        ctx->r5 = saved_r5;
        ctx->r6 = saved_r6;
    }
}
REDIRECT(func_8000F510, osViSwapBuffer_recomp)     // osViSwapBuffer
// func_8000F314 might not be osViSetEvent - stub for safety
extern "C" void func_8000F314(uint8_t* rdram, recomp_context* ctx) {
    fprintf(stderr, "[SFRush] STUB func_8000F314 (VI set functions)\n");
}
REDIRECT(func_8000F388, osViBlack_recomp)          // osViBlack
REDIRECT(func_8000F660, osViSetSpecialFeatures_recomp) // osViSetSpecialFeatures
REDIRECT(func_8000F1A4, osViGetCurrentFramebuffer_recomp) // osViGetCurrentFramebuffer

// --- PI (Peripheral Interface / ROM) ---
// func_80004820: osPiStartDma wrapper
// N64 SDK signature: osPiStartDma(OSIoMesg *mb, s32 pri, s32 dir, u32 devAddr, void *vAddr, size_t nbytes, OSMesgQueue *mq)
// Args: a0=mb, a1=pri, a2=dir, a3=devAddr, sp+0x10=vAddr, sp+0x14=nbytes, sp+0x18=mq
// We implement it directly using recomp::do_rom_read and send completion message
extern "C" void func_80004820(uint8_t* rdram, recomp_context* ctx) {
    uint32_t mb = (uint32_t)ctx->r4;
    uint32_t pri = (uint32_t)ctx->r5;
    uint32_t dir = (uint32_t)ctx->r6;
    uint32_t devAddr = (uint32_t)ctx->r7;
    // Read stack args
    gpr dramAddr = MEM_W(0x10, ctx->r29);
    uint32_t nbytes = (uint32_t)MEM_W(0x14, ctx->r29);
    uint32_t mq = (uint32_t)MEM_W(0x18, ctx->r29);

    // devAddr is ROM offset — add rom_base for physical address
    uint32_t physical_addr = devAddr + 0x10000000;

    if (g_os_call_count < 200) {
        g_os_call_count++;
        fprintf(stderr, "[OS] osPiStartDma(dev=0x%08X, dram=0x%08X, size=0x%X, mq=0x%08X, dir=%d)\n",
                devAddr, (uint32_t)dramAddr, nbytes, mq, dir);
    }

    if (dir == 0) {
        // Read from ROM to RDRAM
        recomp::do_rom_read(rdram, dramAddr, physical_addr, nbytes);
    }

    // Send completion message to the message queue
    if (mq != 0) {
        ctx->r4 = (uint64_t)(int64_t)(int32_t)mq;
        ctx->r5 = 0; // msg = NULL
        ctx->r6 = 0; // flag = OS_MESG_NOBLOCK
        osSendMesg_recomp(rdram, ctx);
    }

    ctx->r2 = 0; // return success
}

REDIRECT(func_8000E7A4, osPiStartDma_recomp)      // osPiStartDma
REDIRECT(func_8000EA20, osEPiStartDma_recomp)      // osEPiStartDma
REDIRECT(func_8000EC50, osCreatePiManager_recomp)  // osCreatePiManager
REDIRECT(func_8000E678, osCartRomInit_recomp)       // osCartRomInit
REDIRECT(func_80015710, osPiGetStatus_recomp)       // osPiGetStatus variant
REDIRECT(func_80015760, osPiGetStatus_recomp)       // osPiGetStatus variant 2

// func_800040B0: PI status read (2 instructions, falls through to func_800040B8)
// Just return 0 (PI not busy)
extern "C" void func_800040B0(uint8_t* rdram, recomp_context* ctx) {
    ctx->r6 = 0; // PI_STATUS = 0 (not busy)
}

// func_800040B8: Wait for PI idle, then read 32-bit word from ROM via uncached access
// Original: reads *(0xA0000000 | osRomBase | a0) and stores to *a1
// a0 = ROM offset, a1 = destination pointer in RDRAM
// On PC: read from the loaded ROM buffer using recomp::do_rom_pio
extern "C" void func_800040B8(uint8_t* rdram, recomp_context* ctx) {
    // a0 = ROM offset (e.g., 0xFFB000)
    // a1 = destination RDRAM pointer
    uint32_t rom_offset = (uint32_t)ctx->r4;
    uint32_t dest_addr = (uint32_t)ctx->r5;

    // The ROM is 8MB, so mask the offset
    rom_offset &= 0x7FFFFF;

    // Read from loaded ROM and store to RDRAM destination
    // physical_addr = rom_base + rom_offset = 0x10000000 + rom_offset
    uint32_t physical_addr = 0x10000000 + rom_offset;
    recomp::do_rom_pio(rdram, (gpr)(int32_t)dest_addr, physical_addr);
    ctx->r2 = 0; // return 0 (success)
}

// --- SP (RSP) ---
REDIRECT(func_8000FDC0, osSpTaskLoad_recomp)       // osSpTaskLoad
REDIRECT(func_8000FCF0, osSpTaskStartGo_recomp)    // osSpTaskStartGo

// --- AI (Audio) ---
REDIRECT(func_8000A1F0, osAiSetFrequency_recomp)   // osAiSetFrequency
REDIRECT(func_80007F4C, osAiSetNextBuffer_recomp)  // osAiSetNextBuffer
REDIRECT(func_8000A2A0, osAiGetLength_recomp)       // osAiGetLength
REDIRECT(func_80010C50, osAiGetStatus_recomp)       // osAiGetStatus

// --- SI (Controller) ---
REDIRECT(func_80009AE0, osContInit_recomp)          // osContInit
REDIRECT(func_80010350, osContStartReadData_recomp) // osContStartReadData
REDIRECT(func_80010460, osContGetReadData_recomp)   // osContGetReadData

// --- Memory / Cache ---
REDIRECT(func_80005880, osVirtualToPhysical_recomp) // osVirtualToPhysical
REDIRECT(func_80004650, osInvalDCache_recomp)       // osInvalDCache
REDIRECT(func_800046D0, osInvalICache_recomp)       // osInvalICache
REDIRECT(func_800054B0, osWritebackDCache_recomp)   // osWritebackDCache
REDIRECT(func_8000E250, osWritebackDCacheAll_recomp) // osWritebackDCacheAll

// --- COP0 / Interrupt ---
REDIRECT(func_8000E5F0, __osDisableInt_recomp)     // __osDisableInt
REDIRECT(func_8000E610, __osRestoreInt_recomp)     // __osRestoreInt
REDIRECT(func_80004F90, osSetIntMask_recomp)        // osSetIntMask
// COP0 Status register — different signatures from standard _recomp functions
extern "C" void func_8000D870(uint8_t* rdram, recomp_context* ctx) {
    // MTC0 Status — write ctx->r4 to status register
    cop0_status_write(ctx, ctx->r4);
}
extern "C" void func_8000D880(uint8_t* rdram, recomp_context* ctx) {
    // MFC0 Status — read status register into ctx->r2 (return value)
    ctx->r2 = cop0_status_read(ctx);
}
REDIRECT(func_8000F9C0, osGetCount_recomp)          // osGetCount (read Count register)

// --- Init ---
REDIRECT(func_80003E20, osInitialize_recomp)        // osInitialize / __osInitialize
REDIRECT(func_80003A10, __osSetFpcCsr_recomp)       // __osSetFpcCsr

// --- Timer ---
REDIRECT(func_800053B0, osSetTimer_recomp)          // osSetTimer
REDIRECT(func_80005030, osGetTime_recomp)           // osGetTime

// --- Heap allocator with FPU fix ---
// The game computes allocation sizes from float operations using f0 (90.0).
// Due to FPU register state not being properly initialized in the recomp,
// the computed value at 0x80162AE8 is garbage (0xFDDE2E10 instead of 0x1D0).
// We detect and fix the corrupted values before they cause allocations.
extern "C" void func_80007C50(uint8_t* rdram, recomp_context* ctx) {
    // Fix known corrupted values from mupen comparison
    uint32_t val_ae0 = *(uint32_t*)(rdram + 0x162AE0);
    uint32_t val_ae4 = *(uint32_t*)(rdram + 0x162AE4);
    uint32_t val_ae8 = *(uint32_t*)(rdram + 0x162AE8);
    static bool fixed = false;
    if (!fixed && val_ae8 != 0 && val_ae8 > 0x10000) {
        // Values are corrupted from bad float computation
        *(uint32_t*)(rdram + 0x162AE0) = 0x00000170;
        *(uint32_t*)(rdram + 0x162AE4) = 0x000000E5;
        *(uint32_t*)(rdram + 0x162AE8) = 0x000001D0;
        fprintf(stderr, "[ALLOC FIX] Corrected 0x80162AE0-AE8 from [0x%X,0x%X,0x%X] to [0x170,0xE5,0x1D0]\n",
                val_ae0, val_ae4, val_ae8);
        fixed = true;
    }

    // Native bump allocator
    uint32_t sp = (uint32_t)ctx->r29;
    uint32_t alloc_size = *(uint32_t*)(rdram + ((sp + 0x10) & 0x7FFFFF));
    uint32_t a2 = (uint32_t)ctx->r6;
    uint32_t heap_off = (a2 & 0x7FFFFF);
    uint32_t base = *(uint32_t*)(rdram + heap_off);
    uint32_t cur = *(uint32_t*)(rdram + heap_off + 4);
    uint32_t total = *(uint32_t*)(rdram + heap_off + 8);
    uint32_t end = base + total;
    uint32_t aligned = (cur + 0xF) & ~0xF;
    if (alloc_size > 0x100000 || aligned + alloc_size > end) {
        ctx->r2 = 0; ctx->r3 = 0; return;
    }
    *(uint32_t*)(rdram + heap_off + 4) = aligned + alloc_size;
    ctx->r2 = (uint64_t)(int64_t)(int32_t)aligned;
    ctx->r3 = ctx->r2;
}

// --- TLB ---
REDIRECT(func_8000E2D0, osUnmapTLBAll_recomp)      // TLB probe/unmap
REDIRECT(func_80015860, osUnmapTLBAll_recomp)       // TLB read/write

// --- 64-bit FPU ---
// func_80006DC0 uses 64-bit FPU ops (trunc.l, cvt.l, dmfc1/dmtc1)
// These don't exist on PC — stub as no-op
extern "C" void func_80006DC0(uint8_t* rdram, recomp_context* ctx) {
    // 64-bit FPU operations not needed on PC
}
