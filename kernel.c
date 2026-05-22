#include <stdint.h>

static uint8_t *fb;
static int pitch = 320, width = 320, height = 200;

/* Mouse */
static int mx = 160, my = 100;
static int mbtn = 0, pbtn = 0;
static int mcycle = 0;
static uint8_t mbytes[3];

/* Window */
static int win_x = 60, win_y = 40;
static int win_w = 200, win_h = 120;
static int dragging = 0;
static int drag_off_x = 0, drag_off_y = 0;

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
#define LGRAY   0x07

/* I/O */
static inline void outb(uint16_t p, uint8_t v) { __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p) { uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }

/* Pixel */
static void pp(int x, int y, uint8_t c) { if(x>=0&&x<width&&y>=0&&y<height)fb[y*pitch+x]=c; }
static void fr(int x,int y,int w,int h,uint8_t c){for(int dy=0;dy<h;dy++)for(int dx=0;dx<w;dx++)pp(x+dx,y+dy,c);}
static void hl(int x,int y,int w,uint8_t c){for(int i=0;i<w;i++)pp(x+i,y,c);}
static void vl(int x,int y,int h,uint8_t c){for(int i=0;i<h;i++)pp(x,y+i,c);}

/* Simple Black Arrow Cursor */
static uint8_t cbg[6*8];
static void csave(void){int idx=0;for(int dy=0;dy<6;dy++)for(int dx=0;dx<8;dx++){int px=mx+dx,py=my+dy;cbg[idx++]=(px>=0&&px<width&&py>=0&&py<height)?fb[py*pitch+px]:0;}}
static void crest(void){int idx=0;for(int dy=0;dy<6;dy++)for(int dx=0;dx<8;dx++){int px=mx+dx,py=my+dy;if(px>=0&&px<width&&py>=0&&py<height)fb[py*pitch+px]=cbg[idx];idx++;}}
static void cdraw(void){
    static const int arrow[6][8]={
        {1,0,0,0,0,0,0,0},
        {1,1,0,0,0,0,0,0},
        {1,1,1,0,0,0,0,0},
        {1,1,1,1,0,0,0,0},
        {0,1,0,0,0,0,0,0},
        {0,0,1,1,0,0,0,0},
    };
    for(int dy=0;dy<6;dy++)for(int dx=0;dx<8;dx++)if(arrow[dy][dx])pp(mx+dx,my+dy,BLACK);
}

/* Desktop */
static void draw_wp(void){fr(0,0,320,176,TEAL);}
static void draw_tb(void){
    int ty=176;fr(0,ty,320,24,WHITE);hl(0,ty,320,GRAY);
    fr(4,ty+4,50,16,GRAY);hl(4,ty+4,50,WHITE);vl(4,ty+4,16,WHITE);hl(4,ty+19,50,DGRAY);vl(53,ty+4,16,DGRAY);
    /* Simple START text using pixels */
    pp(10,180,BLACK);pp(11,180,BLACK);pp(14,180,BLACK);pp(15,180,BLACK);pp(18,180,BLACK);pp(19,180,BLACK);
    pp(10,181,BLACK);pp(12,181,BLACK);pp(14,181,BLACK);pp(16,181,BLACK);pp(18,181,BLACK);pp(20,181,BLACK);
}
static void draw_menu(void){
    int sx=4,sy=132;fr(sx,sy,100,30,WHITE);hl(sx,sy,100,GRAY);vl(sx,sy,30,GRAY);hl(sx,sy+29,100,DGRAY);vl(sx+99,sy,30,DGRAY);
    fr(sx+6,sy+6,88,18,RED);
    pp(sx+14,sy+10,WHITE);pp(sx+16,sy+10,WHITE);pp(sx+22,sy+10,WHITE);pp(sx+26,sy+10,WHITE);
}

