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
} LineInfo;

void SetupTextBuffer(void);
void InitializeTextBuffer(void);
void InitializeLineInfos(void);
void InsertChar(TextBuffer *buffer, char ch);
void RemoveChar(TextBuffer *buffer);
void KeyController(void);
void TextBufferController(void);
void FreeTextBuffer(void);
void FreeLineInfos(void);
void FreeBufferMemory(void);
int CalculateCursorPosX(int previousY);
void BlinkCursor(void);
void CalculateCursorPosition(int key);
void CalculateSelection(int key);
void ProcessLines(void);
void UpdateView(void);
void LoadFile(void);
void SaveFile(void);

#endif