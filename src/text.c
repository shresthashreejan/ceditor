#include "raylib.h"

#include "constants.h"

int line = 0;
Vector2 cursorPos = {0,0};

char text[MAX_LINES][MAX_TEXT_LENGTH];
int characters[MAX_LINES] = {0};

void CursorTracker(void) {}

void KeyController(void) {
    if(IsKeyPressed(KEY_ENTER)) {
        if(line < MAX_LINES) {
            line++;
            characters[line] = 0;
        }
    }
    if(IsKeyPressed(KEY_BACKSPACE)) {
        if(characters[line] >= 0) {
            text[line][characters[line]] = '\0';
            characters[line]--; 
        }
    }
    int key = GetCharPressed();
    while(key > 0) {
        if(key >= 32 && key <= 125) {
            if(characters[line] < MAX_TEXT_LENGTH) {
                text[line][characters[line]] = (char)key;
                characters[line]++;
            }
        }
        key = GetCharPressed();
    }
    for(int i = 0; i <= line; i++) {
        DrawText(text[i], cursorPos.x, cursorPos.y + i * 20, 20, BLACK);
    }
}

void TextController(void) {
    KeyController();
}