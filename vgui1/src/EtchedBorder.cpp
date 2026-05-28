#include <VGUI_EtchedBorder.h>
#include <VGUI_Panel.h>

namespace vgui
{

EtchedBorder::EtchedBorder() : Border(2, 2, 2, 2)
{
}

void EtchedBorder::paint(Panel* panel)
{
	int wide, tall;
	panel->getSize(wide, tall);

	// GoldSrc etched border: dark line then light line offset by 1px
	// Creates an inset/groove appearance
	// Outer dark shadow
	drawSetColor(128, 128, 128, 0);
	drawFilledRect(0, 0, wide - 1, 1);           // top
	drawFilledRect(0, 0, 1, tall - 1);            // left
	drawFilledRect(1, tall - 2, wide, tall - 1);  // inner bottom
	drawFilledRect(wide - 2, 1, wide - 1, tall);  // inner right

	// Inner highlight
	drawSetColor(255, 255, 255, 0);
	drawFilledRect(1, 1, wide - 2, 2);            // top highlight
	drawFilledRect(1, 1, 2, tall - 2);            // left highlight
	drawFilledRect(0, tall - 1, wide, tall);      // bottom
	drawFilledRect(wide - 1, 0, wide, tall);      // right
}

}
