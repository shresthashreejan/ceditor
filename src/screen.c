#include "stdio.h"

#include "raylib.h"

#include "screen.h"
#include "constants.h"
#include "text_buffer.h"
#include "config.h"

bool showFps = false;

void ScreenController(void)
{
    ClearBackground(RAYWHITE);
    if (KeyController()) TextBufferController(); else UpdateCursorState();
    RenderTextBuffer();
    RenderFrameRate();
}

void RenderFrameRate(void)
{
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) && IsKeyPressed(KEY_F))
    {
        showFps = !showFps;
    }
    if (showFps) DrawCustomFPS();
}

void DrawCustomFPS(void)
{
    char fpsText[32];
    sprintf(fpsText, "%d FPS", GetFPS());

    Vector2 position = {
        GetScreenWidth() - (BOTTOM_BAR_FONT_SIZE * 5),
        GetScreenHeight() - BOTTOM_BAR_FONT_SIZE
    };

    DrawTextEx(font, fpsText, position, BOTTOM_BAR_FONT_SIZE, DRAW_TEXT_SPACING, WHITE);
}

void DrawBottomBar(void)
{
    DrawRectangle(0, GetScreenHeight() - BOTTOM_BAR_HEIGHT, GetScreenWidth(), BOTTOM_BAR_HEIGHT, BOTTOM_BAR_COLOR);
}

void DrawHelpText(char helpText[256])
{
    Vector2 position = {0, GetScreenHeight() - BOTTOM_BAR_FONT_SIZE};
    DrawTextEx(font, helpText, position, BOTTOM_BAR_FONT_SIZE, DRAW_TEXT_SPACING, WHITE);
}