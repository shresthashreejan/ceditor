#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "raylib.h"

#include "text_buffer.h"
#include "constants.h"
#include "config.h"

TextBuffer textBuffer;
int lineInfosCapacity = 0;
LineInfo *lineInfos = NULL;
double lastBlinkTime;
Font font;
int sidebarWidth = SIDEBAR_WIDTH;
char *copiedText = NULL;

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
    textBuffer.cursorVisible = true;
    textBuffer.text[0] = '\0';
    textBuffer.lineCount = 0;
    textBuffer.selectionStart = 0;
    textBuffer.selectionEnd = 0;
    textBuffer.hasSelectionStarted = false;
    textBuffer.hasAllSelected = false;
    textBuffer.hasSelection = false;
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
        if(textBuffer.hasSelection && textBuffer.hasAllSelected) {
            FreeTextBuffer();
            InitializeTextBuffer();
        }
        RemoveChar(&textBuffer);
    }

    if(IsKeyPressed(KEY_LEFT) && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT)) {
        if(textBuffer.cursorPos.x > 0) textBuffer.cursorPos.x--;
    }

    if(IsKeyPressed(KEY_RIGHT) && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT)) {
        if(textBuffer.cursorPos.x < textBuffer.length) textBuffer.cursorPos.x++;
    }

    if((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_LEFT)) {
        if(textBuffer.cursorPos.x > 0) {
            LineInfo *currentLine = &lineInfos[(int)textBuffer.cursorPos.y];
            textBuffer.cursorPos.x = currentLine->lineStart;
        }
    }

    if((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_RIGHT)) {
        if(textBuffer.cursorPos.x > 0) {
            LineInfo *currentLine = &lineInfos[(int)textBuffer.cursorPos.y];
            textBuffer.cursorPos.x = currentLine->lineEnd; // lineEnd stores null terminator
        }
    }

    if(IsKeyPressed(KEY_UP)) {
        if (textBuffer.cursorPos.y > 0) {
            int previousY = textBuffer.cursorPos.y;
            textBuffer.cursorPos.y--;
            if (textBuffer.cursorPos.y < 0 || textBuffer.cursorPos.y >= textBuffer.lineCount) {
                textBuffer.cursorPos.y = previousY;
                return;
            }
            int cursorX = CalculateCursorPosX(previousY);
            textBuffer.cursorPos.x = cursorX;
        }
    }

    if(IsKeyPressed(KEY_DOWN)) {
        if (textBuffer.cursorPos.y < textBuffer.lineCount - 1) {
            int previousY = textBuffer.cursorPos.y;
            textBuffer.cursorPos.y++;
            if (textBuffer.cursorPos.y < 0 || textBuffer.cursorPos.y >= textBuffer.lineCount) {
                textBuffer.cursorPos.y = previousY;
                return;
            }
            int cursorX = CalculateCursorPosX(previousY);
            textBuffer.cursorPos.x = cursorX;
        }
    }

    if((IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) && IsKeyPressed(KEY_LEFT)) {
        if(textBuffer.cursorPos.x > 0) {
            if(!textBuffer.hasSelectionStarted) {
                textBuffer.selectionEnd = textBuffer.cursorPos.x;
                textBuffer.hasSelectionStarted = true;
                textBuffer.hasAllSelected = false;
            }
            if(textBuffer.cursorPos.x == lineInfos[(int)textBuffer.cursorPos.y].lineStart) {
                textBuffer.cursorPos.y--;
            }
            textBuffer.cursorPos.x--;
            textBuffer.selectionStart = textBuffer.cursorPos.x;
            textBuffer.hasSelection = true;
        }
    }

    if((IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) && IsKeyPressed(KEY_RIGHT)) {
        if(textBuffer.cursorPos.x < textBuffer.length) {
            if(!textBuffer.hasSelectionStarted) {
                textBuffer.selectionStart = textBuffer.cursorPos.x;
                textBuffer.hasSelectionStarted = true;
                textBuffer.hasAllSelected = false;
            }
            if((textBuffer.cursorPos.x > lineInfos[(int)textBuffer.cursorPos.y].lineEnd - 1) && textBuffer.cursorPos.y < textBuffer.lineCount) {
                textBuffer.cursorPos.y++;
                textBuffer.cursorPos.x = lineInfos[(int)textBuffer.cursorPos.y].lineStart;
            } else {
                textBuffer.cursorPos.x++;
            }
            textBuffer.selectionEnd = textBuffer.cursorPos.x;
            textBuffer.hasSelection = true;
        }
    }

    if((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_C)) {
        if(textBuffer.hasSelection) {
            int selectionLength = textBuffer.selectionEnd - textBuffer.selectionStart;
            if(selectionLength > 0) {
                if(copiedText != NULL) {
                    free(copiedText);
                }
                copiedText = (char *)malloc(selectionLength + 1);
                strncpy(copiedText, &textBuffer.text[textBuffer.selectionStart], selectionLength);
                copiedText[selectionLength] = '\0';
                textBuffer.hasSelectionStarted = false;
            }
        }
    }

    if((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_V)) {
        if(copiedText != NULL) {
            int copiedLength = strlen(copiedText);
            if (textBuffer.length + copiedLength >= textBuffer.capacity) {
                textBuffer.capacity = (textBuffer.length + copiedLength) * 2;
                textBuffer.text = (char *)realloc(textBuffer.text, textBuffer.capacity);
            }

            memmove(&textBuffer.text[(int)textBuffer.cursorPos.x + copiedLength], &textBuffer.text[(int)textBuffer.cursorPos.x], textBuffer.length - textBuffer.cursorPos.x + 1);
            memcpy(&textBuffer.text[(int)textBuffer.cursorPos.x], copiedText, copiedLength);

            textBuffer.length += copiedLength;
            textBuffer.cursorPos.x += copiedLength;
            textBuffer.text[textBuffer.length] = '\0';
        }
    }

    if((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_A)) {
        if(textBuffer.cursorPos.x > 0 && textBuffer.cursorPos.x <= textBuffer.length) {
            textBuffer.selectionStart = 0;
            textBuffer.selectionEnd = textBuffer.length;
            textBuffer.hasSelectionStarted = false;
            textBuffer.hasAllSelected = true;
            textBuffer.hasSelection = true;
        }
    }
}

