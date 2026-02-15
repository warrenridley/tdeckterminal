#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

// Special key codes
#define KEY_NONE        0
#define KEY_ENTER       '\n'
#define KEY_BACKSPACE   '\b'
#define KEY_TAB         '\t'
#define KEY_ESC         27
#define KEY_UP          128
#define KEY_DOWN        129
#define KEY_LEFT        130
#define KEY_RIGHT       131

class Keyboard {
public:
    Keyboard();
    void init();
    bool available();
    char read();
    void update();

private:
    bool _initialized;
    unsigned long _lastTrackball;
    void _initTrackball();
    char _checkTrackball();
};

#endif // KEYBOARD_H
