#include "interrupts.h"
#include "console.h"
#include "io.h"

struct idt_entry {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  zero;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr ip;
static irq_handler_t handlers[16];

extern void idt_load(uint32_t);
extern void isr0(); extern void isr1(); extern void isr2(); extern void isr3();
extern void isr4(); extern void isr5(); extern void isr6(); extern void isr7();
extern void isr8(); extern void isr9(); extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();
extern void irq0(); extern void irq1(); extern void irq2(); extern void irq3();
extern void irq4(); extern void irq5(); extern void irq6(); extern void irq7();
extern void irq8(); extern void irq9(); extern void irq10(); extern void irq11();
extern void irq12(); extern void irq13(); extern void irq14(); extern void irq15();

/* Set by the scheduler; lets the timer IRQ change the stack we return on. */
uint32_t (*schedule_hook)(uint32_t esp) = 0;

static void set_gate(int n, uint32_t base) {
    idt[n].base_low  = base & 0xFFFF;
    idt[n].sel       = 0x08;
    idt[n].zero      = 0;
    idt[n].flags     = 0x8E;
    idt[n].base_high = (base >> 16) & 0xFFFF;
}

static void pic_remap(void) {
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    outb(0x21, 0x00); outb(0xA1, 0x00);
}

static void pic_eoi(int irq) {
    if (irq >= 8) outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

void irq_install(int irq, irq_handler_t h) {
    handlers[irq] = h;
}

void idt_init(void) {
    ip.limit = sizeof(idt) - 1;
    ip.base  = (uint32_t)&idt;
    for (int i = 0; i < 256; i++) set_gate(i, 0);

    uint32_t isrs[32] = {
        (uint32_t)isr0,(uint32_t)isr1,(uint32_t)isr2,(uint32_t)isr3,
        (uint32_t)isr4,(uint32_t)isr5,(uint32_t)isr6,(uint32_t)isr7,
        (uint32_t)isr8,(uint32_t)isr9,(uint32_t)isr10,(uint32_t)isr11,
        (uint32_t)isr12,(uint32_t)isr13,(uint32_t)isr14,(uint32_t)isr15,
        (uint32_t)isr16,(uint32_t)isr17,(uint32_t)isr18,(uint32_t)isr19,
        (uint32_t)isr20,(uint32_t)isr21,(uint32_t)isr22,(uint32_t)isr23,
        (uint32_t)isr24,(uint32_t)isr25,(uint32_t)isr26,(uint32_t)isr27,
        (uint32_t)isr28,(uint32_t)isr29,(uint32_t)isr30,(uint32_t)isr31,
    };
    for (int i = 0; i < 32; i++) set_gate(i, isrs[i]);

    uint32_t irqs[16] = {
        (uint32_t)irq0,(uint32_t)irq1,(uint32_t)irq2,(uint32_t)irq3,
        (uint32_t)irq4,(uint32_t)irq5,(uint32_t)irq6,(uint32_t)irq7,
        (uint32_t)irq8,(uint32_t)irq9,(uint32_t)irq10,(uint32_t)irq11,
        (uint32_t)irq12,(uint32_t)irq13,(uint32_t)irq14,(uint32_t)irq15,
    };
    for (int i = 0; i < 16; i++) set_gate(32 + i, irqs[i]);

    pic_remap();
    idt_load((uint32_t)&ip);
}

uint32_t isr_handler(registers_t* r) {
    uint32_t int_no = r->int_no;
    uint32_t ret = (uint32_t)r;

    if (int_no < 32) {
        console_write("\n[EXCEPTION ");
        console_dec(int_no);
        console_write(" err=");
        console_hex(r->err_code);
        console_write("] halting\n");
        for (;;) __asm__ volatile("cli; hlt");
    }

    int irq = int_no - 32;
    if (handlers[irq]) handlers[irq](r);
    if (irq == 0 && schedule_hook) ret = schedule_hook((uint32_t)r);
    pic_eoi(irq);
    return ret;
}
