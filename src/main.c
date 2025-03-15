#include "raylib.h"

#include "config.h"
#include "screen.h"

int main(void) {
    SetupConfiguration();
    while(!WindowShouldClose()) {
        BeginDrawing();
            ScreenController();
        EndDrawing();
    }
    CloseWindow();
    return 0;
}