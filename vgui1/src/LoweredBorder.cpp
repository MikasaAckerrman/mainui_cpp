#include <VGUI_LoweredBorder.h>
#include <VGUI_Panel.h>

namespace vgui
{

LoweredBorder::LoweredBorder() : Border(2, 2, 2, 2)
{
}

void LoweredBorder::paint(Panel* panel)
{
	int wide, tall;
	panel->getSize(wide, tall);

	// GoldSrc lowered/sunken border (text field, inset look):
	// Top-left = dark shadow, bottom-right = white highlight
	// Outer shadow (top-left)
	drawSetColor(128, 128, 128, 0);
	drawFilledRect(0, 0, wide, 1);     // top
	drawFilledRect(0, 0, 1, tall);     // left

	// Inner shadow
	drawSetColor(64, 64, 64, 0);
	drawFilledRect(1, 1, wide - 1, 2); // top inner
	drawFilledRect(1, 1, 2, tall - 1); // left inner

	// Inner highlight (bottom-right)
	drawSetColor(216, 216, 216, 0);
	drawFilledRect(1, tall - 2, wide - 1, tall - 1); // bottom inner
	drawFilledRect(wide - 2, 1, wide - 1, tall - 1); // right inner

	// Outer highlight
	drawSetColor(255, 255, 255, 0);
	drawFilledRect(0, tall - 1, wide, tall);   // bottom
	drawFilledRect(wide - 1, 0, wide, tall);   // right
}

}
