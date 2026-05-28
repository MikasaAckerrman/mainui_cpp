#ifndef VGUI_BITMAP_H
#define VGUI_BITMAP_H

#include <VGUI.h>
#include <VGUI_Image.h>

namespace vgui
{

class Panel;

class Bitmap : public Image
{
public:
	Bitmap();
	Bitmap(int wide, int tall);
public:
	virtual void setRGBA(int x, int y, uchar r, uchar g, uchar b, uchar a);
	virtual void setSize(int wide, int tall);
	virtual void paint(Panel* panel);
private:
	int _id;
	bool _uploaded;
	uchar* _rgba;
};

}

#endif
