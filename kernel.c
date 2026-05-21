#include <stdint.h>

/* ===================================================================
   TFD OS v3.0 — REAL VGA GRAPHICS DESKTOP
   Forces Mode 13h directly — NO text mode fallback
   Guaranteed real pixels — same method as RGB test
   =================================================================== */

static uint8_t *fb;
static int pitch = 320, width = 320, height = 200;

/* Mouse */
static int mx = 160, my = 100;
static int mbtn = 0, pbtn = 0;
static int mcycle = 0;
static uint8_t mbytes[3];

/* Menu */
static int menu = 0;

/* Colors */
#define BLACK   0x00
#define BLUE    0x01
#define GREEN   0x02
#define TEAL    0x03
#define RED     0x04
#define YELLOW  0x06
#define GRAY    0x07
#define DGRAY   0x08
#define WHITE   0x0F

/* I/O */
static inline void outb(uint16_t p, uint8_t v) { __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p) { uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }
static inline void iowait(void) { outb(0x80,0); }

/* ==================== REAL VGA MODE 13h ==================== */
static void force_vga_mode13(void) {
    /* FULL register sequence — every register that the RGB test used */
    outb(0x3C2, 0x63);           /* Miscellaneous Output Register */
    
    /* Sequencer registers */
    outb(0x3C4, 0x00); outb(0x3C5, 0x03);  /* Reset */
    outb(0x3C4, 0x01); outb(0x3C5, 0x01);  /* Clocking Mode */
    outb(0x3C4, 0x02); outb(0x3C5, 0x0F);  /* Map Mask */
    outb(0x3C4, 0x03); outb(0x3C5, 0x00);  /* Character Map Select */
    outb(0x3C4, 0x04); outb(0x3C5, 0x0E);  /* Memory Mode */
    
    /* CRTC registers */
    outb(0x3D4, 0x00); outb(0x3D5, 0x5F);  /* Horizontal Total */
    outb(0x3D4, 0x01); outb(0x3D5, 0x4F);  /* Horizontal Display End */
    outb(0x3D4, 0x02); outb(0x3D5, 0x50);  /* Start Horizontal Blank */
    outb(0x3D4, 0x03); outb(0x3D5, 0x82);  /* End Horizontal Blank */
    outb(0x3D4, 0x04); outb(0x3D5, 0x54);  /* Start Horizontal Retrace */
    outb(0x3D4, 0x05); outb(0x3D5, 0x80);  /* End Horizontal Retrace */
    outb(0x3D4, 0x06); outb(0x3D5, 0xBF);  /* Vertical Total */
    outb(0x3D4, 0x07); outb(0x3D5, 0x00);  /* Overflow */
    outb(0x3D4, 0x08); outb(0x3D5, 0x00);  /* Preset Row Scan */
    outb(0x3D4, 0x09); outb(0x3D5, 0x41);  /* Maximum Scan Line */
    outb(0x3D4, 0x0A); outb(0x3D5, 0x00);  /* Cursor Start */
    outb(0x3D4, 0x0B); outb(0x3D5, 0x00);  /* Cursor End */
    outb(0x3D4, 0x0C); outb(0x3D5, 0x00);  /* Start Address High */
    outb(0x3D4, 0x0D); outb(0x3D5, 0x00);  /* Start Address Low */
    outb(0x3D4, 0x0E); outb(0x3D5, 0x00);  /* Cursor Location High */
    outb(0x3D4, 0x0F); outb(0x3D5, 0x00);  /* Cursor Location Low */
    outb(0x3D4, 0x10); outb(0x3D5, 0x9C);  /* Vertical Retrace Start */
    outb(0x3D4, 0x11); outb(0x3D5, 0x8E);  /* Vertical Retrace End */
    outb(0x3D4, 0x12); outb(0x3D5, 0x8F);  /* Vertical Display End */
    outb(0x3D4, 0x13); outb(0x3D5, 0x28);  /* Offset */
    outb(0x3D4, 0x14); outb(0x3D5, 0x40);  /* Underline Location */
    outb(0x3D4, 0x15); outb(0x3D5, 0x96);  /* Start Vertical Blank */
    outb(0x3D4, 0x16); outb(0x3D5, 0xB9);  /* End Vertical Blank */
    outb(0x3D4, 0x17); outb(0x3D5, 0xA3);  /* Mode Control */
    
    /* Graphics Controller registers */
    outb(0x3CE, 0x00); outb(0x3CF, 0x00);  /* Set/Reset */
    outb(0x3CE, 0x01); outb(0x3CF, 0x00);  /* Enable Set/Reset */
    outb(0x3CE, 0x02); outb(0x3CF, 0x00);  /* Color Compare */
    outb(0x3CE, 0x03); outb(0x3CF, 0x00);  /* Data Rotate */
    outb(0x3CE, 0x04); outb(0x3CF, 0x00);  /* Read Map Select */
    outb(0x3CE, 0x05); outb(0x3CF, 0x40);  /* Graphics Mode */
    outb(0x3CE, 0x06); outb(0x3CF, 0x05);  /* Miscellaneous */
    outb(0x3CE, 0x07); outb(0x3CF, 0x0F);  /* Color Don't Care */
    outb(0x3CE, 0x08); outb(0x3CF, 0xFF);  /* Bit Mask */
    
    /* Attribute Controller registers */
    inb(0x3DA);                              /* Reset flip-flop */
    outb(0x3C0, 0x30); outb(0x3C0, 0x41);   /* Palette Enable */
    outb(0x3C0, 0x33); outb(0x3C0, 0x00);   /* Pixel Panning */
    outb(0x3C0, 0x20);                       /* Enable Video */
    
    fb = (uint8_t*)0xA0000;
    pitch = 320;
    width = 320;
    height = 200;
}

