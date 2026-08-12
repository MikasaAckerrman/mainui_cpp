/*
TrackerScheme.h - Source Engine-style scheme loader for mainui_cpp
Copyright (C) 2024 DragonSlayer Team

Loads resource/TrackerScheme.res and maps its Colors/BaseSettings
to mainui_cpp global color variables and per-control style settings.
This gives CS 1.6 PC-style visual appearance driven by a single .res file.
*/

#ifndef TRACKERCHEME_H
#define TRACKERCHEME_H

#include "enginecallback_menu.h"

// Extended color set driven by TrackerScheme.res
struct SchemeColors
{
	// Frame / Window
	unsigned int frameBgColor;
	unsigned int frameTitleBarBg;
	unsigned int frameTitleBarFg;
	unsigned int frameBorderColor;
	unsigned int frameTitleBarTop;
	unsigned int frameTitleBarBottom;

	// Buttons
	unsigned int buttonTextColor;
	unsigned int buttonBgColor;
	unsigned int buttonArmedTextColor;
	unsigned int buttonArmedBgColor;
	unsigned int buttonDepressedTextColor;

	// Labels
	unsigned int labelTextColor;
	unsigned int labelBrightColor;
	unsigned int labelDimColor;
	unsigned int labelDisabledFg1;
	unsigned int labelDisabledFg2;

	// List / Table
	unsigned int listTextColor;
	unsigned int listBgColor;
	unsigned int listSelectedTextColor;
	unsigned int listSelectedBgColor;
	unsigned int listHeaderTextColor;

	// Text entry / Field
	unsigned int fieldTextColor;
	unsigned int fieldBgColor;
	unsigned int fieldSelectedTextColor;
	unsigned int fieldSelectedBgColor;

	// Tabs / PropertySheet
	unsigned int tabTextColor;
	unsigned int tabSelectedTextColor;
	unsigned int tabActiveBgColor;
	unsigned int tabInactiveBgColor;

	// Menu
	unsigned int menuTextColor;
	unsigned int menuBgColor;
	unsigned int menuArmedTextColor;
	unsigned int menuArmedBgColor;

	// Borders (as colors for bevel drawing)
	unsigned int borderBright;
	unsigned int borderDark;
	unsigned int borderSelection;
	// Inner (second) bevel highlight/shadow, drawn 1px inside the outer bevel.
	unsigned int borderInnerBright;
	unsigned int borderInnerDark;

	// Frame body gradient bands (subtle highlight at top of client, shadow at
	// bottom). White/black with low alpha - overlaid on the frame background.
	unsigned int frameHighlightBand;
	unsigned int frameShadowBand;

	// Generic
	unsigned int bgColor;
	unsigned int fgColor;
	unsigned int windowBgColor;
	unsigned int windowFgColor;

	// Check mark glyph (canon: BrightControlText = gold 196,181,80)
	unsigned int checkMarkColor;
	// Slider track behind handle (canon: ControlDarkBG 90,106,80)
	unsigned int sliderBgColor;

	// Default menu font, parsed from Fonts/DefaultFont/1 in TrackerScheme.res.
	// Empty name = use the engine's hardcoded default (Tahoma). When set, it
	// flows through CFontManager::VidInit so the user can re-skin the engine
	// font without rebuilding - just edit the .res and drop a TTF that the
	// FontManager can resolve.
	char menuFontName[64];
	int  menuFontTall;
	int  menuFontWeight;
};

extern SchemeColors g_Scheme;

// Load and apply TrackerScheme.res (called after UI_ApplyCustomColors in UI_Init)
void UI_LoadTrackerScheme( void );

// ---------------------------------------------------------------------------
// Generic role access.
//
// The named fields above are a convenience VIEW over a much larger table: the
// real resource/TrackerScheme.res shipped with CS 1.6 defines ~147 BaseSettings
// roles, and every one of them is now stored verbatim at parse time. Widgets
// that need a role which has no dedicated field (ScrollBar*, CheckButton.*,
// SectionedListPanel.*, Tooltip.*, ComboBoxButton.*, ...) ask for it by name
// instead of growing the struct and the parser in lockstep.
//
// Why by name and not more fields: adding a role used to mean editing the
// struct, the built-in scheme text AND a 54-branch stricmp chain. Three places
// to keep in sync is how 112 of 147 canonical roles ended up silently dropped.
//
// Scheme_Role returns fallback when the role is absent from the scheme.
// A role explicitly set to "Blank" (0 0 0 0) is PRESENT and returns 0, which is
// a meaningful value in VGUI (draw nothing) - hence the separate Has query.
unsigned int Scheme_Role( const char *role, unsigned int fallback );
bool         Scheme_HasRole( const char *role );

// Numeric roles: Frame.TransitionEffectTime (0.25), Menu.TextInset (6),
// MainMenu.MenuItemHeight (30), Frame.OutOfFocusAlpha (128), Main.Title1.X ...
// Times are fractional in the Source scheme, so this is float, not int.
float Scheme_Metric( const char *role, float fallback );
bool  Scheme_HasMetric( const char *role );

// How many roles the active scheme defined. Used by the test harness to catch a
// parser that silently stops early, and by the diagnostic console output.
int Scheme_RoleCount( void );

// Path the active scheme was read from, e.g. "resource/schemes/TrackerScheme_Source.res"
// or "(built-in)". Useful in diagnostics: "my colours did not change" is almost
// always "a different file than you edited was loaded".
const char *Scheme_ActivePath( void );

// Registers the ui_scheme cvar plus ui_scheme_list / ui_scheme_reload.
// Must run before UI_LoadTrackerScheme, otherwise ui_scheme reads as empty and
// a selected variant is ignored on the first load.
void UI_RegisterSchemeCommands( void );

// Query helpers
inline unsigned int Scheme_GetColor( unsigned int schemeColor, unsigned int fallback )
{
	return schemeColor ? schemeColor : fallback;
}

// GoldSrc-style inset border (sunken look: dark top+left, bright bottom+right)
inline void DrawGoldSrcInset( int x, int y, int w, int h )
{
	unsigned int dark = Scheme_GetColor( g_Scheme.borderDark, 0xFF282E22 );
	unsigned int bright = Scheme_GetColor( g_Scheme.borderBright, 0xFF889180 );
	UI_FillRect( x, y, w, 1, dark );            // top
	UI_FillRect( x, y, 1, h, dark );            // left
	UI_FillRect( x, y + h - 1, w, 1, bright );  // bottom
	UI_FillRect( x + w - 1, y, 1, h, bright );  // right
}

// GoldSrc-style raised border (raised look: bright top+left, dark bottom+right)
inline void DrawGoldSrcRaised( int x, int y, int w, int h )
{
	unsigned int dark = Scheme_GetColor( g_Scheme.borderDark, 0xFF282E22 );
	unsigned int bright = Scheme_GetColor( g_Scheme.borderBright, 0xFF889180 );
	UI_FillRect( x, y, w, 1, bright );          // top
	UI_FillRect( x, y, 1, h, bright );          // left
	UI_FillRect( x, y + h - 1, w, 1, dark );    // bottom
	UI_FillRect( x + w - 1, y, 1, h, dark );    // right
}

#endif // TRACKERCHEME_H
