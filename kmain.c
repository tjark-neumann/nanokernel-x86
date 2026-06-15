#include "console.h"
#include "gdt.h"
#include "interrupts.h"
#include "timer.h"
#include "keyboard.h"
#include "pmm.h"
#include "paging.h"
#include "sched.h"

static void delay(volatile uint32_t n) { while (n--) __asm__ volatile("nop"); }

static void task_a(void) {
    for (;;) { console_write("[A]"); delay(8000000); }
}
static void task_b(void) {
    for (;;) { console_write("[B]"); delay(8000000); }
}

void kmain(void) {
    console_init();
    console_write("nanokernel booting\n");

    gdt_init();        console_write("gdt   ok\n");
    idt_init();        console_write("idt   ok\n");
    pmm_init();        console_write("pmm   ok, free frames="); console_dec(pmm_free_count()); console_putc('\n');
    paging_init();     console_write("paging on\n");

    uint32_t a = pmm_alloc();
    uint32_t b = pmm_alloc();
    console_write("alloc frame "); console_hex(a); console_write(" and "); console_hex(b); console_putc('\n');
    pmm_free(a);
    console_write("freed "); console_hex(a); console_write(", free frames="); console_dec(pmm_free_count()); console_putc('\n');

    timer_init(100);   console_write("timer 100hz\n");
    keyboard_init();   console_write("keyboard ok\n");

    sched_init();
    sched_spawn(task_a);
    sched_spawn(task_b);
    console_write("scheduler: spawned 2 tasks, enabling interrupts\n");

    __asm__ volatile("sti");
    for (;;) __asm__ volatile("hlt");
}
