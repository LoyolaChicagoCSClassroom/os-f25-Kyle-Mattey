.global ide_read_buffer

ide_read_buffer:
    pushl %edi
    pushl %ecx
    
    movl 12(%esp), %edi
    movl 16(%esp), %ecx
    movw $0x1F0, %dx
    
    rep insw
    
    popl %ecx
    popl %edi
    ret
