#include "raylib.h"
#include "raygui.h"

#include "constants.h"

static char text[10][MAX_TEXT_LENGTH] = {0};
static int lineCount = 1; // TODO: Make line count dynamic
static int cursorPos = 0;

void InsertNewLine(void) {
    if(IsKeyPressed(KEY_ENTER)) {
        lineCount++;
        cursorPos++;
    }
}

void CursorTracker(void) {
    if(IsKeyPressed(KEY_DOWN) && cursorPos < lineCount) {
        cursorPos++;
    }
    if(IsKeyPressed(KEY_UP) && cursorPos >= 0) {
        cursorPos--;
    }
}

void TextController(void) {
    InsertNewLine();
    CursorTracker();
    for(int i = 0; i < lineCount; i++) {
        GuiTextBox((Rectangle){ 0, 0 + (i * 20), GetScreenWidth(), 20 }, text[i], MAX_TEXT_LENGTH, i == cursorPos ? true : false);
    }
}