/* ==================== PIXEL DRAWING ==================== */
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

/* ==================== FONT ==================== */
static void dchar(int x, int y, char c, uint8_t fg, uint8_t bg) {
    static const uint8_t glyphs[26][8] = {
        [0]={0x38,0x6C,0xC6,0xFE,0xC6,0xC6,0xC6,0x00},
        [1]={0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0x00},
        [2]={0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00},
        [3]={0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0x00},
        [4]={0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0x00},
        [5]={0xFE,0x62,0x68,0x78,0x68,0x60,0xF0,0x00},
        [6]={0x3C,0x66,0xC0,0xDE,0xC6,0x66,0x3A,0x00},
        [7]={0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00},
        [8]={0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
        [11]={0xF0,0x60,0x60,0x60,0x62,0x66,0xFE,0x00},
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

/* ==================== MOUSE CURSOR ==================== */
static uint8_t cbg[16*16];
static void csave(void) {
    int idx = 0;
    for (int dy = 0; dy < 16; dy++)
        for (int dx = 0; dx < 16; dx++) {
            int px = mx + dx, py = my + dy;
            cbg[idx++] = (px >= 0 && px < width && py >= 0 && py < height) ? fb[py * pitch + px] : 0;
        }
}
static void crest(void) {
    int idx = 0;
    for (int dy = 0; dy < 16; dy++)
        for (int dx = 0; dx < 16; dx++) {
            int px = mx + dx, py = my + dy;
            if (px >= 0 && px < width && py >= 0 && py < height) fb[py * pitch + px] = cbg[idx];
            idx++;
        }
}
static void cdraw(void) {
    static const int arrow[10][16] = {
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},{1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0},{1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0},{1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0},
        {1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0},
    };
    for (int dy = 0; dy < 10; dy++)
        for (int dx = 0; dx < 16; dx++)
            if (arrow[dy][dx]) pp(mx + dx, my + dy, WHITE);
}

/* ==================== DESKTOP ==================== */
static void desktop(void) {
    fr(0, 0, width, height - 24, TEAL);
    /* Shutdown icon */
    fr(20, 20, 32, 32, BLACK);
    fr(24, 24, 24, 24, RED);
    dstr(12, 56, "SHUTDOWN", WHITE, TEAL);
}
static void taskbar(void) {
    fr(0, height - 24, width, 24, GRAY);
    hl(0, height - 24, width, WHITE);
    fr(2, height - 22, 50, 20, GRAY);
    hl(2, height - 22, 50, WHITE);
    vl(2, height - 22, 20, WHITE);
    hl(2, height - 3, 50, DGRAY);
    vl(52, height - 22, 20, DGRAY);
    dstr(8, height - 18, "START", BLACK, GRAY);
    dstr(width - 50, height - 18, "12:00", BLACK, GRAY);
}
static void startmenu(void) {
    int sx = 2, sy = height - 24 - 40;
    fr(sx, sy, 100, 40, GRAY);
    hl(sx, sy, 100, WHITE);
    vl(sx, sy, 40, WHITE);
    hl(sx, sy + 39, 100, DGRAY);
    vl(sx + 99, sy, 40, DGRAY);
    dstr(sx + 8, sy + 8, "SHUTDOWN", RED, GRAY);
}

/* ==================== MOUSE HANDLING ==================== */
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
    if (inr(mx, my, 2, height - 22, 50, 20)) menu = !menu;
    if (menu && inr(mx, my, 10, height - 24 - 40 + 8, 84, 16)) {
        fr(0, 0, width, height, BLACK);
        dstr(width/2 - 40, height/2, "SHUTDOWN", RED, BLACK);
        for (volatile int d = 0; d < 500000; d++);
        outb(0x64, 0xFE);
        while (1) __asm__ volatile("hlt");
    }
    if (inr(mx, my, 20, 20, 32, 32)) {
        fr(0, 0, width, height, BLACK);
        dstr(width/2 - 40, height/2, "SHUTDOWN", RED, BLACK);
        for (volatile int d = 0; d < 500000; d++);
        outb(0x64, 0xFE);
        while (1) __asm__ volatile("hlt");
    }
}

/* ==================== MAIN ==================== */
void kernel_main(uint32_t magic, uint32_t addr) {
    (void)magic; (void)addr;
    
    /* FORCE VGA Mode 13h — no VESA, no text mode fallback */
    force_vga_mode13();
    
    /* Set bright palette */
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
        desktop();
        taskbar();
        if (menu) startmenu();
        mpoll();
        clicks();
        csave();
        cdraw();
        for (volatile int d = 0; d < 8000; d++);
    }
}
