[BITS 32]

global get_registers

get_registers:
    pusha

    mov eax, ds
    push eax

    pushfd ; push eflags
    
    mov eax, cs
    push eax

    ; push eip later

    ret
