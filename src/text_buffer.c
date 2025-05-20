#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "raylib.h"
#include "raygui.h"
#include "raymath.h"
#include "rlgl.h"

#include "text_buffer.h"
#include "constants.h"
#include "config.h"
#include "screen.h"

TextBuffer textBuffer;
LineBuffer *lineBuffer = NULL;
PreviousBufferState prevBufferState = { NULL, 0, 0, 0 };
CurrentBufferState currentBufferState = { NULL, 0, 0, 0 };
const char *filePath = NULL;
char *copiedText = NULL;
char lineNumberInput[32];
char saveFileInput[256];
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
    KEY_S,
    KEY_G,
    KEY_Z,
    KEY_R,
    KEY_TAB,
    KEY_DELETE
};
int nonPrintableKeysLength = sizeof(nonPrintableKeys) / sizeof(nonPrintableKeys[0]);
float keyDownElapsedTime = 0.0f;
float keyDownDelay = 0.0f;
float maxLineWidth = 0;
float lastCursorUpdateTime;
float lineHeight = FONT_SIZE + TEXT_MARGIN;
double lastBlinkTime;
bool cursorIdle = true;
bool showLineNumberInput = false;
bool showSaveFileInput = false;
bool hasFile = false;
Vector2 scroll = {0, 0};
Rectangle viewport = {0};
Rectangle totalView = {0};
Rectangle panelView = {0};
Rectangle lineNumberInputBox = {0};
Rectangle saveFileInputBox = {0};

/* SETUP */
void SetupTextBuffer(void)
{
    InitializeTextBuffer();
    InitializeLineBuffer();
    lastBlinkTime = GetTime();
    UpdateSidebarWidth();
    TextBufferController();
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
    textBuffer.renderSelection = false;
}

void InitializeLineBuffer(void)
{
    lineBufferCapacity = 1024;
    lineBuffer = (LineBuffer *)malloc(lineBufferCapacity * sizeof(LineBuffer));
}

/* TEXT BUFFER */
void InsertChar(TextBuffer *buffer, char ch)
{
    StorePreviousBufferState();
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
    TextBufferController();
    StoreCurrentBufferState();
}

