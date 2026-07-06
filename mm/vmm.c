#include "mm/vmm.h"
#include "mm/pmm.h"
#include "arch/isr.h"
#include "kernel/kprintf.h"
#include "kernel/string.h"

#define PAGE_DIRECTORY_ENTRIES 1024u
#define PAGE_TABLE_ENTRIES     1024u
#define STATIC_PAGE_TABLES       64u

static uint32_t page_directory[PAGE_DIRECTORY_ENTRIES] __attribute__((aligned(4096)));
static uint32_t low_page_table[PAGE_TABLE_ENTRIES] __attribute__((aligned(4096)));
static uint32_t high_page_table[PAGE_TABLE_ENTRIES] __attribute__((aligned(4096)));
static uint32_t page_table_pool[STATIC_PAGE_TABLES][PAGE_TABLE_ENTRIES] __attribute__((aligned(4096)));
static uint32_t page_table_pool_next;
static uint32_t heap_reserve_next = KERNEL_HEAP_START;

static inline uint32_t read_cr2(void)
{
    uint32_t value;
    __asm__ volatile("mov %%cr2, %0" : "=r"(value));
    return value;
}

static inline void write_cr3(uint32_t value)
{
    __asm__ volatile("mov %0, %%cr3" : : "r"(value) : "memory");
}

static inline void enable_paging(void)
{
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");
}

static inline void invlpg(uint32_t virtual_addr)
{
    __asm__ volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
}

static uint32_t *alloc_page_table(void)
{
    if (page_table_pool_next >= STATIC_PAGE_TABLES) {
        panic("VMM page table pool exhausted");
    }

    uint32_t *table = page_table_pool[page_table_pool_next++];
    memset(table, 0, VMM_PAGE_SIZE);
    return table;
}

void vmm_map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags)
{
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FFu;
    uint32_t *table;

    if ((page_directory[pd_index] & VMM_PAGE_PRESENT) == 0) {
        table = alloc_page_table();
        page_directory[pd_index] = ((uint32_t)table & 0xFFFFF000u) |
                                   VMM_PAGE_PRESENT | VMM_PAGE_WRITE;
    } else {
        table = (uint32_t *)(page_directory[pd_index] & 0xFFFFF000u);
    }

    table[pt_index] = (physical_addr & 0xFFFFF000u) |
                      (flags & 0xFFFu) | VMM_PAGE_PRESENT;
    invlpg(virtual_addr);
}

void vmm_unmap_page(uint32_t virtual_addr)
{
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FFu;
    if ((page_directory[pd_index] & VMM_PAGE_PRESENT) == 0) {
        return;
    }

    uint32_t *table = (uint32_t *)(page_directory[pd_index] & 0xFFFFF000u);
    table[pt_index] = 0;
    invlpg(virtual_addr);
}

uint32_t vmm_get_physical(uint32_t virtual_addr)
{
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FFu;
    if ((page_directory[pd_index] & VMM_PAGE_PRESENT) == 0) {
        return 0;
    }

    uint32_t *table = (uint32_t *)(page_directory[pd_index] & 0xFFFFF000u);
    if ((table[pt_index] & VMM_PAGE_PRESENT) == 0) {
        return 0;
    }

    return (table[pt_index] & 0xFFFFF000u) | (virtual_addr & 0xFFFu);
}

void *vmm_heap_reserve_page(void)
{
    if (heap_reserve_next >= KERNEL_HEAP_LIMIT) {
        return 0;
    }

    void *page = (void *)heap_reserve_next;
    heap_reserve_next += VMM_PAGE_SIZE;
    return page;
}

registers_t *vmm_page_fault(registers_t *regs)
{
    uint32_t fault_addr = read_cr2();
    uint32_t page = fault_addr & 0xFFFFF000u;

    if (page >= KERNEL_HEAP_START && page < heap_reserve_next) {
        uint32_t frame = pmm_alloc_frame();
        if (frame == 0) {
            panic("out of physical memory during page fault");
        }
        vmm_map_page(page, frame, VMM_PAGE_WRITE);
        return regs;
    }

    kprintf("Page fault at %x eip=%x err=%x\n", fault_addr, regs->eip, regs->err_code);
    panic("unhandled page fault");
}

void vmm_init(void)
{
    memset(page_directory, 0, sizeof(page_directory));
    memset(low_page_table, 0, sizeof(low_page_table));
    memset(high_page_table, 0, sizeof(high_page_table));
    page_table_pool_next = 0;

    for (uint32_t i = 0; i < PAGE_TABLE_ENTRIES; ++i) {
        uint32_t phys = i * VMM_PAGE_SIZE;
        low_page_table[i] = phys | VMM_PAGE_PRESENT | VMM_PAGE_WRITE;
        high_page_table[i] = phys | VMM_PAGE_PRESENT | VMM_PAGE_WRITE;
    }

    page_directory[0] = ((uint32_t)low_page_table & 0xFFFFF000u) |
                        VMM_PAGE_PRESENT | VMM_PAGE_WRITE;
    page_directory[KERNEL_HIGH_HALF >> 22] =
        ((uint32_t)high_page_table & 0xFFFFF000u) |
        VMM_PAGE_PRESENT | VMM_PAGE_WRITE;

    isr_register_handler(14, vmm_page_fault);
    write_cr3((uint32_t)page_directory);
    enable_paging();

    kprintf("VMM: paging enabled, high-half alias at %x\n", KERNEL_HIGH_HALF);
}
