ARCH       := i386
TARGET     := i686-elf
CC         := $(TARGET)-gcc
LD         := $(TARGET)-gcc
AS         := nasm
QEMU       := qemu-system-i386
HOST_CC    := clang

BUILD_DIR  := build
ISO_DIR    := $(BUILD_DIR)/isodir
KERNEL_ELF := $(BUILD_DIR)/kernel.elf
KERNEL_ISO := $(BUILD_DIR)/kernel.iso

CFLAGS := -std=gnu11 -ffreestanding -O2 -Wall -Wextra \
          -fno-builtin -fno-stack-protector -fno-pic -fno-pie \
          -m32 -mno-sse -mno-mmx -Iinclude
ASFLAGS := -f elf32
LDFLAGS := -T linker.ld -ffreestanding -nostdlib -Wl,-z,max-page-size=0x1000
LIBS := -lgcc

ASM_SRCS := \
	boot/boot.s \
	boot/descriptors.s \
	boot/interrupt.s \
	boot/switch.s

C_SRCS := \
	kernel/kernel.c \
	kernel/serial.c \
	kernel/kprintf.c \
	kernel/string.c \
	arch/x86/gdt.c \
	arch/x86/idt.c \
	arch/x86/isr.c \
	arch/x86/pic.c \
	arch/x86/pit.c \
	mm/pmm.c \
	mm/vmm.c \
	mm/page_cache.c \
	sched/rbtree.c \
	sched/scheduler.c \
	fs/ata.c \
	fs/ext2.c

OBJS := $(ASM_SRCS:%.s=$(BUILD_DIR)/%.o) $(C_SRCS:%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean run iso check-toolchain syntax-check

all: $(KERNEL_ELF)

check-toolchain:
	@command -v $(CC) >/dev/null || { echo "$(CC) not found"; exit 1; }
	@command -v $(AS) >/dev/null || { echo "$(AS) not found"; exit 1; }

syntax-check:
	$(HOST_CC) -target i386-unknown-none-elf -std=gnu11 -ffreestanding \
		-fno-builtin -fno-stack-protector -mno-sse -mno-mmx \
		-Iinclude -Wall -Wextra -fsyntax-only $(C_SRCS)

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(KERNEL_ELF): linker.ld $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

iso: $(KERNEL_ISO)

$(KERNEL_ISO): $(KERNEL_ELF) grub.cfg
	@mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/kernel.elf
	cp grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(ISO_DIR)

run: $(KERNEL_ELF)
	$(QEMU) -kernel $(KERNEL_ELF) -serial stdio -display none -m 128M

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
