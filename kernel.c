#include <stdint.h>

static uint8_t *fb;

/* Mouse */
static int mx = 100, my = 100;
static int mbtn = 0, pbtn = 0;
static int mcycle = 0;
static uint8_t mbytes[3];

/* Cursor save buffer */
static uint8_t cursor_save[6][8];
static int cursor_drawn = 0;

/* Window */
static int win_x = 50, win_y = 30;
static int win_w = 200, win_h = 100;
static int dragging = 0;
static int drag_ox = 0, drag_oy = 0;

/* Menu */
static int menu_open = 0;

/* Colors */
#define BLACK   0x00
#define TEAL    0x03
#define RED     0x04
#define GRAY    0x07
#define WHITE   0x0F
#define DGRAY   0x08
#define BLUE    0x01

/* I/O */
static inline void outb(uint16_t p, uint8_t v) { __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p) { uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }

/* Pixel */
static void pp(int x, int y, uint8_t c) { if(x>=0&&x<320&&y>=0&&y<200)fb[y*320+x]=c; }
static void fr(int x,int y,int w,int h,uint8_t c){for(int dy=0;dy<h;dy++)for(int dx=0;dx<w;dx++)pp(x+dx,y+dy,c);}
static void hl(int x,int y,int w,uint8_t c){for(int i=0;i<w;i++)pp(x+i,y,c);}
static void vl(int x,int y,int h,uint8_t c){for(int i=0;i<h;i++)pp(x,y+i,c);}

/* Cursor - PROPER save/restore */
static void cursor_save_background(void) {
    for(int dy = 0; dy < 6; dy++)
        for(int dx = 0; dx < 8; dx++) {
            int px = mx + dx, py = my + dy;
            if(px >= 0 && px < 320 && py >= 0 && py < 200)
                cursor_save[dy][dx] = fb[py * 320 + px];
        }
    cursor_drawn = 1;
}

static void cursor_restore_background(void) {
    if(!cursor_drawn) return;
    for(int dy = 0; dy < 6; dy++)
        for(int dx = 0; dx < 8; dx++) {
            int px = mx + dx, py = my + dy;
            if(px >= 0 && px < 320 && py >= 0 && py < 200)
                fb[py * 320 + px] = cursor_save[dy][dx];
        }
    cursor_drawn = 0;
}

static void cursor_draw(void) {
    cursor_save_background();
    static const int shape[6][8] = {
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
    for(int dy = 0; dy < 6; dy++)
        for(int dx = 0; dx < 8; dx++)
            if(shape[dy][dx])
                pp(mx + dx, my + dy, BLACK);
}

/* Desktop */
static void draw_desktop(void) {
    fr(0, 0, 320, 176, TEAL);
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
    
    /* Shutdown button */
    int bx = 230, by = ty + 4;
    fr(bx, by, 80, 16, RED);
    hl(bx, by, 80, WHITE);
    hl(bx, by + 15, 80, DGRAY);
}

static void draw_window(void) {
    /* Window body */
    fr(win_x, win_y, win_w, win_h, WHITE);
    /* Title bar */
    fr(win_x, win_y, win_w, 14, BLUE);
    /* Border */
    hl(win_x, win_y, win_w, BLACK);
    hl(win_x, win_y + win_h - 1, win_w, BLACK);
    vl(win_x, win_y, win_h, BLACK);
    vl(win_x + win_w - 1, win_y, win_h, BLACK);
}

static void draw_menu(void) {
    int sx = 4, sy = 132;
    fr(sx, sy, 80, 28, WHITE);
    hl(sx, sy, 80, GRAY);
    vl(sx, sy, 28, GRAY);
    hl(sx, sy + 27, 80, DGRAY);
    vl(sx + 79, sy, 28, DGRAY);
    fr(sx + 4, sy + 4, 72, 20, RED);
}

/* Mouse */
static int inr(int mx, int my, int x, int y, int w, int h) {
    return (mx >= x && mx < x + w && my >= y && my < y + h);
}

static void mpoll(void) {
    while(inb(0x64) & 1) {
        uint8_t st = inb(0x64), d = inb(0x60);
        if(!(st & 0x20)) continue;
        if(mcycle == 0) { if(d & 0x08) { mbytes[0] = d; mcycle = 1; } }
        else { mbytes[mcycle++] = d;
            if(mcycle == 3) { mcycle = 0;
                int dx = mbytes[1], dy = mbytes[2];
                if(mbytes[0] & 0x10) dx |= ~0xFF;
                if(mbytes[0] & 0x20) dy |= ~0xFF;
                dy = -dy; mbtn = mbytes[0] & 0x07;
                if(dx != 0 || dy != 0) {
                    cursor_restore_background();
                    mx += dx / 2; my += dy / 2;
                    if(mx < 0) mx = 0; if(my < 0) my = 0;
                    if(mx >= 312) mx = 312; if(my >= 194) my = 194;
                    cursor_draw();
                }
            }
        }
    }
}

static void redraw_all(void) {
    draw_desktop();
    draw_window();
    draw_taskbar();
    if(menu_open) draw_menu();
    cursor_draw();
}

/* VGA */
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
    for(int i = 0; i < 16; i++) {
        outb(0x3C9, pal[i][0]); outb(0x3C9, pal[i][1]); outb(0x3C9, pal[i][2]);
    }
    
    /* Init mouse */
    outb(0x64, 0xA8);
    for(volatile int i = 0; i < 100000; i++);
    outb(0x64, 0xD4); outb(0x60, 0xFF);
    for(volatile int i = 0; i < 100000; i++);
    while(inb(0x64) & 1) inb(0x60);
    outb(0x64, 0xD4); outb(0x60, 0xF4);
    
    /* Draw everything once */
    redraw_all();
    
    while(1) {
        mpoll();
        
        /* Handle clicks */
        int click = (pbtn == 0 && (mbtn & 1));
        pbtn = mbtn;
        
        if(click) {
            /* Start button */
            if(inr(mx, my, 4, 180, 50, 16)) {
                menu_open = !menu_open;
                cursor_restore_background();
                if(menu_open) draw_menu();
                else { draw_taskbar(); }
                cursor_draw();
            }
            /* Menu shutdown */
            if(menu_open && inr(mx, my, 8, 136, 72, 20)) {
                fr(0, 0, 320, 200, BLACK);
                for(volatile int d = 0; d < 500000; d++);
                outb(0x64, 0xFE);
                while(1) __asm__ volatile("hlt");
            }
            /* Close menu by clicking elsewhere */
            if(menu_open && !inr(mx, my, 4, 132, 80, 28)) {
                menu_open = 0;
                cursor_restore_background();
                draw_taskbar();
                cursor_draw();
            }
        }
        
        /* Window dragging */
        if(mbtn & 1) {
            if(dragging) {
                cursor_restore_background();
                win_x = mx - drag_ox;
                win_y = my - drag_oy;
                if(win_x < 0) win_x = 0;
                if(win_y < 0) win_y = 0;
                if(win_x + win_w > 320) win_x = 320 - win_w;
                if(win_y + win_h > 200) win_y = 200 - win_h;
                redraw_all();
            } else if(click && inr(mx, my, win_x, win_y, win_w, 14)) {
                dragging = 1;
                drag_ox = mx - win_x;
                drag_oy = my - win_y;
            }
        } else {
            dragging = 0;
        }
        
        for(volatile int d = 0; d < 2000; d++);
    }
}
