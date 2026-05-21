#include <stdint.h>

/* ===================================================================
   TFD OS v3.0 "Foxy" — Real VGA Graphical Desktop
   Clickable Menu + Taskbar + Mouse Cursor + Teal Wallpaper
   By Sadman | 2026
   =================================================================== */

/* ----- VESA/VGA Framebuffer ----- */
static uint8_t *fb;
static int pitch, width, height;

/* ----- Mouse State ----- */
static int mouse_x = 320, mouse_y = 240;
static int mouse_btn = 0;
static int mouse_cycle = 0;
static uint8_t mouse_bytes[3];

/* ----- Start Menu State ----- */
static int menu_open = 0;

/* ----- VGA Palette (Bright Colors) ----- */
static const uint8_t palette[256][3] = {
    [0] = {0x00,0x00,0x00},  /* Black */
    [1] = {0x00,0x00,0x3F},  /* Blue */
    [2] = {0x00,0x3F,0x00},  /* Green */
    [3] = {0x00,0x3F,0x3F},  /* Cyan (Teal) */
    [4] = {0x3F,0x00,0x00},  /* Red */
    [5] = {0x3F,0x00,0x3F},  /* Magenta */
    [6] = {0x3F,0x3F,0x00},  /* Yellow */
    [7] = {0x3F,0x3F,0x3F},  /* White */
    [8] = {0x1F,0x1F,0x1F},  /* Dark Gray */
    [9] = {0x1F,0x1F,0x3F},  /* Light Blue */
    [10]= {0x1F,0x3F,0x1F},  /* Light Green */
    [11]= {0x1F,0x3F,0x3F},  /* Light Cyan */
    [12]= {0x3F,0x1F,0x1F},  /* Light Red */
    [13]= {0x3F,0x1F,0x3F},  /* Light Magenta */
    [14]= {0x3F,0x3F,0x1F},  /* Brown */
    [15]= {0x3F,0x3F,0x3F},  /* Bright White */
};

#define TEAL    3
#define WHITE   15
#define GRAY    7
#define DGRAY   8
#define YELLOW  6
#define RED     4
#define BLACK   0
#define LGREEN  10

/* ----- I/O Ports ----- */
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

