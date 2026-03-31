# Valive OS

Educational x86 OS project that boots from a FAT12 floppy image, loads a second-stage bootloader, enters a protected-mode kernel, and runs core phase-1/phase-2 subsystems (interrupts, shell, memory manager, cooperative tasking).

---

## Highlights

- BIOS boot pipeline: Stage1 → Stage2 → Kernel
- FAT12 file loading for `stage2.bin` and `kernel.bin`
- Protected mode kernel startup (GDT/IDT/PIC/TSS path)
- Drivers: VGA text, serial (COM1), PIT timer, PS/2 keyboard
- Shell commands for runtime diagnostics
- Phase 2 subsystems:
	- Memory management (`kmalloc` / `kcalloc` / `kfree`)
	- Task management (cooperative scheduler)

---

## Project Flow (End-to-End)

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
	-> run shell + scheduler loop
```

---

## Stage Breakdown and Subtasks

## Stage1 (Boot Sector)

Primary responsibilities:
- Initialize minimal real-mode environment
- Read FAT12 metadata and root directory
- Locate and load `STAGE2  BIN`
- Transfer control to Stage2

Key subtasks:
- BIOS disk I/O (`int 13h`)
- FAT12 cluster traversal
- Retry/reset handling for disk reads

## Stage2 (Bootloader, 16-bit C/ASM)

Primary responsibilities:
- Initialize drive geometry
- Parse FAT12 filesystem
- Locate/load `KERNEL  BIN`
- Jump to kernel load address

Key subtasks:
- Disk abstraction (`disk.c`)
- FAT12 parser and file loader (`fat12.c`)
- BIOS wrappers (`x86.asm`)
- Console + formatted logging (`stdio.c`)
- Serial mirror logging for debug visibility

## Kernel (Protected Mode)

Primary responsibilities:
- Enter protected mode and initialize architecture state
- Bring up drivers and interrupt handling
- Run shell and background tasks

Key subtasks:
- Boot and address helpers (`boot.c`)
- GDT/TSS (`gdt.c`)
- IDT + ISR/IRQ stubs (`idt.c`, `interrupts.asm`, `interrupts.c`)
- PIC remap/EOI (`pic.c`)
- Timer and keyboard drivers (`timer.c`, `keyboard.c`)
- Console + serial output (`vga.c`, `serial.c`, `stdio.c`)
- Shell (`shell.c`)

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

Scheduling model summary:
- Cooperative switching (tasks must call `task_yield`, `task_sleep`, or `task_exit`)
- Round-robin task selection among READY tasks
- Sleeping tasks wake on PIT tick timeout

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
		pic.c / pic.h
		timer.c / timer.h
		keyboard.c / keyboard.h
		memory.c / memory.h
		task.c / task.h
		shell.c / shell.h
		vga.c / vga.h
		serial.c / serial.h
		stdio.c / stdio.h
		io.c / io.h
		linker.ld
		makefile
```

---

## Build and Run

## Prerequisites

- `nasm`
- `gcc` + `ld` (i386 capable)
- `qemu-system-i386`
- `mkfs.fat` (dosfstools)
- `mcopy` (mtools)

## Build

```bash
make clean && make
```

## Run

```bash
./run.sh
```

`run.sh` uses serial-to-stdio, so Stage2 and kernel diagnostics are visible directly in terminal.

## Run (headless serial log)

```bash
qemu-system-i386 -drive if=floppy,format=raw,file=build/main_floppy.img -serial stdio -monitor none -display none
```

Useful for CI-style checks and quick validation of boot + self-test logs.

---



## Current Limitations

- Cooperative scheduling only (no timer-preemptive task switch yet)
- No paging/virtual memory yet
- No user mode/syscall boundary yet
- No kernel filesystem/VFS layer yet
- Shell is intentionally minimal (line mode)

---

