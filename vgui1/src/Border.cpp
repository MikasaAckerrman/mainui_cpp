#include <VGUI_Border.h>
#include <VGUI_Panel.h>
#include <VGUI_SurfaceBase.h>
#include <string.h>

namespace vgui
{

Border::Border() : Image()
{
	memset(_inset, 0, sizeof(_inset));
	_panel = null;
}

Border::Border(int left, int top, int right, int bottom) : Image()
{
	_inset[0] = left;
	_inset[1] = top;
	_inset[2] = right;
	_inset[3] = bottom;
	_panel = null;
}

void Border::setInset(int left, int top, int right, int bottom)
{
	_inset[0] = left;
	_inset[1] = top;
	_inset[2] = right;
	_inset[3] = bottom;
}

void Border::getInset(int& left, int& top, int& right, int& bottom)
{
	left = _inset[0];
	top = _inset[1];
	right = _inset[2];
	bottom = _inset[3];
}

void Border::drawFilledRect(int x0, int y0, int x1, int y1)
{
	// Border is friend of Panel, delegate through Image's draw mechanism
	Image::drawFilledRect(x0, y0, x1, y1);
}

void Border::drawOutlinedRect(int x0, int y0, int x1, int y1)
{
	Image::drawOutlinedRect(x0, y0, x1, y1);
}

void Border::drawSetTextPos(int x, int y)
{
	Image::drawSetTextPos(x, y);
}

void Border::drawPrintText(int x, int y, const char* str, int strlen)
{
	Image::drawSetTextPos(x, y);
	Image::drawPrintText(str, strlen);
}

void Border::drawPrintChar(int x, int y, char ch)
{
	Image::drawSetTextPos(x, y);
	Image::drawPrintChar(ch);
}

}
