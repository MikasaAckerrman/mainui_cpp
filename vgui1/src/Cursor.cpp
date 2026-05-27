#include <VGUI_Cursor.h>
#include <VGUI_Bitmap.h>

namespace vgui
{

Cursor::Cursor(DefaultCursor dc)
{
	_dc = dc;
	_bitmap = null;
	_hotspot[0] = 0;
	_hotspot[1] = 0;
}

Cursor::Cursor(Bitmap* bitmap, int hotspotX, int hotspotY)
{
	_dc = dc_user;
	privateInit(bitmap, hotspotX, hotspotY);
}

void Cursor::getHotspot(int& x, int& y)
{
	x = _hotspot[0];
	y = _hotspot[1];
}

void Cursor::privateInit(Bitmap* bitmap, int hotspotX, int hotspotY)
{
	_bitmap = bitmap;
	_hotspot[0] = hotspotX;
	_hotspot[1] = hotspotY;
}

Bitmap* Cursor::getBitmap()
{
	return _bitmap;
}

Cursor::DefaultCursor Cursor::getDefaultCursor()
{
	return _dc;
}

}
