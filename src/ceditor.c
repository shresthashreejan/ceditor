#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if(argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    FILE *file;
    char *buffer;
    long length;

    file = fopen(argv[1], "r");
    if(file == NULL) {
        perror("Error opening file");
        return 1;
    }

    printf("FILE: %s\n", argv[1]);

    // Getting file size
    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);
    printf("FILE SIZE: %ld bytes\n", length);

    // Allocating memory for the entire file content
    buffer = (char *)malloc(length + 1);
    if(buffer == NULL) {
        perror("Memory allocation failed");
        fclose(file);
        return 1;
    }

    fread(buffer, 1, length, file);
    buffer[length] = '\0';
    fclose(file);

    printf("Ctrl+D to save:\n");
    printf("\n%s\n", buffer);

    size_t modified_content_size = 0;
    char *modified_content = NULL;
    int ch;

    while((ch = getchar()) != EOF) {
        modified_content = realloc(modified_content, modified_content_size + 1);
        modified_content[modified_content_size] = '\0';
    }

    file = fopen(argv[1], "w");
    if(file == NULL) {
        perror("Error opening file in write mode");
        free(buffer);
        free(modified_content);
        return 1;
    }

    fwrite(modified_content, 1, modified_content_size, file);
    fclose(file);

    free(buffer);
    free(modified_content);

    printf("\nSAVED\n");
    return 0;
}