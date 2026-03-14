# San Francisco Rush: Extreme Racing - Recompiled

**A static recompilation of San Francisco Rush: Extreme Racing (N64) for PC.**

> **DISCLAIMER: This is NOT an official product.** This is a fan-made preservation project with no affiliation to Atari Games, Midway, or any rights holders. No game assets or ROM data are included in this repository. You must provide your own legally obtained ROM.

---

## What Is This?

Remember screaming down the hills of San Francisco, launching off impossible ramps, finding secret shortcuts, and smashing into oncoming traffic at 150 mph? **San Francisco Rush: Extreme Racing** (1997) is one of the most fun arcade racing games ever made for the N64, and this project aims to bring it to modern PCs through static recompilation.

This project uses [N64Recomp](https://github.com/Mr-Wiseguy/N64Recomp) to translate the original N64 MIPS machine code into C/C++ that runs natively on modern hardware, paired with [RT64](https://github.com/rt64/rt64) for GPU-accelerated N64 RDP emulation. The result: the original game running natively, not through emulation, with the potential for enhanced resolution, widescreen, and modern input support.

## Why This Game?

This is one of **three N64 static recompilation projects** currently in progress:

| Project | Title | Developer | Status |
|---------|-------|-----------|--------|
| **[Rampage Recompiled](https://github.com/sp00nznet/Rampage)** | Rampage: World Tour | Midway | Phase 5 - Runtime Integration |
| **[DKR Recompiled](https://github.com/sp00nznet/diddykongracing)** | Diddy Kong Racing | Rare | Boots, renders, in-game |
| **This project** | San Francisco Rush | Atari Games / Midway | Phase 3 - Build System |

San Francisco Rush is another **Midway title**, which means it likely shares engine architecture with Rampage: World Tour, including:
- **Midway's SN64 sound engine** (proprietary audio format)
- Similar ROM layout and compression (MIO0)
- Shared OS-level conventions from Midway's N64 SDK usage

The goal is to leverage what we've already learned from Rampage and DKR rather than reinventing the wheel. Patterns that work in one Midway title should translate to the other, and the rendering/runtime infrastructure from DKR provides a battle-tested foundation.

## ROM Information

| Field | Value |
|-------|-------|
| **Title** | San Francisco Rush: Extreme Racing |
| **Region** | USA (NTSC) |
| **Game Code** | NSFE |
| **Entry Point** | 0x80000400 |
| **CRC1** | 2A6B1820 |
| **CRC2** | 6ABCF466 |
| **ROM Size** | 8 MB |
| **Format** | .z64 (big-endian) |

**You must provide your own legally obtained ROM.** Place it in the project root as `baserom.z64`.

## Project Status

### Phase 1: Initial Setup (Complete)
- [x] Project structure and build system
- [x] N64Recomp configuration
- [x] ROM header analysis and validation
- [x] SN64 audio bank scan - candidates at `0x70DB90`, `0x70DB94`, `0x72C87C`

### Phase 2: Static Recompilation (Complete)
- [x] Boot segment analysis - 319 functions in initial code segment (113KB)
- [x] Built N64Recomp from source (N64ModernRuntime)
- [x] Boot segment recompilation - 59,839 lines of C across 7 files
- [x] Identified 14 stub functions (CACHE, TLB, COP0, external calls)
- [x] N64ModernRuntime + RT64 submodules integrated
- [x] **Discovered LZSS-compressed game code** at ROM 0x7A7930 (328KB → 497KB decompressed)
- [x] Wrote LZSS decompressor matching the original `func_80003B18` algorithm
- [x] Built patched ROM with decompressed code at correct VRAM mapping
- [x] **Full recompilation: 1,014 functions, 356,001 lines of C, zero errors**
- [x] 15 functions stubbed (CACHE, TLB, COP0, FPU, 1 invalid instruction)

### Phase 3: Build System & Runtime (Current)
- [x] SDL2 window and input initialization (scaffold)
- [x] RT64 rendering context (scaffold)
- [x] OS function stubs (Controller Pak, EEPROM, SI, debug)
- [x] ROM loading and CRC validation
- [ ] Wire CMakeLists.txt to compile with N64ModernRuntime/RT64
- [ ] Create section_table.cpp, main.cpp runtime entry
- [ ] Connect recompiled code to ultramodern/librecomp

### Phase 4: Boot and Render
- [ ] Game boots to title screen
- [ ] Basic rendering pipeline operational
- [ ] Input mapping (keyboard + gamepad)
- [ ] Framebuffer presentation

### Phase 5: Audio and Polish
- [ ] SN64 audio engine analysis (shared effort with Rampage)
- [ ] RSP audio microcode HLE/recompilation
- [ ] Full gameplay loop
- [ ] Configuration and settings

## What's Next / Outstanding Work

The full codebase is now recompiled — **1,014 functions** from both the boot segment and the LZSS-compressed main game code. The priority list:

1. **Build System Integration** - Wire CMakeLists.txt to compile the recompiled functions with N64ModernRuntime, RT64, and SDL2. Use the Extreme-G and Racer projects as templates.

2. **Runtime Wiring** - Create main.cpp, section_table.cpp, and OS stubs to connect recompiled code to ultramodern/librecomp. Implement thread system, message queues, and PI DMA simulation.

3. **Boot to Title Screen** - Get the game booting with RT64 rendering the first frames.

4. **SN64 Audio** - Shared research effort with [Rampage Recompiled](https://github.com/sp00nznet/Rampage). Midway's proprietary sound engine needs reverse engineering. Audio bank candidates identified at ROM offsets `0x70DB90`, `0x70DB94`, `0x72C87C`.

## Technical Details

### Game Code Discovery

The game uses **LZSS compression** — the main game code is stored compressed at the end of the ROM and decompressed into RDRAM at boot:

| Region | VRAM | Size | Source |
|--------|------|------|--------|
| Boot code | `0x80000400-0x8001BDC8` | 113 KB | ROM 0x1000 (IPL load) |
| Boot BSS | `0x8001E2E0-0x8005BB10` | 252 KB | Zeroed |
| **Game code** | **`0x8005BB10-0x800D8060`** | **497 KB** | **ROM 0x7A7930 (LZSS)** |
| Game BSS | `0x800D8060-0x8016A9B0` | 586 KB | Zeroed |

The boot function `func_80003B18` implements the LZSS decompressor with a 4KB sliding window. A patched ROM (`sfrush_recomp.z64`) is generated with the decompressed code placed at the correct linear offset for N64Recomp to process.

## Building

### Prerequisites
- **Visual Studio 2022** (MSVC toolchain)
- **LLVM/Clang** (for shader preprocessing and clang-cl)
- **CMake 3.20+**
- **Ninja** build system
- **N64Recomp** tool (for generating recompiled functions)

### Build Steps
```bash
# Clone with submodules
git clone --recursive https://github.com/sp00nznet/sfrush.git
cd sfrush

# Place your ROM file
cp /path/to/your/rom.z64 baserom.z64

# Configure and build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

## Project Architecture

```
sfrush/
├── src/
│   ├── main/          # Entry point, SDL window, audio, ROM handling
│   └── game/          # Game logic, input, config, debug, APIs
├── include/           # Public headers
├── lib/               # Third-party submodules
│   ├── N64ModernRuntime/  # ultramodern + librecomp runtime
│   └── rt64/              # N64 RDP GPU emulation (D3D12/Vulkan)
├── symbols/           # Function symbol tables
├── patches/           # Custom MIPS patches
├── scripts/           # Python utilities for recompilation
├── tools/             # Symbol extraction and analysis
├── shaders/           # HLSL shaders
├── rsp/               # RSP audio microcode
└── RecompiledFuncs/   # Generated: recompiled N64 functions
```

## Framework Stack

| Layer | Component | Purpose |
|-------|-----------|---------|
| **Recompiler** | N64Recomp | MIPS-to-C static translation |
| **Runtime** | N64ModernRuntime | Execution environment |
| **OS Layer** | ultramodern | N64 OS function reimplementation |
| **Loader** | librecomp | Recompiled function registry |
| **GPU** | RT64 | N64 RDP emulation via D3D12/Vulkan |
| **Audio** | SN64 (TBD) | Midway's proprietary sound engine |
| **Window** | SDL2 | Cross-platform windowing and input |

## Related Projects

- **[ProjectR](https://t3hd0gg.com/project-r/)** - A separate, unrelated project that ports the *arcade* versions of SF Rush: The Rock and Rush 2049 to PC using Vulkan. Different source material (arcade vs N64) and different approach (reverse engineering vs static recompilation), but cool to see the Rush series getting love from multiple angles.

## Credits & Acknowledgments

- **[N64Recomp](https://github.com/Mr-Wiseguy/N64Recomp)** by Mr-Wiseguy - The static recompilation tool that makes this possible
- **[N64ModernRuntime](https://github.com/Mr-Wiseguy/N64ModernRuntime)** - Runtime framework for recompiled N64 games
- **[RT64](https://github.com/rt64/rt64)** - N64 RDP GPU emulation
- **[Rampage Recompiled](https://github.com/sp00nznet/Rampage)** - Sister Midway recomp project, shared engine research
- **[DKR Recompiled](https://github.com/sp00nznet/diddykongracing)** - N64 recomp project with battle-tested patterns

## Legal

This project is for **educational and preservation purposes only**. No copyrighted game assets are included. San Francisco Rush: Extreme Racing is the property of its respective rights holders. This is a fan-made, community-driven effort and is **not affiliated with, endorsed by, or connected to Atari Games, Midway, Warner Bros., or any other entity**.

If you are a rights holder and have concerns, please open an issue and we will address them promptly.

---

*"It's not about the destination. It's about how many shortcuts you found getting there."*
