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

	unsigned int bright = g_Scheme.borderBright ? g_Scheme.borderBright : 0xFF889180;
	unsigned int dark   = g_Scheme.borderDark   ? g_Scheme.borderDark   : 0xFF282E22;

	// Canonical engraved groove = 1px inset outer + 1px raised inner:
	// outer:  TL = dark,   BR = bright (sunken)
	// inner:  TL = bright, BR = dark   (raised)
	// Combined this reads as a recessed channel - the canon CS 1.6 etched
	// look. Previous version had outer/inner swapped, producing a non-canon
	// raised double-bevel.
	schemeBgColor(panel, dark);
	drawFilledRect(0, 0, wide, 1);                    // outer top
	drawFilledRect(0, 0, 1, tall);                    // outer left
	schemeBgColor(panel, bright);
	drawFilledRect(0, tall - 1, wide, tall);          // outer bottom
	drawFilledRect(wide - 1, 0, wide, tall);          // outer right

	schemeBgColor(panel, bright);
	drawFilledRect(1, 1, wide - 1, 2);                // inner top  (raised)
	drawFilledRect(1, 1, 2, tall - 1);                // inner left
	schemeBgColor(panel, dark);
	drawFilledRect(1, tall - 2, wide - 1, tall - 1);  // inner bottom
	drawFilledRect(wide - 2, 1, wide - 1, tall - 1);  // inner right
}

}
