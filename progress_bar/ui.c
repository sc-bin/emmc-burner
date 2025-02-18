#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "ui.h"

// 定义进度条的颜色、位置和宽度
#define PROGRESS_COLOR color_transform(0, 255, 0)
#define PROGRESS_COLOR_DONE color_transform(255, 0, 0)
#define PROGRESS_BG_COLOR color_transform(0, 50, 0)
#define PROGRESS_WIDTH (vinfo.xres * 7 / 10)
#define PROGRESS_HEIGHT (vinfo.yres / 10)
#define PROGRESS_X ((vinfo.xres - PROGRESS_WIDTH) / 2)
#define PROGRESS_Y ((vinfo.yres - PROGRESS_HEIGHT) / 2)

// 定义logo的颜色、位置和字体大小
#define LOGO_STR "W A L N U T    P I"
#define LOGO_COLOR color_transform(100, 100, 100)
#define LOGO_X (PROGRESS_X + (PROGRESS_WIDTH / 4))
#define LOGO_Y (PROGRESS_Y - (PROGRESS_HEIGHT / 2))
#define LOGO_FONT_SIZE vinfo.xres / 10

static int fd_fb0;
static uint8_t *mmap_fb0;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static long int screensize;

struct POSITION
{
    int x;
    int y;
};

int color_transform(int r, int g, int b)
{
    int alpha = 255; // 默认设置为不透明
    switch (vinfo.bits_per_pixel)
    {
    case 32:
        return (alpha << 24) | (r << 16) | (g << 8) | b;
    case 16:
        return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    default:
        fprintf(stderr, "Unsupported color format: %d bits per pixel\n", vinfo.bits_per_pixel);
        return 0;
    }
}
// 打开fb0节点，并使用mmap映射fb0
void ui_init()
{
    fd_fb0 = open("/dev/fb0", O_RDWR);
    if (fd_fb0 == -1)
    {
        perror("Error: cannot open framebuffer device");
        exit(1);
    }

    if (ioctl(fd_fb0, FBIOGET_FSCREENINFO, &finfo) == -1)
    {
        perror("Error reading fixed information");
        exit(2);
    }

    if (ioctl(fd_fb0, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
        perror("Error reading variable information");
        exit(3);
    }

    // printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    mmap_fb0 = (uint8_t *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb0, 0);
    if ((intptr_t)mmap_fb0 == -1)
    {
        perror("Error: failed to map framebuffer device to memory");
        exit(4);
    }
}

// 设置指定位置的像素颜色
void fb_set_pixel(int x, int y, int color)
{
    if (x < 0 || x >= vinfo.xres || y < 0 || y >= vinfo.yres)
        return;

    long int location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) +
                        (y + vinfo.yoffset) * finfo.line_length;

    switch (vinfo.bits_per_pixel)
    {
    case 32:
        *((uint32_t *)(mmap_fb0 + location)) = color;
        break;
    case 16:
        *((uint16_t *)(mmap_fb0 + location)) = color;
        break;
    default:
        fprintf(stderr, "Unsupported color format: %d bits per pixel\n", vinfo.bits_per_pixel);
        break;
    }
}
// 在已打开的mmap_fb0内绘制直线
static void fb_draw_line(struct POSITION start, struct POSITION end, int color, int width)
{
    int x0 = start.x, y0 = start.y;
    int x1 = end.x, y1 = end.y;
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    int half_width = width / 2;

    while (1)
    {
        for (int w = -half_width; w <= half_width; w++)
        {
            for (int h = -half_width; h <= half_width; h++)
            {
                long int location = ((x0 + w + vinfo.xoffset) * (vinfo.bits_per_pixel / 8)) +
                                    ((y0 + h + vinfo.yoffset) * finfo.line_length);
                switch (vinfo.bits_per_pixel)
                {
                case 32:
                    *((uint32_t *)(mmap_fb0 + location)) = color;
                    break;
                case 16:
                    *((uint16_t *)(mmap_fb0 + location)) = color;
                    break;
                default:
                    fprintf(stderr, "Unsupported color format: %d bits per pixel\n", vinfo.bits_per_pixel);
                    break;
                }
            }
        }
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

static void fb_draw_rect(struct POSITION start, struct POSITION end, int color)
{
    int xoffset = vinfo.xoffset;
    int yoffset = vinfo.yoffset;
    int bytes_per_pixel = vinfo.bits_per_pixel / 8;
    int line_length = finfo.line_length;

    for (int x = start.x; x <= end.x; x++)
    {
        for (int y = start.y; y <= end.y; y++)
        {
            long int location = ((x + xoffset) * bytes_per_pixel) + ((y + yoffset) * line_length);
            switch (vinfo.bits_per_pixel)
            {
            case 32:
                *((uint32_t *)(mmap_fb0 + location)) = color;
                break;
            case 16:
                *((uint16_t *)(mmap_fb0 + location)) = color;
                break;
            default:
                fprintf(stderr, "Unsupported color format: %d bits per pixel\n", vinfo.bits_per_pixel);
                break;
            }
        }
    }
}

void render_text(FT_Face face, const char *text, int x, int y, unsigned char *buffer, int width, int height)
{
    FT_GlyphSlot slot = face->glyph;
    int pen_x = x;
    int pen_y = y;

    for (const char *p = text; *p; p++)
    {
        FT_UInt glyph_index = FT_Get_Char_Index(face, *p);
        FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER);
        FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);

        int target_x = pen_x + slot->bitmap_left;
        int target_y = pen_y - slot->bitmap_top;

        for (int j = 0; j < slot->bitmap.rows; j++)
        {
            for (int i = 0; i < slot->bitmap.width; i++)
            {
                if (target_x + i >= 0 && target_x + i < width && target_y + j >= 0 && target_y + j < height)
                {
                    int index = (target_y + j) * width + target_x + i;
                    buffer[index] = slot->bitmap.buffer[j * slot->bitmap.width + i];
                }
            }
        }

        pen_x += slot->advance.x >> 6;
    }
}

