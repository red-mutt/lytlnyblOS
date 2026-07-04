start:
    mov ax, 07C0h
    mov ds, ax

    mov si, title_string
    call print_string

    mov si, message_string
    call print_string

    call load_kernel_from_disk
    jmp 0900h:0000 ; gives control to the kernel by jumping to it's starting point.

load_kernel_from_disk:
    mov ax, 0900h
    mov es, ax
    
    mov ah, 02h ; service number, 
    mov al, 09h ; number of sectors we want to read from (only simple kernel for now, so less than 512 bytes)
    
    mov ch, 0h ; number of track we would like to read from, is just 0.
    mov cl, 02h ; sector number that we would like to read its content, this is the second sector

    mov dh, 0h ; the type of disk we would like to read from, 0h means we are reading from a floppy disk. 
    mov dl, 80h ; this is the hard disk we are reading from, 80h means hard disk #0, 81h would be hard disk #1

    mov bx, 0h ; memory adress that content will be loaded into
    int 13h ; 13h provides services related to hard disk

    ; if successful, carry flag will be set to 0, otherwise carry flag is 1
    jc kernel_load_error

    ret

kernel_load_error:
    mov si, load_error_string
    call print_string

    jmp $

print_string:
    mov ah, 0Eh ; bios number 0Eh, sets for teletype output function
print_char:
    lodsb ; loads byte at SI, into AL and increments SI

    cmp al, 0 ; 0 stored in al if at end of string
    je printing_finished

    int 10h ;bios interrupt 0x10, to print char stored in AL

    jmp print_char
printing_finished:
    ;print new line
    mov al, 10d ; ASCII code for new line
    int 10h 

    ;read current cursor position
    mov ah, 03h ; function to read cursor position
    mov bh, 0 ; page number 0 for default page
    int 10h ; 10h now used to read cursor position

    ;move cursor to beggining
    mov ah, 02h ; function to set cursor position
    mov dl, 0 ; column number (0 for begginign of line)
    int 10h ; 0x10 to set cursor pos

    ret

title_string db 'Welcome to the lytlnybl bootloader!',0
message_string db 'Loading up the kernel for you...',0
load_error_string db 'Oh oh!, there was a problem loading the kernel',0

times 510-($-$$) db 0 ; pads the rest of the bootloader with 510 bytes, aiming for a 512 byte bootloader
dw 0xAA55 ; specifies the end of the bootloader, recognised by bios
