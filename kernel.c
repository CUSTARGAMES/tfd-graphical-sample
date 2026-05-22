#include <stdint.h>

static uint8_t *fb;

/* Mouse */
static int mx = 100, my = 100;
static int mbtn = 0, pbtn = 0;
static int mcycle = 0;
static uint8_t mbytes[3];

/* Cursor save buffer */
static uint8_t cursor_save[12][20];
static int cursor_drawn = 0;

/* Window */
static int win_x = 40, win_y = 30;
static int win_w = 240, win_h = 140;
static int dragging = 0;
static int drag_ox = 0, drag_oy = 0;

/* Menu */
static int menu_open = 0;

/* Colors */
#define BLACK   0x00
#define BLUE    0x01
#define GREEN   0x02
#define TEAL    0x03
#define RED     0x04
#define MAGENTA 0x05
#define BROWN   0x06
#define GRAY    0x07
#define DGRAY   0x08
#define LBLUE   0x09
#define LGREEN  0x0A
#define LCYAN   0x0B
#define LRED    0x0C
#define LMAGENTA 0x0D
#define YELLOW  0x0E
#define WHITE   0x0F

/* I/O */
static inline void outb(uint16_t p, uint8_t v) { __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p) { uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }

/* Pixel */
static void pp(int x, int y, uint8_t c) { if(x>=0&&x<320&&y>=0&&y<200)fb[y*320+x]=c; }
static void fr(int x,int y,int w,int h,uint8_t c){for(int dy=0;dy<h;dy++)for(int dx=0;dx<w;dx++)pp(x+dx,y+dy,c);}
static void hl(int x,int y,int w,uint8_t c){for(int i=0;i<w;i++)pp(x+i,y,c);}
static void vl(int x,int y,int h,uint8_t c){for(int i=0;i<h;i++)pp(x,y+i,c);}

