#include "raylib.h"

#include "constants.h"

void UpdateFrame(void) {
    BeginDrawing();
        ClearBackground(RAYWHITE);
    EndDrawing();
}

void ScreenController(void) {
    UpdateFrame();
}