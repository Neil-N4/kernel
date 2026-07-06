[BITS 32]

MB_MAGIC    equ 0x1BADB002
MB_FLAGS    equ 0x00000003
MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS)

section .multiboot
align 4
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .text
global _start
extern kernel_main

_start:
    cli
    mov edi, eax
    mov esi, ebx
    mov esp, stack_top
    xor ebp, ebp

    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx

    push esi
    push edi
    call kernel_main

.halt:
    cli
    hlt
    jmp .halt