void RemoveChar(TextBuffer *buffer, int key)
{
    StorePreviousBufferState();
    if ((key == KEY_BACKSPACE && buffer->cursorPos.x > 0) || (key == KEY_DELETE && buffer->cursorPos.x < buffer->length))
    {
        if (key == KEY_BACKSPACE)
        {
            memmove(&buffer->text[(int)buffer->cursorPos.x - 1], &buffer->text[(int)buffer->cursorPos.x], buffer->length - buffer->cursorPos.x + 1);
            buffer->cursorPos.x--;
        }
        else
        {
            memmove(&buffer->text[(int)buffer->cursorPos.x], &buffer->text[(int)buffer->cursorPos.x + 1], buffer->length - buffer->cursorPos.x);
        }
        buffer->length--;
        buffer->text[buffer->length] = '\0';
        if (buffer->cursorPos.x == lineBuffer[(int)buffer->cursorPos.y - 1].lineEnd && buffer->cursorPos.y != 0)
        {
            buffer->cursorPos.y--;
        }
        TextBufferController();
        StoreCurrentBufferState();
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
    UpdateView();
}

void UpdateSidebarWidth(void)
{
    char lineNumberStr[16];
    snprintf(lineNumberStr, sizeof(lineNumberStr), "%d", textBuffer.lineCount);
    Vector2 numberSize = MeasureTextEx(font, lineNumberStr, FONT_SIZE, TEXT_MARGIN);
    sidebarWidth = numberSize.x + SIDEBAR_MARGIN;
}

void StorePreviousBufferState(void)
{
    FreePreviousBufferState();

    prevBufferState.length = textBuffer.length;
    prevBufferState.cursorX = (int)textBuffer.cursorPos.x;
    prevBufferState.cursorY = (int)textBuffer.cursorPos.y;
    prevBufferState.text = (char *)malloc(textBuffer.length + 1);
    if (prevBufferState.text) strcpy(prevBufferState.text, textBuffer.text);
}

void StoreCurrentBufferState(void)
{
    FreeCurrentBufferState();

    currentBufferState.length = textBuffer.length;
    currentBufferState.cursorX = (int)textBuffer.cursorPos.x;
    currentBufferState.cursorY = (int)textBuffer.cursorPos.y;
    currentBufferState.text = (char *)malloc(textBuffer.length + 1);
    if (currentBufferState.text) strcpy(currentBufferState.text, textBuffer.text);
}

/* KEY CONTROLLER */
bool KeyController(void)
{
    if (showLineNumberInput || showSaveFileInput) return false;

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
    if (IsKeyDown(KEY_BACKSPACE)) ProcessKeyDownMovement(KEY_BACKSPACE, shift);
    if (IsKeyDown(KEY_DELETE)) ProcessKeyDownMovement(KEY_DELETE, shift);
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
            ClearSelectionIndicator();
            break;

        case KEY_BACKSPACE:
            if (textBuffer.hasSelection && textBuffer.hasAllSelected)
            {
                StorePreviousBufferState();
                FreeTextBuffer();
                InitializeTextBuffer();
                StoreCurrentBufferState();
            }
            else
            {
                RemoveChar(&textBuffer, KEY_BACKSPACE);
            }
            ClearSelectionIndicator();
            break;

        case KEY_DELETE:
            if (textBuffer.hasSelection && textBuffer.hasAllSelected)
            {
                StorePreviousBufferState();
                FreeTextBuffer();
                InitializeTextBuffer();
                StoreCurrentBufferState();
            }
            else
            {
                RemoveChar(&textBuffer, KEY_DELETE);
            }
            ClearSelectionIndicator();
            break;

        case KEY_ESCAPE:
            ClearSelectionIndicator();
            break;

        case KEY_UP:
            CalculateCursorPosition(KEY_UP);
            ClearSelectionIndicator();
            break;

        case KEY_DOWN:
            CalculateCursorPosition(KEY_DOWN);
            ClearSelectionIndicator();
            break;

        case KEY_LEFT:
            if (ctrl)
            {
                textBuffer.cursorPos.x = lineBuffer[(int)textBuffer.cursorPos.y].lineStart;
                ClearSelectionIndicator();
            }
            else if (shift)
            {
                CalculateSelection(KEY_LEFT);
            }
            else
            {
                CalculateCursorPosition(KEY_LEFT);
                ClearSelectionIndicator();
            }
            break;

        case KEY_RIGHT:
            if (ctrl)
            {
                textBuffer.cursorPos.x = lineBuffer[(int)textBuffer.cursorPos.y].lineEnd;
                ClearSelectionIndicator();
            }
            else if (shift)
            {
                CalculateSelection(KEY_RIGHT);
            }
            else
            {
                CalculateCursorPosition(KEY_RIGHT);
                ClearSelectionIndicator();
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
                    SetClipboardText(copiedText);
                    textBuffer.hasSelectionStarted = false;
                }
            }
            break;

        case KEY_V:
            if (ctrl)
            {
                const char *clipboardText = GetClipboardText();
                if (clipboardText)
                {
                    if (copiedText != NULL) free(copiedText);
                    copiedText = malloc(strlen(clipboardText) + 1);
                    if (copiedText) strcpy(copiedText, clipboardText);
                }

                if (copiedText != NULL)
                {
                    StorePreviousBufferState();
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
                    if (!textBuffer.renderSelection) textBuffer.renderSelection = false;
                    StoreCurrentBufferState();
                }
            }
            break;

        case KEY_A:
            if (ctrl) CalculateSelection(KEY_A);
            break;

        case KEY_S:
            if (ctrl)
            {
                if (!hasFile || shift)
                {
                    showSaveFileInput = true;
                }
                else
                {
                    SaveFile();
                }
                ClearSelectionIndicator();
            }
            break;

        case KEY_G:
            if (ctrl) showLineNumberInput = true;
            break;

        case KEY_Z:
            if (ctrl && prevBufferState.text != NULL)
            {
                free(textBuffer.text);
                textBuffer.text = NULL;
                textBuffer.text = (char *)malloc(prevBufferState.length + 1);

                if (textBuffer.text)
                {
                    strcpy(textBuffer.text, prevBufferState.text);
                    textBuffer.length = prevBufferState.length;
                    textBuffer.cursorPos.x = prevBufferState.cursorX;
                    textBuffer.cursorPos.y = prevBufferState.cursorY;
                    textBuffer.capacity = prevBufferState.length + 1;
                    textBuffer.text[prevBufferState.length] = '\0';
                    TextBufferController();
                }
            }
            break;

        case KEY_R:
            if (ctrl && currentBufferState.text != NULL)
            {
                free(textBuffer.text);
                textBuffer.text = NULL;
                textBuffer.text = (char *)malloc(currentBufferState.length + 1);

                if (textBuffer.text)
                {
                    strcpy(textBuffer.text, currentBufferState.text);
                    textBuffer.length = currentBufferState.length;
                    textBuffer.cursorPos.x = currentBufferState.cursorX;
                    textBuffer.cursorPos.y = currentBufferState.cursorY;
                    textBuffer.capacity = currentBufferState.length + 1;
                    textBuffer.text[currentBufferState.length] = '\0';
                    TextBufferController();
                }
            }
            break;

        case KEY_TAB:
            for (int i = 0; i < 4; i++)
            {
                InsertChar(&textBuffer, (char)KEY_SPACE);
            }
            ClearSelectionIndicator();
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
            if (key == KEY_BACKSPACE || key == KEY_DELETE)
            {
                RemoveChar(&textBuffer, key);
            }
            else
            {
                if (shift) CalculateSelection(key); else CalculateCursorPosition(key);
            }
            keyDownElapsedTime = 0.0f;
        }
        RecordCursorActivity();
        if (!shift) ClearSelectionIndicator();
    }
}

