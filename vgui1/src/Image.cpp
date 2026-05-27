#include <VGUI_Image.h>
#include <VGUI_Panel.h>
#include <VGUI_SurfaceBase.h>
#include <VGUI_Font.h>

namespace vgui
{

Image::Image()
{
	_pos[0] = 0;
	_pos[1] = 0;
	_size[0] = 0;
	_size[1] = 0;
	_panel = null;
}

void Image::setPos(int x, int y)
{
	_pos[0] = x;
	_pos[1] = y;
}

void Image::getPos(int& x, int& y)
{
	x = _pos[0];
	y = _pos[1];
}

void Image::getSize(int& wide, int& tall)
{
	wide = _size[0];
	tall = _size[1];
}

void Image::setColor(Color color)
{
	_color = color;
}

void Image::getColor(Color& color)
{
	color = _color;
}

void Image::setSize(int wide, int tall)
{
	_size[0] = wide;
	_size[1] = tall;
}

void Image::drawSetColor(Scheme::SchemeColor sc)
{
	if (_panel)
		_panel->drawSetColor(sc);
}

void Image::drawSetColor(int r, int g, int b, int a)
{
	if (_panel)
		_panel->drawSetColor(r, g, b, a);
}

void Image::drawFilledRect(int x0, int y0, int x1, int y1)
{
	if (_panel)
		_panel->drawFilledRect(x0, y0, x1, y1);
}

void Image::drawOutlinedRect(int x0, int y0, int x1, int y1)
{
	if (_panel)
		_panel->drawOutlinedRect(x0, y0, x1, y1);
}

void Image::drawSetTextFont(Scheme::SchemeFont sf)
{
	if (_panel)
		_panel->drawSetTextFont(sf);
}

void Image::drawSetTextFont(Font* font)
{
	if (_panel)
		_panel->drawSetTextFont(font);
}

void Image::drawSetTextColor(Scheme::SchemeColor sc)
{
	if (_panel)
		_panel->drawSetTextColor(sc);
}

void Image::drawSetTextColor(int r, int g, int b, int a)
{
	if (_panel)
		_panel->drawSetTextColor(r, g, b, a);
}

void Image::drawSetTextPos(int x, int y)
{
	if (_panel)
		_panel->drawSetTextPos(x, y);
}

void Image::drawPrintText(const char* str, int strlen)
{
	if (_panel)
		_panel->drawPrintText(str, strlen);
}

void Image::drawPrintText(int x, int y, const char* str, int strlen)
{
	if (_panel)
		_panel->drawPrintText(x, y, str, strlen);
}

void Image::drawPrintChar(char ch)
{
	if (_panel)
		_panel->drawPrintChar(ch);
}

void Image::drawPrintChar(int x, int y, char ch)
{
	if (_panel)
		_panel->drawPrintChar(x, y, ch);
}

void Image::drawSetTextureRGBA(int id, const char* rgba, int wide, int tall)
{
	if (_panel)
		_panel->drawSetTextureRGBA(id, rgba, wide, tall);
}

void Image::drawSetTexture(int id)
{
	if (_panel)
		_panel->drawSetTexture(id);
}

void Image::drawTexturedRect(int x0, int y0, int x1, int y1)
{
	if (_panel)
		_panel->drawTexturedRect(x0, y0, x1, y1);
}

void Image::paint(Panel* panel)
{
	// Base implementation - empty
}

void Image::doPaint(Panel* panel)
{
	_panel = panel;
	paint(panel);
}

}
