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
    bool isForwardSelection;
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

typedef struct {
    int start;
    int end;
    bool hasStringMatch;
} SearchIndex;

void SetupTextBuffer(void);
void InitializeTextBuffer(void);
void InitializeLineBuffer(void);
void InsertChar(TextBuffer *buffer, char ch);
void RemoveChar(TextBuffer *buffer, int key, bool storeUndoFlag);
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
void DrawIndicatorOverlay(int startIndex, int endIndex, Color color);
void DrawSearchIndicator(void);
void NavigateToLineNumber(int lineNumber);
void DrawLineNumberNavInput(void);
void DrawSaveFileInput(void);
void DrawSearchInput(void);
void DrawOpenFileInput(void);
void DrawFilePathHelpText(void);
void UpdateView(void);
void StoreUndo(void);
void SaveSnapshot(BufferSnapshot *snap, int *top);
void ApplySnapshot(BufferSnapshot *snap);
void Undo(void);
void Redo(void);
void FreeSnapshot(BufferSnapshot *snap);
void FreeHistoryStacks(void);
void SearchText(char prevSearchInput[256], char searchInput[256]);
void CalculateCursorPosBasedOnPosX(int cursorX);
void ResetKeyDownDelay(int key);
void HandleCtrlHoldOperations(int key);
void HandleSelectionDelete(void);
int CalculateCursorPosX(int previousY);
bool KeyController(void);
char *SearchStringIgnoreCase(const char *haystack, const char *needle);

#endif