/* RENDERING */
void RenderTextBuffer(void)
{
    float totalWidth = maxLineWidth + (2 * TEXT_MARGIN);
    float totalHeight = (textBuffer.lineCount * lineHeight) + TEXT_MARGIN;

    viewport = (Rectangle){sidebarWidth, 0, GetScreenWidth() - sidebarWidth, GetScreenHeight() - BOTTOM_BAR_HEIGHT};
    totalView = (Rectangle){0, 0, totalWidth, totalHeight};

    int firstVisibleLine = (int)fmaxf(0, -scroll.y / lineHeight);
    int lastVisibleLine = (int)fminf(textBuffer.lineCount, firstVisibleLine + (panelView.height / lineHeight) + 2);

    GuiScrollPanel(viewport, NULL, totalView, &scroll, &panelView);
    DrawRectangle(0, 0, sidebarWidth, GetScreenHeight(), SIDEBAR_COLOR);

    DrawSidebar(firstVisibleLine, lastVisibleLine, lineHeight, scroll.y);
    DrawTextLines(firstVisibleLine, lastVisibleLine, lineHeight, scroll);
    DrawCursor(lineHeight);
    DrawBottomBar();
    DrawSelectionIndicator();
    DrawLineNumberNavInput();
    DrawSaveFileInput();
}

void DrawSidebar(int firstVisibleLine, int lastVisibleLine, float lineHeight, float scrollPosY)
{
    BeginScissorMode(0, 0, sidebarWidth, GetScreenHeight() - BOTTOM_BAR_HEIGHT);
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
        int cursorLineStart = lineBuffer[(int)textBuffer.cursorPos.y].lineStart;
        int charsInLine = textBuffer.cursorPos.x - cursorLineStart;
        char currentLine[1024];
        strncpy(currentLine, &textBuffer.text[cursorLineStart], charsInLine);
        currentLine[charsInLine] = '\0';

        Vector2 textSize = MeasureTextEx(font, currentLine, FONT_SIZE, TEXT_MARGIN);
        int cursorX = sidebarWidth + (2 * TEXT_MARGIN) + textSize.x + scroll.x;
        int cursorY = TEXT_MARGIN + (textBuffer.cursorPos.y * lineHeight) + scroll.y;
        BeginBlendMode(BLEND_CUSTOM);
            rlSetBlendFactors(RL_ONE, RL_ONE, RL_FUNC_SUBTRACT);
            DrawRectangle(cursorX, cursorY, FONT_SIZE / 2, FONT_SIZE, RAYWHITE);
        EndBlendMode();
    }
    BlinkCursor();
}

