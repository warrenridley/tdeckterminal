#include "keyboard.h"

Keyboard::Keyboard() {
    _initialized = false;
    _lastTrackball = 0;
}

void Keyboard::init() {
    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    Wire.setClock(400000);

    pinMode(KB_INT_PIN, INPUT_PULLUP);
    _initTrackball();
    _initialized = true;
}

void Keyboard::_initTrackball() {
    pinMode(TRACKBALL_UP_PIN, INPUT_PULLUP);
    pinMode(TRACKBALL_DOWN_PIN, INPUT_PULLUP);
    pinMode(TRACKBALL_LEFT_PIN, INPUT_PULLUP);
    pinMode(TRACKBALL_RIGHT_PIN, INPUT_PULLUP);
    pinMode(TRACKBALL_CLICK_PIN, INPUT_PULLUP);
}

bool Keyboard::available() {
    if (!_initialized) return false;

    // Check hardware keyboard interrupt line
    if (digitalRead(KB_INT_PIN) == LOW) return true;

    // Check trackball
    if (_checkTrackball() != KEY_NONE) return true;

    return false;
}

char Keyboard::read() {
    if (!_initialized) return KEY_NONE;

    // Try hardware keyboard first
    if (digitalRead(KB_INT_PIN) == LOW) {
        Wire.requestFrom((uint8_t)KB_I2C_ADDR, (uint8_t)1);
        if (Wire.available()) {
            char c = Wire.read();
            if (c != 0) return c;
        }
    }

    // Try trackball
    char tk = _checkTrackball();
    if (tk != KEY_NONE) return tk;

    return KEY_NONE;
}

char Keyboard::_checkTrackball() {
    unsigned long now = millis();
    if (now - _lastTrackball < 150) return KEY_NONE;  // Debounce

    if (digitalRead(TRACKBALL_UP_PIN) == LOW) {
        _lastTrackball = now;
        return KEY_UP;
    }
    if (digitalRead(TRACKBALL_DOWN_PIN) == LOW) {
        _lastTrackball = now;
        return KEY_DOWN;
    }
    if (digitalRead(TRACKBALL_LEFT_PIN) == LOW) {
        _lastTrackball = now;
        return KEY_LEFT;
    }
    if (digitalRead(TRACKBALL_RIGHT_PIN) == LOW) {
        _lastTrackball = now;
        return KEY_RIGHT;
    }
    if (digitalRead(TRACKBALL_CLICK_PIN) == LOW) {
        _lastTrackball = now;
        return KEY_ENTER;
    }

    return KEY_NONE;
}

void Keyboard::update() {
    // Reserved for future async keyboard handling
}
