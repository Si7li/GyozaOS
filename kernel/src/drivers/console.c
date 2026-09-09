#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <drivers/console.h>
#include <drivers/graphics.h>
#include <drivers/font.h>

#define DEFAULT_FG 0x00FFFFFF
#define DEFAULT_BG 0x00000000
#define TAB_WIDTH  8
#define ANSI_MAX_PARAMS 8
#define DEFAULT_FG_INDEX 7
#define DEFAULT_BG_INDEX 0


// draw_glyph indexes font8x16[] with an unsigned char, so the font must have
// a glyph for every possible byte value. Checked here at compile time rather
// than per-character at runtime.
_Static_assert(FONT_GLYPHS == 256, "font must cover all 256 byte values");

// Everything about the screen lives here and nowhere else. No other module
// gets to touch a pixel.

static size_t console_cols;
static size_t console_rows;
static size_t cur_col;
static size_t cur_row;

static uint32_t fg_color;
static uint32_t bg_color;

typedef enum{
    STATE_NORMAL, // drawing characters
    STATE_ESCAPE, // saw 0x1B, expecting [
    STATE_CSI // collection parameters until a final byte
} ansi_state_t;

static const uint32_t ansi_palette[16] = {
    0x00000000, 0x00AA0000, 0x0000AA00, 0x00AA5500,
    0x000000AA, 0x00AA00AA, 0x0000AAAA, 0x00AAAAAA,
    0x00555555, 0x00FF5555, 0x0055FF55, 0x00FFFF55,
    0x005555FF, 0x00FF55FF, 0x0055FFFF, 0x00FFFFFF,
};

static ansi_state_t ansi_state = STATE_NORMAL;
static uint32_t ansi_params[ANSI_MAX_PARAMS];
static size_t ansi_param_count;
static bool ansi_param_present; // "ESC[m" != "ESC[0m"

// SGR attribute state.
static uint8_t sgr_fg_index;
static uint8_t sgr_bg_index;
static bool sgr_bright; // Bright is a modifier (SGR 1), not a colout do it is tracked seperately and added to the foreground index when set



// col/row are in character cells, not piwxels.
static void draw_glyph(unsigned char c, size_t col, size_t row){
    const uint8_t *glyph = font8x16[c];
    size_t x0 = col * FONT_WIDTH;
    size_t y0 = row * FONT_HEIGHT;

    for(size_t gy = 0; gy < FONT_HEIGHT; gy++){
        uint8_t bits = glyph[gy];
        for (size_t gx = 0; gx < FONT_WIDTH; gx++){
            uint32_t color = (bits & (0x80 >> gx)) ? fg_color : bg_color;
            draw_pixel(x0 + gx, y0 + gy, color);
        }
    }
}

static void scroll(void) {
    graphics_copy_rect(0, 0, 0, FONT_HEIGHT,
                       graphics_width(), graphics_height() - FONT_HEIGHT);
    fill_rect(0, (console_rows - 1) * FONT_HEIGHT,
              graphics_width(), FONT_HEIGHT, bg_color);
}

void console_clear(void){
    fill_rect(0, 0, graphics_width(), graphics_height(), bg_color);
    cur_col = 0;
    cur_row = 0;
}

void console_set_color(uint32_t fg, uint32_t bg) {
    fg_color = fg;
    bg_color = bg;
}
// ANSI COLOR ALGORITHM
// ANSI reset
static void ansi_reset(void){
    for(size_t i = 0; i < ANSI_MAX_PARAMS; i++){
        ansi_params[i] = 0;
    }
    ansi_param_count = 0;
    ansi_param_present = false;
}

static uint32_t ansi_param(size_t n, uint32_t fallback){
    return (n < ansi_param_count) ? ansi_params[n] : fallback;
}

static void sgr_reset(void){
    sgr_fg_index = DEFAULT_FG_INDEX;
    sgr_bg_index = DEFAULT_BG_INDEX;
    sgr_bright = false;
}

static void sgr_apply(void){
    uint8_t fg = sgr_bright ? (sgr_fg_index | 8) : sgr_fg_index;
    console_set_color(ansi_palette[fg & 15], ansi_palette[sgr_bg_index & 15]);
}

