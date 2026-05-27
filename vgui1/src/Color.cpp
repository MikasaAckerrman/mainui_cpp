#include <VGUI_Color.h>
#include <string.h>

namespace vgui
{

Color::Color()
{
	init();
}

Color::Color(int r, int g, int b, int a)
{
	init();
	setColor(r, g, b, a);
}

Color::Color(Scheme::SchemeColor sc)
{
	init();
	setColor(sc);
}

void Color::init()
{
	_color[0] = 0;
	_color[1] = 0;
	_color[2] = 0;
	_color[3] = 0;
	_schemeColor = Scheme::sc_user;
}

void Color::setColor(int r, int g, int b, int a)
{
	_color[0] = (uchar)r;
	_color[1] = (uchar)g;
	_color[2] = (uchar)b;
	_color[3] = (uchar)a;
}

void Color::setColor(Scheme::SchemeColor sc)
{
	_schemeColor = sc;
}

void Color::getColor(int& r, int& g, int& b, int& a)
{
	r = _color[0];
	g = _color[1];
	b = _color[2];
	a = _color[3];
}

void Color::getColor(Scheme::SchemeColor& sc)
{
	sc = _schemeColor;
}

int Color::operator[](int index)
{
	if (index < 0 || index > 3)
		return 0;
	return _color[index];
}

}
