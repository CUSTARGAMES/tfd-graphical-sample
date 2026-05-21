#include <stdint.h>

/* VESA/VGA Framebuffer */
static uint8_t *fb;
static int pitch;
static int width;
static int height;

/* I/O Ports */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Fallback: VGA Mode 13h (320x200x256) */
static void set_vga_mode13(void) {
    outb(0x3C2, 0x63);           /* Misc output */
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
    
    /* Sequencer */
    outb(0x3C4, 0x00); outb(0x3C5, 0x03);
    outb(0x3C4, 0x01); outb(0x3C5, 0x01);
    outb(0x3C4, 0x02); outb(0x3C5, 0x0F);
    outb(0x3C4, 0x03); outb(0x3C5, 0x00);
    outb(0x3C4, 0x04); outb(0x3C5, 0x0E);
    
    /* Graphics Controller */
    outb(0x3CE, 0x00); outb(0x3CF, 0x00);
    outb(0x3CE, 0x01); outb(0x3CF, 0x00);
    outb(0x3CE, 0x02); outb(0x3CF, 0x00);
    outb(0x3CE, 0x03); outb(0x3CF, 0x00);
    outb(0x3CE, 0x04); outb(0x3CF, 0x00);
    outb(0x3CE, 0x05); outb(0x3CF, 0x40);
    outb(0x3CE, 0x06); outb(0x3CF, 0x05);
    outb(0x3CE, 0x07); outb(0x3CF, 0x0F);
    outb(0x3CE, 0x08); outb(0x3CF, 0xFF);
    
    /* Attribute Controller */
    inb(0x3DA);
    outb(0x3C0, 0x30); outb(0x3C0, 0x41);
    outb(0x3C0, 0x33); outb(0x3C0, 0x00);
    outb(0x3C0, 0x20);
    
    fb = (uint8_t*)0xA0000;
    pitch = 320;
    width = 320;
    height = 200;
}

/* Set VGA Palette */
static void set_palette(void) {
    outb(0x3C8, 0);
    /* Standard VGA 16-color palette */
    static const uint8_t palette[16][3] = {
        {0x00,0x00,0x00}, {0x00,0x00,0x2A}, {0x00,0x2A,0x00}, {0x00,0x2A,0x2A},
        {0x2A,0x00,0x00}, {0x2A,0x00,0x2A}, {0x2A,0x15,0x00}, {0x2A,0x2A,0x2A},
        {0x15,0x15,0x15}, {0x15,0x15,0x3F}, {0x15,0x3F,0x15}, {0x15,0x3F,0x3F},
        {0x3F,0x15,0x15}, {0x3F,0x15,0x3F}, {0x3F,0x3F,0x15}, {0x3F,0x3F,0x3F}
    };
    for (int i = 0; i < 16; i++) {
        outb(0x3C9, palette[i][0] / 4);
        outb(0x3C9, palette[i][1] / 4);
        outb(0x3C9, palette[i][2] / 4);
    }
    /* Fill remaining 240 colors with a gradient */
    for (int i = 16; i < 256; i++) {
        outb(0x3C9, (i * 63) / 255);  /* R */
        outb(0x3C9, 0);               /* G */
        outb(0x3C9, 63 - (i * 63) / 255); /* B */
    }
}

/* Draw a single pixel */
static inline void putpixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        fb[y * pitch + x] = color;
    }
}

/* Main kernel */
void kernel_main(uint32_t magic, uint32_t addr) {
    uint32_t *mbi = (uint32_t *)addr;
    uint32_t flags = *mbi;
    
    fb = 0;
    
    /* Try to get VESA framebuffer from GRUB */
    if ((flags & (1 << 11)) && *(mbi + 22) != 0) {
        fb = (uint8_t *)(uint32_t)*(mbi + 22);
        pitch = *(mbi + 24);
        width = *(mbi + 25);
        height = *(mbi + 26);
        
        /* Verify it's 8-bit color */
        if (width == 0 || height == 0 || *(mbi + 27) != 8) {
            fb = 0;
        }
    }
    
    /* Fallback to VGA Mode 13h */
    if (!fb) {
        set_vga_mode13();
    }
    
    set_palette();
    
    /* Fill screen with RGB stripes */
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (x < width / 3) {
                putpixel(x, y, 4);  /* RED - first third */
            } else if (x < (width * 2) / 3) {
                putpixel(x, y, 2);  /* GREEN - middle third */
            } else {
                putpixel(x, y, 1);  /* BLUE - last third */
            }
        }
    }
    
    /* Halt forever */
    while (1) {
        __asm__ volatile("hlt");
    }
}
