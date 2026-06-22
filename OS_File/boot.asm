; boot.asm - 512 byte boot sector
; 역할:
; 1) BIOS가 0x7C00에 이 코드를 올려 실행한다.
; 2) 디스크의 2번째 섹터부터 C 커널을 0x1000에 읽는다.
; 3) GDT를 설정하고 32비트 보호 모드로 전환한다.
; 4) 0x1000 주소의 커널 진입 코드로 이동한다.

[org 0x7C00]
[bits 16]

KERNEL_OFFSET  equ 0x1000
KERNEL_SECTORS equ 32          ; 커널을 최대 32섹터, 즉 16KiB까지 읽음

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [BOOT_DRIVE], dl       ; BIOS가 부팅 드라이브 번호를 DL에 넣어 줌

    mov si, MSG_BOOT
    call print16

    call load_kernel
    call enable_a20
    call switch_to_pm

.hang:
    jmp .hang

; -----------------------------
; BIOS teletype 출력 루틴, 16비트 real mode 전용
; SI = 0으로 끝나는 문자열 주소
; -----------------------------
print16:
    pusha
.next:
    lodsb
    cmp al, 0
    je .done
    mov ah, 0x0E
    mov bh, 0x00
    int 0x10
    jmp .next
.done:
    popa
    ret

; -----------------------------
; BIOS int 13h로 커널을 메모리 0x1000에 읽기
; CHS 기준: Cylinder 0, Head 0, Sector 2부터 읽음
; -----------------------------
load_kernel:
    mov si, MSG_LOAD
    call print16

    mov bx, KERNEL_OFFSET      ; ES:BX = 0000:1000
    mov ah, 0x02               ; BIOS read sectors
    mov al, KERNEL_SECTORS     ; 읽을 섹터 수
    mov ch, 0x00               ; cylinder 0
    mov cl, 0x02               ; sector 2, sector 번호는 1부터 시작
    mov dh, 0x00               ; head 0
    mov dl, [BOOT_DRIVE]
    int 0x13

    jc disk_error
    ret

disk_error:
    mov si, MSG_DISK_ERROR
    call print16
    jmp $

; -----------------------------
; A20 라인을 켜서 1MiB 이상 주소 접근 가능하게 함
; 빠른 A20 방식, QEMU/현대 PC에서 보통 잘 동작
; -----------------------------
enable_a20:
    in al, 0x92
    or al, 00000010b
    out 0x92, al
    ret

; -----------------------------
; 보호 모드 전환
; -----------------------------
switch_to_pm:
    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 0x1                ; CR0의 PE 비트 설정
    mov cr0, eax

    jmp CODE_SEG:init_pm       ; far jump로 CS를 보호 모드 코드 세그먼트로 갱신

; -----------------------------
; GDT: null, code, data 세그먼트
; base=0, limit=4GiB, 32비트 flat model
; -----------------------------
gdt_start:

gdt_null:
    dq 0x0000000000000000

gdt_code:
    dw 0xFFFF                  ; limit low
    dw 0x0000                  ; base low
    db 0x00                    ; base middle
    db 10011010b               ; present, ring0, code, readable
    db 11001111b               ; 4KiB granularity, 32-bit, limit high
    db 0x00                    ; base high

gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b               ; present, ring0, data, writable
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

[bits 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000
    mov esp, ebp

    call KERNEL_OFFSET         ; kernel_entry.asm의 _start가 이 위치에 있음

.pm_hang:
    hlt
    jmp .pm_hang

BOOT_DRIVE db 0
MSG_BOOT db "Booting TinyOS...", 13, 10, 0
MSG_LOAD db "Loading C kernel...", 13, 10, 0
MSG_DISK_ERROR db "Disk read error!", 13, 10, 0

; 부트 섹터는 반드시 512바이트이고, 마지막 2바이트가 0xAA55여야 함
times 510 - ($ - $$) db 0
dw 0xAA55
