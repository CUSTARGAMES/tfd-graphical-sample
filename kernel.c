#include <stdint.h>

static uint8_t *fb;

/* Forward declarations */
static void mpoll(void);
static void redraw(void);
static void draw_wp(void);
static void draw_icons(void);
static void draw_tb(void);
static void draw_win(void);
static void draw_browser(void);
static void draw_menu(void);
static void cd(void);
static void cr(void);
static void snake_update(void);

/* Mouse */
static int mx=160,my=100,mbtn=0,pbtn=0,mcycle=0;
static uint8_t mbytes[3];
static uint8_t csave[12][20];
static int cdrawn=0;

/* Windows */
static int wx=50,wy=40,ww=220,wh=120,drag=0,dox=0,doy=0,win_open=0;
static int bx=60,by=50,bw=200,bh=140,bopen=0;

/* Menu + Animation */
static int menu=0,menu_anim=0,menu_dir=0;

/* Snake game state */
static int snake_on=0;
static int sx[200],sy[200],snl=3,sdir=0,sfx,sfy,ssc=0,sgo=0;
static int stick=0;

/* Colors */
#define BLACK   0x00
#define BLUE    0x01
#define GREEN   0x02
#define RED     0x04
#define GRAY    0x07
#define DGRAY   0x08
#define LBLUE   0x09
#define LGREEN  0x0A
#define LRED    0x0C
#define YELLOW  0x0E
#define WHITE   0x0F
#define ORANGE  0x06
#define PINK    LRED

/* I/O */
static inline void outb(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline uint8_t inb(uint16_t p){uint8_t r;__asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p));return r;}

/* Pixel */
static void pp(int x,int y,uint8_t c){if(x>=0&&x<320&&y>=0&&y<200)fb[y*320+x]=c;}
static void fr(int x,int y,int w,int h,uint8_t c){for(int dy=0;dy<h;dy++)for(int dx=0;dx<w;dx++)pp(x+dx,y+dy,c);}
static void hl(int x,int y,int w,uint8_t c){for(int i=0;i<w;i++)pp(x+i,y,c);}
static void vl(int x,int y,int h,uint8_t c){for(int i=0;i<h;i++)pp(x,y+i,c);}

/* ============ SOUND ============ */
static void beep(int freq,int dur){
    if(freq==0)return;
    uint16_t div=1193180/freq;
    outb(0x43,0xB6);outb(0x42,div&0xFF);outb(0x42,(div>>8)&0xFF);
    outb(0x61,inb(0x61)|3);
    for(volatile int d=0;d<dur*8000;d++)__asm__ volatile("nop");
    outb(0x61,inb(0x61)&~3);
}
static void tada(void){beep(523,25);beep(659,25);beep(784,40);beep(1047,60);}

