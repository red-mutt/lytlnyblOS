[BITS 32]

global set_cr3
global set_cr0
global flush_tlb
global flush_tlb_page
global get_cr2

set_cr3:
    mov eax, [esp + 4]
    mov cr3, eax
    ret

set_cr0:
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    ret

get_cr2:
    mov eax, cr2 
    ret

; flush entire tlb
; reload CR3 with itself
; cpu discards all cached virtual physcal translations

flush_tlb: 
    mov eax, cr3
    mov cr3, eax
    ret

flush_tlb_page:
    mov eax, [esp + 4]
    invlpg [eax]
    ret
