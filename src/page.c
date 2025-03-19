#include "raylib.h"

#include "textline.h"

int lineCount = 0;

void PageController(void) {
    if(IsKeyPressed(KEY_ENTER)) {
        InsertTextLine();
        lineCount++;
    }
}