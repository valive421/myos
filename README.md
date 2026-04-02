# Valive OS

Educational x86 OS project that boots from a FAT12 floppy image, loads a second-stage bootloader, enters a protected-mode kernel, and runs core subsystems (interrupts, shell, memory manager, cooperative tasking) plus Ring3 user tasks via an `int 0x80` syscall ABI and a minimal VFS persisted to a FAT12 data disk.

---

## Demo

https://github.com/user-attachments/assets/d51c8113-1b2d-465b-af87-e882780a4fdc

---

## Quickstart (Build & Run)

### Prerequisites

- `nasm`
- `gcc` + `ld` (i386 capable)
- `qemu-system-i386`
- `mkfs.fat` (dosfstools)
- `mcopy` (mtools)

### Build

```bash
make clean && make
```

### Run

```bash
./run.sh
```

`run.sh` creates/keeps a persistent FAT12 disk image at `build/persist.img` and attaches it to QEMU as an IDE drive.

Notes saved through the VFS (for example via `unote`) are written to `build/persist.img` and survive reboots. `make clean` preserves this image.

To reset/wipe persistent data:

```bash
rm -f build/persist.img
```

### Run (headless serial log)

```bash
qemu-system-i386 \
  -drive if=floppy,format=raw,file=build/main_floppy.img \
  -drive if=ide,format=raw,file=build/persist.img \
  -serial stdio -monitor none -display none
```

Useful for CI-style checks and quick validation of boot + self-test logs.

---

## Highlights

- BIOS boot pipeline: Stage1 → Stage2 → Kernel
- FAT12 file loading for `stage2.bin` and `kernel.bin` (boot floppy)
- Protected mode kernel startup (GDT/IDT/PIC/TSS path)
- Ring3 user-mode tasks + `int 0x80` syscalls
- Minimal VFS syscalls with FAT12-backed persistence on `build/persist.img`
- User program: `unote <file>` notepad (ESC saves + quits)
- Drivers: VGA text, serial (COM1), PIT timer, PS/2 keyboard
- Shell commands for runtime diagnostics
- Phase 2 subsystems:
  - Memory management (`kmalloc` / `kcalloc` / `kfree`)
  - Task management (cooperative scheduler)

---

## System Overview

### End-to-end boot flow

```text
BIOS
  -> loads Stage1 boot sector at 0x7C00
  -> Stage1 reads FAT12 root/FAT and loads STAGE2.BIN
  -> jump to Stage2 entry

Stage2 (16-bit ASM + C)
  -> initialize disk + FAT12 parser
  -> load KERNEL.BIN from FAT12
  -> print progress to VGA + serial
  -> far jump to kernel

Kernel (real mode entry -> protected mode)
  -> bootstrap (A20, GDT handoff)
  -> kmain()
  -> init core subsystems
  -> start background + Ring3 user tasks
  -> run shell + scheduler loop
```

<img width="552" height="2236" alt="flow digram" src="https://github.com/user-attachments/assets/1e2db669-0915-4041-965c-56146e3c5769" />

### Userland → kernel → persistence architecture 

<img width="3578" height="904" alt="1" src="https://github.com/user-attachments/assets/712ca59f-1036-44e9-8546-ed989677d23b" />


---

## Boot-time Memory Map 


<img width="3169" height="2602" alt="mermaid-diagram-2026-04-02-215426" src="https://github.com/user-attachments/assets/f5f44012-3662-41d5-bff3-e4e059e34a20" />


---

## Boot Stages

## Stage1 (Boot Sector)

Primary responsibilities:
- Initialize minimal real-mode environment
- Read FAT12 metadata and root directory
- Locate and load `STAGE2.BIN`
- Transfer control to Stage2

Key subtasks:
- BIOS disk I/O (`int 13h`)
- FAT12 cluster traversal
- Retry/reset handling for disk reads

<img width="552" height="1628" alt="stage 1 flow" src="https://github.com/user-attachments/assets/26ec1d65-afa2-4b45-8b58-e673eb9da7fa" />

## Stage2 (Bootloader, 16-bit C/ASM)

Primary responsibilities:
- Initialize drive geometry
- Parse FAT12 filesystem
- Locate/load `KERNEL.BIN`
- Jump to kernel load address

Diagrams:
- FAT12 / parsing notes
- Stage2 relation to kernel loading

<img width="654" height="2764" alt="sg2 fat12" src="https://github.com/user-attachments/assets/09224300-3db6-4f29-a913-9aa7a3618c40" />
<img width="1266" height="1068" alt="stage 2 relation" src="https://github.com/user-attachments/assets/0bcd24cc-09bf-4188-b698-aa804d811732" />

