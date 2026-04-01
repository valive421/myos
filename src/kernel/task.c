// task.c: Cooperative task scheduler for Phase-2.
//
// Model:
// - Fixed-size task table (TASK_MAX slots).
// - Round-robin scheduling over READY tasks.
// - Each task has its own stack and saved CPU context.
// - Tasks run until they call task_yield/task_sleep/task_exit.

#include "task.h"
#include "gdt.h"
#include "serial.h"
#include "timer.h"
#include "vga.h"

#define TASK_MAX 16
#define TASK_STACK_SIZE 4096
#define USER_STACK_SIZE 4096

#define USER_ARG_MAX 128

typedef struct
{
    uint32_t esp;
    uint32_t ebp;
    uint32_t ebx;
    uint32_t esi;
    uint32_t edi;
    uint32_t eip;
} task_context_t;

struct task
{
    uint32_t id;
    uint32_t state;
    uint32_t runs;
    uint32_t wake_tick;
    uint32_t data0;

    uint8_t is_user;
    uint32_t user_entry;
    uint32_t user_stack_top;
    task_entry_t entry;
    char name[16];
    task_context_t ctx;
    uint8_t stack[TASK_STACK_SIZE];
    uint8_t user_stack[USER_STACK_SIZE];
};

extern void task_enter_user_mode(uint32_t user_eip, uint32_t user_esp);

extern void task_context_switch(task_context_t* from, task_context_t* to);

static task_t g_Tasks[TASK_MAX];
static uint32_t g_NextId = 1;
static uint32_t g_LastIndex = 0;
static task_context_t g_SchedulerCtx;
static task_t* g_CurrentTask = 0;

// Copy task name into fixed 16-byte buffer and always NUL-terminate.
static void str_copy16(char out[16], const char* in)
{
    uint32_t i = 0;
    while (i < 15 && in && in[i])
    {
        out[i] = in[i];
        i++;
    }
    out[i] = 0;

    while (++i < 16)
        out[i] = 0;
}

static void task_bootstrap(void)
{
    task_t* t = g_CurrentTask;
    if (!t || !t->entry)
    {
        for (;;)
            ;
    }

    t->entry(t);

    task_exit(t);
    for (;;)
        ;
}

static void task_user_entry(task_t* task)
{
    if (!task || task->is_user == 0 || task->user_entry == 0 || task->user_stack_top == 0)
        task_exit(task);

    // Ensure Ring3 -> Ring0 transitions land on this task's kernel stack.
    uint32_t kstack_top = (uint32_t)(task->stack + TASK_STACK_SIZE);
    kstack_top &= ~0x0Fu;
    gdt_set_kernel_stack(kstack_top);

    serial_write_str("[K] enter user eip=0x");
    serial_write_hex32(task->user_entry);
    serial_write_str(" usp=0x");
    serial_write_hex32(task->user_stack_top);
    serial_write_str(" ksp0=0x");
    serial_write_hex32(kstack_top);
    serial_write_str("\n");

    task_enter_user_mode(task->user_entry, task->user_stack_top);
    for (;;)
        ;
}

static void task_prepare_initial_context(task_t* t)
{
    uint32_t top = (uint32_t)(t->stack + TASK_STACK_SIZE);
    top &= ~0x0Fu;

    t->ctx.esp = top;
    t->ctx.ebp = top;
    t->ctx.ebx = 0;
    t->ctx.esi = 0;
    t->ctx.edi = 0;
    t->ctx.eip = (uint32_t)task_bootstrap;
}

static uint32_t str_len_max(const char* s, uint32_t max)
{
    if (!s)
        return 0;

    uint32_t n = 0;
    while (n < max && s[n])
        n++;
    return n;
}

