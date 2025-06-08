#include <stddef.h>

#include "raylib.h"

#include "config.h"
#include "constants.h"
#include "screen.h"
#include "text_buffer.h"

Font font;

void SetupConfiguration(void)
{
    SetTargetFPS(60);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, CEDITOR);
    SetWindowMinSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetExitKey(KEY_NULL);
    LoadResources();
    MaximizeWindow();
}

void LoadResources(void)
{
    LoadCustomFont();
    SetupTextBuffer();
}

void UnloadResources(void)
{
    UnloadFont(font);
    FreeBufferMemory();
}

void LoadCustomFont(void)
{
    font = LoadFontEx("./assets/fonts/VictorMono-Regular.ttf", FONT_SIZE, NULL, 0);
}