Key subtasks:
- Disk abstraction (`disk.c`)
- FAT12 parser and file loader (`fat12.c`)
- BIOS wrappers (`x86.asm`)
- Console + formatted logging (`stdio.c`)
- Serial mirror logging for debug visibility

<img width="552" height="1724" alt="stage 2 flow" src="https://github.com/user-attachments/assets/537d6368-7ed4-4402-8fe4-fca6aa38f42b" />

---

## Kernel (Protected Mode)

Primary responsibilities:
- Enter protected mode and initialize architecture state
- Bring up drivers and interrupt handling
- Run shell and background tasks

<img width="552" height="2396" alt="kerenl flow" src="https://github.com/user-attachments/assets/1c6b3331-3827-4f50-b41a-db507bcf0434" />

### Architecture components

- Boot and address helpers (`boot.c`)
- GDT/TSS (`gdt.c`)
- IDT + ISR/IRQ stubs (`idt.c`, `interrupts.asm`, `interrupts.c`)
- PIC remap/EOI (`pic.c`)
- Timer and keyboard drivers (`timer.c`, `keyboard.c`)
- Console + serial output (`vga.c`, `serial.c`, `stdio.c`)
- Shell (`shell.c`)
- Syscall dispatcher (`syscall.c`) + Ring3 entry (`user_mode.asm`)
- ATA PIO disk I/O (`ata.c`) + FAT12 persistence (`fat12.c`)
- Minimal VFS (`vfs.c`) exposed to userland via syscalls

### Interrupt vector layout (IDT) + PIC remap 
<img width="2823" height="496" alt="mermaid-diagram-2026-04-02-215528" src="https://github.com/user-attachments/assets/02d1ac53-1d10-4d3d-9068-1ac7fd7dfb3f" />



### PIC (8259) remap + EOI behavior

Ports:
- Master PIC: command `0x20`, data `0x21`
- Slave PIC: command `0xA0`, data `0xA1`
- EOI value: `0x20`

Remap notes:
- ICW1 = `0x11` (init)
- ICW4 = `0x01` (8086 mode)
- Master ICW3 = `4` (slave on IRQ2)
- Slave ICW3 = `2` (cascade identity)

EOI rule (as implemented in `pic_send_eoi(irq)`):
- If `irq >= 8`, send EOI to slave (`outb(0xA0, 0x20)`)
- Always send EOI to master (`outb(0x20, 0x20)`)

<img width="1420" height="830" alt="mermaid-diagram-2026-04-02-215559" src="https://github.com/user-attachments/assets/bf8a1aaa-3514-415e-a86e-fe3206add751" />


---

## Phase Status

## Phase 1 (Completed)

- Stable Stage1/Stage2/kernel boot chain
- Protected mode kernel startup
- Core drivers and IRQ handling
- Interactive shell baseline
- Serial + VGA diagnostics

## Phase 2.1: Memory Management (Implemented)

Implemented in `src/kernel/memory.c`:
- Free-list heap allocator over fixed kernel heap region
- APIs:
  - `kmalloc(size)`
  - `kcalloc(count, size)`
  - `kfree(ptr)`
- Block splitting and coalescing
- Heap stats:
  - `memory_heap_total()`
  - `memory_heap_used()`
  - `memory_heap_free()`
- Runtime test:
  - `memory_run_self_test()`

<img width="1138" height="604" alt="memory" src="https://github.com/user-attachments/assets/dc8a602a-f95b-4c48-b928-1b09813cf074" />

## Phase 2.2: Task Management (Implemented)

Implemented in `src/kernel/task.c`:
- Fixed-size task table (`TASK_MAX`)
- Cooperative round-robin scheduler
- Per-task saved CPU context (`esp/ebp/ebx/esi/edi/eip`)
- Per-task dedicated kernel stack
- Assembly context-switch routine (`task_switch.asm`)
- Task lifecycle:
  - create, run, sleep, kill, exit
- Introspection APIs for shell/debug
- Runtime task self-test
<img width="1172" height="1676" alt="task" src="https://github.com/user-attachments/assets/0cfd98d0-3fe5-4430-9ee9-ebd1502b796b" />



Scheduling model summary:
- Cooperative switching (tasks must call `task_yield`, `task_sleep`, or `task_exit`)
- Round-robin task selection among READY tasks
- Sleeping tasks wake on PIT tick timeout

## Phase 3: Ring3 Userland + Syscalls + Persistent VFS (Implemented)

- Ring3 tasks entered via IRET, with Ring3→Ring0 transitions through `int 0x80`
- Minimal syscall ABI for console output, heap allocation, sleeping/yielding, and VFS operations
- Minimal in-memory VFS that syncs files to an attached FAT12 data disk (root directory only)
- Ring3 notepad (`unote <file>`): type-only editing, backspace-at-end, ESC saves + quits

