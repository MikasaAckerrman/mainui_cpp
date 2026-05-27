#include <VGUI_Font.h>
#include <VGUI.h>
#include <string.h>

namespace vgui
{

static int _staticFontId = 0;

Font::Font(const char* name, int tall, int wide, float rotation, int weight, bool italic, bool underline, bool strikeout, bool symbol)
{
	init(name, null, 0, tall, wide, rotation, weight, italic, underline, strikeout, symbol);
}

Font::Font(const char* name, void* pFileData, int fileDataLen, int tall, int wide, float rotation, int weight, bool italic, bool underline, bool strikeout, bool symbol)
{
	init(name, pFileData, fileDataLen, tall, wide, rotation, weight, italic, underline, strikeout, symbol);
}

void Font::init(const char* name, void* pFileData, int fileDataLen, int tall, int wide, float rotation, int weight, bool italic, bool underline, bool strikeout, bool symbol)
{
	_name = vgui_strdup(name);
	_plat = null;
	_id = _staticFontId++;
}

BaseFontPlat* Font::getPlat()
{
	return _plat;
}

void Font::getCharRGBA(int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, uchar* rgba)
{
	// Stub - CEngineSurface handles font rendering
}

void Font::getCharABCwide(int ch, int& a, int& b, int& c)
{
	a = 0;
	b = 0;
	c = 0;
}

void Font::getTextSize(const char* text, int& wide, int& tall)
{
	wide = 0;
	tall = 0;
}

int Font::getTall()
{
	return 0;
}

#ifndef _WIN32
int Font::getWide()
{
	return 0;
}
#endif

int Font::getId()
{
	return _id;
}

void Font_Reset()
{
	_staticFontId = 0;
}

}
