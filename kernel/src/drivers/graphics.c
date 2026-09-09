#include <drivers/graphics.h>
#include <lib/string.h>

static uint32_t *fb;
static size_t fb_width;
static size_t fb_height;
static size_t fb_pitch_px;

size_t graphics_width(){return fb_width;}
size_t graphics_height(){return fb_height;}


void graphics_init(uint32_t *framebuffer, size_t width, size_t height, size_t pitch){
    fb = framebuffer;
    fb_width = width;
    fb_height = height;
    fb_pitch_px = pitch / sizeof(uint32_t);

}

void draw_pixel(size_t x, size_t y, uint32_t color) {
    if(x >= fb_width || y >= fb_height){
        return;
    }
    fb[y * fb_pitch_px + x] = color;
}

void fill_rect(size_t x, size_t y, size_t w, size_t h, uint32_t color){
    for (size_t dy = 0; dy < h; dy++){
        for(size_t dx = 0; dx < w; dx++){
            draw_pixel(x + dx, y + dy, color);
        }
    }
}

void graphics_copy_rect(size_t dst_x, size_t dst_y,
                        size_t src_x, size_t src_y,
                        size_t w, size_t h) {
    if (src_x >= fb_width || dst_x >= fb_width
     || src_y >= fb_height || dst_y >= fb_height) {
        return;
    }

    // Clip against whichever rectangle runs off the edge first. draw_pixel
    // is bypassed here, so the bounds checking has to happen once, up front.
    size_t max_x = (src_x > dst_x) ? src_x : dst_x;
    size_t max_y = (src_y > dst_y) ? src_y : dst_y;
    if (w > fb_width - max_x)   { w = fb_width - max_x; }
    if (h > fb_height - max_y)  { h = fb_height - max_y; }

    size_t row_bytes = w * sizeof(uint32_t);

    if (dst_y > src_y) {
        // Destination is below the source, so the rows overlap going down.
        // Copy bottom-to-top or the first row gets smeared over the rest.
        for (size_t dy = h; dy-- > 0; ) {
            memmove(&fb[(dst_y + dy) * fb_pitch_px + dst_x],
                    &fb[(src_y + dy) * fb_pitch_px + src_x], row_bytes);
        }
    } else {
        for (size_t dy = 0; dy < h; dy++) {
            memmove(&fb[(dst_y + dy) * fb_pitch_px + dst_x],
                    &fb[(src_y + dy) * fb_pitch_px + src_x], row_bytes);
        }
    }
}
