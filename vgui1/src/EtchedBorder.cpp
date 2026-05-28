// Include heavy mainui headers BEFORE VGUI_*.h to avoid the `null` macro clash.
extern void UI_FillRect( int x, int y, int width, int height, const unsigned int color );
#include "TrackerScheme.h"

#include <VGUI_SchemeColors.h>
#include <VGUI_EtchedBorder.h>
#include <VGUI_Panel.h>

namespace vgui
{

EtchedBorder::EtchedBorder() : Border(2, 2, 2, 2)
{
}

void EtchedBorder::paint(Panel* panel)
{
	if (!panel)
		return;

	int wide, tall;
	panel->getSize(wide, tall);

	unsigned int bright = g_Scheme.borderBright ? g_Scheme.borderBright : 0xC87A8070;
	unsigned int dark   = g_Scheme.borderDark   ? g_Scheme.borderDark   : 0xC8282C24;

	// GoldSrc etched groove: outer dark + inner bright = recessed channel.
	// Slightly different from previous: more visible groove depth.
	schemeBgColor(panel, dark);
	drawFilledRect(0, 0, wide - 1, 1);            // outer top
	drawFilledRect(0, 0, 1, tall - 1);            // outer left
	drawFilledRect(1, tall - 2, wide, tall - 1);  // inner bottom shadow
	drawFilledRect(wide - 2, 1, wide - 1, tall);  // inner right shadow

	schemeBgColor(panel, bright);
	drawFilledRect(1, 1, wide - 2, 2);            // inner top highlight
	drawFilledRect(1, 1, 2, tall - 2);            // inner left highlight
	drawFilledRect(0, tall - 1, wide, tall);      // outer bottom
	drawFilledRect(wide - 1, 0, wide, tall);      // outer right
}

}
