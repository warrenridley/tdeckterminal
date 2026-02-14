#ifndef TERMINAL_H
#define TERMINAL_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.h"

class Terminal {
public:
    Terminal(TFT_eSPI& tft);

    void init();
    void clear();

    // Printing
    void print(const char* text);
    void print(const String& text);
    void println(const char* text = "");
    void println(const String& text);
    void printf(const char* fmt, ...);
    void printColored(const char* text, uint16_t color);
    void printlnColored(const char* text, uint16_t color);

    // Color control
    void setColor(uint16_t fg);
    void setColor(uint16_t fg, uint16_t bg);
    void resetColor();

    // Cursor & editing
    void setCursor(int col, int row);
    void showCursor(bool show);
    void backspace();
    void scrollUp(int lines = 1);
    void refresh();

    // Getters
    int getCursorCol() const { return _curCol; }
    int getCursorRow() const { return _curRow; }
    int getCols() const { return TERM_COLS; }
    int getRows() const { return TERM_ROWS; }

private:
    TFT_eSPI& _tft;
    char     _buffer[TERM_ROWS][TERM_COLS + 1];
    uint16_t _colorBuf[TERM_ROWS][TERM_COLS];
    bool     _dirty[TERM_ROWS];

    int      _curCol;
    int      _curRow;
    uint16_t _fgColor;
    uint16_t _bgColor;
    bool     _cursorVisible;
    bool     _cursorOn;
    unsigned long _lastBlink;

    void _putChar(char c);
    void _newLine();
    void _renderLine(int row);
    void _drawCursor();
    void _eraseCursor();
};

#endif // TERMINAL_H