/* Cursor */
static void cursor_save_bg(void) {
    for(int dy=0;dy<12;dy++)for(int dx=0;dx<20;dx++){int px=mx+dx,py=my+dy;if(px>=0&&px<320&&py>=0&&py<200)cursor_save[dy][dx]=fb[py*320+px];}
    cursor_drawn=1;
}
static void cursor_restore_bg(void) {
    if(!cursor_drawn)return;
    for(int dy=0;dy<12;dy++)for(int dx=0;dx<20;dx++){int px=mx+dx,py=my+dy;if(px>=0&&px<320&&py>=0&&py<200)fb[py*320+px]=cursor_save[dy][dx];}
    cursor_drawn=0;
}
static void cursor_draw(void) {
    cursor_save_bg();
    static const int shape[12][20]={
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
    for(int dy=0;dy<12;dy++)for(int dx=0;dx<20;dx++)if(shape[dy][dx])pp(mx+dx,my+dy,BLACK);
}

/* ==================== WALLPAPER ==================== */
static void draw_wallpaper(void) {
    /* Teal background */
    fr(0,0,320,176,TEAL);
    
    /* Sun in the middle */
    int sx=160,sy=88,sr=30;
    for(int dy=-sr;dy<=sr;dy++)
        for(int dx=-sr;dx<=sr;dx++)
            if(dx*dx+dy*dy<=sr*sr)
                pp(sx+dx,sy+dy,YELLOW);
    
    /* "TFD" text inside sun (simple pixels) */
    /* T */
    for(int i=0;i<12;i++)pp(148+i,80,BLACK);
    for(int i=0;i<6;i++){pp(152,78+i,BLACK);pp(153,78+i,BLACK);}
    /* F */
    for(int i=0;i<6;i++){pp(160,78+i,BLACK);pp(161,78+i,BLACK);}
    for(int i=0;i<8;i++)pp(160+i,78,BLACK);
    for(int i=0;i<3;i++){pp(160+i,81,BLACK);pp(160+i,82,BLACK);}
    /* D */
    for(int i=0;i<6;i++){pp(170,78+i,BLACK);pp(171,78+i,BLACK);}
    for(int i=0;i<6;i++){pp(176,78+i,BLACK);pp(177,78+i,BLACK);}
    for(int i=0;i<6;i++)pp(172+i,78,BLACK);
    for(int i=0;i<6;i++)pp(172+i,83,BLACK);
}

/* ==================== TASKBAR ==================== */
static void draw_taskbar(void) {
    int ty=176;
    fr(0,ty,320,24,GRAY);
    hl(0,ty,320,WHITE);
    
    /* Start button - GREEN */
    int sx=4,sy=ty+4;
    fr(sx,sy,56,16,GREEN);
    hl(sx,sy,56,WHITE);
    vl(sx,sy,16,WHITE);
    hl(sx,sy+15,56,DGRAY);
    vl(sx+55,sy,16,DGRAY);
    
    /* "Menu" text in button */
    /* M */
    pp(sx+6,sy+3,WHITE);pp(sx+7,sy+3,WHITE);pp(sx+10,sy+3,WHITE);pp(sx+11,sy+3,WHITE);
    pp(sx+6,sy+4,WHITE);pp(sx+8,sy+4,WHITE);pp(sx+10,sy+4,WHITE);pp(sx+12,sy+4,WHITE);
    /* E */
    pp(sx+16,sy+3,WHITE);pp(sx+18,sy+3,WHITE);pp(sx+20,sy+3,WHITE);
    pp(sx+16,sy+4,WHITE);pp(sx+16,sy+5,WHITE);pp(sx+18,sy+5,WHITE);
    /* N */
    pp(sx+24,sy+3,WHITE);pp(sx+25,sy+3,WHITE);pp(sx+28,sy+3,WHITE);pp(sx+29,sy+3,WHITE);
    pp(sx+24,sy+4,WHITE);pp(sx+26,sy+4,WHITE);pp(sx+28,sy+4,WHITE);pp(sx+30,sy+4,WHITE);
    /* U */
    pp(sx+34,sy+3,WHITE);pp(sx+35,sy+3,WHITE);pp(sx+38,sy+3,WHITE);pp(sx+39,sy+3,WHITE);
    pp(sx+34,sy+4,WHITE);pp(sx+38,sy+4,WHITE);
    pp(sx+34,sy+5,WHITE);pp(sx+35,sy+5,WHITE);pp(sx+38,sy+5,WHITE);pp(sx+39,sy+5,WHITE);
}

/* ==================== START MENU ==================== */
static void draw_menu(void) {
    int sx=4,sy=124;
    fr(sx,sy,110,48,WHITE);
    hl(sx,sy,110,GRAY);
    vl(sx,sy,48,GRAY);
    hl(sx,sy+47,110,DGRAY);
    vl(sx+109,sy,48,DGRAY);
    
    /* Shutdown option */
    fr(sx+4,sy+4,102,18,RED);
    /* "SHUTDOWN" text */
    for(int i=0;i<4;i++)pp(sx+10+i,sy+7,WHITE);
    pp(sx+16,sy+7,WHITE);pp(sx+16,sy+8,WHITE);pp(sx+16,sy+9,WHITE);pp(sx+17,sy+9,WHITE);pp(sx+18,sy+8,WHITE);pp(sx+18,sy+7,WHITE);
    for(int i=0;i<4;i++)pp(sx+22+i,sy+7,WHITE);
    
    /* Restart option */
    fr(sx+4,sy+24,102,18,GREEN);
}

/* ==================== WINDOW ==================== */
static void draw_window(void) {
    /* Window body */
    fr(win_x,win_y,win_w,win_h,WHITE);
    
    /* Title bar - Blue */
    fr(win_x,win_y,win_w,18,BLUE);
    
    /* Window border */
    hl(win_x,win_y,win_w,BLACK);
    hl(win_x,win_y+win_h-1,win_w,BLACK);
    vl(win_x,win_y,win_h,BLACK);
    vl(win_x+win_w-1,win_y,win_h,BLACK);
    
    /* Close button [X] */
    int cx=win_x+win_w-20,cy=win_y+2;
    fr(cx,cy,16,14,GRAY);
    hl(cx,cy,16,WHITE);vl(cx,cy,14,WHITE);
    hl(cx,cy+13,16,DGRAY);vl(cx+15,cy,14,DGRAY);
    /* X */
    pp(cx+4,cy+3,BLACK);pp(cx+10,cy+3,BLACK);
    pp(cx+5,cy+4,BLACK);pp(cx+9,cy+4,BLACK);
    pp(cx+6,cy+5,BLACK);pp(cx+8,cy+5,BLACK);
    pp(cx+7,cy+6,BLACK);
    pp(cx+6,cy+7,BLACK);pp(cx+8,cy+7,BLACK);
    pp(cx+5,cy+8,BLACK);pp(cx+9,cy+8,BLACK);
    pp(cx+4,cy+9,BLACK);pp(cx+10,cy+9,BLACK);
    
    /* Maximize button [□] */
    int mx_btn=win_x+win_w-38,my_btn=win_y+2;
    fr(mx_btn,my_btn,16,14,GRAY);
    hl(mx_btn,my_btn,16,WHITE);vl(mx_btn,my_btn,14,WHITE);
    hl(mx_btn,my_btn+13,16,DGRAY);vl(mx_btn+15,my_btn,14,DGRAY);
    /* Square */
    fr(mx_btn+4,my_btn+3,8,8,WHITE);
    hl(mx_btn+4,my_btn+3,8,BLACK);
    hl(mx_btn+4,my_btn+10,8,BLACK);
    vl(mx_btn+4,my_btn+3,8,BLACK);
    vl(mx_btn+11,my_btn+3,8,BLACK);
    
    /* Minimize button [_] */
    int mn_btn=win_x+win_w-56,mn_y=win_y+2;
    fr(mn_btn,mn_y,16,14,GRAY);
    hl(mn_btn,mn_y,16,WHITE);vl(mn_btn,mn_y,14,WHITE);
    hl(mn_btn,mn_y+13,16,DGRAY);vl(mn_btn+15,mn_y,14,DGRAY);
    /* Underscore */
    hl(mn_btn+4,mn_y+9,8,BLACK);
}

/* ==================== MOUSE ==================== */
static int inr(int mx,int my,int x,int y,int w,int h){return(mx>=x&&mx<x+w&&my>=y&&my<y+h);}
static void mpoll(void){
    while(inb(0x64)&1){uint8_t st=inb(0x64),d=inb(0x60);if(!(st&0x20))continue;
        if(mcycle==0){if(d&0x08){mbytes[0]=d;mcycle=1;}}
        else{mbytes[mcycle++]=d;if(mcycle==3){mcycle=0;
            int dx=mbytes[1],dy=mbytes[2];if(mbytes[0]&0x10)dx|=~0xFF;if(mbytes[0]&0x20)dy|=~0xFF;dy=-dy;mbtn=mbytes[0]&0x07;
            if(dx!=0||dy!=0){cursor_restore_bg();mx+=dx/2;my+=dy/2;if(mx<0)mx=0;if(my<0)my=0;if(mx>=300)mx=300;if(my>=188)my=188;cursor_draw();}
        }}
    }
}

/* ==================== REDRAW ALL ==================== */
static void redraw_all(void){
    draw_wallpaper();
    draw_window();
    draw_taskbar();
    if(menu_open)draw_menu();
    cursor_draw();
}

/* ==================== VGA ==================== */
static void init_vga(void){
    outb(0x3C2,0x63);outb(0x3D4,0x00);outb(0x3D5,0x5F);outb(0x3D4,0x01);outb(0x3D5,0x4F);
    outb(0x3D4,0x02);outb(0x3D5,0x50);outb(0x3D4,0x03);outb(0x3D5,0x82);outb(0x3D4,0x04);outb(0x3D5,0x54);
    outb(0x3D4,0x05);outb(0x3D5,0x80);outb(0x3D4,0x06);outb(0x3D5,0xBF);outb(0x3D4,0x09);outb(0x3D5,0x41);
    outb(0x3D4,0x10);outb(0x3D5,0x9C);outb(0x3D4,0x11);outb(0x3D5,0x8E);outb(0x3D4,0x12);outb(0x3D5,0x8F);
    outb(0x3D4,0x13);outb(0x3D5,0x28);outb(0x3D4,0x14);outb(0x3D5,0x40);outb(0x3D4,0x15);outb(0x3D5,0x96);
    outb(0x3D4,0x16);outb(0x3D5,0xB9);outb(0x3D4,0x17);outb(0x3D5,0xA3);
    outb(0x3C4,0x00);outb(0x3C5,0x03);outb(0x3C4,0x01);outb(0x3C5,0x01);outb(0x3C4,0x02);outb(0x3C5,0x0F);
    outb(0x3C4,0x04);outb(0x3C5,0x0E);outb(0x3CE,0x05);outb(0x3CF,0x40);outb(0x3CE,0x06);outb(0x3CF,0x05);
    inb(0x3DA);outb(0x3C0,0x30);outb(0x3C0,0x41);outb(0x3C0,0x33);outb(0x3C0,0x00);outb(0x3C0,0x20);
    fb=(uint8_t*)0xA0000;
}

/* ==================== MAIN ==================== */
void kernel_main(uint32_t magic,uint32_t addr){
    (void)magic;(void)addr;init_vga();
    outb(0x3C8,0);static const uint8_t pal[16][3]={{0,0,0},{0,0,42},{0,42,0},{0,42,42},{42,0,0},{42,0,42},{42,21,0},{42,42,42},{21,21,21},{21,21,63},{21,63,21},{21,63,63},{63,21,21},{63,21,63},{63,63,21},{63,63,63}};
    for(int i=0;i<16;i++){outb(0x3C9,pal[i][0]);outb(0x3C9,pal[i][1]);outb(0x3C9,pal[i][2]);}
    outb(0x64,0xA8);for(volatile int i=0;i<100000;i++);outb(0x64,0xD4);outb(0x60,0xFF);for(volatile int i=0;i<100000;i++);while(inb(0x64)&1)inb(0x60);outb(0x64,0xD4);outb(0x60,0xF4);
    redraw_all();
    while(1){mpoll();int click=(pbtn==0&&(mbtn&1));pbtn=mbtn;
        if(click){if(inr(mx,my,4,180,56,16)){cursor_restore_bg();menu_open=!menu_open;if(menu_open)draw_menu();else{draw_taskbar();}cursor_draw();}
            if(menu_open&&inr(mx,my,8,128,102,18)){fr(0,0,320,200,BLACK);for(volatile int d=0;d<500000;d++);outb(0x64,0xFE);while(1)__asm__ volatile("hlt");}
            if(menu_open&&!inr(mx,my,4,124,110,48)){cursor_restore_bg();menu_open=0;draw_taskbar();cursor_draw();}
        }
        if(mbtn&1){if(dragging){cursor_restore_bg();win_x=mx-drag_ox;win_y=my-drag_oy;if(win_x<0)win_x=0;if(win_y<0)win_y=0;if(win_x+win_w>320)win_x=320-win_w;if(win_y+win_h>200)win_y=200-win_h;redraw_all();}else if(click&&inr(mx,my,win_x,win_y,win_w,18)&&!inr(mx,my,win_x+win_w-56,win_y,54,14)){dragging=1;drag_ox=mx-win_x;drag_oy=my-win_y;}}else{dragging=0;}
        for(volatile int d=0;d<2000;d++);
    }
}
