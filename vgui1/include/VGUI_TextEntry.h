#ifndef VGUI_TEXTENTRY_H
#define VGUI_TEXTENTRY_H

#include <VGUI.h>
#include <VGUI_Panel.h>
#include <VGUI_Dar.h>

namespace vgui
{

class ActionSignal;

class TextEntry : public Panel
{
public:
	TextEntry(const char* text, int x, int y, int wide, int tall);
public:
	virtual void setText(const char* text, int textLen);
	virtual void getText(int offset, char* buf, int bufLen);
	virtual int getTextLength();
	virtual void setEditable(bool state);
	virtual void setFont(Font* font);
	virtual void setFont(Scheme::SchemeFont schemeFont);
	virtual void addActionSignal(ActionSignal* s);
	virtual void fireActionSignal();
	virtual void selectNone();
	virtual void selectAll();
protected:
	virtual void paint();
	virtual void paintBackground();
	virtual void internalMousePressed(MouseCode code);
	virtual void internalCursorMoved(int x, int y);
	virtual void internalMouseReleased(MouseCode code);
	virtual void internalKeyPressed(KeyCode code);
	virtual void internalKeyTyped(KeyCode code);
private:
	char _text[256];
	int _textLen;
	int _cursorPos;
	int _selectStart;
	int _selectEnd;
	int _scrollOffset;
	bool _editable;
	Font* _font;
	Scheme::SchemeFont _schemeFont;
	Dar<ActionSignal*> _actionSignalDar;
};

}

#endif
