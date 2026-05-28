#ifndef VGUI_SCHEMECOLORS_H
#define VGUI_SCHEMECOLORS_H

// VGUI1 wrapper for TrackerScheme.res colors.
//
// This header is intentionally lightweight: it forward-declares vgui::Panel
// instead of including VGUI_Panel.h. Implementations live in SchemeColors.cpp.
//
// Why: VGUI_*.h headers define `null` as a macro, which conflicts with the
// `null` parameter name used in mainui's EventSystem.h pulled in by
// TrackerScheme.h. Keeping the implementation in a .cpp lets callers include
// this header in any order.
//
// VGUI1 uses INVERTED alpha: 0 = opaque, 255 = transparent. These helpers
// unpack a standard ARGB packed color from g_Scheme and feed it to
// drawSetColor / drawSetTextColor with the alpha inverted, so the bridge
// in CEngineSurface produces the correct engine-side alpha.

#include "TrackerScheme.h"

namespace vgui
{
class Panel;

// Apply g_Scheme color as the current fill color (used by drawFilledRect).
void schemeBgColor( Panel *p, unsigned int argb );

// Apply g_Scheme color as the current text color (used by drawPrintText).
void schemeFgColor( Panel *p, unsigned int argb );
}

#endif // VGUI_SCHEMECOLORS_H
