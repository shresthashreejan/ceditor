#include "raylib.h"

#include "config.h"
#include "screen.h"
#include "text_buffer.h"

int main(void) {
    SetupConfiguration();
    InitializeTextBuffer();
    while(!WindowShouldClose()) {
        BeginDrawing();
            ScreenController();
        EndDrawing();
    }
    FreeTextBuffer();
    CloseWindow();
    return 0;
}