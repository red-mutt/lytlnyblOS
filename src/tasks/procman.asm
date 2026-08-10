[BITS 32]

global load_tss

load_tss:
    mov, ax, 0x28
    ltr ax
    ret
