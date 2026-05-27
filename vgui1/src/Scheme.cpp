#include <VGUI_Scheme.h>
#include <VGUI_Font.h>
#include <VGUI_Cursor.h>
#include <string.h>

namespace vgui
{

Scheme::Scheme()
{
	memset(_color, 0, sizeof(_color));
	memset(_font, 0, sizeof(_font));
	memset(_cursor, 0, sizeof(_cursor));
}

void Scheme::setColor(SchemeColor sc, int r, int g, int b, int a)
{
	if (sc < 0 || sc >= sc_last)
		return;
	_color[sc][0] = r;
	_color[sc][1] = g;
	_color[sc][2] = b;
	_color[sc][3] = a;
}

void Scheme::getColor(SchemeColor sc, int& r, int& g, int& b, int& a)
{
	if (sc < 0 || sc >= sc_last)
	{
		r = g = b = a = 0;
		return;
	}
	r = _color[sc][0];
	g = _color[sc][1];
	b = _color[sc][2];
	a = _color[sc][3];
}

void Scheme::setFont(SchemeFont sf, Font* font)
{
	if (sf < 0 || sf >= sf_last)
		return;
	_font[sf] = font;
}

Font* Scheme::getFont(SchemeFont sf)
{
	if (sf < 0 || sf >= sf_last)
		return null;
	return _font[sf];
}

void Scheme::setCursor(SchemeCursor sc, Cursor* cursor)
{
	if (sc < 0 || sc >= scu_last)
		return;
	_cursor[sc] = cursor;
}

Cursor* Scheme::getCursor(SchemeCursor sc)
{
	if (sc < 0 || sc >= scu_last)
		return null;
	return _cursor[sc];
}

}
