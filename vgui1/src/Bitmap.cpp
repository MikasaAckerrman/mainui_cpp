#include <VGUI_Bitmap.h>
#include <VGUI_Panel.h>
#include <string.h>

namespace vgui
{

Bitmap::Bitmap()
{
	_id = 0;
	_uploaded = false;
	_rgba = null;
}

void Bitmap::setSize(int wide, int tall)
{
	Image::setSize(wide, tall);
	if (_rgba)
	{
		delete[] _rgba;
		_rgba = null;
	}
	if (wide > 0 && tall > 0)
	{
		_rgba = new uchar[wide * tall * 4];
		memset(_rgba, 0, wide * tall * 4);
	}
}

void Bitmap::setRGBA(int x, int y, uchar r, uchar g, uchar b, uchar a)
{
	int wide, tall;
	getSize(wide, tall);
	if (!_rgba || x < 0 || y < 0 || x >= wide || y >= tall)
		return;
	int offset = (y * wide + x) * 4;
	_rgba[offset + 0] = r;
	_rgba[offset + 1] = g;
	_rgba[offset + 2] = b;
	_rgba[offset + 3] = a;
}

void Bitmap::paint(Panel* panel)
{
	// Stub - texture upload and rendering handled by CEngineSurface
}

}
