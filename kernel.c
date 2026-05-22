#include <stdint.h>

static uint8_t *fb;

/* Mouse */
static int mx=160,my=100,mbtn=0,pbtn=0,mcycle=0;
static uint8_t mbytes[3];
static uint8_t csave[12][20];
static int cdrawn=0;

/* Window */
static int wx=50,wy=40,ww=220,wh=120;
static int drag=0,dox=0,doy=0;
static int win_open=1;

/* Menu + Animation */
static int menu=0,menu_anim=0,menu_dir=0;

/* Colors */
#define BLACK   0x00
#define BLUE    0x01
#define GREEN   0x02
#define CYAN    0x03
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
#define ORANGE  BROWN
#define PINK    LRED

/* I/O */
static inline void outb(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline uint8_t inb(uint16_t p){uint8_t r;__asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p));return r;}

/* Pixel */
static void pp(int x,int y,uint8_t c){if(x>=0&&x<320&&y>=0&&y<200)fb[y*320+x]=c;}
static void fr(int x,int y,int w,int h,uint8_t c){for(int dy=0;dy<h;dy++)for(int dx=0;dx<w;dx++)pp(x+dx,y+dy,c);}
static void hl(int x,int y,int w,uint8_t c){for(int i=0;i<w;i++)pp(x+i,y,c);}
static void vl(int x,int y,int h,uint8_t c){for(int i=0;i<h;i++)pp(x,y+i,c);}

/* ============ TEXT ENGINE ============ */
/* Simple 5x7 font for pixel text */
static void draw_char(int x,int y,char ch,uint8_t fg,uint8_t bg){
    static const uint8_t font[26][7]={
        {0x1F,0x14,0x14,0x1F,0x00,0x00,0x00}, /* A */
        {0x1F,0x15,0x15,0x0A,0x00,0x00,0x00}, /* B */
        {0x0E,0x11,0x11,0x11,0x00,0x00,0x00}, /* C */
        {0x1F,0x11,0x11,0x0E,0x00,0x00,0x00}, /* D */
        {0x1F,0x15,0x15,0x11,0x00,0x00,0x00}, /* E */
        {0x1F,0x14,0x14,0x10,0x00,0x00,0x00}, /* F */
        {0x0E,0x11,0x15,0x0D,0x00,0x00,0x00}, /* G */
        {0x1F,0x04,0x04,0x1F,0x00,0x00,0x00}, /* H */
        {0x11,0x1F,0x11,0x00,0x00,0x00,0x00}, /* I */
        {0x01,0x11,0x0F,0x00,0x00,0x00,0x00}, /* J */
        {0x1F,0x04,0x0A,0x11,0x00,0x00,0x00}, /* K */
        {0x1F,0x01,0x01,0x01,0x00,0x00,0x00}, /* L */
        {0x1F,0x08,0x04,0x1F,0x00,0x00,0x00}, /* M */
        {0x1F,0x08,0x04,0x1F,0x00,0x00,0x00}, /* N */
        {0x0E,0x11,0x11,0x0E,0x00,0x00,0x00}, /* O */
        {0x1F,0x14,0x14,0x08,0x00,0x00,0x00}, /* P */
        {0x0E,0x11,0x0D,0x03,0x00,0x00,0x00}, /* Q */
        {0x1F,0x14,0x16,0x09,0x00,0x00,0x00}, /* R */
        {0x09,0x15,0x15,0x12,0x00,0x00,0x00}, /* S */
        {0x10,0x10,0x1F,0x10,0x10,0x00,0x00}, /* T */
        {0x0F,0x01,0x01,0x0F,0x00,0x00,0x00}, /* U */
        {0x1E,0x01,0x02,0x1C,0x00,0x00,0x00}, /* V */
        {0x1F,0x02,0x04,0x1F,0x00,0x00,0x00}, /* W */
        {0x11,0x0A,0x04,0x11,0x00,0x00,0x00}, /* X */
        {0x10,0x08,0x07,0x08,0x10,0x00,0x00}, /* Y */
        {0x13,0x15,0x19,0x11,0x00,0x00,0x00}, /* Z */
    };
    if(ch>='a'&&ch<='z')ch-=32;
    if(ch<'A'||ch>'Z')return;
    const uint8_t*g=font[ch-'A'];
    for(int r=0;r<7;r++){uint8_t b=g[r];for(int c=0;c<5;c++){if(b&(0x10>>c))pp(x+c,y+r,fg);else if(bg!=0xFF)pp(x+c,y+r,bg);}}
}
static void draw_text(int x,int y,const char*s,uint8_t fg,uint8_t bg){while(*s){draw_char(x,y,*s,fg,bg);x+=6;s++;}}