---

## Syscall ABI (`int 0x80`)

### Calling convention

- Trigger: `int 0x80`
- Inputs:
  - `EAX` = syscall number
  - `EBX` = arg0
  - `ECX` = arg1
  - `EDX` = arg2
- Output:
  - `EAX` = return value

### Return / error semantics (current behavior)

- Many syscalls return `0` on success.
- Unknown syscall numbers return `0xFFFFFFFF`.
- `SYSCALL_GETC` is non-blocking:
  - returns `0..255` if a character is available
  - returns `0xFFFFFFFF` if no character is available

> Note: console output syscalls currently target VGA only. Serial output can block under host stdio backpressure.

### Syscall table (from `src/user/include/valive/syscall.h`)

| Num | Name | Args (registers) | Returns / semantics |
|---:|---|---|---|
| 0 | `SYSCALL_YIELD` | `()` | `0` |
| 1 | `SYSCALL_SLEEP` | `ticks` (EBX) | `0` |
| 2 | `SYSCALL_EXIT` | `code` (EBX) | does not return (task exits) |
| 3 | `SYSCALL_WRITE` | `const char* s` (EBX) | bytes written to VGA (max 4096) |
| 4 | `SYSCALL_PUTC` | `char c` (EBX low 8 bits) | `0` |
| 5 | `SYSCALL_ALLOC` | `size` (EBX) | pointer (EAX) from `kmalloc` |
| 6 | `SYSCALL_FREE` | `void* ptr` (EBX) | `0` |
| 7 | `SYSCALL_FS_OPEN` | `path` (EBX), `flags` (ECX) | fd / result from `vfs_open` |
| 8 | `SYSCALL_FS_CLOSE` | `fd` (EBX) | result from `vfs_close` |
| 9 | `SYSCALL_FS_READ` | `fd` (EBX), `buf` (ECX), `len` (EDX) | bytes read / result from `vfs_read` |
| 10 | `SYSCALL_FS_WRITE` | `fd` (EBX), `buf` (ECX), `len` (EDX) | bytes written / result from `vfs_write` |
| 11 | `SYSCALL_FS_SEEK` | `fd` (EBX), `offset` (ECX), `whence` (EDX) | result from `vfs_seek` |
| 12 | `SYSCALL_GETC` | `()` | `0..255` char, or `0xFFFFFFFF` if none available |

---

## Shell Commands

- `help` — list commands
- `echo <text>` — print text
- `clear` — clear VGA screen
- `ticks` — live ticks mode (`q` to exit)
- `mem` — heap total/used/free
- `memtest` — memory manager self-test
- `tasks` — list active tasks
- `spawn` — spawn a background counter task
- `kill <id>` — kill task by id
- `tasktest` — task scheduler self-test
- `unote <file>` — open/create a text file in Ring3 notepad (ESC saves+quits)

---

## Repository Structure

```text
src/
  bootloader/
    stage1/
      boot.asm
      makefile
    stage2/
      disk.c / disk.h
      fat12.c / fat12.h
      main.asm / main.c
      memory.c / memory.h
      stdio.c / stdio.h
      x86.asm / x86.h
      linker.ld
      makefile

  kernel/
    main.asm
    kernel.c
    boot.c / boot.h
    gdt.c / gdt.h
    idt.c / idt.h
    interrupts.asm
    interrupts.c / interrupts.h
    user_mode.asm
    syscall.c / syscall.h
    pic.c / pic.h
    timer.c / timer.h
    keyboard.c / keyboard.h
    memory.c / memory.h
    task.c / task.h
    shell.c / shell.h
    vfs.c / vfs.h
    ata.c / ata.h
    fat12.c / fat12.h
    vga.c / vga.h
    serial.c / serial.h
    stdio.c / stdio.h
    io.c / io.h
    linker.ld
    makefile

  user/
    notepad.c
    user_test.c
    include/
      valive/
        syscall.h
        types.h
        vfs.h
```

---

## Current Limitations

- Cooperative scheduling only (no timer-preemptive task switch yet)
- No paging/virtual memory yet
- Ring3 exists, but without paging there is no real memory isolation (CPL separation only)
- FAT12 persistence is a small subset: root directory only, strict 8.3 names (A-Z/0-9/_/-), no subdirectories/LFN
- VFS is intentionally minimal (flat namespace, small fd/file limits, sync-on-close)
- Notepad is intentionally minimal (append/edit-at-end only; ESC saves+quits)
- Shell is intentionally minimal (line mode)