/* Window */
static void draw_window(void){
    /* Window background */
    fr(win_x,win_y,win_w,win_h,WHITE);
    /* Title bar */
    fr(win_x,win_y,win_w,16,BLUE);
    /* Border */
    hl(win_x,win_y,win_w,BLACK);
    hl(win_x,win_y+win_h-1,win_w,BLACK);
    vl(win_x,win_y,win_h,BLACK);
    vl(win_x+win_w-1,win_y,win_h,BLACK);
}

/* Mouse */
static int inr(int mx,int my,int x,int y,int w,int h){return(mx>=x&&mx<x+w&&my>=y&&my<y+h);}
static void mpoll(void){
    while(inb(0x64)&1){
        uint8_t st=inb(0x64),d=inb(0x60);
        if(!(st&0x20))continue;
        if(mcycle==0){if(d&0x08){mbytes[0]=d;mcycle=1;}}
        else{mbytes[mcycle++]=d;if(mcycle==3){mcycle=0;
            int dx=mbytes[1],dy=mbytes[2];
            if(mbytes[0]&0x10)dx|=~0xFF;if(mbytes[0]&0x20)dy|=~0xFF;
            dy=-dy;mbtn=mbytes[0]&0x07;
            if(dx!=0||dy!=0){mx+=dx/2;my+=dy/2;}
            if(mx<0)mx=0;if(my<0)my=0;if(mx>=320)mx=319;if(my>=200)my=199;
        }}
    }
}
static void clicks(void){
    int left_click=(pbtn==0&&(mbtn&1));
    pbtn=mbtn;
    
    /* Dragging */
    if(mbtn&1){
        if(dragging){
            win_x=mx-drag_off_x;win_y=my-drag_off_y;
            if(win_x<0)win_x=0;if(win_y<0)win_y=0;
            if(win_x+win_w>320)win_x=320-win_w;
            if(win_y+win_h>200)win_y=200-win_h;
        }
        else if(inr(mx,my,win_x,win_y,win_w,16)){
            dragging=1;drag_off_x=mx-win_x;drag_off_y=my-win_y;
        }
    }else{dragging=0;}
    
    if(!left_click)return;
    
    /* Start button */
    if(inr(mx,my,4,180,50,16)){menu_open=!menu_open;return;}
    /* Menu shutdown */
    if(menu_open&&inr(mx,my,10,138,88,18)){fr(0,0,320,200,BLACK);for(volatile int d=0;d<500000;d++);outb(0x64,0xFE);while(1)__asm__ volatile("hlt");}
    if(menu_open&&!inr(mx,my,4,132,100,30))menu_open=0;
}

/* VGA */
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

void kernel_main(uint32_t magic,uint32_t addr){
    (void)magic;(void)addr;init_vga();
    outb(0x3C8,0);static const uint8_t pal[16][3]={{0,0,0},{0,0,42},{0,42,0},{0,42,42},{42,0,0},{42,0,42},{42,21,0},{42,42,42},{21,21,21},{21,21,63},{21,63,21},{21,63,63},{63,21,21},{63,21,63},{63,63,21},{63,63,63}};
    for(int i=0;i<16;i++){outb(0x3C9,pal[i][0]);outb(0x3C9,pal[i][1]);outb(0x3C9,pal[i][2]);}
    
    /* Mouse init */
    outb(0x64,0xA8);for(volatile int i=0;i<100000;i++);
    outb(0x64,0xD4);outb(0x60,0xFF);for(volatile int i=0;i<100000;i++);
    while(inb(0x64)&1)inb(0x60);
    outb(0x64,0xD4);outb(0x60,0xF4);for(volatile int i=0;i<100000;i++);
    mcycle=0;mx=160;my=100;mbtn=0;pbtn=0;
    
    while(1){
        crest();draw_wp();draw_window();draw_tb();if(menu_open)draw_menu();
        mpoll();clicks();
        csave();cdraw();
        for(volatile int d=0;d<3000;d++);
    }
}