/* ============ 8x8 FONT ============ */
static void draw_char(int x,int y,char ch,uint8_t fg){
    static const uint8_t font[26][8]={
        {0x18,0x24,0x42,0x7E,0x42,0x42,0x42,0x00},{0x7C,0x42,0x42,0x7C,0x42,0x42,0x7C,0x00},
        {0x3C,0x42,0x40,0x40,0x40,0x42,0x3C,0x00},{0x78,0x44,0x42,0x42,0x42,0x44,0x78,0x00},
        {0x7E,0x40,0x40,0x7C,0x40,0x40,0x7E,0x00},{0x7E,0x40,0x40,0x7C,0x40,0x40,0x40,0x00},
        {0x3C,0x42,0x40,0x4E,0x42,0x42,0x3C,0x00},{0x42,0x42,0x42,0x7E,0x42,0x42,0x42,0x00},
        {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},{0x0E,0x04,0x04,0x04,0x44,0x44,0x38,0x00},
        {0x42,0x44,0x48,0x70,0x48,0x44,0x42,0x00},{0x40,0x40,0x40,0x40,0x40,0x40,0x7E,0x00},
        {0x42,0x66,0x5A,0x42,0x42,0x42,0x42,0x00},{0x42,0x62,0x52,0x4A,0x46,0x42,0x42,0x00},
        {0x3C,0x42,0x42,0x42,0x42,0x42,0x3C,0x00},{0x7C,0x42,0x42,0x7C,0x40,0x40,0x40,0x00},
        {0x3C,0x42,0x42,0x42,0x4A,0x44,0x3A,0x00},{0x7C,0x42,0x42,0x7C,0x48,0x44,0x42,0x00},
        {0x3C,0x42,0x40,0x3C,0x02,0x42,0x3C,0x00},{0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
        {0x42,0x42,0x42,0x42,0x42,0x42,0x3C,0x00},{0x42,0x42,0x42,0x42,0x24,0x24,0x18,0x00},
        {0x42,0x42,0x42,0x5A,0x5A,0x66,0x42,0x00},{0x42,0x24,0x18,0x18,0x18,0x24,0x42,0x00},
        {0x42,0x24,0x18,0x18,0x18,0x18,0x18,0x00},{0x7E,0x02,0x04,0x18,0x20,0x40,0x7E,0x00},
    };
    if(ch>='a'&&ch<='z')ch-=32;if(ch<'A'||ch>'Z')return;
    const uint8_t*g=font[ch-'A'];
    for(int r=0;r<8;r++){uint8_t row=g[r];for(int c=0;c<8;c++){if(row&(0x80>>c))pp(x+c,y+r,fg);}}
}
static void draw_text(int x,int y,const char*s,uint8_t fg){while(*s){if(*s==' '){x+=9;s++;continue;}draw_char(x,y,*s,fg);x+=9;s++;}}

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

/* ============ ICONS ============ */
static void draw_icons(void){
    fr(10,30,32,32,BLACK);fr(14,34,24,24,WHITE);hl(14,34,24,BLUE);draw_text(8,66,"ABOUT",WHITE);
    fr(10,100,32,32,BLACK);fr(14,104,24,24,WHITE);
    fr(16,106,20,4,GREEN);fr(16,112,20,4,GREEN);fr(16,118,20,4,GREEN);
    draw_text(5,136,"BROWSER",WHITE);
}

/* ============ TASKBAR ============ */
static void draw_tb(void){
    int ty=176;fr(0,ty,320,24,GRAY);hl(0,ty,320,WHITE);
    int sx=4,sy=ty+4;fr(sx,sy,56,16,GREEN);
    hl(sx,sy,56,WHITE);vl(sx,sy,16,WHITE);hl(sx,sy+15,56,DGRAY);vl(sx+55,sy,16,DGRAY);
    draw_text(sx+8,sy+3,"MENU",WHITE);
}

/* ============ MENU ============ */
static void draw_menu(void){
    int sx=4,sy=176-4-(menu_anim*5);fr(sx,sy,110,menu_anim*5,GRAY);hl(sx,sy,110,WHITE);
    fr(sx+4,sy+4,102,16,RED);draw_text(sx+8,sy+5,"SHUTDOWN",WHITE);
    fr(sx+4,sy+24,102,16,LGREEN);draw_text(sx+10,sy+25,"RESTART",BLACK);
    fr(sx+4,sy+44,102,16,YELLOW);draw_text(sx+10,sy+45,"SNAKE",BLACK);
}

/* ============ ABOUT WINDOW ============ */
static void draw_win(void){
    if(!win_open)return;
    fr(wx,wy,ww,wh,WHITE);fr(wx,wy,ww,16,BLUE);
    hl(wx,wy,ww,BLACK);hl(wx,wy+wh-1,ww,BLACK);vl(wx,wy,wh,BLACK);vl(wx+ww-1,wy,wh,BLACK);
    draw_text(wx+6,wy+3,"ABOUT TFD OS",WHITE);
    int cx=wx+ww-18,cy=wy+2;fr(cx,cy,14,12,GRAY);
    hl(cx,cy,14,WHITE);vl(cx,cy,12,WHITE);hl(cx,cy+11,14,DGRAY);vl(cx+13,cy,12,DGRAY);
    draw_text(cx+4,cy+2,"X",BLACK);
    draw_text(wx+10,wy+25,"TFD OS v3.0",BLACK);
    draw_text(wx+10,wy+35,"JDM EDITION",BLACK);
    draw_text(wx+10,wy+45,"BY SADMAN",BLACK);
}

/* ============ BROWSER WINDOW ============ */
static void draw_browser(void){
    if(!bopen)return;
    fr(bx,by,bw,bh,WHITE);fr(bx,by,bw,16,BLUE);
    hl(bx,by,bw,BLACK);hl(bx,by+bh-1,bw,BLACK);vl(bx,by,bh,BLACK);vl(bx+bw-1,by,bh,BLACK);
    draw_text(bx+4,by+3,"RETRO GOOGLE",WHITE);
    int cx=bx+bw-18,cy=by+2;fr(cx,cy,14,12,GRAY);
    hl(cx,cy,14,WHITE);vl(cx,cy,12,WHITE);hl(cx,cy+11,14,DGRAY);vl(cx+13,cy,12,DGRAY);
    draw_text(cx+4,cy+2,"X",BLACK);
    fr(bx+30,by+30,140,20,WHITE);hl(bx+30,by+30,140,BLACK);hl(bx+30,by+49,140,BLACK);vl(bx+30,by+30,20,BLACK);vl(bx+169,by+30,20,BLACK);
    draw_text(bx+35,by+33,"GOOGLE",BLUE);
    draw_text(bx+10,by+65,"RETRO SEARCH",BLACK);
    fr(bx+10,by+80,180,1,BLACK);
    draw_text(bx+10,by+90,"I AM FEELING",GRAY);
    draw_text(bx+10,by+100,"LUCKY",GRAY);
}

/* ============ SNAKE GAME ============ */
static void snake_init(void){
    snl=3;sdir=0;ssc=0;sgo=0;
    sx[0]=8;sy[0]=8;sx[1]=7;sy[1]=8;sx[2]=6;sy[2]=8;
    sfx=12;sfy=8;
    fr(0,0,320,200,BLACK);
    for(int x=0;x<16;x++){pp(x*20,0,GREEN);pp(x*20,199,GREEN);}
    for(int y=0;y<16;y++){pp(0,y*20,GREEN);pp(319,y*20,GREEN);}
    snake_on=1;
}
static void snake_update(void){
    if(!snake_on||sgo)return;
    stick++;if(stick<6)return;stick=0;
    if(inb(0x64)&1){uint8_t sc=inb(0x60);char c=0;
        switch(sc){case 0x11:c='w';break;case 0x1F:c='s';break;case 0x1E:c='a';break;case 0x20:c='d';break;case 0x10:c='q';break;}
        if(c=='w')sdir=3;else if(c=='s')sdir=1;else if(c=='a')sdir=2;else if(c=='d')sdir=0;else if(c=='q'){snake_on=0;return;}}
    int nx=sx[0],ny=sy[0];if(sdir==0)nx++;else if(sdir==1)ny++;else if(sdir==2)nx--;else ny--;
    if(nx<0||nx>=16||ny<0||ny>=16){sgo=1;return;}for(int i=0;i<snl;i++)if(sx[i]==nx&&sy[i]==ny){sgo=1;return;}
    fr(sx[snl-1]*20+2,sy[snl-1]*20+2,16,16,BLACK);for(int i=snl-1;i>0;i--){sx[i]=sx[i-1];sy[i]=sy[i-1];}sx[0]=nx;sy[0]=ny;
    for(int i=0;i<snl;i++){fr(sx[i]*20+2,sy[i]*20+2,16,16,i==0?LGREEN:GREEN);}
    if(sx[0]==sfx&&sy[0]==sfy){snl++;ssc+=10;sx[snl-1]=sx[snl-2];sy[snl-1]=sy[snl-2];sfx=((inb(0x40)*123)%16);sfy=((inb(0x40)*789)%16);}
    fr(sfx*20+4,sfy*20+4,12,12,RED);
    if(sgo){fr(80,80,160,60,BLACK);draw_text(100,85,"GAME OVER",RED);draw_text(80,110,"Q TO QUIT",WHITE);}
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
static void redraw(void){draw_wp();draw_icons();draw_tb();draw_win();draw_browser();if(menu)draw_menu();cd();}

/* ============ VGA ============ */
static void init_vga(void){
    outb(0x3C2,0x63);outb(0x3D4,0x00);outb(0x3D5,0x5F);outb(0x3D4,0x01);outb(0x3D5,0x4F);outb(0x3D4,0x02);outb(0x3D5,0x50);outb(0x3D4,0x03);outb(0x3D5,0x82);outb(0x3D4,0x04);outb(0x3D5,0x54);outb(0x3D4,0x05);outb(0x3D5,0x80);outb(0x3D4,0x06);outb(0x3D5,0xBF);outb(0x3D4,0x09);outb(0x3D5,0x41);outb(0x3D4,0x10);outb(0x3D5,0x9C);outb(0x3D4,0x11);outb(0x3D5,0x8E);outb(0x3D4,0x12);outb(0x3D5,0x8F);outb(0x3D4,0x13);outb(0x3D5,0x28);outb(0x3D4,0x14);outb(0x3D5,0x40);outb(0x3D4,0x15);outb(0x3D5,0x96);outb(0x3D4,0x16);outb(0x3D5,0xB9);outb(0x3D4,0x17);outb(0x3D5,0xA3);outb(0x3C4,0x00);outb(0x3C5,0x03);outb(0x3C4,0x01);outb(0x3C5,0x01);outb(0x3C4,0x02);outb(0x3C5,0x0F);outb(0x3C4,0x04);outb(0x3C5,0x0E);outb(0x3CE,0x05);outb(0x3CF,0x40);outb(0x3CE,0x06);outb(0x3CF,0x05);inb(0x3DA);outb(0x3C0,0x30);outb(0x3C0,0x41);outb(0x3C0,0x33);outb(0x3C0,0x00);outb(0x3C0,0x20);fb=(uint8_t*)0xA0000;
}

void kernel_main(uint32_t magic,uint32_t addr){
    (void)magic;(void)addr;init_vga();
    outb(0x3C8,0);static const uint8_t pal[16][3]={{0,0,0},{0,0,42},{0,42,0},{0,42,42},{42,0,0},{42,0,42},{42,21,0},{42,42,42},{21,21,21},{21,21,63},{21,63,21},{21,63,63},{63,21,21},{63,21,63},{63,63,21},{63,63,63}};
    for(int i=0;i<16;i++){outb(0x3C9,pal[i][0]);outb(0x3C9,pal[i][1]);outb(0x3C9,pal[i][2]);}
    outb(0x64,0xA8);for(volatile int i=0;i<100000;i++);outb(0x64,0xD4);outb(0x60,0xFF);for(volatile int i=0;i<100000;i++);while(inb(0x64)&1)inb(0x60);outb(0x64,0xD4);outb(0x60,0xF4);
    tada();
    redraw();
    while(1){
        if(snake_on){snake_update();if(!snake_on)redraw();for(volatile int d=0;d<8000;d++);continue;}
        mpoll();int click=(pbtn==0&&(mbtn&1));int rclick=(pbtn==0&&(mbtn&2));pbtn=mbtn;
        if(rclick&&menu){menu_dir=0;menu_anim=0;menu=0;cr();draw_tb();cd();}
        if(menu_dir==1&&menu_anim<10){menu_anim++;cr();draw_wp();draw_icons();draw_tb();draw_win();draw_browser();if(menu)draw_menu();cd();}
        if(menu_dir==0&&menu_anim>0){menu_anim--;cr();draw_wp();draw_icons();draw_tb();draw_win();draw_browser();if(menu_anim>0)draw_menu();cd();}
        if(menu_anim==0&&menu_dir==0)menu=0;
        if(click){
            if(inr(mx,my,4,180,56,16)){if(!menu){menu=1;menu_dir=1;menu_anim=0;}else{menu_dir=0;}}
            if(menu&&menu_anim==10){int sy=176-4-50;
                if(inr(mx,my,8,sy+4,102,16)){fr(0,0,320,200,BLACK);for(volatile int d=0;d<500000;d++);outb(0x64,0xFE);while(1)__asm__ volatile("hlt");}
                if(inr(mx,my,8,sy+44,102,16)){cr();snake_init();cd();}}
            if(win_open&&inr(mx,my,wx+ww-18,wy+2,14,12)){cr();win_open=0;redraw();}
            if(!win_open&&inr(mx,my,10,30,32,32)){cr();win_open=1;redraw();}
            if(bopen&&inr(mx,my,bx+bw-18,by+2,14,12)){cr();bopen=0;redraw();}
            if(!bopen&&inr(mx,my,10,100,32,32)){cr();bopen=1;redraw();}}
        if(mbtn&1){if(drag){cr();if(win_open&&inr(mx-dox,my-doy,wx,wy,ww,16)){wx=mx-dox;wy=my-doy;}if(bopen&&inr(mx-dox,my-doy,bx,by,bw,16)){bx=mx-dox;by=my-doy;}redraw();}else if(click){if(win_open&&inr(mx,my,wx,wy,ww,16)&&!inr(mx,my,wx+ww-18,wy,14,12)){drag=1;dox=mx-wx;doy=my-wy;}if(bopen&&inr(mx,my,bx,by,bw,16)&&!inr(mx,my,bx+bw-18,by,14,12)){drag=1;dox=mx-bx;doy=my-by;}}}else{drag=0;}
        for(volatile int d=0;d<1500;d++);
    }
}
