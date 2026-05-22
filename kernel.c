#include <stdint.h>

static uint8_t *fb;
static int pitch = 320, width = 320, height = 200;

/* Mouse */
static int mx = 100, my = 100;
static int omx = 100, omy = 100;  /* Old position */
static int mbtn = 0, pbtn = 0;
static int mcycle = 0;
static uint8_t mbytes[3];

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

/* Simple cursor - draw at position */
static void draw_cursor(int cx, int cy, uint8_t color) {
    static const int shape[6][8] = {
        {1,0,0,0,0,0,0,0},
        {1,1,0,0,0,0,0,0},
        {1,1,1,0,0,0,0,0},
        {1,1,1,1,0,0,0,0},
        {0,1,0,0,0,0,0,0},
        {0,0,1,1,0,0,0,0},
    };
    for(int dy=0;dy<6;dy++)
        for(int dx=0;dx<8;dx++)
            if(shape[dy][dx])
                pp(cx+dx, cy+dy, color);
}

/* Clear old cursor by redrawing background */
static void clear_cursor(int cx, int cy) {
    /* Redraw teal background behind cursor */
    for(int dy=0;dy<6;dy++)
        for(int dx=0;dx<8;dx++)
            pp(cx+dx, cy+dy, TEAL);
}

/* Mouse polling */
static void mouse_poll(void) {
    while(inb(0x64) & 1) {
        uint8_t st = inb(0x64);
        uint8_t d = inb(0x60);
        if(!(st & 0x20)) continue;
        
        if(mcycle == 0) {
            if(d & 0x08) { mbytes[0] = d; mcycle = 1; }
        } else {
            mbytes[mcycle++] = d;
            if(mcycle == 3) {
                mcycle = 0;
                int dx = mbytes[1], dy = mbytes[2];
                if(mbytes[0] & 0x10) dx |= ~0xFF;
                if(mbytes[0] & 0x20) dy |= ~0xFF;
                dy = -dy;
                mbtn = mbytes[0] & 0x07;
                
                /* Update position only if moved */
                if(dx != 0 || dy != 0) {
                    omx = mx;
                    omy = my;
                    mx += dx / 2;
                    my += dy / 2;
                    if(mx < 0) mx = 0;
                    if(my < 0) my = 0;
                    if(mx >= 312) mx = 312;
                    if(my >= 194) my = 194;
                }
            }
        }
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
    for(int i = 0; i < 16; i++) {
        outb(0x3C9, pal[i][0]); outb(0x3C9, pal[i][1]); outb(0x3C9, pal[i][2]);
    }
    
    /* DRAW DESKTOP ONCE */
    fr(0, 0, 320, 176, TEAL);   /* Teal wallpaper */
    fr(0, 176, 320, 24, WHITE);  /* White taskbar */
    fr(230, 180, 80, 16, RED);   /* Shutdown button */
    
    /* Init mouse */
    outb(0x64, 0xA8);
    for(volatile int i = 0; i < 100000; i++);
    outb(0x64, 0xD4); outb(0x60, 0xFF);
    for(volatile int i = 0; i < 100000; i++);
    while(inb(0x64) & 1) inb(0x60);
    outb(0x64, 0xD4); outb(0x60, 0xF4);
    
    /* Draw initial cursor */
    draw_cursor(mx, my, BLACK);
    
    while(1) {
        mouse_poll();
        
        /* Only update if mouse moved */
        if(mx != omx || my != omy) {
            clear_cursor(omx, omy);      /* Erase old cursor */
            draw_cursor(mx, my, BLACK);   /* Draw new cursor */
            omx = mx; omy = my;
        }
        
        /* Check shutdown button click */
        if(pbtn == 0 && (mbtn & 1)) {
            if(mx >= 230 && mx < 310 && my >= 180 && my < 196) {
                fr(0, 0, 320, 200, BLACK);
                for(volatile int d = 0; d < 500000; d++);
                outb(0x64, 0xFE);
                while(1) __asm__ volatile("hlt");
            }
        }
        pbtn = mbtn;
        
        for(volatile int d = 0; d < 2000; d++);
    }
}