// Reset scheduler state and mark all task slots as dead.
void tasking_init(void)
{
    for (uint32_t i = 0; i < TASK_MAX; i++)
    {
        g_Tasks[i].id = 0;
        g_Tasks[i].state = TASK_STATE_DEAD;
        g_Tasks[i].runs = 0;
        g_Tasks[i].wake_tick = 0;
        g_Tasks[i].data0 = 0;
        g_Tasks[i].is_user = 0;
        g_Tasks[i].user_entry = 0;
        g_Tasks[i].user_stack_top = 0;
        g_Tasks[i].entry = 0;
        g_Tasks[i].name[0] = 0;
        g_Tasks[i].ctx.esp = 0;
        g_Tasks[i].ctx.ebp = 0;
        g_Tasks[i].ctx.ebx = 0;
        g_Tasks[i].ctx.esi = 0;
        g_Tasks[i].ctx.edi = 0;
        g_Tasks[i].ctx.eip = 0;
    }

    g_NextId = 1;
    g_LastIndex = 0;
    g_CurrentTask = 0;
    g_SchedulerCtx.esp = 0;
    g_SchedulerCtx.ebp = 0;
    g_SchedulerCtx.ebx = 0;
    g_SchedulerCtx.esi = 0;
    g_SchedulerCtx.edi = 0;
    g_SchedulerCtx.eip = 0;
}

// Create a new task in the first free slot.
// Returns task id on success, -1 on failure.
int task_create(const char* name, task_entry_t entry, uint32_t data0)
{
    if (!entry)
        return -1;

    for (uint32_t i = 0; i < TASK_MAX; i++)
    {
        if (g_Tasks[i].state == TASK_STATE_DEAD)
        {
            g_Tasks[i].id = g_NextId++;
            if (g_NextId == 0)
                g_NextId = 1;

            g_Tasks[i].state = TASK_STATE_READY;
            g_Tasks[i].runs = 0;
            g_Tasks[i].wake_tick = 0;
            g_Tasks[i].data0 = data0;
            g_Tasks[i].is_user = 0;
            g_Tasks[i].user_entry = 0;
            g_Tasks[i].user_stack_top = 0;
            g_Tasks[i].entry = entry;
            str_copy16(g_Tasks[i].name, name ? name : "task");
            task_prepare_initial_context(&g_Tasks[i]);
            return (int)g_Tasks[i].id;
        }
    }

    return -1;
}

// Create a Ring3 user task.
// `user_entry` is the user-mode instruction pointer (EIP) to start executing.
int task_create_user(const char* name, uint32_t user_entry)
{
    if (user_entry == 0)
        return -1;

    for (uint32_t i = 0; i < TASK_MAX; i++)
    {
        if (g_Tasks[i].state == TASK_STATE_DEAD)
        {
            g_Tasks[i].id = g_NextId++;
            if (g_NextId == 0)
                g_NextId = 1;

            g_Tasks[i].state = TASK_STATE_READY;
            g_Tasks[i].runs = 0;
            g_Tasks[i].wake_tick = 0;
            g_Tasks[i].data0 = 0;

            g_Tasks[i].is_user = 1;
            g_Tasks[i].user_entry = user_entry;

            uint32_t u_top = (uint32_t)(g_Tasks[i].user_stack + USER_STACK_SIZE);
            u_top &= ~0x0Fu;
            g_Tasks[i].user_stack_top = u_top;

            g_Tasks[i].entry = task_user_entry;
            str_copy16(g_Tasks[i].name, name ? name : "user");
            task_prepare_initial_context(&g_Tasks[i]);
            return (int)g_Tasks[i].id;
        }
    }

    return -1;
}

