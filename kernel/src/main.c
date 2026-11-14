#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

#include <lib/printf.h>

#include <drivers/serial.h>
#include <drivers/font.h>
#include <drivers/graphics.h>
#include <drivers/console.h>

#include <arch/x86_64/gdt.h>
#include <arch/x86_64/idt.h>
#include <arch/x86_64/pic.h>
#include <arch/x86_64/cpu.h>
#include <drivers/pit.h>
#include <drivers/keyboard.h>

// Set the base revision to 6, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

// Halt and catch fire function.
static void hcf(void) {
    for (;;) {
        asm ("hlt");
    }
}

// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.
void kmain(void) {
    //Serial Init
    serial_init();
    kprintf_add_sink(serial_putc);
    kprintf("serial: loaded\n");
    gdt_init();
    kprintf("gdt: loaded\n");
    idt_init();
    kprintf("idt: loaded\n");
    pic_init();
    kprintf("pic: remapped\n");
    pit_init(100);
    kprintf("pit: 100 Hz\n");
    keyboard_init();
    kprintf("keyboard: ready\n");
    
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        serial_puts("kernel: framebuffer drawn, halting\n");
        hcf();
    }

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    // Fetch the first framebuffer.
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    graphics_init(framebuffer->address, framebuffer->width,
        framebuffer->height, framebuffer->pitch);
    kprintf("graphics: loaded\n");
    console_init();

    kprintf_add_sink(console_putc);
    
    // Color Debugging
    fill_rect(framebuffer->width/2 - 300,framebuffer->height/2 + 25,50,25,0xFFFF2E2E);
    fill_rect(framebuffer->width/2 - 250,framebuffer->height/2 + 25,50,25,0xFFFF5533);
    fill_rect(framebuffer->width/2 - 200,framebuffer->height/2 + 25,50,25,0xFFFFFF00);
    fill_rect(framebuffer->width/2 - 150,framebuffer->height/2 + 25,50,25,0xFF00D100);
    fill_rect(framebuffer->width/2 - 100,framebuffer->height/2 + 25,50,25,0xFF5C5CFF);
    fill_rect(framebuffer->width/2 -  50,framebuffer->height/2 + 25,50,25,0xFFD100D1);
    fill_rect(framebuffer->width/2      ,framebuffer->height/2 + 25,50,25,0xFFFF5C77);
    fill_rect(framebuffer->width/2 +  50,framebuffer->height/2 + 25,50,25,0xFFD45959);
    fill_rect(framebuffer->width/2 + 100,framebuffer->height/2 + 25,50,25,0xFF808080);
    fill_rect(framebuffer->width/2 + 150,framebuffer->height/2 + 25,50,25,0xFF0D0D0D);


    kprintf("console: loaded\n");
 
    kprintf("\e[0;30mBlack \e[1;30mbold Black \e[0;90mhigh intensity Black\n");
    kprintf("\e[0;31mRed \e[1;31mbold Red \e[0;91mhigh intensity Red\n");
    kprintf("\e[0;32mGreen \e[1;32mbold Green \e[0;92mhigh intensity Green\n");
    kprintf("\e[0;33mYellow \e[1;33mbold Yellow \e[0;93mhigh intensity Yellow\n");
    kprintf("\e[0;34mBlue \e[1;34mbold Blue \e[0;94mhigh intensity Blue\n");
    kprintf("\e[0;35mPurple \e[1;35mbold Purple \e[0;95mhigh intensity Purple\n");
    kprintf("\e[0;36mCyan \e[1;36mbold Cyan \e[0;96mhigh intensity Cyan\n");
    kprintf("\e[0;37mWhite \e[1;37mbold White \e[0;97mhigh intensity White\n");
    kprintf("\e[97;40m Black BG \e[0m \e[30;100m bright Black BG \e[0m\n");
    kprintf("\e[97;41m Red BG \e[0m \e[30;101m bright Red BG \e[0m\n");
    kprintf("\e[97;42m Green BG \e[0m \e[30;102m bright Green BG \e[0m\n");
    kprintf("\e[97;43m Yellow BG \e[0m \e[30;103m bright Yellow BG \e[0m\n");
    kprintf("\e[97;44m Blue BG \e[0m \e[30;104m bright Blue BG \e[0m\n");
    kprintf("\e[97;45m Purple BG \e[0m \e[30;105m bright Purple BG \e[0m\n");
    kprintf("\e[97;46m Cyan BG \e[0m \e[30;106m bright Cyan BG \e[0m\n");
    kprintf("\e[97;47m White BG \e[0m \e[30;107m bright White BG \e[0m\n");
    kprintf("\n");
    kprintf("\e[93;41mbright yellow on red\e[0m normal again\n");
    kprintf("\e[44mblue to end of line\e[K\e[0m\n");
    kprintf("\e[93;41mboth set\e[49m bg reset\e[39m fg reset\e[0m\n");
    kprintf("\e[0m");
    // Deviding by 0 (Error Handling test)
    #if 0
    asm volatile("int3");
    volatile int zero = 0;
    volatile int x = 10 / zero;
    #endif
    sti();    // interrupts on: from here the timer ticks
     kprintf("type something:\n");
    for (;;) {
        int c = keyboard_getchar();
        if (c < 0) {
            hlt();          // nothing yet: sleep until the next interrupt
            continue;
        }
        kprintf("%c", c);
    }
    hcf();
}
