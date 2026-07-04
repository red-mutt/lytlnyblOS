;global start
;global print_string
;global enter_protected
;global p_mode_main

; no org code starts at 0x0900 though
[bits 16]
start:
    mov ax, cs
    mov ds, ax

    mov si, hello_string - start
    call print_string

    jmp enter_protected

print_string:
    mov ah, 0Eh

print_char:
    lodsb ; sets al = [DS:SI++]

    cmp al, 0
    je done
    
    int 10h

    jmp print_char

done:
    ret

enter_protected:
    cli ;disable interrupts
    lgdt [gdtr - start] ; load GDT registor with start address of GDT
    mov eax, cr0
    or eax, 1 ;set protection enable bit in control register 0 (cr0)
    mov cr0, eax

    ; perform far jump to selector 08h (offset into GDT, pointing at a 32bit
    ; PM code segment descriptor)
    ; to load CS with proper PM32 descriptor)


    CODE_SEG equ gdt_code - gdt_start
    jmp CODE_SEG:p_mode_main
[bits 32]
p_mode_main:
    mov ax, 10h
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x9000

    mov byte [0xB8000], 'P'
    mov byte [0xB8001], 0x02

    ; go into C

    extern kernel_main
    call kernel_main
hang:
    hlt
    jmp hang

hello_string db 'Hello World!, i am lytlnyblOS, in real mode', 0

gdt_start:
gdt_null:
    dq 0
gdt_code:
    dw 0xFFFF ; limit
    dw 0x0000 ; base_low
    db 0x00 ;base_middle
    db 0x9A ;access
    db 0xCF ;flags + limit high 4 bits
    db 0x00 ;base_high
gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0xCF
    db 0x00
gdt_end:
gdtr:
    dw gdt_end - gdt_start - 1 ; set manually for testing
    dd gdt_start