// Create a Ring3 user task and pass one string argument (arg0).
// The arg string is copied into the task's user stack.
// The initial user ESP is prepared so the user entry can be declared as:
//   void user_main(const char* arg0);
int task_create_user_arg(const char* name, uint32_t user_entry, const char* arg0)
{
    if (user_entry == 0)
        return -1;

    for (uint32_t i = 0; i < TASK_MAX; i++)
    {
        if (g_Tasks[i].state == TASK_STATE_DEAD)
        {
            g_Tasks[i].id = g_NextId++;
            if (g_NextId == 0)
                g_NextId = 1;

            g_Tasks[i].state = TASK_STATE_READY;
            g_Tasks[i].runs = 0;
            g_Tasks[i].wake_tick = 0;
            g_Tasks[i].data0 = 0;

            g_Tasks[i].is_user = 1;
            g_Tasks[i].user_entry = user_entry;

            uint32_t u_top = (uint32_t)(g_Tasks[i].user_stack + USER_STACK_SIZE);
            u_top &= ~0x0Fu;

            uint32_t sp = u_top;
            uint32_t user_arg_ptr = 0;

            if (arg0 && arg0[0])
            {
                uint32_t len = str_len_max(arg0, USER_ARG_MAX - 1u);

                // Copy string at the very top of the user stack (above initial ESP).
                sp -= (len + 1u);
                char* dst = (char*)sp;
                for (uint32_t j = 0; j < len; j++)
                    dst[j] = arg0[j];
                dst[len] = 0;
                user_arg_ptr = sp;

                // Align down for pushes.
                sp &= ~0x03u;
            }

            // Fake return address + arg0 (cdecl). Function is expected not to return.
            sp -= 4;
            *(uint32_t*)sp = user_arg_ptr;
            sp -= 4;
            *(uint32_t*)sp = 0;

            g_Tasks[i].user_stack_top = sp;

            g_Tasks[i].entry = task_user_entry;
            str_copy16(g_Tasks[i].name, name ? name : "user");
            task_prepare_initial_context(&g_Tasks[i]);
            return (int)g_Tasks[i].id;
        }
    }

    return -1;
}

// Kill task by id. Returns 1 if killed, 0 if not found.
int task_kill(uint32_t id)
{
    for (uint32_t i = 0; i < TASK_MAX; i++)
    {
        if (g_Tasks[i].state != TASK_STATE_DEAD && g_Tasks[i].id == id)
        {
            g_Tasks[i].state = TASK_STATE_DEAD;
            return 1;
        }
    }

    return 0;
}

// Put current task to sleep for `ticks` timer ticks.
void task_sleep(task_t* task, uint32_t ticks)
{
    if (!task)
        return;

    task->wake_tick = timer_get_ticks() + ticks;
    task->state = TASK_STATE_SLEEPING;
    task_context_switch(&task->ctx, &g_SchedulerCtx);
}

// Yield CPU cooperatively to next task.
void task_yield(task_t* task)
{
    if (!task)
        return;

    task->state = TASK_STATE_READY;
    task_context_switch(&task->ctx, &g_SchedulerCtx);
}

// Mark task as dead; scheduler will skip it.
void task_exit(task_t* task)
{
    if (!task)
        return;

    task->state = TASK_STATE_DEAD;
    task_context_switch(&task->ctx, &g_SchedulerCtx);
}

// Generic task-local integer payload getter.
uint32_t task_get_data(task_t* task)
{
    return task ? task->data0 : 0;
}

// Generic task-local integer payload setter.
void task_set_data(task_t* task, uint32_t value)
{
    if (task)
        task->data0 = value;
}

// Run one scheduling step:
// - wake sleeping tasks whose timeout passed
// - select next READY task in round-robin order
// - switch into selected task context
void task_schedule(void)
{
    uint32_t now = timer_get_ticks();

    for (uint32_t step = 0; step < TASK_MAX; step++)
    {
        uint32_t idx = (g_LastIndex + 1 + step) % TASK_MAX;
        task_t* t = &g_Tasks[idx];

        if (t->state == TASK_STATE_DEAD)
            continue;

        if (t->state == TASK_STATE_SLEEPING)
        {
            if (now < t->wake_tick)
                continue;
            t->state = TASK_STATE_READY;
        }

        if (t->state != TASK_STATE_READY)
            continue;

        t->state = TASK_STATE_RUNNING;
        t->runs++;
        g_LastIndex = idx;
        g_CurrentTask = t;

        // Prepare ring0 stack for any CPL change (Ring3 syscalls/IRQs).
        uint32_t kstack_top = (uint32_t)(t->stack + TASK_STACK_SIZE);
        kstack_top &= ~0x0Fu;
        gdt_set_kernel_stack(kstack_top);

        task_context_switch(&g_SchedulerCtx, &t->ctx);
        g_CurrentTask = 0;

        if (t->state == TASK_STATE_RUNNING)
            t->state = TASK_STATE_READY;

        return;
    }
}

