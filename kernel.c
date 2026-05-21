#include <stdint.h>

/* ===================================================================
   TFD OS v3.0 — Graphical Desktop DEMO
   Real VGA Pixels + Mouse + Clickable Start Menu + Working Shutdown
   No apps, no file system — just a beautiful prototype
   By Sadman | 2026
   =================================================================== */

/* ----- Framebuffer ----- */
static uint8_t *fb;
static int pitch, width, height;

/* ----- Mouse ----- */
static int mouse_x = 160, mouse_y = 100;
static int mouse_btn = 0, prev_btn = 0;
static int mouse_cycle = 0;
static uint8_t mouse_bytes[3];

/* ----- Menu State ----- */
static int menu_open = 0;

/* ----- Colors (Bright VGA Palette) ----- */
#define BLACK   0
#define BLUE    1
#define GREEN   2
#define TEAL    3
#define RED     4
#define YELLOW  6
#define GRAY    7
#define DGRAY   8
#define WHITE   15

/* ----- I/O ----- */
static inline void outb(uint16_t p, uint8_t v) { __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p) { uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }
static inline void iowait(void) { outb(0x80,0); }

/* ----- Pixel Drawing ----- */
static void putpixel(int x, int y, uint8_t c) {
    if (x >= 0 && x < width && y >= 0 && y < height)
        fb[y * pitch + x] = c;
}

static void fill_rect(int x, int y, int w, int h, uint8_t c) {
    for (int dy = 0; dy < h; dy++)
        for (int dx = 0; dx < w; dx++)
            putpixel(x + dx, y + dy, c);
}

static void hline(int x, int y, int w, uint8_t c) {
    for (int i = 0; i < w; i++) putpixel(x + i, y, c);
}

static void vline(int x, int y, int h, uint8_t c) {
    for (int i = 0; i < h; i++) putpixel(x, y + i, c);
}

/* ----- 8x8 Font (Simplified) ----- */
static const uint8_t font_chars[26][8] = {
    [0]  = {0x38,0x6C,0xC6,0xFE,0xC6,0xC6,0xC6,0x00}, /* A */
    [2]  = {0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00}, /* C */
    [4]  = {0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0x00}, /* E */
    [6]  = {0x3C,0x66,0xC0,0xDE,0xC6,0x66,0x3A,0x00}, /* G */
    [8]  = {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, /* I */
    [11] = {0xF0,0x60,0x60,0x60,0x62,0x66,0xFE,0x00}, /* L */
    [12] = {0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00}, /* M */
    [13] = {0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00}, /* N */
    [14] = {0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00}, /* O */
    [17] = {0xFC,0x66,0x66,0x7C,0x6C,0x66,0xE6,0x00}, /* R */
    [18] = {0x7C,0xC6,0x60,0x38,0x0C,0xC6,0x7C,0x00}, /* S */
    [19] = {0x7E,0x5A,0x18,0x18,0x18,0x18,0x3C,0x00}, /* T */
    [20] = {0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00}, /* U */
};

static void draw_char(int x, int y, char c, uint8_t fg, uint8_t bg) {
    if (c < 'A' || c > 'Z') return;
    const uint8_t *g = font_chars[c - 'A'];
    for (int row = 0; row < 8; row++) {
        uint8_t bits = g[row];
        for (int col = 0; col < 8; col++) {
            putpixel(x + col, y + row, (bits & (0x80 >> col)) ? fg : bg);
        }
    }
}

static void draw_string(int x, int y, const char *s, uint8_t fg, uint8_t bg) {
    while (*s) {
        if (*s == ' ') { x += 8; s++; continue; }
        draw_char(x, y, *s, fg, bg);
        x += 8;
        s++;
    }
}

/* ----- Mouse Cursor ----- */
static uint8_t cursor_bg[16*16];

static void cursor_save(void) {
    int idx = 0;
    for (int dy = 0; dy < 16; dy++)
        for (int dx = 0; dx < 16; dx++) {
            int px = mouse_x + dx, py = mouse_y + dy;
            cursor_bg[idx++] = (px >= 0 && px < width && py >= 0 && py < height)
                               ? fb[py * pitch + px] : 0;
        }
}

static void cursor_restore(void) {
    int idx = 0;
    for (int dy = 0; dy < 16; dy++)
        for (int dx = 0; dx < 16; dx++) {
            int px = mouse_x + dx, py = mouse_y + dy;
            if (px >= 0 && px < width && py >= 0 && py < height)
                fb[py * pitch + px] = cursor_bg[idx];
            idx++;
        }
}

static void cursor_draw(void) {
    static const int arrow[16][16] = {
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},{1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0},{1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0},{1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0},{1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0},
    };
    for (int dy = 0; dy < 10; dy++)
        for (int dx = 0; dx < 16; dx++)
            if (arrow[dy][dx])
                putpixel(mouse_x + dx, mouse_y + dy, WHITE);
}

