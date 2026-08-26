[BITS 32]

global save_context

KPROCESS_REGS equ 4

REG_EIP equ 0
REG_CS equ 4
REG_EFLAGS equ 8
REG_DS equ 12
REG_EDI equ 16
REG_ESI equ 20
REG_EBP equ 24
REG_ESP equ 28
REG_EBX equ 32
REG_EDX equ 36
REG_ECX equ 40
REG_EAX equ 44

save_context:
    mov esi, [esp + 4]

    mov [esi + KPROCESS_REGS + REG_EAX], eax
    mov [esi + KPROCESS_REGS + REG_EBX], ebx
    mov [esi + KPROCESS_REGS + REG_ECX], ecx
    mov [esi + KPROCESS_REGS + REG_EDX], edx

    mov [esi + KPROCESS_REGS + REG_ESI], esi
    mov [esi + KPROCESS_REGS + REG_EDI], edi
    mov [esi + KPROCESS_REGS + REG_EBP], ebp

    move [esi + KPROCESS_REGS + REG_ESP], esp 

    mov eas, ds
    mov [esi + KPROCESS_REGS + REG_DS], eax

    ret


    
