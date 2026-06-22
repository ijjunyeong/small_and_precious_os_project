/*
 * kernel.c - TinyOS 32-bit C kernel
 * 표준 라이브러리 없이 동작하는 freestanding C 코드입니다.
 * 구현 기능:
 * - VGA 텍스트 모드 출력
 * - PS/2 키보드 입력, 영어 소문자/숫자 중심
 * - help, clear, about, echo, reboot, halt 명령어
 */

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

#define COLOR_WHITE_ON_BLACK 0x0F

static int cursor_row = 0;
static int cursor_col = 0;
static uint8_t text_color = COLOR_WHITE_ON_BLACK;

/* -----------------------------
   포트 입출력: 하드웨어와 직접 통신
   ----------------------------- */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* -----------------------------
   VGA 출력 관련 함수
   ----------------------------- */
static uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

static void move_cursor(void) {
    uint16_t pos = (uint16_t)(cursor_row * VGA_WIDTH + cursor_col);

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void clear_screen(void) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[y * VGA_WIDTH + x] = vga_entry(' ', text_color);
        }
    }
    cursor_row = 0;
    cursor_col = 0;
    move_cursor();
}

static void scroll_if_needed(void) {
    if (cursor_row < VGA_HEIGHT) return;

    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[(y - 1) * VGA_WIDTH + x] = VGA_MEMORY[y * VGA_WIDTH + x];
        }
    }

    for (int x = 0; x < VGA_WIDTH; x++) {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', text_color);
    }

    cursor_row = VGA_HEIGHT - 1;
}

static void putchar(char c) {
    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
    } else if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
            VGA_MEMORY[cursor_row * VGA_WIDTH + cursor_col] = vga_entry(' ', text_color);
        }
    } else {
        VGA_MEMORY[cursor_row * VGA_WIDTH + cursor_col] = vga_entry(c, text_color);
        cursor_col++;
        if (cursor_col >= VGA_WIDTH) {
            cursor_col = 0;
            cursor_row++;
        }
    }

    scroll_if_needed();
    move_cursor();
}

static void print(const char* s) {
    while (*s) {
        putchar(*s++);
    }
}

/* -----------------------------
   문자열 처리: 표준 라이브러리 없이 직접 구현
   ----------------------------- */
static int str_equal(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static int starts_with(const char* s, const char* prefix) {
    while (*prefix) {
        if (*s++ != *prefix++) return 0;
    }
    return 1;
}

/* -----------------------------
   키보드 입력
   현재는 Shift/CapsLock 미지원, 간단한 영문 소문자 키맵만 사용
   ----------------------------- */
static const char scancode_to_ascii[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'', '`', 0, '\\',
    'z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
};

static char keyboard_getchar(void) {
    uint8_t scancode;

    for (;;) {
        while ((inb(0x64) & 1) == 0) {
            /* 키보드 버퍼에 데이터가 들어올 때까지 대기 */
        }

        scancode = inb(0x60);

        /* 키를 뗄 때 발생하는 release scancode는 무시 */
        if (scancode & 0x80) continue;

        if (scancode < 128) {
            char c = scancode_to_ascii[scancode];
            if (c != 0) return c;
        }
    }
}

/* -----------------------------
   시스템 명령어
   ----------------------------- */
static void reboot(void) {
    print("Rebooting...\n");

    /* 8042 키보드 컨트롤러를 통해 CPU reset 요청 */
    while (inb(0x64) & 0x02) {
    }
    outb(0x64, 0xFE);

    for (;;) {
        __asm__ volatile ("hlt");
    }
}

static void halt(void) {
    print("CPU halted. Close QEMU window to exit.\n");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

static void prompt(void) {
    print("tinyos> ");
}

static void process_command(char* cmd) {
    if (str_equal(cmd, "")) {
        return;
    }

    if (str_equal(cmd, "help")) {
        print("Commands:\n");
        print("  help      - show this help\n");
        print("  clear     - clear screen\n");
        print("  about     - show OS info\n");
        print("  echo TEXT - print TEXT\n");
        print("  reboot    - reboot emulator\n");
        print("  halt      - halt CPU\n");
    } else if (str_equal(cmd, "clear")) {
        clear_screen();
    } else if (str_equal(cmd, "about")) {
        print("TinyOS: 16-bit bootloader + 32-bit protected mode C kernel.\n");
        print("Built for NASM, GCC, LD and QEMU.\n");
    } else if (starts_with(cmd, "echo ")) {
        print(cmd + 5);
        print("\n");
    } else if (str_equal(cmd, "reboot")) {
        reboot();
    } else if (str_equal(cmd, "halt")) {
        halt();
    } else {
        print("Unknown command: ");
        print(cmd);
        print("\nType 'help'.\n");
    }
}

void kernel_main(void) {
    char input[128];
    int len = 0;

    clear_screen();
    print("TinyOS started in 32-bit protected mode.\n");
    print("Type 'help' and press Enter.\n\n");
    prompt();

    for (;;) {
        char c = keyboard_getchar();

        if (c == '\n') {
            putchar('\n');
            input[len] = 0;
            process_command(input);
            len = 0;
            prompt();
        } else if (c == '\b') {
            if (len > 0) {
                len--;
                putchar('\b');
            }
        } else {
            if (len < 127) {
                input[len++] = c;
                putchar(c);
            }
        }
    }
}
