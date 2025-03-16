#include "raylib.h"

#include "constants.h"

char input[256] = "";
int letterCount = 0;
int lineCount = 0;
Vector2 cursorPos = {0,0};

void CursorTracker(void) {}

void KeyController(void) {    
    int key = GetCharPressed();
    while(key > 0) {
        if(key == KEY_ENTER) {
            input[letterCount] = '\n';
            letterCount++;
            lineCount++;
        }else if(key >= 32 && key <= 125) {
            if(letterCount < 255) {
                input[letterCount] = (char)key;
                letterCount++;
            }
        }
        key = GetCharPressed();
    }
    // TODO: Iterate and modify the input array and render entire array instead of rendering individual letters
    for(int i = 0; i < letterCount; i++) {
        if (input[i] == '\n') {
            cursorPos.y += 20;
        } else {
            DrawText(&input[i], cursorPos.x, cursorPos.y, 20, BLACK); // This does not work
        }
    }
}

void TextController(void) {
    KeyController();
}