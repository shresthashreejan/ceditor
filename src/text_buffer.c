#include <stdlib.h>
#include <string.h>

#include "raylib.h"

#include "text_buffer.h"
#include "constants.h"
#include "config.h"

TextBuffer textBuffer;
int lineInfosCapacity = 0;
LineInfo *lineInfos = NULL;
double lastBlinkTime;
Font font;

void SetupTextBuffer(void) {
    InitializeTextBuffer();
    InitializeLineInfos();
    lastBlinkTime = GetTime();
    font = GetFont();
}

void InitializeTextBuffer(void) {
    textBuffer.text = (char *)malloc(1024);
    textBuffer.length = 0;
    textBuffer.capacity = 1024;
    textBuffer.cursorPos.x = 0;
    textBuffer.cursorPos.y = 0;
    textBuffer.cursorVisible = false;
    textBuffer.text[0] = '\0';
    textBuffer.lineCount = 0;
}

void InitializeLineInfos(void) {
    lineInfosCapacity = 16;
    lineInfos = (LineInfo *)malloc(lineInfosCapacity * sizeof(LineInfo));
}

void InsertChar(TextBuffer *buffer, char ch) {
    if(buffer->length + 1 >= buffer->capacity) {
        buffer->capacity *= 2;
        buffer->text = (char *)realloc(buffer->text, buffer->capacity);
    }
    memmove(&buffer->text[(int)buffer->cursorPos.x + 1], &buffer->text[(int)buffer->cursorPos.x], buffer->length - buffer->cursorPos.x + 1);
    buffer->text[(int)buffer->cursorPos.x] = ch;
    buffer->length++;
    buffer->cursorPos.x++;
    buffer->text[buffer->length] = '\0';
}

void RemoveChar(TextBuffer *buffer) {
    if(buffer->cursorPos.x > 0) {
        memmove(&buffer->text[(int)buffer->cursorPos.x - 1], &buffer->text[(int)buffer->cursorPos.x], buffer->length - buffer->cursorPos.x + 1);
        buffer->length--;
        buffer->cursorPos.x--;
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
        textBuffer.cursorPos.y++;
    }

    if(IsKeyPressed(KEY_BACKSPACE)) {
        RemoveChar(&textBuffer);
    }

    if(IsKeyPressed(KEY_LEFT)) {
        if(textBuffer.cursorPos.x > 0) textBuffer.cursorPos.x--;
    }

    if(IsKeyPressed(KEY_RIGHT)) {
        if(textBuffer.cursorPos.x < textBuffer.length) textBuffer.cursorPos.x++;
    }

    if(IsKeyPressed(KEY_UP)) {
        if(textBuffer.cursorPos.y > 0) {
            textBuffer.cursorPos.y--;
            // TODO: Calculate cursorPos.x
        }
    }

    if(IsKeyPressed(KEY_DOWN)) {
        if(textBuffer.cursorPos.y < textBuffer.lineCount - 1) {
            textBuffer.cursorPos.y++;
            for(int i = 0; i <= textBuffer.cursorPos.y; i++) {
                if(textBuffer.cursorPos.x >= lineInfos[i].lineStart && textBuffer.cursorPos.x <= lineInfos[i].lineEnd) {
                    // TODO: Calculate cursor pos x
                }
            }
        }
    }
}

void TextBufferController(void) {
    int lineStart = 0;
    textBuffer.lineCount = 0;
    for(int i = 0; i <= textBuffer.length; i++) {
        if(textBuffer.text[i] == '\n' || i == textBuffer.length) {

            if(textBuffer.lineCount >= lineInfosCapacity) {
                int newCapacity = lineInfosCapacity * 2;
                lineInfos = (LineInfo *)realloc(lineInfos, newCapacity * sizeof(LineInfo));
                lineInfosCapacity = newCapacity;
            }

            int lineEnd = i;
            int lineLength = lineEnd - lineStart;
            char line[1024];
            strncpy(line, &textBuffer.text[lineStart], lineLength);
            line[lineLength] = '\0';
            Vector2 linePos = {TEXT_MARGIN, TEXT_MARGIN + textBuffer.lineCount * FONT_SIZE};
            DrawTextEx(font, line, linePos, FONT_SIZE, TEXT_MARGIN, BLACK);

            lineInfos[textBuffer.lineCount].lineCount = textBuffer.lineCount;
            lineInfos[textBuffer.lineCount].lineLength = lineLength;
            lineInfos[textBuffer.lineCount].lineStart = lineStart;
            lineInfos[textBuffer.lineCount].lineEnd = lineEnd;

            lineStart = i + 1;
            textBuffer.lineCount++;
        }
    }

    double currentTime = GetTime();
    if(currentTime - lastBlinkTime >= BLINK_INTERVAL) {
        textBuffer.cursorVisible = !textBuffer.cursorVisible;
        lastBlinkTime = currentTime;
    }

    if(textBuffer.cursorVisible) {
        int cursorLineStart = 0;
        for(int i = 0; i < textBuffer.cursorPos.x; i++) {
            if(textBuffer.text[i] == '\n') {
                cursorLineStart = i + 1;
            }
        }

        int charsInLine = textBuffer.cursorPos.x - cursorLineStart;
        char currentLine[1024];
        strncpy(currentLine, &textBuffer.text[cursorLineStart], charsInLine);
        currentLine[charsInLine] = '\0';

        Vector2 textSize = MeasureTextEx(font, currentLine, FONT_SIZE, TEXT_MARGIN);
        int cursorX = TEXT_MARGIN + textSize.x;
        int cursorY = TEXT_MARGIN + textBuffer.cursorPos.y * FONT_SIZE;
        DrawLine(cursorX, cursorY, cursorX, cursorY + FONT_SIZE, BLACK);
    }
}

void FreeTextBuffer(void) {
    free(textBuffer.text);
    textBuffer.text = NULL;
}

void FreeLineInfos(void) {
    free(lineInfos);
    lineInfos = NULL;
    lineInfosCapacity = 0;
}

void FreeBufferMemory(void) {
    FreeTextBuffer();
    FreeLineInfos();
}