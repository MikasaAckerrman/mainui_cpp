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

	// Generic
	unsigned int bgColor;
	unsigned int fgColor;
	unsigned int windowBgColor;
	unsigned int windowFgColor;
};

extern SchemeColors g_Scheme;

// Load and apply TrackerScheme.res (called after UI_ApplyCustomColors in UI_Init)
void UI_LoadTrackerScheme( void );

// Query helpers
inline unsigned int Scheme_GetColor( unsigned int schemeColor, unsigned int fallback )
{
	return schemeColor ? schemeColor : fallback;
}

// GoldSrc-style inset border (sunken look: dark top+left, bright bottom+right)
inline void DrawGoldSrcInset( int x, int y, int w, int h )
{
	unsigned int dark = Scheme_GetColor( g_Scheme.borderDark, 0xFF30342B );
	unsigned int bright = Scheme_GetColor( g_Scheme.borderBright, 0xFF767D6A );
	UI_FillRect( x, y, w, 1, dark );            // top
	UI_FillRect( x, y, 1, h, dark );            // left
	UI_FillRect( x, y + h - 1, w, 1, bright );  // bottom
	UI_FillRect( x + w - 1, y, 1, h, bright );  // right
}

// GoldSrc-style raised border (raised look: bright top+left, dark bottom+right)
inline void DrawGoldSrcRaised( int x, int y, int w, int h )
{
	unsigned int dark = Scheme_GetColor( g_Scheme.borderDark, 0xFF30342B );
	unsigned int bright = Scheme_GetColor( g_Scheme.borderBright, 0xFF767D6A );
	UI_FillRect( x, y, w, 1, bright );          // top
	UI_FillRect( x, y, 1, h, bright );          // left
	UI_FillRect( x, y + h - 1, w, 1, dark );    // bottom
	UI_FillRect( x + w - 1, y, 1, h, dark );    // right
}

#endif // TRACKERCHEME_H
