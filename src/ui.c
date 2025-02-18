#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

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
    printf("The framebuffer device was opened successfully.\n");

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

    printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    mmap_fb0 = (uint8_t *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb0, 0);
    if ((intptr_t)mmap_fb0 == -1)
    {
        perror("Error: failed to map framebuffer device to memory");
        exit(4);
    }
    printf("The framebuffer device was mapped to memory successfully.\n");
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

void ui_draw_progress(int progress)
{
    if (progress < 0 || progress > 100)
    {
        fprintf(stderr, "Progress value must be between 0 and 100\n");
        return;
    }

    // 计算进度条的位置和大小
    int bar_width = vinfo.xres / 2;
    int bar_height = vinfo.yres / 10;
    int bar_x = (vinfo.xres - bar_width) / 2;
    int bar_y = (vinfo.yres - bar_height) / 2;

    // 计算进度条的结束位置
    int progress_width = (bar_width * progress) / 100;
    int progress_x_end = bar_x + progress_width;

    // 定义颜色
    int green_color = color_transform(0, 255, 0);

    // 绘制背景矩形
    struct POSITION start_bg = {bar_x, bar_y};
    struct POSITION end_bg = {bar_x + bar_width, bar_y + bar_height};
    fb_draw_rect(start_bg, end_bg, color_transform(100, 100, 100)); // 灰色背景

    // 绘制进度条矩形
    struct POSITION start_progress = {bar_x, bar_y};
    struct POSITION end_progress = {progress_x_end, bar_y + bar_height};
    fb_draw_rect(start_progress, end_progress, green_color);
}