/* ----- Desktop ----- */
static void draw_desktop(void) {
    fill_rect(0, 0, width, height - 24, TEAL);
    
    /* Terminal icon */
    fill_rect(20, 20, 32, 32, BLACK);
    fill_rect(24, 24, 24, 24, GRAY);
    draw_string(14, 56, "TERMINAL", WHITE, TEAL);
    
    /* Shutdown icon */
    fill_rect(20, 100, 32, 32, BLACK);
    fill_rect(24, 104, 24, 24, RED);
    draw_string(12, 136, "SHUTDOWN", WHITE, TEAL);
}

static void draw_taskbar(void) {
    fill_rect(0, height - 24, width, 24, GRAY);
    hline(0, height - 24, width, WHITE);
    
    /* Start button */
    fill_rect(2, height - 22, 50, 20, GRAY);
    hline(2, height - 22, 50, WHITE);
    vline(2, height - 22, 20, WHITE);
    hline(2, height - 3, 50, DGRAY);
    vline(52, height - 22, 20, DGRAY);
    draw_string(8, height - 18, "START", BLACK, GRAY);
    
    /* Clock */
    draw_string(width - 50, height - 18, "12:00", BLACK, GRAY);
}

static void draw_menu(void) {
    int mx = 2, my = height - 24 - 60;
    fill_rect(mx, my, 100, 60, GRAY);
    hline(mx, my, 100, WHITE);
    vline(mx, my, 60, WHITE);
    hline(mx, my + 59, 100, DGRAY);
    vline(mx + 99, my, 60, DGRAY);
    
    draw_string(mx + 8, my + 8, "TERMINAL", BLACK, GRAY);
    draw_string(mx + 8, my + 28, "SHUTDOWN", RED, GRAY);
}

/* ----- Mouse Handling ----- */
static int in_rect(int mx, int my, int x, int y, int w, int h) {
    return (mx >= x && mx < x + w && my >= y && my < y + h);
}

static void mouse_poll(void) {
    if (!(inb(0x64) & 0x20)) return;
    uint8_t d = inb(0x60);
    
    if (mouse_cycle == 0) {
        if (d & 0x08) { mouse_bytes[0] = d; mouse_cycle = 1; }
    } else {
        mouse_bytes[mouse_cycle++] = d;
        if (mouse_cycle == 3) {
            mouse_cycle = 0;
            int dx = mouse_bytes[1], dy = mouse_bytes[2];
            if (mouse_bytes[0] & 0x10) dx |= ~0xFF;
            if (mouse_bytes[0] & 0x20) dy |= ~0xFF;
            dy = -dy;
            mouse_btn = mouse_bytes[0] & 0x07;
            mouse_x += dx / 2;
            mouse_y += dy / 2;
            if (mouse_x < 0) mouse_x = 0;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_x >= width) mouse_x = width - 1;
            if (mouse_y >= height) mouse_y = height - 1;
        }
    }
}

static void check_clicks(void) {
    int clicked = (prev_btn == 0 && mouse_btn != 0);
    prev_btn = mouse_btn;
    if (!clicked) return;
    
    /* Start button */
    if (in_rect(mouse_x, mouse_y, 2, height - 22, 50, 20)) {
        menu_open = !menu_open;
    }
    
    /* Menu - Shutdown */
    if (menu_open) {
        int mx = 2, my = height - 24 - 60;
        if (in_rect(mouse_x, mouse_y, mx + 8, my + 28, 84, 16)) {
            /* REAL SHUTDOWN */
            for (int i = 0; i < width * height; i++) fb[i] = BLACK;
            draw_string(width/2 - 40, height/2, "SHUTTING DOWN...", RED, BLACK);
            for (volatile int d = 0; d < 500000; d++);
            outb(0x64, 0xFE);
            outb(0x604, 0x2000);
            outb(0xB004, 0x2000);
            __asm__ volatile("cli; hlt");
        }
    }
    
    /* Desktop Shutdown icon */
    if (in_rect(mouse_x, mouse_y, 20, 100, 32, 32)) {
        for (int i = 0; i < width * height; i++) fb[i] = BLACK;
        draw_string(width/2 - 40, height/2, "SHUTTING DOWN...", RED, BLACK);
        for (volatile int d = 0; d < 500000; d++);
        outb(0x64, 0xFE);
        outb(0x604, 0x2000);
        outb(0xB004, 0x2000);
        __asm__ volatile("cli; hlt");
    }
}

