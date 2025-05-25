#ifndef SCREEN_H_
#define SCREEN_H_

void UpdateFrame(void);
void ScreenController(void);
void RenderFrameRate(void);
void DrawCustomFPS(void);
void DrawBottomBar(void);
void RenderHelpText(char helpText[256]);

#endif