#ifndef KEYBOARD_H
#define KEYBOARD_H

void keyboard_init(void);       // start listening on IRQ 1
int  keyboard_getchar(void);    // next typed character, or -1 if none yet

#endif