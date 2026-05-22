#include <stdint.h>

/* ===================================================================
   TFD OS v3.0 "Foxy" — VGA Mode 13h Desktop
   Skyblue + White Taskbar + Start Menu + Working Mouse
   By Sadman | 2026
   =================================================================== */

static uint8_t *fb;
static int pitch = 320, width = 320, height = 200;

/* Mouse */
static int mx = 160, my = 100;
static int mbtn = 0, pbtn = 0;
static int mcycle = 0;
static uint8_t mbytes[3];

/* Menu */
static int menu_open = 0;

/* Colors */
#define BLACK   0x00
#define SKYBLUE 0x03
#define RED     0x04
#define GRAY    0x07
#define WHITE   0x0F
#define DGRAY   0x08
#define GREEN   0x02

/* I/O */
static inline void outb(uint16_t p, uint8_t v) { __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p) { uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }

/* Pixel */
static void pp(int x, int y, uint8_t c) {
    if (x >= 0 && x < width && y >= 0 && y < height) fb[y * pitch + x] = c;
}
static void fr(int x, int y, int w, int h, uint8_t c) {
    for (int dy = 0; dy < h; dy++)
        for (int dx = 0; dx < w; dx++)
            pp(x + dx, y + dy, c);
}
static void hl(int x, int y, int w, uint8_t c) {
    for (int i = 0; i < w; i++) pp(x + i, y, c);
}
static void vl(int x, int y, int h, uint8_t c) {
    for (int i = 0; i < h; i++) pp(x, y + i, c);
}

/* Font */
static void dchar(int x, int y, char c, uint8_t fg, uint8_t bg) {
    static const uint8_t glyphs[26][8] = {
        [0]={0x38,0x6C,0xC6,0xFE,0xC6,0xC6,0xC6,0x00},
        [3]={0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0x00},
        [4]={0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0x00},
        [7]={0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00},
        [12]={0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00},
        [13]={0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00},
        [14]={0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00},
        [17]={0xFC,0x66,0x66,0x7C,0x6C,0x66,0xE6,0x00},
        [18]={0x7C,0xC6,0x60,0x38,0x0C,0xC6,0x7C,0x00},
        [19]={0x7E,0x5A,0x18,0x18,0x18,0x18,0x3C,0x00},
        [20]={0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00},
    };
    if (c < 'A' || c > 'Z') return;
    const uint8_t *g = glyphs[c - 'A'];
    for (int r = 0; r < 8; r++) {
        uint8_t b = g[r];
        for (int col = 0; col < 8; col++)
            pp(x + col, y + r, (b & (0x80 >> col)) ? fg : bg);
    }
}
static void dstr(int x, int y, const char *s, uint8_t fg, uint8_t bg) {
    while (*s) { if (*s == ' ') { x += 8; s++; continue; } dchar(x, y, *s, fg, bg); x += 8; s++; }
}

/* Mouse Cursor */
static uint8_t cbg[12*20];
static void csave(void) {
    int idx = 0;
    for (int dy = 0; dy < 12; dy++)
        for (int dx = 0; dx < 20; dx++) {
            int px = mx + dx, py = my + dy;
            cbg[idx++] = (px >= 0 && px < width && py >= 0 && py < height) ? fb[py * pitch + px] : 0;
        }
}
static void crest(void) {
    int idx = 0;
    for (int dy = 0; dy < 12; dy++)
        for (int dx = 0; dx < 20; dx++) {
            int px = mx + dx, py = my + dy;
            if (px >= 0 && px < width && py >= 0 && py < height) fb[py * pitch + px] = cbg[idx];
            idx++;
        }
}
static void cdraw(void) {
    static const int arrow[12][20] = {
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    };
    for (int dy = 0; dy < 12; dy++)
        for (int dx = 0; dx < 20; dx++)
            if (arrow[dy][dx]) pp(mx + dx, my + dy, WHITE);
}

/* Desktop */
static void draw_wallpaper(void) {
    fr(0, 0, 320, 176, SKYBLUE);
}
static void draw_taskbar(void) {
    int ty = 176;
    fr(0, ty, 320, 24, WHITE);
    hl(0, ty, 320, GRAY);
    
    /* Start button */
    int sx = 4, sy = ty + 4;
    fr(sx, sy, 50, 16, GRAY);
    hl(sx, sy, 50, WHITE);
    vl(sx, sy, 16, WHITE);
    hl(sx, sy + 15, 50, DGRAY);
    vl(sx + 49, sy, 16, DGRAY);
    dstr(sx + 6, sy + 2, "START", BLACK, GRAY);
}
static void draw_menu(void) {
    int sx = 4, sy = 176 - 44;
    fr(sx, sy, 100, 44, WHITE);
    hl(sx, sy, 100, GRAY);
    vl(sx, sy, 44, GRAY);
    hl(sx, sy + 43, 100, DGRAY);
    vl(sx + 99, sy, 44, DGRAY);
    
    /* Shutdown option */
    dstr(sx + 8, sy + 8, "SHUTDOWN", RED, WHITE);
    /* Restart option */
    dstr(sx + 8, sy + 24, "RESTART", BLACK, WHITE);
}

