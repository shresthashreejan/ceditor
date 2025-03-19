#include "raylib.h"

int initialSize = 1;

void InsertTextLine(void) {
    char **charArray = malloc(initialSize * sizeof(char *));
    if(charArray == NULL) {
        printf("Memory allocation failed!\n");
        CloseWindow();
        return 1;
    }
}