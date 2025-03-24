#include <stdlib.h>
#include <string.h>

#include "raylib.h"

#include "text_buffer.h"
#include "constants.h"

TextBuffer textBuffer;

void InitializeTextBuffer(void) {
    textBuffer.text = (char *)malloc(1024);
    textBuffer.length = 0;
    textBuffer.capacity = 1024;
    textBuffer.cursorPos = 0;
    textBuffer.text[0] = '\0';
}

void InsertChar(TextBuffer *buffer, char ch) {
    if(buffer->length + 1 >= buffer->capacity) {
        buffer->capacity *= 2;
        buffer->text = (char *)realloc(buffer->text, buffer->capacity);
    }
    memmove(&buffer->text[buffer->cursorPos + 1], &buffer->text[buffer->cursorPos], buffer->length - buffer->cursorPos + 1);
    buffer->text[buffer->cursorPos] = ch;
    buffer->length++;
    buffer->cursorPos++;
    buffer->text[buffer->length] = '\0';
}

void RemoveChar(TextBuffer *buffer) {
    if(buffer->cursorPos > 0) {
        memmove(&buffer->text[buffer->cursorPos - 1], &buffer->text[buffer->cursorPos], buffer->length - buffer->cursorPos + 1);
        buffer->length--;
        buffer->cursorPos--;
        buffer->text[buffer->length] = '\0';
    }
}

void KeyController(void) {
    int key = GetCharPressed();
    while(key > 0) {
        if(key >= 32 && key <= 126) {
            InsertChar(&textBuffer, (char)key);
        }
        key = GetCharPressed();
    }

    if(IsKeyPressed(KEY_ENTER)) {
        InsertChar(&textBuffer, '\n');
    }

    if(IsKeyPressed(KEY_BACKSPACE)) {
        RemoveChar(&textBuffer);
    }

    if(IsKeyPressed(KEY_LEFT)) {
        if(textBuffer.cursorPos > 0) textBuffer.cursorPos--;
    }

    if(IsKeyPressed(KEY_RIGHT)) {
        if(textBuffer.cursorPos < textBuffer.length) textBuffer.cursorPos++;
    }
}

void TextBufferController(void) {
    int lineStart = 0;
    int lineNumber = 0;
    for(int i = 0; i <= textBuffer.length; i++) {
        if(textBuffer.text[i] == '\n' || i == textBuffer.length) {
            int lineEnd = i;
            int lineLength = lineEnd - lineStart;
            char line[1024];
            strncpy(line, &textBuffer.text[lineStart], lineLength);
            line[lineLength] = '\0';

            DrawText(line, TEXT_MARGIN, TEXT_MARGIN + lineNumber * FONT_SIZE, FONT_SIZE, BLACK);
            lineStart = i + 1;
            lineNumber++;
        }
    }

    int cursorLine = 0;
    int cursorLineStart = 0;
    for(int i = 0; i < textBuffer.cursorPos; i++) {
        if(textBuffer.text[i] == '\n') {
            cursorLine++;
            cursorLineStart = i + 1;
        }
    }

    int charsInLine = textBuffer.cursorPos - cursorLineStart;
    char currentLine[1024];
    strncpy(currentLine, &textBuffer.text[cursorLineStart], charsInLine);
    currentLine[charsInLine] = '\0';

    int cursorX = TEXT_MARGIN + MeasureText(currentLine, FONT_SIZE);
    int cursorY = TEXT_MARGIN + cursorLine * FONT_SIZE;
    DrawLine(cursorX, cursorY, cursorX, cursorY + FONT_SIZE, BLACK);
}

void FreeTextBuffer(void) {
    free(textBuffer.text);
    textBuffer.text = NULL;
}