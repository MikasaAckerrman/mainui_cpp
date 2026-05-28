#include <VGUI_LineBorder.h>
#include <VGUI_Panel.h>

namespace vgui
{

LineBorder::LineBorder() : Border(1, 1, 1, 1)
{
	_color = Color(0, 0, 0, 0);
	_thickness = 1;
}

LineBorder::LineBorder(Color color) : Border(1, 1, 1, 1)
{
	_color = color;
	_thickness = 1;
}

LineBorder::LineBorder(int thickness) : Border(thickness, thickness, thickness, thickness)
{
	_color = Color(0, 0, 0, 0);
	_thickness = thickness;
}

LineBorder::LineBorder(Color color, int thickness) : Border(thickness, thickness, thickness, thickness)
{
	_color = color;
	_thickness = thickness;
}

void LineBorder::paint(Panel* panel)
{
	int wide, tall;
	panel->getSize(wide, tall);

	int r, g, b, a;
	_color.getColor(r, g, b, a);
	drawSetColor(r, g, b, a);

	for (int i = 0; i < _thickness; i++)
	{
		drawFilledRect(i, i, wide - i, i + 1);           // top
		drawFilledRect(i, tall - i - 1, wide - i, tall - i); // bottom
		drawFilledRect(i, i + 1, i + 1, tall - i - 1);   // left
		drawFilledRect(wide - i - 1, i + 1, wide - i, tall - i - 1); // right
	}
}

}