/* Mouse handling */
static int inr(int mx, int my, int x, int y, int w, int h) {
    return (mx >= x && mx < x + w && my >= y && my < y + h);
}
static void mpoll(void) {
    if (!(inb(0x64) & 0x20)) return;
    uint8_t d = inb(0x60);
    if (mcycle == 0) { if (d & 0x08) { mbytes[0] = d; mcycle = 1; } }
    else { mbytes[mcycle++] = d;
        if (mcycle == 3) { mcycle = 0;
            int dx = mbytes[1], dy = mbytes[2];
            if (mbytes[0] & 0x10) dx |= ~0xFF;
            if (mbytes[0] & 0x20) dy |= ~0xFF;
            dy = -dy; mbtn = mbytes[0] & 0x07;
            mx += dx / 2; my += dy / 2;
            if (mx < 0) mx = 0; if (my < 0) my = 0;
            if (mx >= 320) mx = 319; if (my >= 200) my = 199;
        }
    }
}
static void clicks(void) {
    int clk = (pbtn == 0 && mbtn != 0);
    pbtn = mbtn;
    if (!clk) return;
    
    /* Start button */
    if (inr(mx, my, 4, 180, 50, 16)) {
        menu_open = !menu_open;
        return;
    }
    
    /* Menu - Shutdown */
    if (menu_open && inr(mx, my, 12, 176 - 44 + 8, 84, 16)) {
        fr(0, 0, 320, 200, BLACK);
        dstr(120, 90, "SHUTDOWN", RED, BLACK);
        for (volatile int d = 0; d < 500000; d++);
        outb(0x64, 0xFE);
        while (1) __asm__ volatile("hlt");
    }
    
    /* Menu - Restart */
    if (menu_open && inr(mx, my, 12, 176 - 44 + 24, 84, 16)) {
        fr(0, 0, 320, 200, BLACK);
        dstr(120, 90, "RESTART", GREEN, BLACK);
        for (volatile int d = 0; d < 500000; d++);
        outb(0x64, 0xFE);
    }
    
    /* Click outside menu closes it */
    if (menu_open && !inr(mx, my, 4, 176 - 44, 100, 44)) {
        menu_open = 0;
    }
}

/* VGA Init */
static void init_vga(void) {
    outb(0x3C2, 0x63);
    outb(0x3D4, 0x00); outb(0x3D5, 0x5F);
    outb(0x3D4, 0x01); outb(0x3D5, 0x4F);
    outb(0x3D4, 0x02); outb(0x3D5, 0x50);
    outb(0x3D4, 0x03); outb(0x3D5, 0x82);
    outb(0x3D4, 0x04); outb(0x3D5, 0x54);
    outb(0x3D4, 0x05); outb(0x3D5, 0x80);
    outb(0x3D4, 0x06); outb(0x3D5, 0xBF);
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
}

void kernel_main(uint32_t magic, uint32_t addr) {
    (void)magic; (void)addr;
    
    init_vga();
    
    /* Set palette */
    outb(0x3C8, 0);
    static const uint8_t pal[16][3] = {
        {0x00,0x00,0x00},{0x00,0x00,0x2A},{0x00,0x2A,0x00},{0x00,0x2A,0x2A},
        {0x2A,0x00,0x00},{0x2A,0x00,0x2A},{0x2A,0x15,0x00},{0x2A,0x2A,0x2A},
        {0x15,0x15,0x15},{0x15,0x15,0x3F},{0x15,0x3F,0x15},{0x15,0x3F,0x3F},
        {0x3F,0x15,0x15},{0x3F,0x15,0x3F},{0x3F,0x3F,0x15},{0x3F,0x3F,0x3F},
    };
    for (int i = 0; i < 16; i++) {
        outb(0x3C9, pal[i][0]); outb(0x3C9, pal[i][1]); outb(0x3C9, pal[i][2]);
    }
    
    /* Init mouse */
    outb(0x64, 0xA8);
    for (volatile int i = 0; i < 10000; i++);
    outb(0x64, 0xD4); outb(0x60, 0xFF);
    for (volatile int i = 0; i < 10000; i++);
    while (inb(0x64) & 1) inb(0x60);
    outb(0x64, 0xD4); outb(0x60, 0xF4);
    
    while (1) {
        crest();
        draw_wallpaper();
        draw_taskbar();
        if (menu_open) draw_menu();
        mpoll();
        clicks();
        csave();
        cdraw();
        for (volatile int d = 0; d < 4000; d++);
    }
}