/* ============ CURSOR ============ */
static void cs(void){for(int dy=0;dy<12;dy++)for(int dx=0;dx<20;dx++){int px=mx+dx,py=my+dy;if(px>=0&&px<320&&py>=0&&py<200)csave[dy][dx]=fb[py*320+px];}cdrawn=1;}
static void cr(void){if(!cdrawn)return;for(int dy=0;dy<12;dy++)for(int dx=0;dx<20;dx++){int px=mx+dx,py=my+dy;if(px>=0&&px<320&&py>=0&&py<200)fb[py*320+px]=csave[dy][dx];}cdrawn=0;}
static void cd(void){cs();static const int s[12][20]={{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},{1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};for(int dy=0;dy<12;dy++)for(int dx=0;dx<20;dx++)if(s[dy][dx])pp(mx+dx,my+dy,BLACK);}

/* ============ WALLPAPER ============ */
static void draw_wp(void){
    for(int y=0;y<140;y++){uint8_t c=y<50?BLUE:y<90?LBLUE:y<120?PINK:ORANGE;fr(0,y,320,1,c);}
    int sx=160,sy=100,sr=35;for(int dy=-sr;dy<=sr;dy++)for(int dx=-sr;dx<=sr;dx++)if(dx*dx+dy*dy<=sr*sr)pp(sx+dx,sy+dy,RED);
    for(int y=120;y<176;y++){int fw=(y-120)*3;int fx=160-fw/2;for(int x=fx;x<fx+fw;x++)pp(x,y,BLACK);}
    for(int i=0;i<30;i++){int bx=(i*37+123)%300+10;int by=(i*53+89)%60+10;fr(bx,by,3,3,PINK);}
}

/* ============ DESKTOP ICON ============ */
static void draw_icon(void){
    fr(10,30,32,32,BLACK);fr(14,34,24,24,WHITE);
    hl(14,34,24,BLUE);draw_text(8,70,"About",WHITE,0xFF);
}

/* ============ TASKBAR ============ */
static void draw_tb(void){
    int ty=176;fr(0,ty,320,24,GRAY);hl(0,ty,320,WHITE);
    int sx=4,sy=ty+4;fr(sx,sy,56,16,GREEN);
    hl(sx,sy,56,WHITE);vl(sx,sy,16,WHITE);hl(sx,sy+15,56,DGRAY);vl(sx+55,sy,16,DGRAY);
    draw_text(sx+8,sy+3,"MENU",WHITE,0xFF);
}

/* ============ MENU ============ */
static void draw_menu(void){
    int sx=4,sy=176-4-(menu_anim*5);fr(sx,sy,110,menu_anim*5,GRAY);hl(sx,sy,110,WHITE);
    fr(sx+4,sy+4,102,16,RED);draw_text(sx+12,sy+6,"SHUTDOWN",WHITE,0xFF);
    fr(sx+4,sy+24,102,16,LGREEN);draw_text(sx+14,sy+26,"RESTART",BLACK,0xFF);
}

/* ============ WINDOW ============ */
static void draw_win(void){
    if(!win_open)return;
    fr(wx,wy,ww,wh,WHITE);fr(wx,wy,ww,16,BLUE);
    hl(wx,wy,ww,BLACK);hl(wx,wy+wh-1,ww,BLACK);vl(wx,wy,wh,BLACK);vl(wx+ww-1,wy,wh,BLACK);
    draw_text(wx+6,wy+3,"ABOUT TFD OS",WHITE,0xFF);
    int cx=wx+ww-18,cy=wy+2;fr(cx,cy,14,12,GRAY);
    hl(cx,cy,14,WHITE);vl(cx,cy,12,WHITE);hl(cx,cy+11,14,DGRAY);vl(cx+13,cy,12,DGRAY);
    draw_text(cx+3,cy+2,"X",BLACK,0xFF);
    /* About text */
    draw_text(wx+10,wy+25,"TFD OS v3.0",BLACK,0xFF);
    draw_text(wx+10,wy+35,"JDM Edition",BLACK,0xFF);
    draw_text(wx+10,wy+45,"By Sadman 2026",BLACK,0xFF);
    draw_text(wx+10,wy+55,"Custom VGA GUI",BLACK,0xFF);
}

/* ============ MOUSE ============ */
static int inr(int mx,int my,int x,int y,int w,int h){return(mx>=x&&mx<x+w&&my>=y&&my<y+h);}
static void mpoll(void){
    while(inb(0x64)&1){uint8_t st=inb(0x64),d=inb(0x60);if(!(st&0x20))continue;
        if(mcycle==0){if(d&0x08){mbytes[0]=d;mcycle=1;}}
        else{mbytes[mcycle++]=d;if(mcycle==3){mcycle=0;
            int dx=mbytes[1],dy=mbytes[2];if(mbytes[0]&0x10)dx|=~0xFF;if(mbytes[0]&0x20)dy|=~0xFF;dy=-dy;mbtn=mbytes[0]&0x07;
            if(dx!=0||dy!=0){cr();mx+=dx/2;my+=dy/2;if(mx<0)mx=0;if(my<0)my=0;if(mx>=300)mx=300;if(my>=188)my=188;cd();}
        }}
    }
}

/* ============ REDRAW ============ */
static void redraw(void){draw_wp();draw_icon();draw_tb();draw_win();if(menu)draw_menu();cd();}

/* ============ VGA ============ */
static void init_vga(void){
    outb(0x3C2,0x63);outb(0x3D4,0x00);outb(0x3D5,0x5F);outb(0x3D4,0x01);outb(0x3D5,0x4F);outb(0x3D4,0x02);outb(0x3D5,0x50);outb(0x3D4,0x03);outb(0x3D5,0x82);outb(0x3D4,0x04);outb(0x3D5,0x54);outb(0x3D4,0x05);outb(0x3D5,0x80);outb(0x3D4,0x06);outb(0x3D5,0xBF);outb(0x3D4,0x09);outb(0x3D5,0x41);outb(0x3D4,0x10);outb(0x3D5,0x9C);outb(0x3D4,0x11);outb(0x3D5,0x8E);outb(0x3D4,0x12);outb(0x3D5,0x8F);outb(0x3D4,0x13);outb(0x3D5,0x28);outb(0x3D4,0x14);outb(0x3D5,0x40);outb(0x3D4,0x15);outb(0x3D5,0x96);outb(0x3D4,0x16);outb(0x3D5,0xB9);outb(0x3D4,0x17);outb(0x3D5,0xA3);outb(0x3C4,0x00);outb(0x3C5,0x03);outb(0x3C4,0x01);outb(0x3C5,0x01);outb(0x3C4,0x02);outb(0x3C5,0x0F);outb(0x3C4,0x04);outb(0x3C5,0x0E);outb(0x3CE,0x05);outb(0x3CF,0x40);outb(0x3CE,0x06);outb(0x3CF,0x05);inb(0x3DA);outb(0x3C0,0x30);outb(0x3C0,0x41);outb(0x3C0,0x33);outb(0x3C0,0x00);outb(0x3C0,0x20);fb=(uint8_t*)0xA0000;
}

void kernel_main(uint32_t magic,uint32_t addr){
    (void)magic;(void)addr;init_vga();
    outb(0x3C8,0);static const uint8_t pal[16][3]={{0,0,0},{0,0,42},{0,42,0},{0,42,42},{42,0,0},{42,0,42},{42,21,0},{42,42,42},{21,21,21},{21,21,63},{21,63,21},{21,63,63},{63,21,21},{63,21,63},{63,63,21},{63,63,63}};
    for(int i=0;i<16;i++){outb(0x3C9,pal[i][0]);outb(0x3C9,pal[i][1]);outb(0x3C9,pal[i][2]);}
    outb(0x64,0xA8);for(volatile int i=0;i<100000;i++);outb(0x64,0xD4);outb(0x60,0xFF);for(volatile int i=0;i<100000;i++);while(inb(0x64)&1)inb(0x60);outb(0x64,0xD4);outb(0x60,0xF4);
    redraw();
    while(1){
        mpoll();
        int click=(pbtn==0&&(mbtn&1));pbtn=mbtn;
        if(menu_dir==1&&menu_anim<10){menu_anim++;cr();draw_wp();draw_icon();draw_tb();draw_win();if(menu)draw_menu();cd();}
        if(menu_dir==0&&menu_anim>0){menu_anim--;cr();draw_wp();draw_icon();draw_tb();draw_win();if(menu_anim>0)draw_menu();cd();}
        if(menu_anim==0&&menu_dir==0)menu=0;
        if(click){
            if(inr(mx,my,4,180,56,16)){if(!menu){menu=1;menu_dir=1;menu_anim=0;}else{menu_dir=0;}}
            if(menu&&menu_anim==10){int sy=176-4-50;if(inr(mx,my,8,sy+4,102,16)){fr(0,0,320,200,BLACK);for(volatile int d=0;d<500000;d++);outb(0x64,0xFE);while(1)__asm__ volatile("hlt");}}
            if(win_open&&inr(mx,my,wx+ww-18,wy+2,14,12)){cr();win_open=0;redraw();}
            if(!win_open&&inr(mx,my,10,30,32,32)){cr();win_open=1;redraw();}
        }
        if(mbtn&1){if(drag){cr();wx=mx-dox;wy=my-doy;if(wx<0)wx=0;if(wy<0)wy=0;if(wx+ww>320)wx=320-ww;if(wy+wh>200)wy=200-wh;redraw();}else if(click&&win_open&&inr(mx,my,wx,wy,ww,16)&&!inr(mx,my,wx+ww-18,wy,14,12)){drag=1;dox=mx-wx;doy=my-wy;}}else{drag=0;}
        for(volatile int d=0;d<1500;d++);
    }
}
