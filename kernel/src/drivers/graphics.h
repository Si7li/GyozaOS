#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stddef.h>
#include <stdint.h>

size_t graphics_width(void);
size_t graphics_height(void);

void graphics_init(uint32_t *framebuffer, size_t width, size_t height, size_t pitch);

void draw_pixel(size_t x, size_t y, uint32_t color);

void fill_rect(size_t x, size_t y, size_t w, size_t h, uint32_t color);

void graphics_copy_rect(size_t dst_x, 
    size_t dst_y,size_t src_x, size_t src_y,
    size_t w, size_t h);

#endif