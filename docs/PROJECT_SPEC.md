# Project Specification Mapping

This project is derived from the supplied monolithic kernel specification:

- Architecture: x86 IA-32, 32-bit protected mode.
- Boot protocol: Multiboot-compatible entry.
- Toolchain: `i686-elf-gcc`, `nasm`, GNU linker, QEMU.
- Build mode: `-ffreestanding`, `-nostdlib`, no libc.

## Module 1: Core Hardware and Boot

- `boot/boot.s` defines the Multiboot header, disables interrupts, installs a 16 KiB temporary stack, preserves Multiboot registers, and calls `kernel_main`.
- `arch/x86/gdt.c` defines kernel/user code and data segments plus a TSS descriptor.
- `arch/x86/idt.c`, `boot/interrupt.s`, and `arch/x86/isr.c` install 256 IDT gates and route CPU exceptions/IRQs through a unified C dispatcher.
- `arch/x86/pic.c` remaps IRQ0 and IRQ1 to vectors 32 and 33.

## Module 2: Memory Management

- `mm/pmm.c` parses the Multiboot memory map and tracks 4 KiB physical frames with a deterministic bitmap scan.
- `mm/vmm.c` creates x86 two-level paging, identity maps the first 4 MiB, aliases the kernel range at `0xC0000000`, and handles page faults with `CR2` diagnostics.
- `mm/page_cache.c` implements a static direct-mapped sector cache used by filesystem I/O.

## Module 3: CFS-Style Scheduler

- `include/sched/task.h` defines task state, saved interrupt-frame stack pointer, and `vruntime`.
- `sched/rbtree.c` implements a malloc-free red-black tree using links embedded in the preallocated task structures.
- `sched/scheduler.c` charges each PIT tick to the current task and selects the minimum-`vruntime` runnable task.
- `boot/interrupt.s` supports preemption by allowing the ISR dispatcher to return a replacement saved stack frame.

## Module 4: EXT2 Filesystem

- `fs/ata.c` implements primary-master ATA PIO sector I/O using ports `0x1F0` through `0x1F7`.
- `include/fs/ext2.h` defines packed EXT2 superblock, block group descriptor, inode, and directory entry structures.
- `fs/ext2.c` mounts an EXT2 volume, reads inodes, traverses directory entries, resolves absolute paths such as `/bin/init`, reads file data, and allocates blocks near a requested goal block via the block bitmap.
