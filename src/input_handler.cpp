#include "input_handler.h"
#include <M5Cardputer.h>

void inputInit() {}

char inputGetKey() {
    M5Cardputer.update();

    if (!M5Cardputer.Keyboard.isChange()) {
        return 0;
    }

    if (!M5Cardputer.Keyboard.isPressed()) {
        return 0;
    }

    auto ks = M5Cardputer.Keyboard.keysState();

    // Priority order: special keys first
    if (ks.enter) return '\n';
    if (ks.del)   return '\b';
    if (ks.tab)   return '\t';
    if (ks.space) return ' ';

    // Printable characters (from framework keyPressed logic)
    if (!ks.word.empty()) {
        char c = ks.word[0];
        // Map common keys
        if (c == ',')  return ',';
        if (c == '.')  return '.';
        if (c == '[')  return '[';
        if (c == ']')  return ']';
        return c;
    }

    return 0;
}