/* ----- 8x8 Font ----- */
static const uint8_t font[95][8] = {
    [0]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    [1]={0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00},
    [33]={0x38,0x6C,0xC6,0xFE,0xC6,0xC6,0xC6,0x00}, /* A */
    [34]={0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0x00}, /* B */
    [35]={0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00}, /* C */
    [36]={0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0x00}, /* D */
    [37]={0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0x00}, /* E */
    [38]={0xFE,0x62,0x68,0x78,0x68,0x60,0xF0,0x00}, /* F */
    [39]={0x3C,0x66,0xC0,0xDE,0xC6,0x66,0x3A,0x00}, /* G */
    [40]={0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00}, /* H */
    [41]={0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, /* I */
    [44]={0xF0,0x60,0x60,0x60,0x62,0x66,0xFE,0x00}, /* L */
    [45]={0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00}, /* M */
    [47]={0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00}, /* O */
    [50]={0xFC,0x66,0x66,0x7C,0x6C,0x66,0xE6,0x00}, /* R */
    [51]={0x7C,0xC6,0x60,0x38,0x0C,0xC6,0x7C,0x00}, /* S */
    [52]={0x7E,0x5A,0x18,0x18,0x18,0x18,0x3C,0x00}, /* T */
    [83]={0x00,0x00,0x7C,0xC0,0x7C,0x06,0xFC,0x00}, /* s */
    [84]={0x30,0x30,0xFC,0x30,0x30,0x36,0x1C,0x00}, /* t */
    [85]={0x00,0x00,0xC6,0xC6,0xC6,0xC6,0x76,0x00}, /* u */
    [90]={0x00,0x00,0xFE,0x8C,0x18,0x32,0xFE,0x00}, /* z */
};

static void draw_char(int x, int y, char c, uint8_t fg, uint8_t bg) {
    if (c < 32 || c > 126) return;
    const uint8_t *g = font[c - 32];
    for (int row = 0; row < 8; row++) {
        uint8_t bits = g[row];
        for (int col = 0; col < 8; col++) {
            putpixel(x + col, y + row, (bits & (0x80 >> col)) ? fg : bg);
        }
    }
}

static void draw_string(int x, int y, const char *s, uint8_t fg, uint8_t bg) {
    while (*s) { draw_char(x, y, *s++, fg, bg); x += 8; }
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
    /* White arrow cursor */
    static const int arrow[16][16] = {
        {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0},
        {1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0},
        {1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0},
        {1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0},
        {1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0},
        {1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
    };
    for (int dy = 0; dy < 16; dy++)
        for (int dx = 0; dx < 16; dx++)
            if (arrow[dy][dx])
                putpixel(mouse_x + dx, mouse_y + dy, WHITE);
}

/* ----- Desktop Drawing ----- */
static void draw_wallpaper(void) {
    fill_rect(0, 0, width, height - 28, TEAL);
}

static void draw_taskbar(void) {
    fill_rect(0, height - 28, width, 28, GRAY);
    hline(0, height - 28, width, WHITE);
    
    /* Start button */
    fill_rect(4, height - 24, 56, 20, GRAY);
    hline(4, height - 24, 56, WHITE);
    vline(4, height - 24, 20, WHITE);
    hline(4, height - 5, 56, DGRAY);
    vline(60, height - 24, 20, DGRAY);
    draw_string(10, height - 20, "Start", BLACK, GRAY);
    
    /* Clock */
    draw_string(width - 60, height - 20, "12:00", BLACK, GRAY);
}

static void draw_desktop_icons(void) {
    /* Terminal icon */
    fill_rect(30, 30, 32, 32, BLACK);
    fill_rect(34, 34, 24, 24, GRAY);
    hline(34, 34, 24, WHITE);
    vline(34, 34, 24, WHITE);
    draw_string(26, 66, "Terminal", WHITE, TEAL);
    
    /* Notepad icon */
    fill_rect(30, 110, 32, 32, BLACK);
    fill_rect(34, 114, 24, 24, WHITE);
    hline(34, 114, 24, YELLOW);
    hline(34, 122, 24, YELLOW);
    hline(34, 130, 24, YELLOW);
    draw_string(28, 146, "Notepad", WHITE, TEAL);
    
    /* Games icon */
    fill_rect(30, 190, 32, 32, BLACK);
    fill_rect(34, 194, 24, 24, RED);
    draw_string(30, 226, "Games", WHITE, TEAL);
}

static void draw_start_menu(void) {
    int mx = 4, my = height - 28 - 120;
    fill_rect(mx, my, 140, 120, GRAY);
    hline(mx, my, 140, WHITE);
    vline(mx, my, 120, WHITE);
    hline(mx, my + 119, 140, DGRAY);
    vline(mx + 139, my, 120, DGRAY);
    
    draw_string(mx + 8, my + 10, "Terminal", BLACK, GRAY);
    draw_string(mx + 8, my + 34, "Notepad", BLACK, GRAY);
    draw_string(mx + 8, my + 58, "Games", BLACK, GRAY);
    draw_string(mx + 8, my + 82, "Shutdown", RED, GRAY);
}

/* ----- Mouse Polling ----- */
static void mouse_poll(void) {
    static int cycle = 0;
    static uint8_t bytes[3];
    
    if (!(inb(0x64) & 0x20)) return;
    uint8_t d = inb(0x60);
    
    if (cycle == 0) {
        if (d & 0x08) { bytes[0] = d; cycle = 1; }
    } else {
        bytes[cycle++] = d;
        if (cycle == 3) {
            cycle = 0;
            int dx = bytes[1], dy = bytes[2];
            if (bytes[0] & 0x10) dx |= ~0xFF;
            if (bytes[0] & 0x20) dy |= ~0xFF;
            dy = -dy;
            mouse_btn = bytes[0] & 0x07;
            mouse_x += dx / 2;
            mouse_y += dy / 2;
            if (mouse_x < 0) mouse_x = 0;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_x >= width) mouse_x = width - 1;
            if (mouse_y >= height) mouse_y = height - 1;
        }
    }
}

/* ----- Click Detection ----- */
static int in_rect(int mx, int my, int x, int y, int w, int h) {
    return (mx >= x && mx < x + w && my >= y && my < y + h);
}

static void handle_clicks(void) {
    static int prev_btn = 0;
    int clicked = (prev_btn == 0 && mouse_btn != 0);
    prev_btn = mouse_btn;
    
    if (!clicked) return;
    
    /* Start button */
    if (in_rect(mouse_x, mouse_y, 4, height - 24, 56, 20)) {
        menu_open = !menu_open;
    }
    
    /* Start menu items */
    if (menu_open) {
        int mx = 4, my = height - 28 - 120;
        if (in_rect(mouse_x, mouse_y, mx + 8, my + 10, 124, 20)) {
            menu_open = 0;
        }
        if (in_rect(mouse_x, mouse_y, mx + 8, my + 58, 124, 20)) {
            menu_open = 0;
        }
        if (in_rect(mouse_x, mouse_y, mx + 8, my + 82, 124, 20)) {
            menu_open = 0;
            /* Shutdown - just halt for now */
            while (1) __asm__ volatile("hlt");
        }
    }
    
    /* Desktop icons */
    if (in_rect(mouse_x, mouse_y, 30, 30, 32, 50)) {
        menu_open = 0;
    }
    if (in_rect(mouse_x, mouse_y, 30, 110, 32, 50)) {
        menu_open = 0;
    }
    if (in_rect(mouse_x, mouse_y, 30, 190, 32, 50)) {
        menu_open = 0;
    }
}

/* ----- VGA Mode 13h Fallback ----- */
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
    outb(0x3D4, 0x0A); outb(0x3D5, 0x00);
    outb(0x3D4, 0x0B); outb(0x3D5, 0x00);
    outb(0x3D4, 0x0C); outb(0x3D5, 0x00);
    outb(0x3D4, 0x0D); outb(0x3D5, 0x00);
    outb(0x3D4, 0x0E); outb(0x3D5, 0x00);
    outb(0x3D4, 0x0F); outb(0x3D5, 0x00);
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
    outb(0x3C4, 0x03); outb(0x3C5, 0x00);
    outb(0x3C4, 0x04); outb(0x3C5, 0x0E);
    outb(0x3CE, 0x00); outb(0x3CF, 0x00);
    outb(0x3CE, 0x01); outb(0x3CF, 0x00);
    outb(0x3CE, 0x02); outb(0x3CF, 0x00);
    outb(0x3CE, 0x03); outb(0x3CF, 0x00);
    outb(0x3CE, 0x04); outb(0x3CF, 0x00);
    outb(0x3CE, 0x05); outb(0x3CF, 0x40);
    outb(0x3CE, 0x06); outb(0x3CF, 0x05);
    outb(0x3CE, 0x07); outb(0x3CF, 0x0F);
    outb(0x3CE, 0x08); outb(0x3CF, 0xFF);
    inb(0x3DA);
    outb(0x3C0, 0x30); outb(0x3C0, 0x41);
    outb(0x3C0, 0x33); outb(0x3C0, 0x00);
    outb(0x3C0, 0x20);
    fb = (uint8_t*)0xA0000;
    pitch = 320; width = 320; height = 200;
}

