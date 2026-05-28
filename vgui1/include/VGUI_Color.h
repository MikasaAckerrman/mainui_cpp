#ifndef VGUI_COLOR_H
#define VGUI_COLOR_H

#include <VGUI.h>
#include <VGUI_Scheme.h>

namespace vgui
{

class Color
{
public:
	Color();
	Color(int r, int g, int b, int a);
	Color(Scheme::SchemeColor sc);
public:
	void init();
	void setColor(int r, int g, int b, int a);
	void setColor(Scheme::SchemeColor sc);
	void getColor(int& r, int& g, int& b, int& a);
	void getColor(Scheme::SchemeColor& sc);
	int operator[](int index);
private:
	int _color[4];
	Scheme::SchemeColor _schemeColor;
};

}

#endif
