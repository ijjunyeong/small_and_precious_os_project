; kernel_entry.asm - C 커널로 들어가기 위한 32비트 진입 코드
[bits 32]

global _start
extern kernel_main

_start:
    call kernel_main

.hang:
    hlt
    jmp .hang
