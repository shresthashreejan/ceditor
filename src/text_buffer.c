#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

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
BufferSnapshot undoStack[HISTORY_LIMIT];
BufferSnapshot redoStack[HISTORY_LIMIT];
SearchIndex searchIndex;
const char *filePath = NULL;
char *copiedText = NULL;
char lineNumberInput[32];
char saveFileInput[256];
char searchInput[256];
char filepathHelpText[256];
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
    KEY_DELETE,
    KEY_F
};
int nonPrintableKeysLength = sizeof(nonPrintableKeys) / sizeof(nonPrintableKeys[0]);
int keyDownKeys[] = {
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_BACKSPACE,
    KEY_DELETE,
    KEY_Z,
    KEY_R
};
int keyDownKeysLength = sizeof(keyDownKeys) / sizeof(keyDownKeys[0]);
int undoTop = -1;
int redoTop = -1;
int matchPosition;
int searchIterationCount = 0;
int highestXOffset = 0;
float keyDownElapsedTime = 0.0f;
float keyDownDelay = 0.0f;
float maxLineWidth = 0;
float lastCursorUpdateTime;
float lineHeight = FONT_SIZE + TEXT_MARGIN;
double lastBlinkTime;
bool cursorIdle = true;
bool showLineNumberInput = false;
bool showSaveFileInput = false;
bool showSearchInput = false;
bool showSearchHelpText = false;
bool showSaveHelpText = false;
bool hasFile = false;
Vector2 scroll = {0, 0};
Rectangle viewport = {0};
Rectangle totalView = {0};
Rectangle panelView = {0};
Rectangle lineNumberInputBox = {0};
Rectangle saveFileInputBox = {0};
Rectangle searchInputBox = {0};

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
    textBuffer.text = (char *)malloc(BUFFER_SIZE);
    textBuffer.length = 0;
    textBuffer.capacity = BUFFER_SIZE;
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
    lineBufferCapacity = BUFFER_SIZE;
    lineBuffer = (LineBuffer *)malloc(lineBufferCapacity * sizeof(LineBuffer));
}

/* TEXT BUFFER */
void InsertChar(TextBuffer *buffer, char ch)
{
    StoreUndo();
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
    showSaveHelpText = false;
}

void RemoveChar(TextBuffer *buffer, int key, bool storeUndoFlag)
{
    if (storeUndoFlag) StoreUndo();
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
        showSaveHelpText = false;
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

            char line[BUFFER_SIZE];
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

/* SIDEBAR */
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
    if (showLineNumberInput || showSaveFileInput || showSearchInput) return false;

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

    for (int i = 0; i < keyDownKeysLength; i++)
    {
        if (IsKeyDown(keyDownKeys[i]))
        {
            ProcessKeyDown(keyDownKeys[i], ctrl, shift);
        }
        if (keyDownDelay != 0.0f) ResetKeyDownDelay(keyDownKeys[i]);
    }

    return isAnyKeyPressed;
}

