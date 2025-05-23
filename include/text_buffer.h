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
    bool renderSelection;
} TextBuffer;

typedef struct {
    int lineCount;
    int lineLength;
    int lineStart;
    int lineEnd;
} LineBuffer;

typedef struct {
    char *text;
    int length;
    int cursorX;
    int cursorY;
} BufferSnapshot;

void SetupTextBuffer(void);
void InitializeTextBuffer(void);
void InitializeLineBuffer(void);
void InsertChar(TextBuffer *buffer, char ch);
void RemoveChar(TextBuffer *buffer, int key);
void TextBufferController(void);
void FreeTextBuffer(void);
void FreeLineBuffer(void);
void FreeBufferMemory(void);
void BlinkCursor(void);
void CalculateCursorPosition(int key);
void CalculateSelection(int key);
void UpdateView(void);
void LoadFile(void);
void SaveFile(void);
void ProcessKey(int key, bool ctrl, bool shift);
void ProcessKeyDown(int key, bool ctrl, bool shift);
void RenderTextBuffer(void);
void UpdateSidebarWidth(void);
void DrawSidebar(int firstVisibleLine, int lastVisibleLine, float lineHeight, float scrollPosY);
void DrawTextLines(int firstVisibleLine, int lastVisibleLine, float lineHeight, Vector2 scroll);
void DrawCursor(float lineHeight);
void RecordCursorActivity(void);
void UpdateCursorState(void);
void DrawSelectionIndicator(void);
void ClearSelectionIndicator(void);
void NavigateToLineNumber(int lineNumber);
void DrawLineNumberNavInput(void);
void DrawSaveFileInput(void);
void DrawSearchInput(void);
void UpdateView(void);
void StoreUndo(void);
void SaveSnapshot(BufferSnapshot *snap, int *top);
void ApplySnapshot(BufferSnapshot *snap);
void Undo(void);
void Redo(void);
void FreeSnapshot(BufferSnapshot *snap);
void FreeHistoryStacks(void);
int CalculateCursorPosX(int previousY);
bool KeyController(void);

#endif