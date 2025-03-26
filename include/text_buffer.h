#ifndef TEXT_BUFFER_H_
#define TEXT_BUFFER_H_

typedef struct {
    char *text;
    int length;
    int capacity;
    Vector2 cursorPos;
    bool cursorVisible;
    int lineCount;
} TextBuffer;

void InitializeTextBuffer(void);
void InsertChar(TextBuffer *buffer, char ch);
void RemoveChar(TextBuffer *buffer);
void KeyController(void);
void TextBufferController(void);
void FreeTextBuffer(void);

#endif