void DrawLineNumberNavInput(void)
{
    if (showLineNumberInput)
    {
        lineNumberInputBox = (Rectangle){GetScreenWidth() - INPUT_BOX_WIDTH - 12, 0, INPUT_BOX_WIDTH, INPUT_BOX_HEIGHT};
        GuiTextBox(lineNumberInputBox, lineNumberInput, 32, showLineNumberInput);
        DrawOperationHelpText(KEY_G);
        if (IsKeyPressed(KEY_ENTER))
        {
            int line = atoi(lineNumberInput);
            if (line > 0 && line <= textBuffer.lineCount) NavigateToLineNumber(line);
            showLineNumberInput = false;
            strcpy(lineNumberInput, "");
        }

        if (IsKeyPressed(KEY_ESCAPE))
        {
            showLineNumberInput = false;
            strcpy(lineNumberInput, "");
        }
    }
}

void DrawSaveFileInput(void)
{
    if (showSaveFileInput)
    {
        saveFileInputBox = (Rectangle){(GetScreenWidth() - (INPUT_BOX_WIDTH * 2)) / 2, (GetScreenHeight() - BOTTOM_BAR_FONT_SIZE - (INPUT_BOX_HEIGHT * 2)) / 2, INPUT_BOX_WIDTH * 2, INPUT_BOX_HEIGHT * 2};
        GuiTextBox(saveFileInputBox, saveFileInput, 256, showSaveFileInput);
        DrawOperationHelpText(KEY_S);
        filePath = saveFileInput;

        if (IsKeyPressed(KEY_ENTER))
        {
            SaveFile();
            showSaveFileInput = false;
            strcpy(saveFileInput, "");
        }

        if (IsKeyPressed(KEY_ESCAPE))
        {
            showSaveFileInput = false;
            strcpy(saveFileInput, "");
        }
    }
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
    UpdateView();
}

int CalculateCursorPosX(int previousY)
{
    if (textBuffer.cursorPos.y < 0) return lineBuffer[previousY].lineStart;
    if (textBuffer.cursorPos.y >= textBuffer.lineCount) return lineBuffer[previousY].lineEnd;

    int offset = textBuffer.cursorPos.x - lineBuffer[previousY].lineStart;
    int newLineStart = lineBuffer[(int)textBuffer.cursorPos.y].lineStart;
    int newLineEnd = lineBuffer[(int)textBuffer.cursorPos.y].lineEnd;
    int newXPos = newLineStart + offset;
    return (newXPos > newLineEnd) ? newLineEnd : newXPos;
}

void NavigateToLineNumber(int lineNumber)
{
    if (lineNumber && lineNumber > 0 && lineNumber <= textBuffer.lineCount)
    {
        textBuffer.cursorPos.y = lineNumber - 1;
        textBuffer.cursorPos.x = lineBuffer[(int)textBuffer.cursorPos.y].lineStart;
        UpdateView();
    }
}

