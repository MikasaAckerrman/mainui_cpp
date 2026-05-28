#include <VGUI_RaisedBorder.h>
#include <VGUI_Panel.h>

namespace vgui
{

RaisedBorder::RaisedBorder() : Border(2, 2, 2, 2)
{
}

void RaisedBorder::paint(Panel* panel)
{
	int wide, tall;
	panel->getSize(wide, tall);

	// GoldSrc raised border (button-up look):
	// Top-left = white highlight, bottom-right = dark shadow
	// Outer highlight (top-left)
	drawSetColor(255, 255, 255, 0);
	drawFilledRect(0, 0, wide, 1);     // top
	drawFilledRect(0, 0, 1, tall);     // left

	// Inner highlight
	drawSetColor(216, 216, 216, 0);
	drawFilledRect(1, 1, wide - 1, 2); // top inner
	drawFilledRect(1, 1, 2, tall - 1); // left inner

	// Inner shadow (bottom-right)
	drawSetColor(128, 128, 128, 0);
	drawFilledRect(1, tall - 2, wide - 1, tall - 1); // bottom inner
	drawFilledRect(wide - 2, 1, wide - 1, tall - 1); // right inner

	// Outer shadow
	drawSetColor(64, 64, 64, 0);
	drawFilledRect(0, tall - 1, wide, tall);   // bottom
	drawFilledRect(wide - 1, 0, wide, tall);   // right
}

}