task_t* task_current(void)
{
    return g_CurrentTask;
}

int task_current_is_user(void)
{
    return (g_CurrentTask && g_CurrentTask->is_user) ? 1 : 0;
}

int task_any_user_alive(void)
{
    for (uint32_t i = 0; i < TASK_MAX; i++)
    {
        if (g_Tasks[i].state != TASK_STATE_DEAD && g_Tasks[i].is_user)
            return 1;
    }

    return 0;
}

// Number of active (non-dead) tasks.
uint32_t task_count(void)
{
    uint32_t n = 0;
    for (uint32_t i = 0; i < TASK_MAX; i++)
    {
        if (g_Tasks[i].state != TASK_STATE_DEAD)
            n++;
    }
    return n;
}

// Maximum number of task slots supported by this scheduler.
uint32_t task_max_slots(void)
{
    return TASK_MAX;
}

// Read per-slot task info for shell/debug diagnostics.
int task_get_info(uint32_t slot, task_info_t* out)
{
    if (!out || slot >= TASK_MAX)
        return 0;

    out->id = g_Tasks[slot].id;
    out->state = g_Tasks[slot].state;
    out->runs = g_Tasks[slot].runs;
    for (uint32_t i = 0; i < 16; i++)
        out->name[i] = g_Tasks[slot].name[i];

    return (g_Tasks[slot].state != TASK_STATE_DEAD);
}

// Self-test task A: increments by 1 until 10, then exits.
static void selftest_task_a(task_t* task)
{
    while (task_get_data(task) < 10)
    {
        task_set_data(task, task_get_data(task) + 1);
        task_yield(task);
    }

    task_exit(task);
}

// Self-test task B: increments by 2 until 20, then exits.
static void selftest_task_b(task_t* task)
{
    while (task_get_data(task) < 20)
    {
        task_set_data(task, task_get_data(task) + 2);
        task_yield(task);
    }

    task_exit(task);
}

// Scheduler self-test:
// - create two deterministic tasks
// - run scheduler multiple steps
// - verify final counters
int task_run_self_test(void)
{
    tasking_init();

    int id_a = task_create("testA", selftest_task_a, 0);
    int id_b = task_create("testB", selftest_task_b, 0);
    if (id_a < 0 || id_b < 0)
        return 0;

    for (uint32_t i = 0; i < 128 && task_count() > 0; i++)
        task_schedule();

    uint32_t a_ok = 0;
    uint32_t b_ok = 0;

    for (uint32_t i = 0; i < TASK_MAX; i++)
    {
        if (g_Tasks[i].id == (uint32_t)id_a)
            a_ok = (g_Tasks[i].data0 == 10);
        if (g_Tasks[i].id == (uint32_t)id_b)
            b_ok = (g_Tasks[i].data0 == 20);
    }

    tasking_init();
    return (a_ok && b_ok) ? 1 : 0;
}

// Example background task used in runtime demos.
// Updates a marker on screen periodically and sleeps for 1 tick.
static void counter_task(task_t* task)
{
    for (;;)
    {
        uint32_t val = task_get_data(task) + 1;
        task_set_data(task, val);

        if ((val % 100u) == 0u)
        {
            uint32_t col = (uint32_t)(task->id % 80u);
            char c = (char)('0' + ((val / 100u) % 10u));
            vga_write_at(3, (uint8_t)col, c);
        }

        task_sleep(task, 1);
    }
}

// Spawn one demo counter task.
int task_spawn_counter(void)
{
    return task_create("counter", counter_task, 0);
}
