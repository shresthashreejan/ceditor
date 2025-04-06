#include "raylib.h"

#include "screen.h"
#include "constants.h"
#include "text_buffer.h"

bool showFps = false;

void ScreenController(void) {
    ClearBackground(RAYWHITE);
    KeyController();
    TextBufferController();
    UpdateView();
    RenderFrameRate();
}

void RenderFrameRate(void) {
    if((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_F)) {
        showFps = !showFps;
    }
    if(showFps) {
        DrawFPS(GetScreenWidth() - 100, GetScreenHeight() - FONT_SIZE);
    }
}