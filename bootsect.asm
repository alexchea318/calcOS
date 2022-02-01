.code16
.intel_syntax noprefix

.global _start

_start:
    mov ax, cs
    mov ds, ax
    mov ss, ax
    mov sp, _start
    mov bx, offset str
    mov ax, 3
    int 0x10
    call _vivod
    mov ax, 0x1000
    mov es, ax
    mov ah, 0x02
    mov dl, 0x01
    mov dh, 0x00
    mov cl, 0x01
    mov ch, 0x00
    mov al, 13
    mov bx, 0x00
    int 0x13
    jmp _input

_vivod:
    mov al, [bx]
    test al, al
    jz end_puts

    mov ah, 0x0e
    int 0x10
    add bx, 1
    jmp _vivod

    end_puts:
    ret


_input:
    mov ah, 0
    int 0x16
    mov bh, al
    mov al, bh
    mov ah, 0x0e
    int 0x10
    jmp _prepare

gdt:
    .byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    .byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00
    .byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00

gdt_info:
    .word gdt_info - gdt
    .word gdt, 0

_prepare:
    cli
    lgdt gdt_info
    in al, 0x92
    or al, 2
    out 0x92, al
    mov eax,cr0
    or al,1
    mov cr0, eax
    jmp 0x8:_protected_mode

str:
    .string "Press: 1-green,2-blue, 3-red, 4-yellow, 5-gray, 6-white: "

.code32
_protected_mode:
    mov ax, 0x10
    mov es, ax
    mov ds, ax
    mov ss, ax
    call 0x10000
    
    .zero(512-($ - _start) - 2)
    .byte 0x55, 0xAA

