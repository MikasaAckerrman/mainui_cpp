// Include heavy mainui headers BEFORE VGUI_*.h to avoid the `null` macro clash.
extern void UI_FillRect( int x, int y, int width, int height, const unsigned int color );
#include "TrackerScheme.h"

#include <VGUI_SchemeColors.h>
#include <VGUI_LoweredBorder.h>
#include <VGUI_Panel.h>

namespace vgui
{

LoweredBorder::LoweredBorder() : Border(1, 1, 1, 1)
{
}

void LoweredBorder::paint(Panel* panel)
{
	if (!panel)
		return;

	int wide, tall;
	panel->getSize(wide, tall);

	unsigned int bright = g_Scheme.borderBright ? g_Scheme.borderBright : 0xFF889180;
	unsigned int dark   = g_Scheme.borderDark   ? g_Scheme.borderDark   : 0xFF282E22;

	// Sunken: dark on top + left, bright on bottom + right
	schemeBgColor(panel, dark);
	drawFilledRect(0, 0, wide, 1);
	drawFilledRect(0, 0, 1, tall);

	schemeBgColor(panel, bright);
	drawFilledRect(0, tall - 1, wide, tall);
	drawFilledRect(wide - 1, 0, wide, tall);
}

}
