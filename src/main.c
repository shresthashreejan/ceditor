#include "raylib.h"

#include "constants.h"

int screenWidth = SCREEN_WIDTH;
int screenHeight = SCREEN_HEIGHT;

void UpdateFrame(void) {
    BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Hello World", (screenWidth - MeasureText("Hello World", 20)) / 2, screenHeight / 2, 20, BLACK);
    EndDrawing();
}

int main(void) {
    SetTargetFPS(1000);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, CEDITOR);
    SetExitKey(KEY_NULL);
    while(!WindowShouldClose()) {
        if(IsWindowResized()) {
            // TODO: Implement min screen w and h
            int currentWidth = GetScreenWidth();
            int currentHeight = GetScreenHeight();
            screenWidth = currentWidth >= SCREEN_WIDTH ? currentWidth : SCREEN_WIDTH;
            screenHeight = currentHeight >= SCREEN_HEIGHT ? currentHeight : SCREEN_HEIGHT;
        }
        UpdateFrame();
    }
    CloseWindow();
    return 0;
}