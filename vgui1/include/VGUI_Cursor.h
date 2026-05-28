#ifndef VGUI_CURSOR_H
#define VGUI_CURSOR_H

#include <VGUI.h>

namespace vgui
{

class Bitmap;

class Cursor
{
public:
	enum DefaultCursor
	{
		dc_user = 0,
		dc_none,
		dc_arrow,
		dc_ibeam,
		dc_hourglass,
		dc_crosshair,
		dc_up,
		dc_sizenwse,
		dc_sizenesw,
		dc_sizewe,
		dc_sizens,
		dc_sizeall,
		dc_no,
		dc_hand,
		dc_last
	};
public:
	Cursor(DefaultCursor dc);
	Cursor(Bitmap* bitmap, int hotspotX, int hotspotY);
public:
	void getHotspot(int& x, int& y);
	Bitmap* getBitmap();
	DefaultCursor getDefaultCursor();
private:
	void privateInit(Bitmap* bitmap, int hotspotX, int hotspotY);
private:
	DefaultCursor _dc;
	Bitmap* _bitmap;
	int _hotspot[2];
};

}

#endif
