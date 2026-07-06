#include "mm/pmm.h"
#include "kernel/kprintf.h"
#include "kernel/string.h"

extern uint8_t _kernel_start;
extern uint8_t _kernel_end;

static uint32_t frame_bitmap[PMM_MAX_FRAMES / 32u];
static uint32_t total_frames_count;
static uint32_t free_frames_count;

static void bitmap_set(uint32_t frame)
{
    frame_bitmap[frame / 32u] |= 1u << (frame % 32u);
}

static void bitmap_clear(uint32_t frame)
{
    frame_bitmap[frame / 32u] &= ~(1u << (frame % 32u));
}

static int bitmap_test(uint32_t frame)
{
    return (frame_bitmap[frame / 32u] & (1u << (frame % 32u))) != 0;
}

static uint32_t align_up(uint32_t value)
{
    return (value + PMM_FRAME_SIZE - 1u) & ~(PMM_FRAME_SIZE - 1u);
}

static uint32_t align_down(uint32_t value)
{
    return value & ~(PMM_FRAME_SIZE - 1u);
}

void pmm_reserve_range(uint32_t base, uint32_t length)
{
    uint32_t start = align_down(base) / PMM_FRAME_SIZE;
    uint32_t end = align_up(base + length) / PMM_FRAME_SIZE;
    if (end > PMM_MAX_FRAMES) {
        end = PMM_MAX_FRAMES;
    }

    for (uint32_t frame = start; frame < end; ++frame) {
        if (!bitmap_test(frame)) {
            bitmap_set(frame);
            if (free_frames_count > 0) {
                --free_frames_count;
            }
        }
    }
}

void pmm_release_range(uint32_t base, uint32_t length)
{
    uint32_t start = align_up(base) / PMM_FRAME_SIZE;
    uint32_t end = align_down(base + length) / PMM_FRAME_SIZE;
    if (end > PMM_MAX_FRAMES) {
        end = PMM_MAX_FRAMES;
    }

    for (uint32_t frame = start; frame < end; ++frame) {
        if (bitmap_test(frame)) {
            bitmap_clear(frame);
            ++free_frames_count;
        }
    }
}

static void release_range64(uint64_t base, uint64_t length)
{
    uint64_t end = base + length;
    if (base >= 0x100000000ull) {
        return;
    }
    if (end > 0x100000000ull) {
        end = 0x100000000ull;
    }
    pmm_release_range((uint32_t)base, (uint32_t)(end - base));
}

void pmm_init(const multiboot_info_t *mbi)
{
    memset(frame_bitmap, 0xFF, sizeof(frame_bitmap));
    total_frames_count = PMM_MAX_FRAMES;
    free_frames_count = 0;

    if (mbi != 0 && (mbi->flags & MULTIBOOT_INFO_MEM_MAP) != 0) {
        uint32_t offset = 0;
        while (offset < mbi->mmap_length) {
            const multiboot_mmap_entry_t *entry =
                (const multiboot_mmap_entry_t *)(mbi->mmap_addr + offset);

            if (entry->type == 1) {
                release_range64(entry->addr, entry->len);
            }

            offset += entry->size + sizeof(entry->size);
        }
    } else {
        pmm_release_range(0x00100000u, 64u * 1024u * 1024u);
    }

    pmm_reserve_range(0, 0x00100000u);
    pmm_reserve_range((uint32_t)&_kernel_start,
                      (uint32_t)(&_kernel_end - &_kernel_start));

    kprintf("PMM: %u free frames of %u total tracked\n",
            free_frames_count, total_frames_count);
}

uint32_t pmm_alloc_frame(void)
{
    for (uint32_t word = 0; word < PMM_MAX_FRAMES / 32u; ++word) {
        if (frame_bitmap[word] == 0xFFFFFFFFu) {
            continue;
        }

        for (uint32_t bit = 0; bit < 32u; ++bit) {
            uint32_t frame = word * 32u + bit;
            if (!bitmap_test(frame)) {
                bitmap_set(frame);
                --free_frames_count;
                return frame * PMM_FRAME_SIZE;
            }
        }
    }

    return 0;
}

void pmm_free_frame(uint32_t phys_addr)
{
    uint32_t frame = phys_addr / PMM_FRAME_SIZE;
    if (frame >= PMM_MAX_FRAMES || !bitmap_test(frame)) {
        return;
    }

    bitmap_clear(frame);
    ++free_frames_count;
}

uint32_t pmm_total_frames(void)
{
    return total_frames_count;
}

uint32_t pmm_free_frames(void)
{
    return free_frames_count;
}