int CalculateCursorPosX(int previousY) {
    LineInfo *currentLine = &lineInfos[previousY];
    LineInfo *newLine = &lineInfos[(int)textBuffer.cursorPos.y];
    int offset = textBuffer.cursorPos.x - currentLine->lineStart;
    int newXPos = newLine->lineStart + offset;
    if (newXPos > newLine->lineEnd) {
        newXPos = newLine->lineEnd;
    }
    return newXPos;
}

void TextBufferController(void) {
    DrawRectangle(0, 0, sidebarWidth, GetScreenHeight(), SIDEBAR_COLOR);
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

            int lineNumber = textBuffer.lineCount + 1;
            char lineNumberStr[16];
            snprintf(lineNumberStr, sizeof(lineNumberStr), "%d", lineNumber);
            Vector2 numberSize = MeasureTextEx(font, lineNumberStr, FONT_SIZE, TEXT_MARGIN);
            if(numberSize.x >= sidebarWidth) {
                sidebarWidth = numberSize.x + SIDEBAR_MARGIN;
            }
            int xPos = sidebarWidth - numberSize.x - SIDEBAR_MARGIN;
            int yPos = TEXT_MARGIN + textBuffer.lineCount * FONT_SIZE;
            DrawTextEx(font, lineNumberStr, (Vector2){xPos, yPos}, FONT_SIZE, TEXT_MARGIN, BLACK);

            Vector2 linePos = {sidebarWidth + TEXT_MARGIN, yPos};
            DrawTextEx(font, line, linePos, FONT_SIZE, TEXT_MARGIN, BLACK);

            lineInfos[textBuffer.lineCount].lineCount = textBuffer.lineCount;
            lineInfos[textBuffer.lineCount].lineLength = lineLength;
            lineInfos[textBuffer.lineCount].lineStart = lineStart;
            lineInfos[textBuffer.lineCount].lineEnd = lineEnd;

            lineStart = i + 1;
            textBuffer.lineCount++;
        }
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
        int cursorX = sidebarWidth + TEXT_MARGIN + textSize.x;
        int cursorY = TEXT_MARGIN + textBuffer.cursorPos.y * FONT_SIZE;
        DrawLine(cursorX, cursorY, cursorX, cursorY + FONT_SIZE, BLACK);
    }
}

void BlinkCursor(void) {
    double currentTime = GetTime();
    if(currentTime - lastBlinkTime >= BLINK_INTERVAL) {
        textBuffer.cursorVisible = !textBuffer.cursorVisible;
        lastBlinkTime = currentTime;
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