void fb_place_string(const char *text, int x, int y, int font_size, int color)
{
    // 打开字体文件
    FT_Library library;
    if (FT_Init_FreeType(&library))
    {
        fprintf(stderr, "Could not init FreeType Library\n");
        return;
    }
    FT_Face face;
    if (FT_New_Face(library, "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 0, &face))
    {
        fprintf(stderr, "Could not open font\n");
        FT_Done_FreeType(library);
        return;
    }
    // 设置字体大小
    FT_Set_Char_Size(face, font_size * 64, 0, 30, 0);
    // 创建一个缓冲区来存储渲染的文字
    unsigned char *text_buffer = (unsigned char *)malloc(vinfo.xres * vinfo.yres);
    memset(text_buffer, 0, vinfo.xres * vinfo.yres);

    // 渲染文字
    render_text(face, text, x, y, text_buffer, vinfo.xres, vinfo.yres);
    // 将渲染的文字写入帧缓冲区
    for (int y = 0; y < vinfo.yres; y++)
    {
        for (int x = 0; x < vinfo.xres; x++)
        {
            int location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) +
                           (y + vinfo.yoffset) * finfo.line_length;

            if (text_buffer[y * vinfo.xres + x])
            {
                int r = (color >> 16) & 0xFF;
                int g = (color >> 8) & 0xFF;
                int b = color & 0xFF;
                *(mmap_fb0 + location) = b;     // B
                *(mmap_fb0 + location + 1) = g; // G
                *(mmap_fb0 + location + 2) = r; // R
            }
        }
    }
    free(text_buffer);
}

void ui_draw_progress(int progress)
{
    if (progress < 0 || progress > 100)
    {
        fprintf(stderr, "Progress value must be between 0 and 100\n");
        return;
    }

    int progress_width = (PROGRESS_WIDTH * progress) / 100;
    int progress_x_end = PROGRESS_X + progress_width;

    struct POSITION start_bg = {PROGRESS_X, PROGRESS_Y};
    struct POSITION end_bg = {PROGRESS_X + PROGRESS_WIDTH, PROGRESS_Y + PROGRESS_HEIGHT};
    fb_draw_rect(start_bg, end_bg, PROGRESS_BG_COLOR); // 灰色背景

    // 绘制进度条矩形
    struct POSITION start_progress = {PROGRESS_X, PROGRESS_Y};
    struct POSITION end_progress = {progress_x_end, PROGRESS_Y + PROGRESS_HEIGHT};
    if (progress < 100)
        fb_draw_rect(start_progress, end_progress, PROGRESS_COLOR);
    else
        fb_draw_rect(start_progress, end_progress, PROGRESS_COLOR_DONE);
}

void draw_logo()
{
    fb_place_string(LOGO_STR, LOGO_X, LOGO_Y, LOGO_FONT_SIZE, LOGO_COLOR);
}
