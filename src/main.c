#include "raylib.h"

#include "config.h"
#include "constants.h"
#include "screen.h"

int main(void) {
    SetupConfiguration();
    while(!WindowShouldClose()) {
        ScreenController();
    }
    CloseWindow();
    return 0;
}