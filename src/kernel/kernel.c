// kernel.c: Top-level kernel initialization and main runtime loop.
//
// Responsibilities:
// - bring up core subsystems (GDT/IDT/PIC/timer/keyboard/memory/tasking)
// - run boot self-tests
// - provide visible runtime status (heartbeat, IRQ status)
// - call task scheduler + shell each loop iteration

#include "boot.h"
#include "gdt.h"
#include "idt.h"
#include "interrupts.h"
#include "keyboard.h"
#include "memory.h"
#include "pic.h"
#include "serial.h"
#include "shell.h"
#include "stdio.h"
#include "task.h"
#include "timer.h"
#include "vga.h"

extern void user_test_entry(void);

static void write_fixed_text(uint8_t row, uint8_t col, const char* s)
{
	while (*s && col < 80)
	{
		vga_write_at(row, col, *s);
		col++;
		s++;
	}
}

// Draw compact IRQ health flags at fixed screen positions.
static void draw_irq_status(uint8_t irq0_ok, uint8_t irq1_ok)
{
	write_fixed_text(1, 0, "IRQ0:");
	vga_write_at(1, 5, irq0_ok ? 'O' : '-');
	vga_write_at(1, 6, irq0_ok ? 'K' : '-');

	write_fixed_text(1, 9, "IRQ1:");
	vga_write_at(1, 14, irq1_ok ? 'O' : '-');
	vga_write_at(1, 15, irq1_ok ? 'K' : '-');
}

void kmain(void)
{
	// Early platform setup and diagnostics channel.
	boot_enable_a20_fast();
	vga_init();
	serial_init();
	serial_write_str("[K] entered kmain\n");

	printf("Hello from protected mode kernel.\n");
	printf("Kernel C init...\n");

	gdt_init();
	printf("GDT ready\n");

	idt_init();
	printf("IDT ready\n");

	// Phase-2 memory manager bring-up + validation.
	memory_init();
	serial_write_str("[K] heap total bytes=0x");
	serial_write_hex32(memory_heap_total());
	serial_write_str("\n");
	printf("Heap ready: total=%u bytes\n", memory_heap_total());
	int heap_ok = memory_run_self_test();
	printf("Heap self-test: %s\n", heap_ok ? "PASS" : "FAIL");
	serial_write_str(heap_ok ? "[K] heap self-test PASS\n" : "[K] heap self-test FAIL\n");

	// Phase-2 tasking bring-up + validation.
	tasking_init();
	int task_ok = task_run_self_test();
	printf("Task self-test: %s\n", task_ok ? "PASS" : "FAIL");
	serial_write_str(task_ok ? "[K] task self-test PASS\n" : "[K] task self-test FAIL\n");

	tasking_init();
	int counter_id = task_spawn_counter();
	if (counter_id > 0)
		printf("Task counter started id=%d\n", counter_id);

	int user_id = task_create_user("user", (uint32_t)user_test_entry);
	if (user_id > 0)
		printf("User task started id=%d\n", user_id);

	// Timer/keyboard + interrupt controller setup.
	timer_init(100);
	keyboard_init();
	printf("Timer+Keyboard ready\n");

	pic_remap(0x20, 0x28);
	pic_mask_all();
	pic_unmask_timer_keyboard();
	printf("PIC remapped\n");

	__asm__ __volatile__("sti");
	printf("IRQs enabled (timer+keyboard)\n");

	// Shell starts after interrupts and basic services are ready.
	shell_init();

	uint32_t last_heartbeat = 0;
	uint32_t last_status_draw = 0;
	uint8_t heartbeat_on = 0;
	uint8_t irq0_ok = 0;
	uint8_t irq1_ok = 0;

	draw_irq_status(irq0_ok, irq1_ok);
	serial_write_str("[K] waiting for IRQ0/IRQ1\n");

	// Main kernel loop:
	// - update diagnostics
	// - run one scheduler step
	// - process shell input
	// - halt until next interrupt
	for (;;)
	{
		uint32_t now = timer_get_ticks();

		if (!irq0_ok && interrupts_get_irq0_count() > 0)
		{
			irq0_ok = 1;
			//printf("IRQ0 ok\n");
			serial_write_str("[K] IRQ0 ok\n");
		}

		if (!irq1_ok && interrupts_get_irq1_count() > 0)
		{
			irq1_ok = 1;
			//printf("IRQ1 ok\n");
			serial_write_str("[K] IRQ1 ok\n");
		}

		if ((now - last_heartbeat) >= 100)
		{
			last_heartbeat = now;
			heartbeat_on ^= 1u;
			vga_write_at(0, 79, heartbeat_on ? '*' : ' ');
		}

		if ((now - last_status_draw) >= 10)
		{
			last_status_draw = now;
			draw_irq_status(irq0_ok, irq1_ok);
		}

		task_schedule();
		shell_poll();
		__asm__ __volatile__("hlt");
	}
}
