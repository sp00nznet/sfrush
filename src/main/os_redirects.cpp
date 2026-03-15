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

static bool g_trace_os = true; // Set to false after boot to reduce spam
static int g_os_call_count = 0;

#define REDIRECT(vram_func, ultra_func) \
    extern "C" void vram_func(uint8_t* rdram, recomp_context* ctx) { \
        if (g_trace_os && g_os_call_count < 200) { \
            g_os_call_count++; \
            fprintf(stderr, "[OS] " #ultra_func " (via " #vram_func ") a0=0x%llX a1=0x%llX a2=0x%llX\n", \
                    (unsigned long long)ctx->r4, (unsigned long long)ctx->r5, (unsigned long long)ctx->r6); \
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
REDIRECT(func_80004930, osRecvMesg_recomp)        // osRecvMesg
REDIRECT(func_80005470, osSendMesg_recomp)        // osSendMesg
REDIRECT(func_80005430, osJamMesg_recomp)         // osJamMesg

// --- Event messages ---
// These are wrappers that call osSetEventMesg with specific event types.
// They take (mq, msg) as args and internally set the event type.
// Safe to stub — ultramodern handles events via its own mechanism.
extern "C" void func_80004DD0(uint8_t* rdram, recomp_context* ctx) {
    // VI event — redirect to osSetEventMesg with OS_EVENT_VI (14)
    // ctx->r4 = mq, ctx->r5 = msg, set ctx->r6 = event type
    ctx->r6 = 14; // OS_EVENT_VI
    osSetEventMesg_recomp(rdram, ctx);
}
extern "C" void func_80004E40(uint8_t* rdram, recomp_context* ctx) {
    ctx->r6 = 0; // OS_EVENT_SP
    osSetEventMesg_recomp(rdram, ctx);
}
extern "C" void func_80004EB0(uint8_t* rdram, recomp_context* ctx) {
    ctx->r6 = 11; // OS_EVENT_DP
    osSetEventMesg_recomp(rdram, ctx);
}
extern "C" void func_80004F20(uint8_t* rdram, recomp_context* ctx) {
    ctx->r6 = 4; // OS_EVENT_SI
    osSetEventMesg_recomp(rdram, ctx);
}

// --- VI (Video Interface) ---
REDIRECT(func_800043B0, osCreateViManager_recomp)  // osCreateViManager
// func_80004540 is NOT simple osViSetMode — it's a 2-arg wrapper. Stubbed separately.
extern "C" void func_80004540(uint8_t* rdram, recomp_context* ctx) {
    fprintf(stderr, "[SFRush] STUB func_80004540 (VI mode wrapper, 2 args)\n");
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
REDIRECT(func_8000E7A4, osPiStartDma_recomp)      // osPiStartDma
REDIRECT(func_8000EA20, osEPiStartDma_recomp)      // osEPiStartDma
REDIRECT(func_8000EC50, osCreatePiManager_recomp)  // osCreatePiManager
REDIRECT(func_800040B0, osPiGetStatus_recomp)      // osPiGetStatus (tiny: 8 bytes)
REDIRECT(func_800040B8, osPiGetStatus_recomp)      // osPiGetStatus wait variant
REDIRECT(func_8000E678, osCartRomInit_recomp)       // osCartRomInit
REDIRECT(func_80015710, osPiGetStatus_recomp)       // osPiGetStatus variant
REDIRECT(func_80015760, osPiGetStatus_recomp)       // osPiGetStatus variant 2

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

// --- TLB ---
REDIRECT(func_8000E2D0, osUnmapTLBAll_recomp)      // TLB probe/unmap
REDIRECT(func_80015860, osUnmapTLBAll_recomp)       // TLB read/write

// --- 64-bit FPU ---
// func_80006DC0 uses 64-bit FPU ops (trunc.l, cvt.l, dmfc1/dmtc1)
// These don't exist on PC — stub as no-op
extern "C" void func_80006DC0(uint8_t* rdram, recomp_context* ctx) {
    // 64-bit FPU operations not needed on PC
}
