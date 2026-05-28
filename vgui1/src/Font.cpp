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
	_tall = tall;
	_wide = wide;
}

BaseFontPlat* Font::getPlat()
{
	return _plat;
}

void Font::getCharRGBA(int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, uchar* rgba)
{
	// Stub - CEngineSurface handles font rendering via engine
}

void Font::getCharABCwide(int ch, int& a, int& b, int& c)
{
	// Approximate character metrics based on font height
	// GoldSrc uses roughly 60% of tall as average char width
	int charW = _wide > 0 ? _wide : (_tall * 6) / 10;
	if (charW <= 0) charW = 8;

	a = 0;       // no leading overhang
	b = charW;   // character body width
	c = 0;       // no trailing overhang
}

void Font::getTextSize(const char* text, int& wide, int& tall)
{
	tall = _tall > 0 ? _tall : 14;
	wide = 0;

	if (!text)
		return;

	int len = (int)strlen(text);
	for (int i = 0; i < len; i++)
	{
		int a, b, c;
		getCharABCwide((unsigned char)text[i], a, b, c);
		wide += a + b + c;
	}
}

int Font::getTall()
{
	return _tall > 0 ? _tall : 14;
}

#ifndef _WIN32
int Font::getWide()
{
	return _wide > 0 ? _wide : (_tall * 6) / 10;
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
