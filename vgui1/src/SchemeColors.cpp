// SchemeColors.cpp - bridge between TrackerScheme g_Scheme palette and VGUI1
// inverted-alpha drawing API.
//
// Include heavy mainui headers BEFORE VGUI_*.h to avoid the `null` macro
// clash (VGUI defines `null` as 0, mainui has parameters named `null`).

// Forward-declare UI_FillRect so TrackerScheme.h's inline helpers can be
// parsed without pulling in Utils.h (which carries heavy mainui state).
extern void UI_FillRect( int x, int y, int width, int height, const unsigned int color );

#include "TrackerScheme.h"

#include <VGUI_SchemeColors.h>
#include <VGUI_Panel.h>
#include <VGUI_SurfaceBase.h>

namespace vgui
{

void schemeBgColor( Panel *p, unsigned int argb )
{
	if ( !p )
		return;
	SurfaceBase *sb = p->getSurfaceBase();
	if ( !sb )
		return;
	int a = ( argb >> 24 ) & 0xFF;
	int r = ( argb >> 16 ) & 0xFF;
	int g = ( argb >> 8 ) & 0xFF;
	int b = argb & 0xFF;
	sb->drawSetColor( r, g, b, 255 - a ); // VGUI inverted alpha
}

void schemeFgColor( Panel *p, unsigned int argb )
{
	if ( !p )
		return;
	SurfaceBase *sb = p->getSurfaceBase();
	if ( !sb )
		return;
	int a = ( argb >> 24 ) & 0xFF;
	int r = ( argb >> 16 ) & 0xFF;
	int g = ( argb >> 8 ) & 0xFF;
	int b = argb & 0xFF;
	sb->drawSetTextColor( r, g, b, 255 - a );
}

}