static void set_palette(void) {
    outb(0x3C8, 0);
    for (int i = 0; i < 16; i++) {
        outb(0x3C9, palette[i][0]);
        outb(0x3C9, palette[i][1]);
        outb(0x3C9, palette[i][2]);
    }
    for (int i = 16; i < 256; i++) {
        outb(0x3C9, i & 0x3F);
        outb(0x3C9, (i >> 2) & 0x3F);
        outb(0x3C9, 63 - (i & 0x3F));
    }
}

/* ----- Mouse Init ----- */
static void mouse_init(void) {
    outb(0x64, 0xA8);
    for (volatile int i = 0; i < 10000; i++);
    outb(0x64, 0xD4); outb(0x60, 0xFF);
    for (volatile int i = 0; i < 10000; i++);
    while (inb(0x64) & 1) inb(0x60);
    outb(0x64, 0xD4); outb(0x60, 0xF4);
}

/* ----- Main ----- */
void kernel_main(uint32_t magic, uint32_t addr) {
    uint32_t *mbi = (uint32_t *)addr;
    uint32_t flags = *mbi;
    fb = 0;

    /* Try VESA */
    if ((flags & (1 << 11)) && *(mbi + 22) != 0) {
        fb = (uint8_t *)(uint32_t)*(mbi + 22);
        pitch = *(mbi + 24);
        width = *(mbi + 25);
        height = *(mbi + 26);
        if (width == 0 || height == 0) fb = 0;
    }

    /* Fallback */
    if (!fb) set_vga_mode13();

    set_palette();
    mouse_init();

    /* Main GUI loop */
    while (1) {
        cursor_restore();
        
        draw_wallpaper();
        draw_desktop_icons();
        draw_taskbar();
        if (menu_open) draw_start_menu();
        
        mouse_poll();
        handle_clicks();
        
        cursor_save();
        cursor_draw();
        
        for (volatile int d = 0; d < 10000; d++);
    }
}
