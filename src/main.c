#include "raylib.h"

#include "config.h"
#include "screen.h"
#include "text_buffer.h"

int main(int argc, char *argv[])
{
    SetupConfiguration();
    if(argc > 0)
    {
        filePath = argv[1];
        LoadFile();
    }
    while(!WindowShouldClose())
    {
        BeginDrawing();
            ScreenController();
        EndDrawing();
    }
    UnloadResources();
    CloseWindow();
    return 0;
}