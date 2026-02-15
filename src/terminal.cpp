#include "terminal.h"
#include <stdarg.h>

Terminal::Terminal(TFT_eSPI& tft) : _tft(tft) {
    _curCol = 0;
    _curRow = 0;
    _fgColor = COLOR_FG;
    _bgColor = COLOR_BG;
    _cursorVisible = true;
    _cursorOn = false;
    _lastBlink = 0;
}

void Terminal::init() {
    _tft.init();
    _tft.setRotation(1);               // Landscape 320x240
    _tft.fillScreen(COLOR_BG);
    _tft.setTextFont(1);               // GLCD 6x8 monospace
    _tft.setTextSize(1);
    _tft.setTextColor(COLOR_FG, COLOR_BG);
    clear();
}

void Terminal::clear() {
    for (int r = 0; r < TERM_ROWS; r++) {
        memset(_buffer[r], ' ', TERM_COLS);
        _buffer[r][TERM_COLS] = '\0';
        for (int c = 0; c < TERM_COLS; c++) {
            _colorBuf[r][c] = _fgColor;
        }
        _dirty[r] = true;
    }
    _curCol = 0;
    _curRow = 0;
    _tft.fillScreen(COLOR_BG);
}

// ---- Printing ----

void Terminal::print(const char* text) {
    while (*text) {
        _putChar(*text++);
    }
    refresh();
}

void Terminal::print(const String& text) {
    print(text.c_str());
}

void Terminal::println(const char* text) {
    print(text);
    _newLine();
    refresh();
}

void Terminal::println(const String& text) {
    println(text.c_str());
}

void Terminal::printf(const char* fmt, ...) {
    char buf[PRINTF_BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    print(buf);
}

void Terminal::printColored(const char* text, uint16_t color) {
    uint16_t prev = _fgColor;
    _fgColor = color;
    print(text);
    _fgColor = prev;
}

void Terminal::printlnColored(const char* text, uint16_t color) {
    uint16_t prev = _fgColor;
    _fgColor = color;
    println(text);
    _fgColor = prev;
}

// ---- Color ----

void Terminal::setColor(uint16_t fg) {
    _fgColor = fg;
}

void Terminal::setColor(uint16_t fg, uint16_t bg) {
    _fgColor = fg;
    _bgColor = bg;
}

void Terminal::resetColor() {
    _fgColor = COLOR_FG;
    _bgColor = COLOR_BG;
}

// ---- Cursor & Editing ----

void Terminal::setCursor(int col, int row) {
    _curCol = constrain(col, 0, TERM_COLS - 1);
    _curRow = constrain(row, 0, TERM_ROWS - 1);
}

void Terminal::showCursor(bool show) {
    _cursorVisible = show;
}

void Terminal::backspace() {
    if (_curCol > 0) {
        _curCol--;
        _buffer[_curRow][_curCol] = ' ';
        _colorBuf[_curRow][_curCol] = _fgColor;
        _dirty[_curRow] = true;
        refresh();
    }
}

void Terminal::scrollUp(int lines) {
    for (int i = 0; i < lines; i++) {
        for (int r = 0; r < TERM_ROWS - 1; r++) {
            memcpy(_buffer[r], _buffer[r + 1], TERM_COLS + 1);
            memcpy(_colorBuf[r], _colorBuf[r + 1], TERM_COLS * sizeof(uint16_t));
            _dirty[r] = true;
        }
        // Clear bottom line
        memset(_buffer[TERM_ROWS - 1], ' ', TERM_COLS);
        _buffer[TERM_ROWS - 1][TERM_COLS] = '\0';
        for (int c = 0; c < TERM_COLS; c++) {
            _colorBuf[TERM_ROWS - 1][c] = _fgColor;
        }
        _dirty[TERM_ROWS - 1] = true;
    }
    refresh();
}

// ---- Internal ----

void Terminal::_putChar(char c) {
    if (c == '\n') { _newLine(); return; }
    if (c == '\r') { _curCol = 0; return; }
    if (c == '\t') {
        int spaces = 4 - (_curCol % 4);
        for (int i = 0; i < spaces; i++) _putChar(' ');
        return;
    }
    if (c == '\b') { backspace(); return; }

    if (_curCol >= TERM_COLS) {
        _newLine();
    }

    _buffer[_curRow][_curCol] = c;
    _colorBuf[_curRow][_curCol] = _fgColor;
    _dirty[_curRow] = true;
    _curCol++;
}

void Terminal::_newLine() {
    _curCol = 0;
    _curRow++;
    if (_curRow >= TERM_ROWS) {
        _curRow = TERM_ROWS - 1;
        scrollUp(1);
    }
}

void Terminal::_renderLine(int row) {
    int y = row * TERM_FONT_H;
    _tft.fillRect(0, y, SCREEN_WIDTH, TERM_FONT_H, _bgColor);

    for (int c = 0; c < TERM_COLS; c++) {
        if (_buffer[row][c] != ' ') {
            _tft.drawChar(c * TERM_FONT_W, y,
                          _buffer[row][c],
                          _colorBuf[row][c], _bgColor, 1);
        }
    }
    _dirty[row] = false;
}

void Terminal::refresh() {
    // Render dirty lines
    for (int r = 0; r < TERM_ROWS; r++) {
        if (_dirty[r]) {
            _renderLine(r);
        }
    }
    // Blink cursor
    if (_cursorVisible) {
        unsigned long now = millis();
        if (now - _lastBlink > 500) {
            _cursorOn = !_cursorOn;
            _lastBlink = now;
            if (_cursorOn) _drawCursor();
            else           _eraseCursor();
        }
    }
}

void Terminal::_drawCursor() {
    int x = _curCol * TERM_FONT_W;
    int y = _curRow * TERM_FONT_H;
    _tft.fillRect(x, y + TERM_FONT_H - 2, TERM_FONT_W, 2, _fgColor);
}

void Terminal::_eraseCursor() {
    int x = _curCol * TERM_FONT_W;
    int y = _curRow * TERM_FONT_H;
    _tft.fillRect(x, y + TERM_FONT_H - 2, TERM_FONT_W, 2, _bgColor);
    // Redraw character if present
    char ch = _buffer[_curRow][_curCol];
    if (ch != ' ') {
        _tft.drawChar(x, _curRow * TERM_FONT_H, ch,
                       _colorBuf[_curRow][_curCol], _bgColor, 1);
    }
}
