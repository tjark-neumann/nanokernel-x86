#include "sched.h"
#include <stdint.h>

#define MAX_TASKS  8
#define STACK_SIZE 4096

typedef struct {
    uint32_t esp;
    int used;
} task_t;

static task_t  tasks[MAX_TASKS];
static uint8_t stacks[MAX_TASKS][STACK_SIZE] __attribute__((aligned(16)));
static int current;
static int count;

extern uint32_t (*schedule_hook)(uint32_t esp);

/* Called from the timer IRQ. Saves the current stack, picks the next task,
   returns the stack to resume on. */
static uint32_t schedule(uint32_t esp) {
    tasks[current].esp = esp;
    do { current = (current + 1) % MAX_TASKS; } while (!tasks[current].used);
    return tasks[current].esp;
}

void sched_init(void) {
    for (int i = 0; i < MAX_TASKS; i++) tasks[i].used = 0;
    tasks[0].used = 1;          /* the boot/kmain context becomes task 0 */
    current = 0;
    count = 1;
    schedule_hook = schedule;
}

void sched_spawn(void (*entry)(void)) {
    if (count >= MAX_TASKS) return;
    int i = count++;
    tasks[i].used = 1;

    uint32_t* sp = (uint32_t*)(stacks[i] + STACK_SIZE);
    *(--sp) = 0x202;            /* eflags (IF set) */
    *(--sp) = 0x08;            /* cs */
    *(--sp) = (uint32_t)entry; /* eip */
    *(--sp) = 0;               /* err_code */
    *(--sp) = 0;               /* int_no */
    *(--sp) = 0;               /* eax */
    *(--sp) = 0;               /* ecx */
    *(--sp) = 0;               /* edx */
    *(--sp) = 0;               /* ebx */
    *(--sp) = 0;               /* esp (ignored by popa) */
    *(--sp) = 0;               /* ebp */
    *(--sp) = 0;               /* esi */
    *(--sp) = 0;               /* edi */
    *(--sp) = 0x10;            /* ds */
    tasks[i].esp = (uint32_t)sp;
}
