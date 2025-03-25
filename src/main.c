#include "raylib.h"

#include "config.h"
#include "screen.h"

int main(void) {
    SetupConfiguration();
    LoadResources();
    while(!WindowShouldClose()) {
        BeginDrawing();
            ScreenController();
        EndDrawing();
    }
    UnloadResources();
    CloseWindow();
    return 0;
}