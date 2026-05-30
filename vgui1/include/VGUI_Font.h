#ifndef VGUI_FONT_H
#define VGUI_FONT_H

#include <VGUI.h>

namespace vgui
{

class BaseFontPlat;

class Font
{
public:
	Font(const char* name, int tall, int wide, float rotation, int weight, bool italic, bool underline, bool strikeout, bool symbol);
	Font(const char* name, void* pFileData, int fileDataLen, int tall, int wide, float rotation, int weight, bool italic, bool underline, bool strikeout, bool symbol);
	~Font();
public:
	void getCharABCwide(int ch, int& a, int& b, int& c);
	void getTextSize(const char* text, int& wide, int& tall);
	void getCharRGBA(int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, uchar* rgba);
	int getTall();
	int getWide();
	int getId();
	BaseFontPlat* getPlat();
private:
	void init(const char* name, void* pFileData, int fileDataLen, int tall, int wide, float rotation, int weight, bool italic, bool underline, bool strikeout, bool symbol);
private:
	char* _name;
	BaseFontPlat* _plat;
	int _id;
	int _tall;
	int _wide;
};

class BaseFontPlat
{
public:
	virtual ~BaseFontPlat() {}
};

void Font_Reset();

}

#endif
