#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "raylib.h"
#include "raygui.h"
#include "raymath.h"

#include "text_buffer.h"
#include "constants.h"
#include "config.h"

Font font;
TextBuffer textBuffer;
LineInfo *lineInfos = NULL;
char *copiedText = NULL;
int lineInfosCapacity = 0;
int sidebarWidth = SIDEBAR_WIDTH;
double lastBlinkTime;
float lastCursorUpdateTime = 0.0f;
float keyDownDelay = 0.0f;
float maxLineWidth = 0;
Vector2 scroll = {0, 0};
Rectangle viewport = {0};
Rectangle totalView = {0};
Rectangle panelView = {0};
int lastLineOnView = 0;
int firstLineOnView = 0;

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
        } else {
            RemoveChar(&textBuffer);
        }
    }

    if(IsKeyPressed(KEY_ESCAPE)) {
        if(textBuffer.hasSelectionStarted) textBuffer.hasSelectionStarted = false;
    }

    if(IsKeyPressed(KEY_LEFT) && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT)) {
        CalculateCursorPosition(KEY_LEFT);
    }

    if(IsKeyDown(KEY_LEFT) && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT)) {
        keyDownDelay += GetFrameTime();
        if(keyDownDelay >= KEY_DOWN_DELAY) {
            lastCursorUpdateTime += GetFrameTime();
            if(lastCursorUpdateTime >= CURSOR_UPDATE_INTERVAL) {
                CalculateCursorPosition(KEY_LEFT);
                lastCursorUpdateTime -= CURSOR_UPDATE_INTERVAL;
            }
        }
    }

    if(IsKeyPressed(KEY_RIGHT) && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT)) {
        CalculateCursorPosition(KEY_RIGHT);
    }

    if(IsKeyDown(KEY_RIGHT) && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT)) {
        keyDownDelay += GetFrameTime();
        if(keyDownDelay >= KEY_DOWN_DELAY) {
            lastCursorUpdateTime += GetFrameTime();
            if(lastCursorUpdateTime >= CURSOR_UPDATE_INTERVAL) {
                CalculateCursorPosition(KEY_RIGHT);
                lastCursorUpdateTime -= CURSOR_UPDATE_INTERVAL;
            }
        }
    }

    if(IsKeyReleased(KEY_LEFT) || IsKeyReleased(KEY_RIGHT) || IsKeyReleased(KEY_UP) || IsKeyReleased(KEY_DOWN)) {
        keyDownDelay = 0.0f;
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
            textBuffer.cursorPos.x = currentLine->lineEnd;
        }
    }

    if(IsKeyPressed(KEY_UP)) {
        CalculateCursorPosition(KEY_UP);
    }

    if(IsKeyDown(KEY_UP)) {
        keyDownDelay += GetFrameTime();
        if(keyDownDelay >= KEY_DOWN_DELAY) {
            lastCursorUpdateTime += GetFrameTime();
            if(lastCursorUpdateTime >= CURSOR_UPDATE_INTERVAL) {
                CalculateCursorPosition(KEY_UP);
                lastCursorUpdateTime -= CURSOR_UPDATE_INTERVAL;
            }
        }
    }

    if(IsKeyPressed(KEY_DOWN)) {
        CalculateCursorPosition(KEY_DOWN);
    }

    if(IsKeyDown(KEY_DOWN)) {
        keyDownDelay += GetFrameTime();
        if(keyDownDelay >= KEY_DOWN_DELAY) {
            lastCursorUpdateTime += GetFrameTime();
            if(lastCursorUpdateTime >= CURSOR_UPDATE_INTERVAL) {
                CalculateCursorPosition(KEY_DOWN);
                lastCursorUpdateTime -= CURSOR_UPDATE_INTERVAL;
            }
        }
    }

    if((IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) && IsKeyPressed(KEY_LEFT)) {
        CalculateSelection(KEY_LEFT);
    }

    if((IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) && IsKeyDown(KEY_LEFT)) {
        keyDownDelay += GetFrameTime();
        if(keyDownDelay >= KEY_DOWN_DELAY) {
            lastCursorUpdateTime += GetFrameTime();
            if(lastCursorUpdateTime >= CURSOR_UPDATE_INTERVAL) {
                CalculateSelection(KEY_LEFT);
                lastCursorUpdateTime -= CURSOR_UPDATE_INTERVAL;
            }
        }
    }

    if((IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) && IsKeyPressed(KEY_RIGHT)) {
        CalculateSelection(KEY_RIGHT);
    }

    if((IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) && IsKeyDown(KEY_RIGHT)) {
        keyDownDelay += GetFrameTime();
        if(keyDownDelay >= KEY_DOWN_DELAY) {
            lastCursorUpdateTime += GetFrameTime();
            if(lastCursorUpdateTime >= CURSOR_UPDATE_INTERVAL) {
                CalculateSelection(KEY_RIGHT);
                lastCursorUpdateTime -= CURSOR_UPDATE_INTERVAL;
            }
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
            for(int i = 0; i < copiedLength; i++) {
                if(copiedText[i] == '\n') {
                    textBuffer.cursorPos.y++;
                }
            }
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
    if(newXPos > newLine->lineEnd) {
        newXPos = newLine->lineEnd;
    }
    return newXPos;
}

void ProcessLines(void) {
    int lineStart = 0;
    maxLineWidth = 0;
    textBuffer.lineCount = 0;
    for(int i = 0; i <= textBuffer.length; i++) {
        if(textBuffer.text[i] == '\n' || i == textBuffer.length) {
            if(textBuffer.lineCount >= lineInfosCapacity) {
                lineInfosCapacity *= 2;
                lineInfos = (LineInfo *)realloc(lineInfos, lineInfosCapacity * sizeof(LineInfo));
            }

            lineInfos[textBuffer.lineCount] = (LineInfo){
                .lineCount = textBuffer.lineCount,
                .lineStart = lineStart,
                .lineEnd = i,
                .lineLength = i - lineStart
            };

            char line[1024];
            strncpy(line, &textBuffer.text[lineStart], lineInfos[textBuffer.lineCount].lineLength);
            line[lineInfos[textBuffer.lineCount].lineLength] = '\0';
            Vector2 textSize = MeasureTextEx(font, line, FONT_SIZE, TEXT_MARGIN);
            if(textSize.x > maxLineWidth) maxLineWidth = textSize.x;

            lineStart = i + 1;
            textBuffer.lineCount++;
        }
    }
}

void TextBufferController(void) {
    if(!textBuffer.text) return;

    ProcessLines();

    float lineHeight = FONT_SIZE + TEXT_MARGIN;
    float totalWidth = maxLineWidth + (2 * TEXT_MARGIN);
    float totalHeight = (textBuffer.lineCount * lineHeight) + TEXT_MARGIN;

    viewport = (Rectangle){sidebarWidth, 0, GetScreenWidth() - sidebarWidth, GetScreenHeight()};
    totalView = (Rectangle){0, 0, totalWidth, totalHeight};
    UpdateView();
    GuiScrollPanel(viewport, NULL, totalView, &scroll, &panelView);
    DrawRectangle(0, 0, sidebarWidth, GetScreenHeight(), SIDEBAR_COLOR);

    BeginScissorMode(0, 0, sidebarWidth, GetScreenHeight());
        for (int i = 0; i < textBuffer.lineCount; i++) {
            char lineNumberStr[16];
            snprintf(lineNumberStr, sizeof(lineNumberStr), "%d", i + 1);
            Vector2 numberSize = MeasureTextEx(font, lineNumberStr, FONT_SIZE, TEXT_MARGIN);
            if(numberSize.x >= sidebarWidth) {
                sidebarWidth = numberSize.x + SIDEBAR_MARGIN;
            }
            Vector2 linePos = {
                sidebarWidth - numberSize.x - SIDEBAR_MARGIN,
                (i * lineHeight) + scroll.y
            };
            DrawTextEx(font, lineNumberStr, linePos, FONT_SIZE, TEXT_MARGIN, BLACK);
            lastLineOnView = i + 1;
        }
    EndScissorMode();

    BeginScissorMode(panelView.x, panelView.y, panelView.width, panelView.height);
        for (int i = 0; i < textBuffer.lineCount; i++) {
            if (i >= lineInfosCapacity) break;

            char line[1024];
            strncpy(line, &textBuffer.text[lineInfos[i].lineStart], lineInfos[i].lineLength);
            line[lineInfos[i].lineLength] = '\0';

            Vector2 linePos = {
                sidebarWidth + TEXT_MARGIN + scroll.x,
                (i * lineHeight) + scroll.y
            };
            DrawTextEx(font, line, linePos, FONT_SIZE, TEXT_MARGIN, BLACK);
        }
    EndScissorMode();

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
        int cursorX = sidebarWidth + TEXT_MARGIN + textSize.x + scroll.x;
        int cursorY = TEXT_MARGIN + (textBuffer.cursorPos.y * lineHeight) + scroll.y;
        DrawLine(cursorX, cursorY, cursorX, cursorY + FONT_SIZE, BLACK);
    }
}