static void csi_sgr(void){
    if(ansi_param_count == 0){
        sgr_reset();
        sgr_apply();
        return;
    }

    for(size_t i = 0; i < ansi_param_count; i++){
        uint32_t p = ansi_params[i];

        if(p == 0) { sgr_reset(); }
        else if (p == 1) { sgr_bright = true; }
        else if (p == 22) { sgr_bright = false; }
        else if (p >= 30 && p <= 37) { sgr_fg_index = (uint8_t)(p - 30); }
        else if (p == 39) { sgr_fg_index = DEFAULT_FG_INDEX; }
        else if (p >= 40 && p <= 47) { sgr_bg_index = (uint8_t)(p - 40); }
        else if (p == 49) { sgr_bg_index = DEFAULT_BG_INDEX; }
        else if (p >= 90 && p <= 97) { sgr_fg_index = (uint8_t)(p -90 +8); }
        else if (p >= 100 && p <= 107) { sgr_bg_index = (uint8_t)(p -100 + 8); }
    }
    sgr_apply();
}

 static void erase_cells(size_t row, size_t col_from, size_t col_to){
    if(col_from >= col_to || row >= console_rows)
        return;
    if(col_to > console_cols)
        col_to = console_cols;
    fill_rect(col_from * FONT_WIDTH, row * FONT_HEIGHT, (col_to - col_from) * FONT_WIDTH, FONT_HEIGHT, bg_color);
 }

 // 'K' -- erase in line the cursor doet not move
 static void csi_erase_line(void){
    switch (ansi_param(0,0))
    {
    case 0:
        erase_cells(cur_row, cur_col, console_cols);
        break;
    
    case 1:
        erase_cells(cur_row, 0, cur_col + 1);
        break;
    case 2:
        erase_cells(cur_row, 0, console_cols);
        break;        
    default:
        break;
    }
 }

 // 'J' -- erase in display
 static void csi_erase_discplay(void){
    switch(ansi_param(0,0)){
        case 0:
            erase_cells(cur_row, cur_col, console_cols);
            for(size_t r = cur_row + 1; r < console_rows; r++)
                erase_cells(r,0, console_cols);
            break;
        case 1:
            for(size_t r = 0; r < cur_row; r++)
                erase_cells(r, 0, console_cols);
            break;
        default:
            fill_rect(0,0,graphics_width(), graphics_height(), bg_color);
            break;
    }
 }

 // 'H' and 'f' -- absolute position, 1-based on the wire, 0-based internally
 static void csi_cursor_position(void){
    uint32_t row = ansi_param(0,1);
    uint32_t col = ansi_param(1,1);

    if(row == 0) { row = 1;}
    if(col == 0) { col = 1;}

    cur_row = row - 1;
    cur_col = col - 1;

    if(cur_row >= console_rows) { cur_row = console_rows - 1; }
    if(cur_col >= console_cols) { cur_col = console_cols - 1; }

}

// 'A' : move cursor up
// 'B' : move cursor down
// 'C' : move cursor forward
// 'D' : move cursor backward
static void csi_cursor_move(unsigned char final){
    uint32_t n = ansi_param(0,1);
    if(n==0) { n = 1; }

    switch(final){
    case 'A':
        cur_row = (n > cur_row) ? 0 : cur_row - n;
        break;
    case 'B':
        cur_row += n;
        if(cur_row >= console_rows) { cur_row = console_rows - 1; }
        break;
    case 'C':
        cur_col += n;
        if(cur_col >= console_cols) { cur_col = console_cols - 1; }
        break;
    case 'D':
        cur_col = (n > cur_col) ? 0 : cur_col - n;
        break;
    default:
        break;
    }
}

static void csi_execute(unsigned char final){
    switch(final){
    case 'm':
        csi_sgr();
        break;
    case 'J':
        csi_erase_discplay();
        break;
    case 'K':
        csi_erase_line();
        break;
    case 'H':
    case 'f':
        csi_cursor_position();
        break;
    case 'A':
    case 'B':
    case 'C':
    case 'D':
        csi_cursor_move(final);
        break;
    default:
        break;
    }
}

static bool ansi_feed(unsigned char c){
    switch(ansi_state){
    case STATE_NORMAL:
        if(c == 0x1B){
            ansi_reset();
            ansi_state = STATE_ESCAPE;
            return true;
        }
        return false;
    case STATE_ESCAPE:
        if(c == '['){
            ansi_state = STATE_CSI;
        }
        else if (c == 0x1B){
            ansi_reset(); // interrupted output: restart
        }
        else{
            ansi_state = STATE_NORMAL;
        }
        return true;
    
    case STATE_CSI:
        if(c == 0x1B){
            ansi_reset();
            ansi_state = STATE_ESCAPE;
            return true;
        }
        if(c >= '0' && c <= '9'){
            ansi_param_present = true;
            if(ansi_param_count < ANSI_MAX_PARAMS){
                uint32_t v = ansi_params[ansi_param_count] * 10 + (uint32_t)(c - '0');
                ansi_params[ansi_param_count] = (v > 9999) ? 9999 : v;
            }
            return true;
        }
        if(c == ';'){
            ansi_param_present = true;
            if(ansi_param_count < ANSI_MAX_PARAMS){
                ansi_param_count++;
            }
            return true;
        }
        if(c >= 0x20 && c <= 0x2F){
            return true;
        }
        if(c >= 0x40 && c <= 0x7E){
            if(ansi_param_present && ansi_param_count < ANSI_MAX_PARAMS){
                ansi_param_count++;
            }
            csi_execute(c);
            ansi_state = STATE_NORMAL;
            return true;
        }
        ansi_state = STATE_NORMAL;
        return true;
    }
    return false;
}
void console_putc(char ch) {
    unsigned char c = (unsigned char)ch;
    
    if(ansi_feed(c)){
        return;
    }

    switch (c) {
    case '\n':
        cur_col = 0;
        cur_row++;
        break;

    case '\r':
        cur_col = 0;
        break;

    case '\b':
        if (cur_col > 0) {
            cur_col--;
            draw_glyph(' ', cur_col, cur_row);
        }
        break;

    case '\t':
        // Advance to the next tab stop. Terminates even if the line wraps,
        // because cur_col becomes 0 and 0 % TAB_WIDTH == 0.
        do {
            console_putc(' ');
        } while (cur_col % TAB_WIDTH != 0);
        break;

    default:
        if (c < 0x20 || c == 0x7F) {
            return;
        }
        draw_glyph(c, cur_col, cur_row);
        cur_col++;
        break;
    }

    if (cur_col >= console_cols) {
        cur_col = 0;
        cur_row++;
    }

    while (cur_row >= console_rows) {
        scroll();
        cur_row--;
    }
}

void console_init(){
    console_cols = graphics_width() / FONT_WIDTH;
    console_rows = graphics_height() / FONT_HEIGHT;

    ansi_reset();
    ansi_state = STATE_NORMAL;
    sgr_reset();
    sgr_apply();

    console_clear(); 
}