void UpdateView(void)
{
    if (textBuffer.cursorPos.y < 0 || textBuffer.cursorPos.y >= textBuffer.lineCount) return;

    float cursorX;
    float cursorY = textBuffer.cursorPos.y * lineHeight;

    int cursorLineStart = lineBuffer[(int)textBuffer.cursorPos.y].lineStart;
    int charsInLine = textBuffer.cursorPos.x - cursorLineStart;
    char currentLine[1024];
    strncpy(currentLine, &textBuffer.text[cursorLineStart], charsInLine);
    currentLine[charsInLine] = '\0';

    Vector2 textSize = MeasureTextEx(font, currentLine, FONT_SIZE, TEXT_MARGIN);
    cursorX = textSize.x;

    float cursorScreenX = sidebarWidth + TEXT_MARGIN + cursorX + scroll.x;
    float cursorScreenY = TEXT_MARGIN + cursorY + scroll.y;

    if (cursorScreenX < sidebarWidth + TEXT_MARGIN)
    {
        scroll.x += (sidebarWidth + TEXT_MARGIN) - cursorScreenX;
    }
    else if (cursorScreenX > (panelView.width + sidebarWidth - TEXT_MARGIN))
    {
        scroll.x -= cursorScreenX - (panelView.width + sidebarWidth - TEXT_MARGIN);
    }

    if (cursorScreenY < TEXT_MARGIN)
    {
        scroll.y += TEXT_MARGIN - cursorScreenY;
    }
    else if (cursorScreenY > panelView.height - lineHeight)
    {
        scroll.y -= cursorScreenY - (panelView.height - lineHeight);
    }
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
                textBuffer.renderSelection = true;
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
                textBuffer.renderSelection = true;
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
                textBuffer.renderSelection = true;
            }
            break;

        default:
            break;
    }
    UpdateView();
}

void DrawSelectionIndicator(void)
{
    if (!textBuffer.hasSelection || !textBuffer.renderSelection) return;
    int selStart = textBuffer.selectionStart;
    int selEnd = textBuffer.selectionEnd;
    if (selStart > selEnd)
    {
        int tmp = selStart;
        selStart = selEnd;
        selEnd = tmp;
    }

    for (int i = 0; i < textBuffer.lineCount; i++)
    {
        int lineStart = lineBuffer[i].lineStart;
        int lineEnd = lineBuffer[i].lineEnd;

        int start = (selStart > lineStart) ? selStart : lineStart;
        int end = (selEnd < lineEnd) ? selEnd : lineEnd;
        if (end <= start) continue;

        int prefixLen = start - lineStart;
        char prefix[1024];
        strncpy(prefix, &textBuffer.text[lineStart], prefixLen);
        prefix[prefixLen] = '\0';
        Vector2 prefixSize = MeasureTextEx(font, prefix, FONT_SIZE, TEXT_MARGIN);

        int selectionLen = end - start;
        char selectionText[1024];
        strncpy(selectionText, &textBuffer.text[start], selectionLen);
        selectionText[selectionLen] = '\0';
        Vector2 selectionSize = MeasureTextEx(font, selectionText, FONT_SIZE, TEXT_MARGIN);

        int rectX = sidebarWidth + TEXT_MARGIN + (int)scroll.x + (int)prefixSize.x;
        int rectY = TEXT_MARGIN + (int)(i * lineHeight) + (int)scroll.y;

        BeginBlendMode(BLEND_CUSTOM);
            rlSetBlendFactors(RL_ONE, RL_ONE, RL_FUNC_SUBTRACT);
            DrawRectangle(rectX, rectY, (int)selectionSize.x, FONT_SIZE, WHITE);
        EndBlendMode();
    }
}

void ClearSelectionIndicator(void)
{
    if (textBuffer.renderSelection) textBuffer.renderSelection = false;
    if (textBuffer.hasSelectionStarted) textBuffer.hasSelectionStarted = false;
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
    textBuffer.renderSelection = false;

    hasFile = true;
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

void FreePreviousBufferState(void)
{
    if (prevBufferState.text != NULL)
    {
        free(prevBufferState.text);
        prevBufferState.text = NULL;
    }
}

void FreeCurrentBufferState(void)
{
    if (currentBufferState.text != NULL)
    {
        free(currentBufferState.text);
        currentBufferState.text = NULL;
    }
}

void FreeBufferMemory(void)
{
    FreeTextBuffer();
    FreeLineBuffer();
    FreePreviousBufferState();
    FreeCurrentBufferState();
}