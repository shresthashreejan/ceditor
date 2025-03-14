#include "raylib.h"

#include "constants.h"
#include "screen.h"

void SetupConfiguration(void) {
    SetTargetFPS(1000);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, CEDITOR);
    SetWindowMinSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetExitKey(KEY_NULL);
}