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
LineBuffer *lineBuffer = NULL;
const char *filePath = NULL;
char *copiedText = NULL;
int lineBufferCapacity = 0;
int sidebarWidth = SIDEBAR_WIDTH;
int nonPrintableKeys[] = {
    KEY_ENTER,
    KEY_BACKSPACE,
    KEY_ESCAPE,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_C,
    KEY_V,
    KEY_A,
    KEY_S
};
int nonPrintableKeysLength = sizeof(nonPrintableKeys) / sizeof(nonPrintableKeys[0]);
float keyDownElapsedTime = 0.0f;
float keyDownDelay = 0.0f;
float maxLineWidth = 0;
float lastCursorUpdateTime;
double lastBlinkTime;
bool cursorIdle = true;
Vector2 scroll = {0, 0};
Rectangle viewport = {0};
Rectangle totalView = {0};
Rectangle panelView = {0};

/* SETUP */
void SetupTextBuffer(void)
{
    InitializeTextBuffer();
    InitializeLineBuffer();
    lastBlinkTime = GetTime();
    font = GetFont();
    UpdateSidebarWidth();
}

void InitializeTextBuffer(void)
{
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

void InitializeLineBuffer(void)
{
    lineBufferCapacity = 1024;
    lineBuffer = (LineBuffer *)malloc(lineBufferCapacity * sizeof(LineBuffer));
}

/* TEXT BUFFER */
void InsertChar(TextBuffer *buffer, char ch)
{
    if (buffer->length + 1 >= buffer->capacity)
    {
        buffer->capacity *= 2;
        buffer->text = (char *)realloc(buffer->text, buffer->capacity);
    }
    memmove(&buffer->text[(int)buffer->cursorPos.x + 1], &buffer->text[(int)buffer->cursorPos.x], buffer->length - buffer->cursorPos.x + 1);
    buffer->text[(int)buffer->cursorPos.x] = ch;
    buffer->length++;
    buffer->cursorPos.x++;
    buffer->text[buffer->length] = '\0';
}

void RemoveChar(TextBuffer *buffer)
{
    if (buffer->cursorPos.x > 0)
    {
        memmove(&buffer->text[(int)buffer->cursorPos.x - 1], &buffer->text[(int)buffer->cursorPos.x], buffer->length - buffer->cursorPos.x + 1);
        buffer->length--;
        buffer->cursorPos.x--;
        buffer->text[buffer->length] = '\0';
        if (buffer->cursorPos.x == lineBuffer[(int)buffer->cursorPos.y - 1].lineEnd && buffer->cursorPos.y != 0)
        {
            buffer->cursorPos.y--;
        }
    }
}

void TextBufferController(void)
{
    if (!textBuffer.text) return;
    int lineStart = 0;
    maxLineWidth = 0;
    textBuffer.lineCount = 0;
    for (int i = 0; i <= textBuffer.length; i++)
    {
        if (textBuffer.text[i] == '\n' || i == textBuffer.length)
        {
            if (textBuffer.lineCount >= lineBufferCapacity)
            {
                lineBufferCapacity *= 2;
                lineBuffer = (LineBuffer *)realloc(lineBuffer, lineBufferCapacity * sizeof(LineBuffer));
            }

            lineBuffer[textBuffer.lineCount] = (LineBuffer) {
                .lineCount = textBuffer.lineCount,
                .lineStart = lineStart,
                .lineEnd = i,
                .lineLength = i - lineStart
            };

            char line[1024];
            strncpy(line, &textBuffer.text[lineStart], lineBuffer[textBuffer.lineCount].lineLength);
            line[lineBuffer[textBuffer.lineCount].lineLength] = '\0';
            Vector2 textSize = MeasureTextEx(font, line, FONT_SIZE, TEXT_MARGIN);
            if (textSize.x > maxLineWidth) maxLineWidth = textSize.x;

            lineStart = i + 1;
            textBuffer.lineCount++;
            UpdateSidebarWidth();
        }
    }
}

void UpdateSidebarWidth(void)
{
    char lineNumberStr[16];
    snprintf(lineNumberStr, sizeof(lineNumberStr), "%d", textBuffer.lineCount);
    Vector2 numberSize = MeasureTextEx(font, lineNumberStr, FONT_SIZE, TEXT_MARGIN);
    sidebarWidth = numberSize.x + SIDEBAR_MARGIN;
}

/* KEY CONTROLLER */
bool KeyController(void)
{
    int key = GetCharPressed();
    bool isAnyKeyPressed = false;
    while (key > 0)
    {
        if (key >= 32 && key <= 126)
        {
            InsertChar(&textBuffer, (char)key);
            isAnyKeyPressed = true;
        }
        key = GetCharPressed();
    }

    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    for (int i = 0; i < nonPrintableKeysLength; i++)
    {
        if (IsKeyPressed(nonPrintableKeys[i]))
        {
            ProcessKey(nonPrintableKeys[i], ctrl, shift);
            RecordCursorActivity();
            isAnyKeyPressed = true;
        }
    }

    if (IsKeyDown(KEY_UP)) ProcessKeyDownMovement(KEY_UP, shift);
    if (IsKeyDown(KEY_DOWN)) ProcessKeyDownMovement(KEY_DOWN, shift);
    if (IsKeyDown(KEY_LEFT)) ProcessKeyDownMovement(KEY_LEFT, shift);
    if (IsKeyDown(KEY_RIGHT)) ProcessKeyDownMovement(KEY_RIGHT, shift);
    if (IsKeyReleased(KEY_LEFT) || IsKeyReleased(KEY_RIGHT) || IsKeyReleased(KEY_UP) || IsKeyReleased(KEY_DOWN)) keyDownDelay = 0.0f;
    return isAnyKeyPressed;
}

void ProcessKey(int key, bool ctrl, bool shift)
{
    switch (key)
    {
        case KEY_ENTER:
            InsertChar(&textBuffer, '\n');
            textBuffer.cursorPos.y++;
            break;

        case KEY_BACKSPACE:
            if (textBuffer.hasSelection && textBuffer.hasAllSelected)
            {
                FreeTextBuffer();
                InitializeTextBuffer();
            }
            else
            {
                RemoveChar(&textBuffer);
            }
            break;

        case KEY_ESCAPE:
            if (textBuffer.hasSelectionStarted) textBuffer.hasSelectionStarted = false;
            break;

        case KEY_UP:
            CalculateCursorPosition(KEY_UP);
            break;

        case KEY_DOWN:
            CalculateCursorPosition(KEY_DOWN);
            break;

        case KEY_LEFT:
            if (ctrl)
            {
                textBuffer.cursorPos.x = lineBuffer[(int)textBuffer.cursorPos.y].lineStart;
            }
            else if (shift)
            {
                CalculateSelection(KEY_LEFT);
            }
            else
            {
                CalculateCursorPosition(KEY_LEFT);
            }
            break;

        case KEY_RIGHT:
            if (ctrl)
            {
                textBuffer.cursorPos.x = lineBuffer[(int)textBuffer.cursorPos.y].lineEnd;
            }
            else if (shift)
            {
                CalculateSelection(KEY_RIGHT);
            }
            else
            {
                CalculateCursorPosition(KEY_RIGHT);
            }
            break;

        case KEY_C:
            if (ctrl && textBuffer.hasSelection)
            {
                int selectionLength = textBuffer.selectionEnd - textBuffer.selectionStart;
                if (selectionLength > 0)
                {
                    if (copiedText != NULL) free(copiedText);
                    copiedText = (char *)malloc(selectionLength + 1);
                    strncpy(copiedText, &textBuffer.text[textBuffer.selectionStart], selectionLength);
                    copiedText[selectionLength] = '\0';
                    textBuffer.hasSelectionStarted = false;
                }
            }
            break;

        case KEY_V:
            if (ctrl && copiedText != NULL)
            {
                int copiedLength = strlen(copiedText);
                if (textBuffer.length + copiedLength >= textBuffer.capacity)
                {
                    textBuffer.capacity = (textBuffer.length + copiedLength) * 2;
                    textBuffer.text = (char *)realloc(textBuffer.text, textBuffer.capacity);
                }

                memmove(&textBuffer.text[(int)textBuffer.cursorPos.x + copiedLength], &textBuffer.text[(int)textBuffer.cursorPos.x], textBuffer.length - textBuffer.cursorPos.x + 1);
                memcpy(&textBuffer.text[(int)textBuffer.cursorPos.x], copiedText, copiedLength);

                textBuffer.length += copiedLength;
                textBuffer.cursorPos.x += copiedLength;
                textBuffer.text[textBuffer.length] = '\0';
                for (int i = 0; i < copiedLength; i++)
                {
                    if (copiedText[i] == '\n')
                    {
                        textBuffer.cursorPos.y++;
                    }
                }
            }
            break;

        case KEY_A:
            if (ctrl) CalculateSelection(KEY_A);
            break;

        case KEY_S:
            if (ctrl) SaveFile();
            break;

        default:
            break;
    }
}

void ProcessKeyDownMovement(int key, bool shift)
{
    float frameTime = GetFrameTime();
    keyDownDelay += frameTime;
    keyDownElapsedTime += frameTime;
    if (keyDownDelay >= KEY_DOWN_DELAY)
    {
        if (keyDownElapsedTime >= KEY_DOWN_INTERVAL)
        {
            if (shift) CalculateSelection(key); else CalculateCursorPosition(key);
            keyDownElapsedTime = 0.0f;
        }
        RecordCursorActivity();
    }
}

/* RENDERING */
void RenderTextBuffer(void)
{
    float lineHeight = FONT_SIZE + TEXT_MARGIN;
    float totalWidth = maxLineWidth + (2 * TEXT_MARGIN);
    float totalHeight = (textBuffer.lineCount * lineHeight) + TEXT_MARGIN;

    viewport = (Rectangle){sidebarWidth, 0, GetScreenWidth() - sidebarWidth, GetScreenHeight()};
    totalView = (Rectangle){0, 0, totalWidth, totalHeight};

    int firstVisibleLine = (int)fmaxf(0, -scroll.y / lineHeight);
    int lastVisibleLine = (int)fminf(textBuffer.lineCount, firstVisibleLine + (panelView.height / lineHeight) + 2);

    // UpdateView();
    GuiScrollPanel(viewport, NULL, totalView, &scroll, &panelView);
    DrawRectangle(0, 0, sidebarWidth, GetScreenHeight(), SIDEBAR_COLOR);

    DrawSidebar(firstVisibleLine, lastVisibleLine, lineHeight, scroll.y);
    DrawTextLines(firstVisibleLine, lastVisibleLine, lineHeight, scroll);
    DrawCursor(lineHeight);
}

void DrawSidebar(int firstVisibleLine, int lastVisibleLine, float lineHeight, float scrollPosY)
{
    BeginScissorMode(0, 0, sidebarWidth, GetScreenHeight());
        if (lastVisibleLine == 0) lastVisibleLine += 1;
        for (int i = firstVisibleLine; i < lastVisibleLine; i++)
        {
            char lineNumberStr[16];
            snprintf(lineNumberStr, sizeof(lineNumberStr), "%d", i + 1);
            Vector2 numberSize = MeasureTextEx(font, lineNumberStr, FONT_SIZE, TEXT_MARGIN);
            Vector2 linePos = {
                sidebarWidth - numberSize.x - SIDEBAR_MARGIN,
                (i * lineHeight) + scrollPosY
            };
            DrawTextEx(font, lineNumberStr, linePos, FONT_SIZE, TEXT_MARGIN, BLACK);
        }
    EndScissorMode();
}

void DrawTextLines(int firstVisibleLine, int lastVisibleLine, float lineHeight, Vector2 scroll)
{
    BeginScissorMode(panelView.x, panelView.y, panelView.width, panelView.height);
        for (int i = firstVisibleLine; i < lastVisibleLine; i++)
        {
            if (i >= lineBufferCapacity) break;

            char line[1024];
            strncpy(line, &textBuffer.text[lineBuffer[i].lineStart], lineBuffer[i].lineLength);
            line[lineBuffer[i].lineLength] = '\0';

            Vector2 linePos = {
                sidebarWidth + TEXT_MARGIN + scroll.x,
                (i * lineHeight) + scroll.y
            };
            DrawTextEx(font, line, linePos, FONT_SIZE, TEXT_MARGIN, BLACK);
        }
    EndScissorMode();
}

void DrawCursor(float lineHeight)
{
    if (textBuffer.cursorVisible)
    {
        int cursorLineStart = 0;
        for (int i = 0; i < textBuffer.cursorPos.x; i++)
        {
            if (textBuffer.text[i] == '\n')
            {
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
        DrawRectangle(cursorX, cursorY, FONT_SIZE / 2, FONT_SIZE, CURSOR_COLOR);
    }
    BlinkCursor();
}

/* CURSOR */
void RecordCursorActivity(void)
{
    lastCursorUpdateTime = GetTime();
    cursorIdle = false;
}

void UpdateCursorState(void)
{
    double now = GetTime();
    if (!cursorIdle && now - lastCursorUpdateTime >= CURSOR_IDLE_INTERVAL)
    {
        cursorIdle = true;
        lastCursorUpdateTime = now;
    }
}

void BlinkCursor(void)
{
    if (cursorIdle)
    {
        double currentTime = GetTime();
        if (currentTime - lastBlinkTime >= BLINK_INTERVAL)
        {
            textBuffer.cursorVisible = !textBuffer.cursorVisible;
            lastBlinkTime = currentTime;
        }
    }
    else
    {
        textBuffer.cursorVisible = true;
    }
}

void CalculateCursorPosition(int key)
{
    switch (key)
    {
        case KEY_LEFT:
            if (textBuffer.cursorPos.x > 0)
            {
                if (textBuffer.cursorPos.x == lineBuffer[(int)textBuffer.cursorPos.y].lineStart)
                {
                    textBuffer.cursorPos.y--;
                }
                textBuffer.cursorPos.x--;
            }
            break;

        case KEY_RIGHT:
            if (textBuffer.cursorPos.x < textBuffer.length)
            {
                if ((textBuffer.cursorPos.x >= lineBuffer[(int)textBuffer.cursorPos.y].lineEnd) && textBuffer.cursorPos.y < textBuffer.lineCount)
                {
                    textBuffer.cursorPos.y++;
                    textBuffer.cursorPos.x = lineBuffer[(int)textBuffer.cursorPos.y].lineStart;
                }
                else
                {
                    textBuffer.cursorPos.x++;
                }
            }
            break;

        case KEY_UP:
            if (textBuffer.cursorPos.y > 0)
            {
                int previousY = textBuffer.cursorPos.y;
                if (textBuffer.cursorPos.y < 0 || textBuffer.cursorPos.y >= textBuffer.lineCount)
                {
                    textBuffer.cursorPos.y = previousY;
                    return;
                }
                else
                {
                    textBuffer.cursorPos.y--;
                }
                int cursorX = CalculateCursorPosX(previousY);
                textBuffer.cursorPos.x = cursorX;
            }
            break;

        case KEY_DOWN:
            if (textBuffer.cursorPos.y < textBuffer.lineCount - 1)
            {
                int previousY = textBuffer.cursorPos.y;
                if (textBuffer.cursorPos.y < 0 || textBuffer.cursorPos.y >= textBuffer.lineCount)
                {
                    textBuffer.cursorPos.y = previousY;
                    return;
                }
                else
                {
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

int CalculateCursorPosX(int previousY)
{
    int offset = textBuffer.cursorPos.x - lineBuffer[previousY].lineStart;
    int newLineStart = lineBuffer[(int)textBuffer.cursorPos.y].lineStart;
    int newLineEnd = lineBuffer[(int)textBuffer.cursorPos.y].lineEnd;
    int newXPos = newLineStart + offset;
    return (newXPos > newLineEnd) ? newLineEnd : newXPos;
}

/* SELECTION */
void CalculateSelection(int key)
{
    switch (key)
    {
        case KEY_LEFT:
            if (textBuffer.cursorPos.x > 0)
            {
                if (!textBuffer.hasSelectionStarted)
                {
                    textBuffer.hasSelectionStarted = true;
                    textBuffer.selectionEnd = textBuffer.cursorPos.x;
                }
                if (textBuffer.cursorPos.x == lineBuffer[(int)textBuffer.cursorPos.y].lineStart)
                {
                    textBuffer.cursorPos.y--;
                }
                textBuffer.cursorPos.x--;
                textBuffer.selectionStart = textBuffer.cursorPos.x;
                textBuffer.hasSelection = true;
                textBuffer.hasAllSelected = false;
            }
            break;

        case KEY_RIGHT:
            if (textBuffer.cursorPos.x < textBuffer.length)
            {
                if (!textBuffer.hasSelectionStarted)
                {
                    textBuffer.hasSelectionStarted = true;
                    textBuffer.selectionStart = textBuffer.cursorPos.x;
                }
                if ((textBuffer.cursorPos.x >= lineBuffer[(int)textBuffer.cursorPos.y].lineEnd) && textBuffer.cursorPos.y < textBuffer.lineCount)
                {
                    textBuffer.cursorPos.y++;
                    textBuffer.cursorPos.x = lineBuffer[(int)textBuffer.cursorPos.y].lineStart;
                }
                else
                {
                    textBuffer.cursorPos.x++;
                }
                textBuffer.selectionEnd = textBuffer.cursorPos.x;
                textBuffer.hasSelection = true;
                textBuffer.hasAllSelected = false;
            }
            break;

        case KEY_A:
            if (textBuffer.cursorPos.x >= 0 && textBuffer.cursorPos.x <= textBuffer.length)
            {
                textBuffer.selectionStart = 0;
                textBuffer.selectionEnd = textBuffer.length;
                textBuffer.hasSelectionStarted = false;
                textBuffer.hasSelection = true;
                textBuffer.hasAllSelected = true;
            }
            break;

        default:
            break;
    }
}

/* FILE HANDLING */
void LoadFile(void)
{
    FILE *file = fopen(filePath, "r");
    if (file == NULL)
    {
        printf("Error: Unable to open file %s\n", filePath);
        return;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *fileContent = (char *)malloc(fileSize + 1);
    if (fileContent == NULL)
    {
        printf("Error: Memory allocation failed for file content\n");
        fclose(file);
        return;
    }

    fread(fileContent, 1, fileSize, file);
    fileContent[fileSize] = '\0';
    fclose(file);

    free(textBuffer.text);
    textBuffer.text = fileContent;
    textBuffer.length = fileSize;
    textBuffer.capacity = fileSize + 1;
    textBuffer.cursorPos.x = 0;
    textBuffer.cursorPos.y = 0;
    textBuffer.cursorVisible = true;
    textBuffer.lineCount = 0;
    textBuffer.selectionStart = 0;
    textBuffer.selectionEnd = 0;
    textBuffer.hasSelectionStarted = false;
    textBuffer.hasAllSelected = false;
    textBuffer.hasSelection = false;

    TextBufferController();
}

void SaveFile(void)
{
    FILE *file = fopen(filePath, "w");
    if (file == NULL)
    {
        printf("Error: Unable to open file %s for saving\n", filePath);
        return;
    }

    fwrite(textBuffer.text, sizeof(char), textBuffer.length, file);
    fclose(file);
    printf("File saved to %s\n", filePath);
}

/* MEMORY DEALLOCATION */
void FreeTextBuffer(void)
{
    free(textBuffer.text);
    textBuffer.text = NULL;
}

void FreeLineBuffer(void)
{
    free(lineBuffer);
    lineBuffer = NULL;
    lineBufferCapacity = 0;
}

void FreeBufferMemory(void)
{
    FreeTextBuffer();
    FreeLineBuffer();
}