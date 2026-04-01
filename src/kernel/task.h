#pragma once

#include "types.h"

typedef struct task task_t;
typedef void (*task_entry_t)(task_t* task);

enum
{
    TASK_STATE_DEAD = 0,
    TASK_STATE_READY = 1,
    TASK_STATE_RUNNING = 2,
    TASK_STATE_SLEEPING = 3
};

typedef struct
{
    uint32_t id;
    uint32_t state;
    uint32_t runs;
    char name[16];
} task_info_t;

void tasking_init(void);
int task_create(const char* name, task_entry_t entry, uint32_t data0);
int task_create_user(const char* name, uint32_t user_entry);
int task_create_user_arg(const char* name, uint32_t user_entry, const char* arg0);
int task_kill(uint32_t id);
void task_schedule(void);
uint32_t task_count(void);
uint32_t task_max_slots(void);
int task_get_info(uint32_t slot, task_info_t* out);

task_t* task_current(void);
int task_current_is_user(void);
int task_any_user_alive(void);

void task_sleep(task_t* task, uint32_t ticks);
void task_yield(task_t* task);
void task_exit(task_t* task);
uint32_t task_get_data(task_t* task);
void task_set_data(task_t* task, uint32_t value);

int task_run_self_test(void);
int task_spawn_counter(void);
