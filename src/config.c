#include <stddef.h>

#include "raylib.h"

#include "config.h"
#include "constants.h"
#include "screen.h"
#include "text_buffer.h"

Font customFont;

void SetupConfiguration(void) {
    SetTargetFPS(0);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, CEDITOR);
    SetWindowMinSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetExitKey(KEY_NULL);
}

void LoadResources(void) {
    LoadCustomFont();
    SetupTextBuffer();
}

void UnloadResources(void) {
    UnloadFont(customFont);
    FreeBufferMemory();
}

Font GetFont(void) {
    return customFont;
}

void LoadCustomFont(void) {
    customFont = LoadFontEx("./assets/fonts/JuliaMono-Medium.ttf", FONT_SIZE * 2, NULL, 0);
}