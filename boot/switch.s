[BITS 32]

section .text
global switch_to
global task_entry_trampoline
extern scheduler_task_entry

; void switch_to(uint32_t *old_esp_slot, uint32_t new_esp)
; Kept as a cooperative context-switch primitive. Preemption uses the
; interrupt-frame return path in interrupt.s.
switch_to:
    pusha
    mov eax, [esp + 36]
    mov edx, [esp + 40]
    mov [eax], esp
    mov esp, edx
    popa
    ret

task_entry_trampoline:
    push eax
    call scheduler_task_entry
.halt:
    cli
    hlt
    jmp .halt
