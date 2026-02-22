/**
 * San Francisco Rush: Extreme Racing - Recompiled
 * OS Function Stubs
 *
 * Stub implementations for libultra (N64 OS) functions that are either
 * not yet implemented or not needed for PC recompilation.
 *
 * Based on patterns from Rampage Recompiled and DKR Recompiled.
 * As a Midway title, SF Rush uses similar OS conventions to Rampage.
 */

#include <cstdio>
#include <cstdint>

extern "C" {

// ── Controller Pak (Save) Functions ──
// SF Rush saves ghost data and records to Controller Pak
// Returning error for now - save support comes in Phase 5

// PFS error codes
#define PFS_ERR_NOPACK  1

int osPfsInitPak(void* mq, void* pfs, int channel) {
    fprintf(stderr, "[STUB] osPfsInitPak(channel=%d) -> NOPACK\n", channel);
    return PFS_ERR_NOPACK;
}

int osPfsReadWriteFile(void* pfs, int file_no, int flag, int offset, int size, void* buf) {
    fprintf(stderr, "[STUB] osPfsReadWriteFile(file=%d, flag=%d)\n", file_no, flag);
    return PFS_ERR_NOPACK;
}

int osPfsAllocateFile(void* pfs, uint16_t company, uint32_t game, const char* name, int ext, int size) {
    fprintf(stderr, "[STUB] osPfsAllocateFile(\"%s\")\n", name ? name : "(null)");
    return PFS_ERR_NOPACK;
}

int osPfsDeleteFile(void* pfs, uint16_t company, uint32_t game, const char* name, int ext) {
    fprintf(stderr, "[STUB] osPfsDeleteFile(\"%s\")\n", name ? name : "(null)");
    return PFS_ERR_NOPACK;
}

int osPfsFindFile(void* pfs, uint16_t company, uint32_t game, const char* name, int ext, int* file_no) {
    fprintf(stderr, "[STUB] osPfsFindFile(\"%s\")\n", name ? name : "(null)");
    return PFS_ERR_NOPACK;
}

int osPfsFreeBlocks(void* pfs, int* blocks) {
    if (blocks) *blocks = 0;
    return PFS_ERR_NOPACK;
}

// ── SI (Serial Interface) Functions ──

void __osSiGetAccess(void) {
    // Serial interface mutex - not needed on PC
}

void __osSiRelAccess(void) {
    // Serial interface mutex release
}

int osContInit(void* mq, uint8_t* bitpattern, void* status) {
    fprintf(stderr, "[STUB] osContInit()\n");
    if (bitpattern) *bitpattern = 0x01; // Controller 1 present
    return 0;
}

// ── Debug Functions ──

void rmonPrintf(const char* fmt, ...) {
    // N64 debug monitor print - suppress
}

void __osSpSetStatus(uint32_t status) {
    // RSP status register write
}

// ── EEPROM Functions ──
// SF Rush may use EEPROM for settings

#define EEPROM_TYPE_4K  1

int osEepromProbe(void* mq) {
    fprintf(stderr, "[STUB] osEepromProbe() -> 4K\n");
    return EEPROM_TYPE_4K;
}

int osEepromRead(void* mq, uint8_t address, uint8_t* buffer) {
    fprintf(stderr, "[STUB] osEepromRead(addr=0x%02X)\n", address);
    // Return zeros (blank EEPROM)
    if (buffer) {
        for (int i = 0; i < 8; i++) buffer[i] = 0;
    }
    return 0;
}

int osEepromWrite(void* mq, uint8_t address, const uint8_t* buffer) {
    fprintf(stderr, "[STUB] osEepromWrite(addr=0x%02X)\n", address);
    // TODO: Persist to file for save support
    return 0;
}

} // extern "C"