void ResetKeyDownDelay(int key)
{
    if (IsKeyReleased(key))
    {
        keyDownDelay = 0.0f;
    }
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
            if (textBuffer.hasSelection && textBuffer.hasAllSelected && textBuffer.renderSelection)
            {
                StoreUndo();
                FreeTextBuffer();
                InitializeTextBuffer();
            }
            else if (textBuffer.hasSelection && !textBuffer.hasAllSelected && textBuffer.renderSelection)
            {
                HandleSelectionDelete();
            }
            else if (ctrl && !shift)
            {
                HandleCtrlHoldOperations(KEY_BACKSPACE);
            }
            else
            {
                RemoveChar(&textBuffer, KEY_BACKSPACE, true);
            }
            ClearSelectionIndicator();
            break;

        case KEY_DELETE:
            if (textBuffer.hasSelection && textBuffer.hasAllSelected && textBuffer.renderSelection)
            {
                StoreUndo();
                FreeTextBuffer();
                InitializeTextBuffer();
            }
            else if (textBuffer.hasSelection && !textBuffer.hasAllSelected && textBuffer.renderSelection)
            {
                HandleSelectionDelete();
            }
            else if (ctrl && !shift)
            {
                HandleCtrlHoldOperations(KEY_DELETE);
            }
            else
            {
                RemoveChar(&textBuffer, KEY_DELETE, true);
            }
            ClearSelectionIndicator();
            break;

        case KEY_ESCAPE:
            ClearSelectionIndicator();
            break;

        case KEY_UP:
            if (shift)
            {
                CalculateSelection(KEY_UP);
            }
            else
            {
                CalculateCursorPosition(KEY_UP);
                ClearSelectionIndicator();
            }
            break;

        case KEY_DOWN:
            if (shift)
            {
                CalculateSelection(KEY_DOWN);
            }
            else
            {
                CalculateCursorPosition(KEY_DOWN);
                ClearSelectionIndicator();
            }
            break;

        case KEY_LEFT:
            if (ctrl && !shift)
            {
                HandleCtrlHoldOperations(KEY_LEFT);
                ClearSelectionIndicator();
            }
            else if (!ctrl && shift)
            {
                CalculateSelection(KEY_LEFT);
            }
            else if (ctrl && shift)
            {
                textBuffer.cursorPos.x = lineBuffer[(int)textBuffer.cursorPos.y].lineStart;
                ClearSelectionIndicator();
            }
            else
            {
                CalculateCursorPosition(KEY_LEFT);
                ClearSelectionIndicator();
            }
            break;

        case KEY_RIGHT:
            if (ctrl && !shift)
            {
                HandleCtrlHoldOperations(KEY_RIGHT);
                ClearSelectionIndicator();
            }
            else if (!ctrl && shift)
            {
                CalculateSelection(KEY_RIGHT);
            }
            else if (ctrl && shift)
            {
                textBuffer.cursorPos.x = lineBuffer[(int)textBuffer.cursorPos.y].lineEnd;
                ClearSelectionIndicator();
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
                    StoreUndo();
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

        case KEY_F:
            if (ctrl && !shift)
            {
                showSearchInput = true;
                showSearchHelpText = true;
            }
            break;

        case KEY_Z:
            if (ctrl && !shift)
            {
                Undo();
            }
            else if (ctrl && shift)
            {
                Redo();
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

void ProcessKeyDown(int key, bool ctrl, bool shift)
{
    float frameTime = GetFrameTime();
    keyDownDelay += frameTime;
    keyDownElapsedTime += frameTime;
    if (keyDownDelay >= KEY_DOWN_DELAY)
    {
        if (keyDownElapsedTime >= KEY_DOWN_INTERVAL)
        {
            switch (key)
            {
                case KEY_BACKSPACE:
                    if (ctrl && !shift)
                    {
                        HandleCtrlHoldOperations(KEY_BACKSPACE);
                    }
                    else if (!ctrl && !shift)
                    {
                        RemoveChar(&textBuffer, key, true);
                    }
                    break;

                case KEY_DELETE:
                    if (ctrl && !shift)
                    {
                        HandleCtrlHoldOperations(KEY_DELETE);
                    }
                    else if (!ctrl && !shift)
                    {
                        RemoveChar(&textBuffer, key, true);
                    }
                    break;

                case KEY_Z:
                    if (ctrl) Undo();
                    break;

                case KEY_R:
                    if (ctrl) Redo();
                    break;

                case KEY_LEFT:
                    if (ctrl && !shift)
                    {
                        HandleCtrlHoldOperations(KEY_LEFT);
                        ClearSelectionIndicator();
                    }
                    else if (!ctrl && shift)
                    {
                        CalculateSelection(key);
                    }
                    else
                    {
                        CalculateCursorPosition(key);
                    }
                    break;

                case KEY_RIGHT:
                    if (ctrl && !shift)
                    {
                        HandleCtrlHoldOperations(KEY_RIGHT);
                        ClearSelectionIndicator();
                    }
                    else if (!ctrl && shift)
                    {
                        CalculateSelection(key);
                    }
                    else
                    {
                        CalculateCursorPosition(key);
                    }
                    break;

                default:
                    if (!ctrl && shift) CalculateSelection(key); else CalculateCursorPosition(key);
                    break;
            }
            keyDownElapsedTime = 0.0f;
        }
        RecordCursorActivity();
        if (!shift) ClearSelectionIndicator();
    }
}

void HandleCtrlHoldOperations(int key)
{
    switch (key)
    {
        case KEY_LEFT:
            if (textBuffer.cursorPos.x > 0)
            {
                int lineStart = lineBuffer[(int)textBuffer.cursorPos.y].lineStart;
                if ((int)textBuffer.cursorPos.x == lineStart && textBuffer.cursorPos.y > 0)
                {
                    textBuffer.cursorPos.y--;
                    textBuffer.cursorPos.x = lineBuffer[(int)textBuffer.cursorPos.y].lineEnd;
                    return;
                }
                for (int i = (int)textBuffer.cursorPos.x - 1; i >= lineStart; i--)
                {
                    if (textBuffer.text[i] == KEY_SPACE)
                    {
                        textBuffer.cursorPos.x = i;
                        break;
                    }
                    if (i == lineStart)
                    {
                        textBuffer.cursorPos.x = lineStart;
                        break;
                    }
                }
            }
            break;

        case KEY_RIGHT:
            if (textBuffer.cursorPos.x < textBuffer.length)
            {
                int lineEnd = lineBuffer[(int)textBuffer.cursorPos.y].lineEnd;
                if (textBuffer.cursorPos.y < textBuffer.lineCount - 1 && (int)textBuffer.cursorPos.x == lineEnd)
                {
                    textBuffer.cursorPos.y++;
                    textBuffer.cursorPos.x = lineBuffer[(int)textBuffer.cursorPos.y].lineStart;
                    return;
                }
                for (int i = (int)textBuffer.cursorPos.x + 1; i <= lineEnd; i++)
                {
                    if (textBuffer.text[i] == KEY_SPACE)
                    {
                        textBuffer.cursorPos.x = i;
                        break;
                    }
                    if (i == lineEnd)
                    {
                        textBuffer.cursorPos.x = lineEnd;
                        break;
                    }
                }
            }
            break;

        case KEY_BACKSPACE:
            if (textBuffer.cursorPos.x > 0)
            {
                int wordStartIndex;
                int wordEndIndex;
                int lineStart = lineBuffer[(int)textBuffer.cursorPos.y].lineStart;
                if ((int)textBuffer.cursorPos.x == lineStart && textBuffer.cursorPos.y > 0)
                {
                    int prevLineIndex = (int)textBuffer.cursorPos.y - 1;
                    textBuffer.cursorPos.x = lineBuffer[prevLineIndex].lineEnd;
                    RemoveChar(&textBuffer, KEY_DELETE, true);
                    return;
                }
                for (int i = (int)textBuffer.cursorPos.x - 1; i >= lineStart; i--)
                {
                    if (textBuffer.text[i] == KEY_SPACE)
                    {
                        wordEndIndex = textBuffer.cursorPos.x;
                        wordStartIndex = i;
                        break;
                    }
                    if (i == lineStart)
                    {
                        wordEndIndex = textBuffer.cursorPos.x;
                        wordStartIndex = lineStart;
                        break;
                    }
                }
                if (wordEndIndex > wordStartIndex)
                {
                    StoreUndo();
                    for (int i = wordStartIndex; i < wordEndIndex; i++)
                    {
                        RemoveChar(&textBuffer, KEY_BACKSPACE, false);
                    }
                }
            }
            break;

        case KEY_DELETE:
            if (textBuffer.cursorPos.x < textBuffer.length)
            {
                int wordStartIndex;
                int wordEndIndex;
                int lineEnd = lineBuffer[(int)textBuffer.cursorPos.y].lineEnd;
                if (textBuffer.cursorPos.y < textBuffer.lineCount - 1 && (int)textBuffer.cursorPos.x == lineEnd)
                {
                    int nextLineIndex = textBuffer.cursorPos.y + 1;
                    textBuffer.cursorPos.x = lineBuffer[nextLineIndex].lineStart;
                    RemoveChar(&textBuffer, KEY_BACKSPACE, true);
                    return;
                }
                for (int i = (int)textBuffer.cursorPos.x + 1; i <= lineEnd; i++)
                {
                    if (textBuffer.text[i] == KEY_SPACE)
                    {
                        wordStartIndex = textBuffer.cursorPos.x;
                        wordEndIndex = i;
                        break;
                    }
                    if (i == lineEnd)
                    {
                        wordStartIndex = textBuffer.cursorPos.x;
                        wordEndIndex = lineEnd;
                        break;
                    }
                }
                if (wordEndIndex > wordStartIndex)
                {
                    StoreUndo();
                    for (int i = wordStartIndex; i < wordEndIndex; i++)
                    {
                        RemoveChar(&textBuffer, KEY_DELETE, false);
                    }
                }
            }
            break;

        default:
            break;
    }
    UpdateView();
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
    DrawSearchIndicator();
    DrawLineNumberNavInput();
    DrawSaveFileInput();
    DrawSearchInput();
    DrawSaveHelpText();
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
            DrawTextEx(font, lineNumberStr, linePos, FONT_SIZE, TEXT_MARGIN, RAYWHITE);
        }
    EndScissorMode();
}

void DrawTextLines(int firstVisibleLine, int lastVisibleLine, float lineHeight, Vector2 scroll)
{
    BeginScissorMode(panelView.x, panelView.y, panelView.width, panelView.height);
        for (int i = firstVisibleLine; i < lastVisibleLine; i++)
        {
            if (i >= lineBufferCapacity) break;

            char line[BUFFER_SIZE];
            strncpy(line, &textBuffer.text[lineBuffer[i].lineStart], lineBuffer[i].lineLength);
            line[lineBuffer[i].lineLength] = '\0';

            Vector2 linePos = {
                sidebarWidth + TEXT_MARGIN + scroll.x,
                (i * lineHeight) + scroll.y
            };
            DrawTextEx(font, line, linePos, FONT_SIZE, TEXT_MARGIN, RAYWHITE);
        }
    EndScissorMode();
}

void DrawCursor(float lineHeight)
{
    if (textBuffer.cursorVisible)
    {
        int cursorLineStart = lineBuffer[(int)textBuffer.cursorPos.y].lineStart;
        int charsInLine = textBuffer.cursorPos.x - cursorLineStart;
        char currentLine[BUFFER_SIZE];
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
        char helpText[256] = "Enter line number.";
        showSaveHelpText = false;
        DrawHelpText(helpText);
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
        saveFileInputBox = (Rectangle){(GetScreenWidth() - (INPUT_BOX_WIDTH)) / 2, (GetScreenHeight() - BOTTOM_BAR_FONT_SIZE - (INPUT_BOX_HEIGHT)) / 2, INPUT_BOX_WIDTH, INPUT_BOX_HEIGHT};
        GuiTextBox(saveFileInputBox, saveFileInput, 256, showSaveFileInput);
        char helpText[256] = "Enter file path to save.";
        showSaveHelpText = false;
        DrawHelpText(helpText);

        if (saveFileInput[0] != '\0')
        {
            filePath = saveFileInput;
            hasFile = true;

            if (IsKeyPressed(KEY_ENTER))
            {
                SaveFile();
                showSaveFileInput = false;
            }
        }

        if (IsKeyPressed(KEY_ESCAPE))
        {
            showSaveFileInput = false;
        }
    }
}

void DrawSearchInput(void)
{
    static char prevSearchInput[256] = "";
    if (showSearchInput)
    {
        searchInputBox = (Rectangle){GetScreenWidth() - INPUT_BOX_WIDTH - 12, 0, INPUT_BOX_WIDTH, INPUT_BOX_HEIGHT};
        GuiTextBox(searchInputBox, searchInput, 256, showSearchInput);

        char searchHelpText[256] = "Enter search string.";
        char matchHelpText[256];
        sprintf(matchHelpText, "Match found at line %d.", matchPosition);

        showSaveHelpText = false;
        if (showSearchHelpText) DrawHelpText(searchHelpText);
        if (!showSearchHelpText) DrawHelpText(matchHelpText);
        SearchText(prevSearchInput, searchInput);
    }
}

void DrawSaveHelpText(void)
{
    if (showSaveHelpText)
    {
        DrawHelpText(filepathHelpText);
    }
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
    DrawIndicatorOverlay(selStart, selEnd, LIGHTBLUE);
}

void DrawSearchIndicator(void)
{
    if (searchIndex.hasStringMatch) DrawIndicatorOverlay(searchIndex.start, searchIndex.end, GREEN);
}

void DrawIndicatorOverlay(int startIndex, int endIndex, Color color)
{
    for (int i = 0; i < textBuffer.lineCount; i++)
    {
        int lineStart = lineBuffer[i].lineStart;
        int lineEnd = lineBuffer[i].lineEnd;

        int start = (startIndex > lineStart) ? startIndex : lineStart;
        int end = (endIndex < lineEnd) ? endIndex : lineEnd;
        if (end <= start) continue;

        int prefixLen = start - lineStart;
        char prefix[BUFFER_SIZE];
        strncpy(prefix, &textBuffer.text[lineStart], prefixLen);
        prefix[prefixLen] = '\0';
        Vector2 prefixSize = MeasureTextEx(font, prefix, FONT_SIZE, TEXT_MARGIN);

        int selectionLen = end - start;
        char selectionText[BUFFER_SIZE];
        strncpy(selectionText, &textBuffer.text[start], selectionLen);
        selectionText[selectionLen] = '\0';
        Vector2 selectionSize = MeasureTextEx(font, selectionText, FONT_SIZE, TEXT_MARGIN);

        int rectX = sidebarWidth + TEXT_MARGIN + (int)scroll.x + (int)prefixSize.x;
        int rectY = TEXT_MARGIN + (int)(i * lineHeight) + (int)scroll.y;

        BeginBlendMode(BLEND_CUSTOM);
            rlSetBlendFactors(RL_ONE, RL_ONE, RL_FUNC_SUBTRACT);
            DrawRectangle(rectX, rectY, (int)selectionSize.x, FONT_SIZE, RAYWHITE);
            DrawRectangle(rectX, rectY, (int)selectionSize.x, FONT_SIZE, color);
        EndBlendMode();
    }
}

void UpdateView(void)
{
    if (textBuffer.cursorPos.y < 0 || textBuffer.cursorPos.y >= textBuffer.lineCount) return;

    float cursorX;
    float cursorY = textBuffer.cursorPos.y * lineHeight;

    int cursorLineStart = lineBuffer[(int)textBuffer.cursorPos.y].lineStart;
    int charsInLine = textBuffer.cursorPos.x - cursorLineStart;
    char currentLine[BUFFER_SIZE];
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

/* TEXT SEARCH */
void SearchText(char prevSearchInput[256], char searchInput[256])
{
    if (searchInput[0] != '\0')
    {
        size_t searchStringLen = strlen(searchInput);
        if (IsKeyPressed(KEY_ENTER))
        {
            char *searchStartPtr = textBuffer.text;

            if (strcmp(prevSearchInput, searchInput) != 0)
            {
                strcpy(prevSearchInput, searchInput);
                searchIndex.hasStringMatch = false;
            }
            else if (searchIndex.hasStringMatch)
            {
                searchStartPtr = textBuffer.text + searchIndex.end;
            }

            char *match = SearchStringIgnoreCase(searchStartPtr, searchInput);
            if (match != NULL)
            {
                searchIndex.hasStringMatch = true;
                searchIndex.start = (int)(match - textBuffer.text);
                searchIndex.end = searchIndex.start + searchStringLen;
                showSearchHelpText = false;
                CalculateCursorPosBasedOnPosX(searchIndex.start);
                matchPosition = textBuffer.cursorPos.y + 1;
                searchIterationCount++;
                UpdateView();
            }
            else
            {
                if (searchIterationCount > 0)
                {
                    searchIndex.hasStringMatch = false;
                    searchStartPtr = textBuffer.text;
                    SearchText(prevSearchInput, searchInput);
                }
            }
        }
    }

    if (IsKeyPressed(KEY_ESCAPE))
    {
        showSearchInput = false;
        searchIndex.hasStringMatch = false;
    }
}

char *SearchStringIgnoreCase(const char *haystack, const char *needle)
{
    if (*needle == '\0') return (char *)haystack;

    while (*haystack)
    {
        const char *h = haystack;
        const char *n = needle;

        while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n))
        {
            h++;
            n++;
        }

        if (*n == '\0') return (char *)haystack;
        haystack++;
    }
    return NULL;
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
                highestXOffset = 0;
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
                highestXOffset = 0;
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
            else if (textBuffer.cursorPos.y == 0)
            {
                textBuffer.cursorPos.x = lineBuffer[(int)textBuffer.cursorPos.y].lineStart;
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
            else if (textBuffer.cursorPos.y == textBuffer.lineCount - 1)
            {
                textBuffer.cursorPos.x = lineBuffer[(int)textBuffer.cursorPos.y].lineEnd;
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

    if (newXPos > newLineEnd)
    {
        if (offset > highestXOffset)
        {
            highestXOffset = offset;
        }
        return newLineEnd;
    }

    if (highestXOffset)
    {
        int xPosition = newLineStart + highestXOffset;
        return xPosition <= newLineEnd ? xPosition : newLineEnd;
    }

    return newXPos;
}

void CalculateCursorPosBasedOnPosX(int cursorX)
{
    for (int i = 0; i < textBuffer.lineCount; i++)
    {
        if (cursorX >= lineBuffer[i].lineStart && cursorX <= lineBuffer[i].lineEnd)
        {
            textBuffer.cursorPos.x = cursorX;
            textBuffer.cursorPos.y = lineBuffer[i].lineCount;
        }
    }
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

/* SELECTION */
void CalculateSelection(int key)
{
    switch (key)
    {
        case KEY_LEFT:
            if (textBuffer.cursorPos.x > 0)
            {
                if (textBuffer.hasSelection && textBuffer.hasSelectionStarted && textBuffer.isForwardSelection)
                {
                    if (textBuffer.cursorPos.x == lineBuffer[(int)textBuffer.cursorPos.y].lineStart)
                    {
                        textBuffer.cursorPos.y--;
                    }
                    textBuffer.cursorPos.x--;
                    textBuffer.selectionEnd = textBuffer.cursorPos.x;
                }
                else
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
                    textBuffer.isForwardSelection = false;
                }
                textBuffer.hasSelection = true;
                textBuffer.hasAllSelected = false;
                textBuffer.renderSelection = true;
            }
            break;

        case KEY_RIGHT:
            if (textBuffer.cursorPos.x < textBuffer.length)
            {
                if (textBuffer.hasSelection && textBuffer.hasSelectionStarted && !textBuffer.isForwardSelection)
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
                    textBuffer.selectionStart = textBuffer.cursorPos.x;
                }
                else
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
                    textBuffer.isForwardSelection = true;
                }
                textBuffer.hasSelection = true;
                textBuffer.hasAllSelected = false;
                textBuffer.renderSelection = true;
            }
            break;

        case KEY_UP:
            if (textBuffer.cursorPos.y > 0)
            {
                if (textBuffer.hasSelection && textBuffer.hasSelectionStarted && textBuffer.isForwardSelection)
                {
                    textBuffer.cursorPos.y--;
                    int offset = textBuffer.cursorPos.x - lineBuffer[(int)textBuffer.cursorPos.y + 1].lineStart;
                    if (lineBuffer[(int)textBuffer.cursorPos.y].lineStart + offset > lineBuffer[(int)textBuffer.cursorPos.y].lineEnd)
                    {
                        textBuffer.cursorPos.x = lineBuffer[(int)textBuffer.cursorPos.y].lineEnd;
                    }
                    else
                    {
                        textBuffer.cursorPos.x = lineBuffer[(int)textBuffer.cursorPos.y].lineStart + offset;
                    }
                    textBuffer.selectionEnd = textBuffer.cursorPos.x;
                }
                else
                {
                    if (!textBuffer.hasSelectionStarted)
                    {
                        textBuffer.hasSelectionStarted = true;
                        textBuffer.selectionEnd = textBuffer.cursorPos.x;
                    }
                    textBuffer.cursorPos.y--;
                    int offset = textBuffer.cursorPos.x - lineBuffer[(int)textBuffer.cursorPos.y + 1].lineStart;
                    if (lineBuffer[(int)textBuffer.cursorPos.y].lineStart + offset > lineBuffer[(int)textBuffer.cursorPos.y].lineEnd)
                    {
                        textBuffer.cursorPos.x = lineBuffer[(int)textBuffer.cursorPos.y].lineEnd;
                    }
                    else
                    {
                        textBuffer.cursorPos.x = lineBuffer[(int)textBuffer.cursorPos.y].lineStart + offset;
                    }
                    textBuffer.selectionStart = textBuffer.cursorPos.x;
                    textBuffer.isForwardSelection = false;
                }
                textBuffer.hasSelection = true;
                textBuffer.hasAllSelected = false;
                textBuffer.renderSelection = true;
            }
            break;

        case KEY_DOWN:
            if (textBuffer.cursorPos.y < textBuffer.lineCount - 1)
            {
                if (textBuffer.hasSelection && textBuffer.hasSelectionStarted && !textBuffer.isForwardSelection)
                {
                    textBuffer.cursorPos.y++;
                    int offset = textBuffer.cursorPos.x - lineBuffer[(int)textBuffer.cursorPos.y - 1].lineStart;
                    if (lineBuffer[(int)textBuffer.cursorPos.y].lineStart + offset > lineBuffer[(int)textBuffer.cursorPos.y].lineEnd || textBuffer.cursorPos.y == textBuffer.lineCount - 1)
                    {
                        textBuffer.cursorPos.x = lineBuffer[(int)textBuffer.cursorPos.y].lineEnd;
                    }
                    else
                    {
                        textBuffer.cursorPos.x = lineBuffer[(int)textBuffer.cursorPos.y].lineStart + offset;
                    }
                    textBuffer.selectionStart = textBuffer.cursorPos.x;
                }
                else
                {
                    if (!textBuffer.hasSelectionStarted)
                    {
                        textBuffer.hasSelectionStarted = true;
                        textBuffer.selectionStart = textBuffer.cursorPos.x;
                    }
                    textBuffer.cursorPos.y++;
                    int offset = textBuffer.cursorPos.x - lineBuffer[(int)textBuffer.cursorPos.y - 1].lineStart;
                    if (lineBuffer[(int)textBuffer.cursorPos.y].lineStart + offset > lineBuffer[(int)textBuffer.cursorPos.y].lineEnd || textBuffer.cursorPos.y == textBuffer.lineCount - 1)
                    {
                        textBuffer.cursorPos.x = lineBuffer[(int)textBuffer.cursorPos.y].lineEnd;
                    }
                    else
                    {
                        textBuffer.cursorPos.x = lineBuffer[(int)textBuffer.cursorPos.y].lineStart + offset;
                    }
                    textBuffer.selectionEnd = textBuffer.cursorPos.x;
                    textBuffer.isForwardSelection = true;
                }
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

void ClearSelectionIndicator(void)
{
    if (textBuffer.renderSelection) textBuffer.renderSelection = false;
    if (textBuffer.hasSelectionStarted) textBuffer.hasSelectionStarted = false;
}

void HandleSelectionDelete(void)
{
    int selectionLength = textBuffer.selectionEnd - textBuffer.selectionStart;
    if (selectionLength > 0)
    {
        int operationKey = textBuffer.isForwardSelection ? KEY_BACKSPACE : KEY_DELETE;
        StoreUndo();
        for (int i = textBuffer.selectionStart; i < textBuffer.selectionEnd; i++)
        {
            RemoveChar(&textBuffer, operationKey, false);
        }
    }
}

/* FILE HANDLING */
void LoadFile(void)
{
    FILE *file = fopen(filePath, "r");
    if (file == NULL)
    {
        printf("Error: File not found at the specified path: %s\n", filePath);
        exit(1);
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
        sprintf(filepathHelpText, "No file found at %s\n", filePath);
        showSaveHelpText = true;
        return;
    }

    fwrite(textBuffer.text, sizeof(char), textBuffer.length, file);
    fclose(file);
    sprintf(filepathHelpText, "File saved to %s\n", filePath);
    showSaveHelpText = true;
}

/* BUFFER SNAPSHOT */
void StoreUndo(void)
{
    SaveSnapshot(undoStack, &undoTop);
    for (int i = 0; i <= redoTop; i++)
    {
        FreeSnapshot(&redoStack[i]);
    }
    redoTop = -1;
}

void SaveSnapshot(BufferSnapshot *snap, int *top)
{
    if (*top == HISTORY_LIMIT - 1)
    {
        FreeSnapshot(&snap[0]);
        memmove(&snap[0], &snap[1], sizeof(BufferSnapshot) * (HISTORY_LIMIT - 1));
        *top = *top - 1;
    }

    *top = *top + 1;
    BufferSnapshot *snapshot = &snap[*top];
    snapshot->length = textBuffer.length;
    snapshot->cursorX = (int)textBuffer.cursorPos.x;
    snapshot->cursorY = (int)textBuffer.cursorPos.y;
    snapshot->text = malloc(textBuffer.length + 1);
    strcpy(snapshot->text, textBuffer.text);
}

void ApplySnapshot(BufferSnapshot *snap)
{
    free(textBuffer.text);
    textBuffer.text = malloc(snap->length + 1);
    strcpy(textBuffer.text, snap->text);
    textBuffer.length = snap->length;
    textBuffer.capacity = snap->length + 1;
    textBuffer.cursorPos.x = snap->cursorX;
    textBuffer.cursorPos.y = snap->cursorY;
    TextBufferController();
}

void Undo(void)
{
    if (undoTop < 0) return;
    SaveSnapshot(redoStack, &redoTop);
    ApplySnapshot(&undoStack[undoTop--]);
}

void Redo(void)
{
    if (redoTop < 0) return;
    SaveSnapshot(undoStack, &undoTop);
    ApplySnapshot(&redoStack[redoTop--]);
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

void FreeSnapshot(BufferSnapshot *snap)
{
    if (snap->text) free(snap->text);
    snap->text = NULL;
}

void FreeHistoryStacks(void)
{
    for (int i = 0; i <= undoTop; i++)
    {
        FreeSnapshot(&undoStack[i]);
    }
    for (int i = 0; i <= redoTop; i++)
    {
        FreeSnapshot(&redoStack[i]);
    }
    undoTop = -1;
    redoTop = -1;
}

void FreeBufferMemory(void)
{
    FreeTextBuffer();
    FreeLineBuffer();
    FreeHistoryStacks();
}