// FIX THIS FUNCTION
void UpdateView(void) {
    int totalPossibleLines = GetScreenHeight() / (FONT_SIZE + TEXT_MARGIN);
    if(textBuffer.cursorPos.y + 1 > lastLineOnView) {
        scroll.y -= FONT_SIZE + TEXT_MARGIN;
        firstLineOnView = lastLineOnView - (totalPossibleLines - 1);
    } else if(textBuffer.cursorPos.y < firstLineOnView) {
        scroll.y += FONT_SIZE + TEXT_MARGIN;
    }
}

void CalculateCursorPosition(int key) {
    switch(key) {
        case KEY_LEFT:
            if(textBuffer.cursorPos.x > 0) {
                if(textBuffer.cursorPos.x == lineInfos[(int)textBuffer.cursorPos.y].lineStart) {
                    textBuffer.cursorPos.y--;
                }
                textBuffer.cursorPos.x--;
            }
            break;
        case KEY_RIGHT:
            if(textBuffer.cursorPos.x < textBuffer.length) {
                if((textBuffer.cursorPos.x >= lineInfos[(int)textBuffer.cursorPos.y].lineEnd) && textBuffer.cursorPos.y < textBuffer.lineCount) {
                    textBuffer.cursorPos.y++;
                    textBuffer.cursorPos.x = lineInfos[(int)textBuffer.cursorPos.y].lineStart;
                } else {
                    textBuffer.cursorPos.x++;
                }
            }
            break;
        case KEY_UP:
            if(textBuffer.cursorPos.y > 0) {
                int previousY = textBuffer.cursorPos.y;
                if (textBuffer.cursorPos.y < 0 || textBuffer.cursorPos.y >= textBuffer.lineCount) {
                    textBuffer.cursorPos.y = previousY;
                    return;
                } else {
                    textBuffer.cursorPos.y--;
                }
                int cursorX = CalculateCursorPosX(previousY);
                textBuffer.cursorPos.x = cursorX;
            }
            break;
        case KEY_DOWN:
            if(textBuffer.cursorPos.y < textBuffer.lineCount - 1) {
                int previousY = textBuffer.cursorPos.y;
                if (textBuffer.cursorPos.y < 0 || textBuffer.cursorPos.y >= textBuffer.lineCount) {
                    textBuffer.cursorPos.y = previousY;
                    return;
                } else {
                    textBuffer.cursorPos.y++;
                }
                int cursorX = CalculateCursorPosX(previousY);
                textBuffer.cursorPos.x = cursorX;
            }
            break;
        default:
            break;
    }
}

void CalculateSelection(int key) {
    switch(key) {
        case KEY_LEFT:
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
            break;
        case KEY_RIGHT:
            if(textBuffer.cursorPos.x < textBuffer.length) {
                if(!textBuffer.hasSelectionStarted) {
                    textBuffer.selectionStart = textBuffer.cursorPos.x;
                    textBuffer.hasSelectionStarted = true;
                    textBuffer.hasAllSelected = false;
                }
                if((textBuffer.cursorPos.x >= lineInfos[(int)textBuffer.cursorPos.y].lineEnd) && textBuffer.cursorPos.y < textBuffer.lineCount) {
                    textBuffer.cursorPos.y++;
                    textBuffer.cursorPos.x = lineInfos[(int)textBuffer.cursorPos.y].lineStart;
                } else {
                    textBuffer.cursorPos.x++;
                }
                textBuffer.selectionEnd = textBuffer.cursorPos.x;
                textBuffer.hasSelection = true;
            }
            break;
        default:
            break;
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