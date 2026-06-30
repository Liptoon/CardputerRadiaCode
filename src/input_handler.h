#pragma once

// Returns the key pressed, or 0 if none.
// Special keys are mapped:
//   '\n'  = Enter
//   '\b'  = Backspace
//   '\t'  = Tab
//   ' '   = Space
char inputGetKey();
void inputInit();
