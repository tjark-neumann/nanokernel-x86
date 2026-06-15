#include "paging.h"
#include <stdint.h>

/* 1024 4 MB pages identity-mapped => full 4 GB. PSE keeps it to one table. */
static uint32_t page_dir[1024] __attribute__((aligned(4096)));

void paging_init(void) {
    for (uint32_t i = 0; i < 1024; i++)
        page_dir[i] = (i * 0x400000) | 0x83;   /* present | rw | page-size(4M) */

    __asm__ volatile("mov %0, %%cr3" : : "r"(page_dir));

    uint32_t cr4;
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= 0x10;                                 /* CR4.PSE */
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));

    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;                           /* CR0.PG */
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}