/* ----- VGA Fallback ----- */
static void set_vga_mode13(void) {
    outb(0x3C2, 0x63);
    outb(0x3D4, 0x00); outb(0x3D5, 0x5F);
    outb(0x3D4, 0x01); outb(0x3D5, 0x4F);
    outb(0x3D4, 0x02); outb(0x3D5, 0x50);
    outb(0x3D4, 0x03); outb(0x3D5, 0x82);
    outb(0x3D4, 0x04); outb(0x3D5, 0x54);
    outb(0x3D4, 0x05); outb(0x3D5, 0x80);
    outb(0x3D4, 0x06); outb(0x3D5, 0xBF);
    outb(0x3D4, 0x07); outb(0x3D5, 0x00);
    outb(0x3D4, 0x08); outb(0x3D5, 0x00);
    outb(0x3D4, 0x09); outb(0x3D5, 0x41);
    outb(0x3D4, 0x10); outb(0x3D5, 0x9C);
    outb(0x3D4, 0x11); outb(0x3D5, 0x8E);
    outb(0x3D4, 0x12); outb(0x3D5, 0x8F);
    outb(0x3D4, 0x13); outb(0x3D5, 0x28);
    outb(0x3D4, 0x14); outb(0x3D5, 0x40);
    outb(0x3D4, 0x15); outb(0x3D5, 0x96);
    outb(0x3D4, 0x16); outb(0x3D5, 0xB9);
    outb(0x3D4, 0x17); outb(0x3D5, 0xA3);
    outb(0x3C4, 0x00); outb(0x3C5, 0x03);
    outb(0x3C4, 0x01); outb(0x3C5, 0x01);
    outb(0x3C4, 0x02); outb(0x3C5, 0x0F);
    outb(0x3C4, 0x04); outb(0x3C5, 0x0E);
    outb(0x3CE, 0x05); outb(0x3CF, 0x40);
    outb(0x3CE, 0x06); outb(0x3CF, 0x05);
    inb(0x3DA);
    outb(0x3C0, 0x30); outb(0x3C0, 0x41);
    outb(0x3C0, 0x33); outb(0x3C0, 0x00);
    outb(0x3C0, 0x20);
    fb = (uint8_t*)0xA0000;
    pitch = 320; width = 320; height = 200;
}

static void set_palette(void) {
    outb(0x3C8, 0);
    static const uint8_t pal[16][3] = {
        {0x00,0x00,0x00},{0x00,0x00,0x3F},{0x00,0x3F,0x00},{0x00,0x3F,0x3F},
        {0x3F,0x00,0x00},{0x3F,0x00,0x3F},{0x3F,0x3F,0x00},{0x3F,0x3F,0x3F},
        {0x1F,0x1F,0x1F},{0x1F,0x1F,0x3F},{0x1F,0x3F,0x1F},{0x1F,0x3F,0x3F},
        {0x3F,0x1F,0x1F},{0x3F,0x1F,0x3F},{0x3F,0x3F,0x1F},{0x3F,0x3F,0x3F},
    };
    for (int i = 0; i < 16; i++) {
        outb(0x3C9, pal[i][0]); outb(0x3C9, pal[i][1]); outb(0x3C9, pal[i][2]);
    }
    for (int i = 16; i < 256; i++) {
        outb(0x3C9, i & 0x3F); outb(0x3C9, (i>>2)&0x3F); outb(0x3C9, 63-(i&0x3F));
    }
}

/* ----- Main ----- */
void kernel_main(uint32_t magic, uint32_t addr) {
    uint32_t *mbi = (uint32_t *)addr;
    fb = 0;

    if ((*mbi & (1<<11)) && *(mbi+22)) {
        fb = (uint8_t*)(uint32_t)*(mbi+22);
        pitch = *(mbi+24); width = *(mbi+25); height = *(mbi+26);
        if (!width || !height) fb = 0;
    }
    if (!fb) set_vga_mode13();
    set_palette();

    /* Init mouse */
    outb(0x64, 0xA8);
    for (volatile int i = 0; i < 10000; i++);
    outb(0x64, 0xD4); outb(0x60, 0xFF);
    for (volatile int i = 0; i < 10000; i++);
    while (inb(0x64) & 1) inb(0x60);
    outb(0x64, 0xD4); outb(0x60, 0xF4);

    while (1) {
        cursor_restore();
        draw_desktop();
        draw_taskbar();
        if (menu_open) draw_menu();
        mouse_poll();
        check_clicks();
        cursor_save();
        cursor_draw();
        for (volatile int d = 0; d < 8000; d++);
    }
}
