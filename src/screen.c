#include "raylib.h"

#include "screen.h"
#include "constants.h"
#include "text_buffer.h"

void ScreenController(void) {
    ClearBackground(RAYWHITE);
    KeyController();
    TextBufferController();
}