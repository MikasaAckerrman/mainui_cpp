#ifndef VGUI_CONSOLE_H
#define VGUI_CONSOLE_H

#include <VGUI.h>
#include <VGUI_Frame.h>

namespace vgui
{

class VguiConsole;
class Panel;
class Label;
class Button;
class TextEntry;
class ConsoleOutputPanel;
class ConsoleScrollBar;

// CS 1.6 PC-accurate console window. Resizable Frame with a scrollable text
// output area, a right-edge scrollbar, and a bottom input bar (text field +
// "RU" indicator + "Подтвердить" button). All coordinates use VS() scaling
// from the 640x480 reference grid.
class VguiConsole : public Frame
{
public:
	VguiConsole(int screenW, int screenH);
public:
	virtual void setSize(int wide, int tall);   // override - relayout children
	void submitCommand();                        // run the current input line
private:
	void layoutChildren();
private:
	ConsoleOutputPanel* _output;
	ConsoleScrollBar*   _scrollbar;
	TextEntry*          _input;
	Label*              _ruLabel;
	Button*             _confirmBtn;
};

// Console buffer / lifecycle helpers (implemented in VguiConsole.cpp).
// These are called by the exported C functions in vgui_main.cpp.
void VGUI_Console_Output(const char* text); // append text to the circular buffer
void VGUI_Console_Show(bool show);          // create (lazy) + show/hide the window
bool VGUI_Console_IsVisible();              // query visibility

}

#ifdef __cplusplus
extern "C" {
#endif

// Exposed to engine / mainui:
void VGUI_ConsoleOutput(const char* text);  // append text to console output
void VGUI_ShowConsole(bool show);           // show/hide console window
bool VGUI_IsConsoleVisible(void);           // query visibility

#ifdef __cplusplus
}
#endif

#endif // VGUI_CONSOLE_H
