.global isr_keyboard
.global isr_timer
.global idt_load

isr_keyboard:
    pusha
    call keyboard_handler
    movb $0x20, %al
    outb %al, $0x20
    popa
    iret

isr_timer:
    pusha
    call timer_handler
    movb $0x20, %al
    outb %al, $0x20
    popa
    iret

idt_load:
    movl 4(%esp), %eax
    lidt (%eax)
    ret
