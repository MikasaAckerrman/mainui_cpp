#ifndef VGUI_LABEL_H
#define VGUI_LABEL_H

#include <VGUI.h>
#include <VGUI_Panel.h>
#include <VGUI_Scheme.h>

namespace vgui
{

class Image;

class Label : public Panel
{
public:
	enum Alignment
	{
		a_northwest = 0,
		a_north,
		a_northeast,
		a_west,
		a_center,
		a_east,
		a_southwest,
		a_south,
		a_southeast
	};
public:
	Label(const char* text);
	Label(const char* text, int x, int y, int wide, int tall);
public:
	virtual void setText(const char* format, ...);
	virtual void getText(char* buf, int bufLen);
	virtual void setFont(Scheme::SchemeFont schemeFont);
	virtual void setFont(Font* font);
	virtual void getTextSize(int& wide, int& tall);
	virtual void getContentSize(int& wide, int& tall);
	virtual void setTextAlignment(Alignment alignment);
	virtual Alignment getTextAlignment();
	virtual void setContentAlignment(Alignment alignment);
	virtual void setImage(Image* image);
	virtual void setContentFitted(bool state);
protected:
	virtual void paintBackground();
	virtual void paint();
	virtual void computeAlignment(int& tx, int& ty, int twide, int ttall, int pwide, int ptall);
protected:
	// protected (not private): Button reimplements paint() with a +1,+1
	// depressed text shift and needs the text/font directly.
	char _text[256];
	Alignment _textAlignment;
	Alignment _contentAlignment;
	Font* _font;
	Scheme::SchemeFont _schemeFont;
	Image* _image;
	bool _contentFitted;
};

}

#endif
