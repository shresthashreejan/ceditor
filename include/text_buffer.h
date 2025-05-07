#ifndef TEXT_BUFFER_H_
#define TEXT_BUFFER_H_

extern const char *filePath;

typedef struct {
    char *text;
    int length;
    int capacity;
    Vector2 cursorPos;
    bool cursorVisible;
    int lineCount;
    int selectionStart;
    int selectionEnd;
    bool hasSelectionStarted;
    bool hasAllSelected;
    bool hasSelection;
} TextBuffer;

typedef struct {
    int lineCount;
    int lineLength;
    int lineStart;
    int lineEnd;
} LineBuffer;

void SetupTextBuffer(void);
void InitializeTextBuffer(void);
void InitializeLineBuffer(void);
void InsertChar(TextBuffer *buffer, char ch);
void RemoveChar(TextBuffer *buffer);
bool KeyController(void);
void TextBufferController(void);
void FreeTextBuffer(void);
void FreeLineBuffer(void);
void FreeBufferMemory(void);
int CalculateCursorPosX(int previousY);
void BlinkCursor(void);
void CalculateCursorPosition(int key);
void CalculateSelection(int key);
void UpdateView(void);
void LoadFile(void);
void SaveFile(void);
void ProcessKey(int key, bool ctrl, bool shift);
void ProcessKeyDownMovement(int key, bool shift);
void RenderTextBuffer(void);
void UpdateSidebarWidth(void);
void DrawSidebar(int firstVisibleLine, int lastVisibleLine, float lineHeight, float scrollPosY);
void DrawTextLines(int firstVisibleLine, int lastVisibleLine, float lineHeight, Vector2 scroll);
void DrawCursor(float lineHeight);
void RecordCursorActivity(void);
void UpdateCursorState(void);

#endif