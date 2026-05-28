#ifndef VGUI_UISCALE_H
#define VGUI_UISCALE_H

// Global scale factor for VGUI1 widgets, derived from physical screen height
// versus mainui's logical 768-unit reference. Set in VGUI_EnsureInitialized.
// Range typically [1.0 .. 3.0]. Use it to scale dialog dimensions, font
// heights, padding, and any pixel constants so the dialog stays readable
// on HD/4K Android devices.

namespace vgui
{
extern float g_vguiScale;

// Convenience integer scaler (rounded, never less than 1).
inline int VS( int v )
{
	int r = (int)((float)v * g_vguiScale + 0.5f);
	return r < 1 ? 1 : r;
}
}

#endif // VGUI_UISCALE_H
