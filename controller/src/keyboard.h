#ifndef KEYBOARD_H
#define KEYBOARD_H

// Mode enumeration
typedef enum {
    MODE_OFF,
    MODE_HEAT,
    MODE_COOL,
    MODE_MATCH
} Mode;

void init_keypad(void);
char poll_keypad(void);
void init_responseLED(void);
void init_keyscan_timer(void);
void set_mode(Mode new_mode);

// Shared globals
extern Mode current_mode;
extern unsigned int last_mode_switch_time;

#endif
