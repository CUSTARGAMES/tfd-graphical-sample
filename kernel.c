#include <stdint.h>

/* ===================================================================
   TFD OS v3.0 "Foxy" — Professional VGA Desktop
   640×480 VESA + 320×200 Fallback
   Teal Wallpaper + Taskbar + Shutdown Button + Mouse Cursor
   By Sadman | 2026
   =================================================================== */

static uint8_t *fb;
static int pitch, width, height;

/* Mouse */
static int mx = 320, my = 240;
static int mbtn = 0, pbtn = 0;
static int mcycle = 0;
static uint8_t mbytes[3];

/* Colors */
#define BLACK   0x00
#define TEAL    0x03
#define RED     0x04
#define GRAY    0x07
#define DGRAY   0x08
#define WHITE   0x0F
#define GREEN   0x02
#define YELLOW  0x0E

/* I/O */
static inline void outb(uint16_t p, uint8_t v) { __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p) { uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }
static inline void iowait(void) { outb(0x80,0); }

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

/* 8x8 Font */
static void dchar(int x, int y, char c, uint8_t fg, uint8_t bg) {
    static const uint8_t glyphs[26][8] = {
        [0]={0x38,0x6C,0xC6,0xFE,0xC6,0xC6,0xC6,0x00},
        [2]={0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00},
        [3]={0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0x00},
        [4]={0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0x00},
        [7]={0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00},
        [8]={0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
        [11]={0xF0,0x60,0x60,0x60,0x62,0x66,0xFE,0x00},
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

/* Windows-style Mouse Cursor */
static uint8_t cbg[20*20];
static void csave(void) {
    int idx = 0;
    for (int dy = 0; dy < 20; dy++)
        for (int dx = 0; dx < 20; dx++) {
            int px = mx + dx, py = my + dy;
            cbg[idx++] = (px >= 0 && px < width && py >= 0 && py < height) ? fb[py * pitch + px] : 0;
        }
}
static void crest(void) {
    int idx = 0;
    for (int dy = 0; dy < 20; dy++)
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
        {1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
    };
    for (int dy = 0; dy < 12; dy++)
        for (int dx = 0; dx < 20; dx++)
            if (arrow[dy][dx]) pp(mx + dx, my + dy, WHITE);
}

/* Desktop */
static void draw_desktop(void) {
    fr(0, 0, width, height - 28, TEAL);
}
static void draw_taskbar(void) {
    int ty = height - 28;
    fr(0, ty, width, 28, GRAY);
    hl(0, ty, width, WHITE);
    
    /* Shutdown button IN the taskbar (right side) */
    int bx = width - 90, by = ty + 4;
    fr(bx, by, 80, 20, RED);
    hl(bx, by, 80, WHITE);
    vl(bx, by, 20, WHITE);
    hl(bx, by + 19, 80, DGRAY);
    vl(bx + 79, by, 20, DGRAY);
    dstr(bx + 8, by + 4, "SHUTDOWN", WHITE, RED);
    
    /* Clock */
    dstr(width - 180, ty + 6, "12:00", BLACK, GRAY);
}

/* Mouse */
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
            if (mx >= width) mx = width - 1; if (my >= height) my = height - 1;
        }
    }
}
static void clicks(void) {
    int clk = (pbtn == 0 && mbtn != 0);
    pbtn = mbtn;
    if (!clk) return;
    
    /* Shutdown button in taskbar */
    int bx = width - 90, by = height - 24;
    if (inr(mx, my, bx, by, 80, 20)) {
        fr(0, 0, width, height, BLACK);
        dstr(width/2 - 40, height/2, "SHUTDOWN", RED, BLACK);
        for (volatile int d = 0; d < 500000; d++);
        outb(0x64, 0xFE);
        outb(0x604, 0x2000);
        outb(0xB004, 0x2000);
        while (1) __asm__ volatile("hlt");
    }
}

/* VGA Fallback */
static void vga13(void) {
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
    pitch = 320; width = 320; height = 200;
}

/* Main */
void kernel_main(uint32_t magic, uint32_t addr) {
    uint32_t *mbi = (uint32_t *)addr;
    fb = 0;

    /* Try GRUB VESA first */
    if ((*mbi & (1 << 11)) && *(mbi + 22)) {
        fb = (uint8_t*)(uint32_t)*(mbi + 22);
        pitch = *(mbi + 24); width = *(mbi + 25); height = *(mbi + 26);
        if (!width || !height) fb = 0;
    }
    if (!fb) vga13();

    /* Bright palette */
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

    /* Init mouse */
    outb(0x64, 0xA8);
    for (volatile int i = 0; i < 10000; i++);
    outb(0x64, 0xD4); outb(0x60, 0xFF);
    for (volatile int i = 0; i < 10000; i++);
    while (inb(0x64) & 1) inb(0x60);
    outb(0x64, 0xD4); outb(0x60, 0xF4);

    while (1) {
        crest();
        draw_desktop();
        draw_taskbar();
        mpoll();
        clicks();
        csave();
        cdraw();
        for (volatile int d = 0; d < 5000; d++);
    }
}
