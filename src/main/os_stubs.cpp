// Stub implementations for internal N64 OS functions
// Based on Extreme-G pattern

#include <cstdio>
#include <cstdint>
#include "recomp.h"

#define STUB(name) \
    extern "C" void name##_recomp(uint8_t* rdram, recomp_context* ctx) { \
        fprintf(stderr, "[SFRush] STUB called: " #name "_recomp\n"); \
    }

// Thread scheduling internals
STUB(__osEnqueueThread)
STUB(__osPopThread)
STUB(__osDequeueThread)

// Timer internals
STUB(__osTimerServicesInit)
STUB(__osTimerInterrupt)
STUB(__osInsertTimer)
STUB(__osSetTimerIntr)

// VI internals
STUB(__osViInit)
STUB(__osViGetCurrentContext)
STUB(__osViSwapContext)

// SP (RSP) internals
STUB(__osSpSetStatus)
STUB(__osSpRawStartDma)
STUB(__osSpDeviceBusy)

// SI (Serial Interface) internals
STUB(__osSiRawReadIo)
STUB(__osSiRawStartDma)
STUB(__osSiCreateAccessQueue)
STUB(__osSiGetAccess)
STUB(__osSiRelAccess)

// PI (Peripheral Interface) internals
STUB(osPiRawReadIo)
STUB(__osPiRawStartDma)
STUB(__osPiDeviceBusy)
STUB(__osPiCreateAccessQueue)
STUB(osEPiRawReadIo)
STUB(osEPiRawWriteIo)

// AI (Audio Interface) internals
STUB(__osAiDeviceBusy)

// Interrupt internals
STUB(__osSetGlobalIntMask)
STUB(__osResetGlobalIntMask)
STUB(__osGetCause)

// COP0 / Status register internals
STUB(__osGetSR)
STUB(__osSetSR)
STUB(__osSetCompare)
STUB(__osGetFpcCsr)

// Thread dispatch internals
STUB(__osEnqueueAndYield)
STUB(__osDispatchThread)
STUB(__osCleanupThread)
