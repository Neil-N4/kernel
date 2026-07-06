# Deterministic x86 Monolithic Kernel

This repository contains a freestanding 32-bit x86 monolithic kernel based on the provided project specification. It targets IA-32 protected mode, Multiboot-compatible boot, QEMU execution, and zero dependency on host operating-system abstractions.

## Implemented Modules

- Boot and hardware foundation: Multiboot header, 16 KiB bootstrap stack, GDT, TSS descriptor, IDT, PIC remap, PIT timer, serial logging on COM1, and assembly interrupt stubs.
- Memory management: Multiboot memory-map parsing, deterministic bitmap-backed 4 KiB physical frame allocation, two-level paging, identity map for the first 4 MiB, high-half kernel alias at `0xC0000000`, page fault diagnostics, and static active sector cache.
- Scheduler: fixed task pool, preemptive CFS-style scheduling, red-black tree runqueue indexed by `vruntime`, weighted tick accounting, and interrupt-frame context switching.
- Filesystem: ATA PIO sector read/write, EXT2 superblock/group/inode parsing, root-relative path lookup, direct and single-indirect file reads, and bitmap-backed sequential block allocation.

## Toolchain

The intended build environment uses:

- `i686-elf-gcc`
- `nasm`
- GNU `ld` through the cross GCC driver
- `qemu-system-i386`
- optional `grub-mkrescue` for ISO creation

On macOS with Homebrew, the non-cross pieces are typically:

```sh
brew install nasm qemu xorriso
```

The cross compiler still needs to be installed or built as `i686-elf-gcc`.

## Build and Run

```sh
make check-toolchain
make
make run
```

To build a GRUB ISO:

```sh
make iso
```

## Host Syntax Check

If the full cross toolchain is not installed but Clang is available, run:

```sh
make syntax-check
```

This checks all C translation units using an `i386-unknown-none-elf` target. It does not assemble NASM sources or link the kernel.

## Repository Layout

```text
boot/          Multiboot entry, descriptor loaders, ISR stubs, context switch hooks
arch/x86/      GDT, IDT, ISR dispatch, PIC, PIT
kernel/        serial console, printf, freestanding string/memory routines, kernel main
mm/            physical memory, paging, page cache
sched/         red-black tree and CFS-style scheduler
fs/            ATA PIO and EXT2
include/       subsystem headers
```

## Notes

The implementation intentionally uses fixed-size pools, static buffers, bounded polling loops, and bitmap scans. There is no libc, heap allocator, or dynamic allocation dependency in scheduler balancing or filesystem lookup paths.
