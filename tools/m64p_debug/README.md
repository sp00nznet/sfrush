# mupen64plus CLI Debugger Tools (SF Rush)

Adapted from the DKR project's debug toolkit. Uses a custom mupen64plus build with `DBG` support for CLI-based debugging.

## Setup

1. Build mupen64plus-core 2.6.0 with `DBG` added to MSVC preprocessor defines (Release x64)
2. Place built binaries in `Release/` subdirectory (mupen64plus-ui-console.exe + DLLs)
3. Or copy from an existing build (e.g., from the DKR project)

## Tools

### sfrush_dump.js — RDRAM dump + DMA tracing
Boots SF Rush, waits for the game to load, then dumps full 8MB RDRAM at intervals.
The dumps can be analyzed to find DMA-loaded code segments.

```
node sfrush_dump.js [seconds] [interval]
```

### sfrush_state.js — Timed state snapshots
Config-driven memory address monitoring.

```
node m64p_state.js sfrush_state.json [total_seconds] [interval_seconds]
```

### read_sfrush_dump.js — RDRAM dump analyzer
Reads an 8MB RDRAM dump and identifies code regions, DMA table entries, and loaded segments.

```
node read_sfrush_dump.js <rdram_dump.bin>
```

### parse_dma_table.js — ROM DMA table parser
Reads the DMA table directly from the ROM file at offset 0x00FFB000.

```
node parse_dma_table.js [rom_path]
```

## Emumode

Always use emumode 1 (cached interpreter). Emumode 0 causes thread faults, emumode 2 doesn't support breakpoints.
