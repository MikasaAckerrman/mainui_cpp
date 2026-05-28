#ifndef VGUI_BORDER_H
#define VGUI_BORDER_H

#include <VGUI.h>
#include <VGUI_Image.h>

namespace vgui
{

class Panel;

class Border : public Image
{
public:
	Border();
	Border(int left, int top, int right, int bottom);
public:
	virtual void setInset(int left, int top, int right, int bottom);
	virtual void getInset(int& left, int& top, int& right, int& bottom);
protected:
	virtual void drawFilledRect(int x0, int y0, int x1, int y1);
	virtual void drawOutlinedRect(int x0, int y0, int x1, int y1);
	virtual void drawSetTextPos(int x, int y);
	virtual void drawPrintText(int x, int y, const char* str, int strlen);
	virtual void drawPrintChar(int x, int y, char ch);
protected:
	int _inset[4];
	Panel* _panel;
};

}

#endif
