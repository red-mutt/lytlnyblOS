[BITS 32]

global set_cr3
global set_cr0

set_cr3:
    mov eax, [esp + 4]
    mov cr3, eax
    ret

set_cr0:
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    ret
