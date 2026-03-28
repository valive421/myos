# Valive OS (16-bit FAT12 Boot Flow)

A small educational x86 OS project that boots from a FAT12 floppy image, loads a second-stage bootloader, then loads and jumps to a kernel.

The project demonstrates:
- BIOS-based disk and video services (`int 13h`, `int 10h`)
- FAT12 file loading in real mode
- Mixed ASM + C stage2 bootloader with GCC (`-m16`)
- Protected mode handoff in kernel
- Reproducible floppy image build pipeline

---

## Table of Contents

1. [Project Goals](#project-goals)
2. [Current Boot Flow](#current-boot-flow)
3. [Architecture Overview](#architecture-overview)
4. [Repository Structure](#repository-structure)
5. [Prerequisites](#prerequisites)
6. [Build and Run](#build-and-run)
7. [Debugging](#debugging)
8. [Important Code Paths](#important-code-paths)
9. [Known Limitations](#known-limitations)
10. [Roadmap Ideas](#roadmap-ideas)
11. [Security and Safety Notes](#security-and-safety-notes)
12. [File-by-File Reference (Brief)](#file-by-file-reference-brief)

---

## Project Goals

- Keep the boot pipeline minimal and easy to inspect.
- Teach real-mode boot concepts using a practical, runnable codebase.
- Separate responsibilities clearly:
  - Stage1: BIOS bootstrap + file discovery
  - Stage2: filesystem/disk/memory abstractions + kernel loading
  - Kernel: first executable payload

---

## Current Boot Flow

```text
BIOS
  -> loads stage1 boot sector at 0000:7C00
  -> stage1 reads FAT12 root + FAT and loads STAGE2.BIN
  -> jumps to stage2 entry
  -> stage2 C runtime initializes
  -> stage2 loads KERNEL.BIN from FAT12 into 0x4000:0000
  -> far jump to kernel entry
  -> kernel prints hello message
```

---

## Architecture Overview

### Stage1 (boot sector, 512 bytes)
- Runs directly from BIOS boot sector.
- Parses root directory to find `STAGE2  BIN`.
- Walks FAT12 chain for stage2 and loads it into memory.
- Transfers control to stage2.

### Stage2 (ASM + C, 16-bit)
- ASM stub sets up stack and calls C entrypoint (`cstart_`).
- C modules provide:
  - BIOS disk abstraction
  - FAT12 parsing and file loading
  - Real-mode memory helper functions
  - Basic text output and `printf`
- Loads `KERNEL  BIN` and jumps to it.

### Kernel (flat binary)
- Prints a real-mode confirmation message first.
- Enters 32-bit protected mode using a minimal GDT setup.
- Prints a protected-mode message using VGA text memory.

---

## Repository Structure

```text
.
├── bochs_config
├── bx_enh_dbg.ini
├── debug.bochsrc
├── debug.sh
├── makefile
├── run.sh
├── snapshot.txt
├── build/
│   └── ... generated artifacts ...
└── src/
    ├── bootloader/
    │   ├── stage1/
    │   │   ├── boot.asm
    │   │   └── makefile
    │   └── stage2/
    │       ├── disk.c / disk.h
    │       ├── fat12.c / fat12.h
    │       ├── linker.ld
    │       ├── linker.lnk
    │       ├── main.asm
    │       ├── main.c
    │       ├── makefile
    │       ├── memory.c / memory.h
    │       ├── stdint.h
    │       ├── stdio.c / stdio.h
    │       ├── x86.asm / x86.h
    └── kernel/
        ├── main.asm
        └── makefile
```

---

## Prerequisites

### Required tools
- `nasm`
- `qemu-system-i386`
- `mkfs.fat` (from `dosfstools`)
- `mcopy` (from `mtools`)
- `gcc` (with `-m16` support)
- `ld` (GNU binutils, i386 linking support)

### Optional (debugging)
- `bochs`

Stage2 is built with GCC/NASM/LD in ELF mode and linked as a raw binary via
`src/bootloader/stage2/linker.ld`.

---

## Build and Run

### Full rebuild
```bash
make clean && make
```

### Run in QEMU
```bash
./run.sh
```

### What `make` produces
- `build/stage1.bin`
- `build/stage2.bin`
- `build/kernel.bin`
- `build/main_floppy.img` (FAT12 floppy image)

The root makefile writes stage1 to sector 0 and copies `stage2.bin` + `kernel.bin` into FAT12 filesystem entries.

---

## Debugging

### Bochs debug run
```bash
./debug.sh
```

Uses:
- `bochs_config` (VM + floppy + GUI debugger settings)
- `debug.bochsrc` (command script; currently empty/placeholder)
- `bx_enh_dbg.ini` (debugger UI preferences)

---

## Important Code Paths

### Stage1 critical routines (`src/bootloader/stage1/boot.asm`)
- `start` / `.after`: environment setup and drive param fetch
- `disk_read`: CHS reads with retries via BIOS `int 13h`
- FAT12 cluster decode logic (`.odd` / `.even` branches)
- final transfer: `jmp KERNEL_LOAD_SEGMENT:KERNEL_LOAD_OFFSET` (for stage1-loaded payload)

### Stage2 entry (`src/bootloader/stage2/main.asm`, `main.c`)
- ASM `entry` passes boot drive and calls `cstart_`
- C `cstart_` sequence:
  1. `DISK_Initialize`
  2. `FAT12_Initialize`
  3. `FAT12_LoadFileByName("KERNEL  BIN", 0x4000, 0x0000, ...)`
  4. `x86_JumpToKernel(0x4000, 0x0000)`

### FAT12 implementation (`src/bootloader/stage2/fat12.c`)
- BPB parsing from sector 0
- root/FAT layout derivation (`fatLba`, `rootDirLba`, `dataLba`)
- 12-bit FAT entry extraction (`offset = n + n/2`)
- root directory scan for 8.3 name

### BIOS interface (`src/bootloader/stage2/x86.asm`)
- `x86_Disk_Read` / `x86_Disk_Reset` / `x86_Disk_GetDriveParams`
- `x86_Video_WriteCharTeletype`
- `x86_div64_32` helper used for division in 16-bit environment
- `x86_JumpToKernel` far jump helper used by stage2 C code

### Kernel entry (`src/kernel/main.asm`)
- sets `DS/ES/SS` to `CS`
- initializes stack
- prints `Hello from kernel (real mode).`
- switches to protected mode and prints `Hello from protected mode kernel!`

### Important code snippets

**Stage2 kernel load call**
```c
FAT12_LoadFileByName(&disk, &fs, "KERNEL  BIN", 0x4000, 0x0000, &kernelFileSize);
```
Loads `KERNEL.BIN` from FAT12 root into memory at `0x4000:0000`.

**FAT12 next-cluster extraction (12-bit packed)**
```c
uint16_t fatOffset = cluster + (cluster / 2);
uint16_t entry = read_u16(&g_fat[fatOffset]);
next = (cluster & 1) ? (entry >> 4) : (entry & 0x0FFF);
```
Decodes cluster chain entries from FAT12 packed format.

**Real-mode far jump to loaded kernel**
```c
x86_JumpToKernel(0x4000, 0x0000);
```
Transfers control from stage2 to kernel entry point.

---

## Known Limitations

- FAT12 root-directory-only loading (no subdirectories).
- Minimal protected mode support only (no paging/IDT/interrupt framework yet).
- Kernel is a flat binary payload (no ELF loader).
- Limited diagnostics and error recovery paths.

---

## Roadmap Ideas

- Load files from subdirectories.
- Add A20 enable routine before entering protected mode.
- Introduce GDT/IDT and basic interrupt handlers.
- Move kernel to C/ASM mixed architecture.
- Add memory map collection (E820) and boot info handoff struct.
- Add automated test boot checks in CI (QEMU serial output assertions).

---

## Security and Safety Notes

- This is an educational OS boot project, not production boot code.
- BIOS-level disk code assumes trusted boot media.
- No signature verification or secure boot chain is implemented.

---

## File-by-File Reference (Brief)

### Root
- **`makefile`**: top-level build orchestration; creates FAT12 image and copies binaries.
- **`run.sh`**: launches QEMU with floppy image.
- **`debug.sh`**: starts Bochs debugger session.
- **`bochs_config`**: Bochs machine + floppy boot configuration.
- **`debug.bochsrc`**: Bochs debugger command script placeholder.
- **`bx_enh_dbg.ini`**: Bochs enhanced debugger UI settings.
- **`snapshot.txt`**: captured debug/boot output sample.

### `src/bootloader/stage1`
- **`boot.asm`**: boot sector, FAT12 scan/load logic, BIOS helpers, transfer to loaded payload.
- **`makefile`**: builds stage1 flat binary.

### `src/bootloader/stage2`
- **`main.asm`**: 16-bit ASM entry and C handoff.
- **`main.c`**: stage2 control flow (`disk -> fat12 -> load kernel -> jump`).
- **`disk.h`**: disk geometry struct + disk API declarations.
- **`disk.c`**: LBA->CHS conversion and sector read retry logic.
- **`fat12.h`**: FAT12 metadata struct and public FAT12 loader APIs.
- **`fat12.c`**: FAT12 parser + root search + cluster-chain file loader.
- **`memory.h`**: segment-range helper declarations.
- **`memory.c`**: real-mode memory helper implementations.
- **`x86.h`**: BIOS/ASM boundary declarations.
- **`x86.asm`**: low-level BIOS wrappers and arithmetic helper.
- **`stdio.h`**: minimal text output API declarations.
- **`stdio.c`**: screen output + compact `printf` implementation.
- **`stdint.h`**: local fixed-width integer typedefs.
- **`linker.ld`**: GNU LD script for stage2 raw binary layout.
- **`linker.lnk`**: legacy OpenWatcom linker file (kept for reference).
- **`makefile`**: compiles/links stage2 objects and outputs `stage2.bin`.

### `src/kernel`
- **`main.asm`**: kernel entry code and hello print routine.
- **`makefile`**: builds kernel flat binary.

---
