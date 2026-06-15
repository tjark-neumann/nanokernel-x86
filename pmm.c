#include "pmm.h"

/* Manage 128 MB of physical RAM (QEMU's default). Frames below 4 MB are
   reserved for the kernel image, page directory, and low memory. */
#define MEM_BYTES   (128u * 1024 * 1024)
#define RESERVED    (4u * 1024 * 1024)
#define NFRAMES     (MEM_BYTES / PAGE_SIZE)

static uint32_t bitmap[NFRAMES / 32];
static uint32_t free_frames;

static void set_used(uint32_t f)  { bitmap[f / 32] |=  (1u << (f % 32)); }
static void set_free(uint32_t f)  { bitmap[f / 32] &= ~(1u << (f % 32)); }
static int  is_used(uint32_t f)   { return bitmap[f / 32] & (1u << (f % 32)); }

void pmm_init(void) {
    free_frames = 0;
    for (uint32_t f = 0; f < NFRAMES; f++) {
        if (f * PAGE_SIZE < RESERVED) set_used(f);
        else { set_free(f); free_frames++; }
    }
}

uint32_t pmm_alloc(void) {
    for (uint32_t f = 0; f < NFRAMES; f++) {
        if (!is_used(f)) {
            set_used(f);
            free_frames--;
            return f * PAGE_SIZE;
        }
    }
    return 0;
}

void pmm_free(uint32_t frame) {
    uint32_t f = frame / PAGE_SIZE;
    if (f < NFRAMES && is_used(f)) {
        set_free(f);
        free_frames++;
    }
}

uint32_t pmm_free_count(void) {
    return free_frames;
}
