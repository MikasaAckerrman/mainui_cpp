/*
TrackerScheme.cpp - Source Engine-style scheme loader for mainui_cpp
Copyright (C) 2024 DragonSlayer Team

Parses resource/TrackerScheme.res in Valve KeyValues format and maps
BaseSettings/Colors to mainui_cpp globals and the extended SchemeColors struct.
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "TrackerScheme.h"

SchemeColors g_Scheme;

// Canonical built-in scheme. This is the SINGLE source of truth for the
// CS 1.6 PC palette: it is parsed through the exact same path as an on-disk
// resource/TrackerScheme.res, so there are no separate hardcoded ARGB
// constants that can drift out of sync. The .res file shipped in
// game_assets/resource/ MUST mirror this text; when present on the device it
// simply overrides this built-in default. Kept as a mutable char[] because
// COM_ParseFile takes a char* cursor.
static char s_defaultScheme[] = R"SCHEME(
"Scheme"
{
	"Colors"
	{
		"White"				"255 255 255 255"
		"OffWhite"			"220 220 220 255"
		"LightGray"			"200 200 200 255"
		"DullWhite"			"160 160 160 255"
		"MedOlive"			"75 80 50 255"
		"DarkOlive"			"55 58 35 255"
		"DarkerOlive"		"35 38 22 255"
		"TitleOlive"		"95 104 79 230"
		"TitleTopEdge"		"105 114 89 255"
		"TitleBottomEdge"	"75 84 59 255"
		"Black"				"0 0 0 255"
		"TransBlack"		"0 0 0 128"
		"ListBG"			"45 50 35 230"
		"FieldBG"			"85 96 75 230"
		"FrameBG"			"95 104 79 230"
		"MainBG"			"95 104 79 255"
		"SelectionBG"		"90 90 50 255"
		"Highlight"			"80 85 50 64"
		"ActiveTabGreen"	"93 102 77 230"
		"TabInactive"		"55 62 43 230"
		"TabActiveText"		"255 255 255 255"
		"WindowBG"			"48 56 40 230"
		"Blank"				"0 0 0 0"
	}

	"BaseSettings"
	{
		"Frame.BgColor"					"FrameBG"
		"Frame.OutOfFocusBgColor"		"FrameBG"
		"FrameTitleBar.BgColor"			"TitleOlive"
		"FrameTitleBar.TextColor"		"White"
		"FrameTitleBar.TopEdgeColor"	"TitleTopEdge"
		"FrameTitleBar.BottomEdgeColor"	"TitleBottomEdge"

		"Border.Bright"					"95 101 88 200"
		"Border.Dark"					"40 44 36 200"
		"Border.Selection"				"Black"
		"Border.InnerBright"			"144 152 128 200"
		"Border.InnerDark"				"58 62 48 200"

		"Frame.HighlightBandColor"		"255 255 255 64"
		"Frame.ShadowBandColor"			"0 0 0 64"

		"Button.TextColor"				"White"
		"Button.BgColor"				"91 99 80 255"
		"Button.ArmedTextColor"			"White"
		"Button.ArmedBgColor"			"107 115 96 255"
		"Button.DepressedTextColor"		"DullWhite"

		"Label.TextColor"				"LightGray"
		"Label.TextBrightColor"			"White"
		"Label.TextDullColor"			"DullWhite"
		"Label.DisabledFgColor1"		"DarkOlive"
		"Label.DisabledFgColor2"		"DarkerOlive"

		"ListPanel.TextColor"			"White"
		"ListPanel.BgColor"				"ListBG"
		"ListPanel.SelectedTextColor"	"White"
		"ListPanel.SelectedBgColor"		"SelectionBG"
		"ListPanel.HeaderTextColor"		"DullWhite"
		"SectionedListPanel.HeaderTextColor" "DullWhite"

		"TextEntry.TextColor"			"White"
		"TextEntry.BgColor"				"FieldBG"
		"TextEntry.SelectedTextColor"	"White"
		"TextEntry.SelectedBgColor"		"SelectionBG"

		"PropertySheet.TextColor"		"DullWhite"
		"PropertySheet.SelectedTextColor" "TabActiveText"
		"PropertySheet.ActiveTabBgColor" "ActiveTabGreen"
		"PropertySheet.InactiveTabBgColor" "TabInactive"
		"PropertySheet.BgColor"			"WindowBG"

		"Menu.TextColor"				"White"
		"Menu.BgColor"					"FrameBG"
		"Menu.ArmedTextColor"			"White"
		"Menu.ArmedBgColor"				"SelectionBG"

		"Panel.FgColor"					"White"
		"Panel.BgColor"					"FrameBG"
	}
}
)SCHEME";

// Simple KeyValues parser state
struct KVParser
{
	char *pFile;
	char token[512];

	bool NextToken()
	{
		pFile = EngFuncs::COM_ParseFile( pFile, token, sizeof( token ) );
		return pFile != NULL;
	}
};

// Parse "R G B" or "R G B A" color string into ARGB uint
static unsigned int ParseColorString( const char *str )
{
	int r = 255, g = 255, b = 255, a = 255;

	if( !str || !str[0] )
		return 0;

	// Handle named colors
	if( !stricmp( str, "White" ) ) return 0xFFFFFFFF;
	if( !stricmp( str, "Black" ) ) return 0xFF000000;
	if( !stricmp( str, "Blank" ) || !stricmp( str, "None" ) ) return 0;
	if( !stricmp( str, "Orange" ) ) return 0xFFC8C8C8;

	int count = sscanf( str, "%d %d %d %d", &r, &g, &b, &a );
	if( count < 3 )
		return 0;
	if( count < 4 )
		a = 255;

	return (a << 24) | (r << 16) | (g << 8) | b;
}

// Try resolving a color value: could be direct "R G B A" or a reference to Colors section
struct ColorDef
{
	char name[64];
	char value[128];
};

#define MAX_COLOR_DEFS 128
static ColorDef s_colorDefs[MAX_COLOR_DEFS];
static int s_numColorDefs = 0;

static const char *LookupColorName( const char *name )
{
	for( int i = 0; i < s_numColorDefs; i++ )
	{
		if( !stricmp( s_colorDefs[i].name, name ) )
			return s_colorDefs[i].value;
	}
	return NULL;
}

static unsigned int ResolveColor( const char *value )
{
	if( !value || !value[0] )
		return 0;

	// First check if it's a named reference to Colors section
	const char *resolved = LookupColorName( value );
	if( resolved )
		return ParseColorString( resolved );

	// Otherwise parse directly
	return ParseColorString( value );
}

// Parse a section recursively - we only need top-level key-value pairs
// in Colors, BaseSettings sections
static void ParseSection( KVParser &kv, void (*handler)(const char *key, const char *value) )
{
	// Expect opening brace
	if( !kv.NextToken() || kv.token[0] != '{' )
		return;

	while( kv.NextToken() )
	{
		if( kv.token[0] == '}' )
			return;

		char key[256];
		Q_strncpy( key, kv.token, sizeof( key ) );

		if( !kv.NextToken() )
			return;

		if( kv.token[0] == '{' )
		{
			// Skip nested sections (Fonts subsections etc)
			int depth = 1;
			while( depth > 0 && kv.NextToken() )
			{
				if( kv.token[0] == '{' ) depth++;
				else if( kv.token[0] == '}' ) depth--;
			}
			continue;
		}

		// key = value pair
		if( handler )
			handler( key, kv.token );
	}
}

// Handler for "Colors" section
static void ColorsHandler( const char *key, const char *value )
{
	if( s_numColorDefs >= MAX_COLOR_DEFS )
		return;

	Q_strncpy( s_colorDefs[s_numColorDefs].name, key, sizeof( s_colorDefs[0].name ) );
	Q_strncpy( s_colorDefs[s_numColorDefs].value, value, sizeof( s_colorDefs[0].value ) );
	s_numColorDefs++;
}

// Handler for "BaseSettings" section
static void BaseSettingsHandler( const char *key, const char *value )
{
	unsigned int color = ResolveColor( value );
	if( !color )
		return;

	// Map Source Engine BaseSettings keys to our scheme struct
	// Frame
	if( !stricmp( key, "Frame.BgColor" ) || !stricmp( key, "BgColor" ) )
		g_Scheme.frameBgColor = color;
	else if( !stricmp( key, "Frame.OutOfFocusBgColor" ) )
	{} // optional
	else if( !stricmp( key, "FrameTitleBar.BgColor" ) || !stricmp( key, "TitleBarBgColor" ) )
		g_Scheme.frameTitleBarBg = color;
	else if( !stricmp( key, "FrameTitleBar.TextColor" ) || !stricmp( key, "TitleBarFgColor" ) )
		g_Scheme.frameTitleBarFg = color;
	else if( !stricmp( key, "FrameTitleBar.TopEdgeColor" ) )
		g_Scheme.frameTitleBarTop = color;
	else if( !stricmp( key, "FrameTitleBar.BottomEdgeColor" ) )
		g_Scheme.frameTitleBarBottom = color;

	// Borders
	else if( !stricmp( key, "Border.Bright" ) || !stricmp( key, "BorderBright" ) )
		g_Scheme.borderBright = color;
	else if( !stricmp( key, "Border.Dark" ) || !stricmp( key, "BorderDark" ) )
		g_Scheme.borderDark = color;
	else if( !stricmp( key, "Border.Selection" ) || !stricmp( key, "BorderSelection" ) )
		g_Scheme.borderSelection = color;
	else if( !stricmp( key, "Border.InnerBright" ) )
		g_Scheme.borderInnerBright = color;
	else if( !stricmp( key, "Border.InnerDark" ) )
		g_Scheme.borderInnerDark = color;

	// Frame body gradient bands
	else if( !stricmp( key, "Frame.HighlightBandColor" ) )
		g_Scheme.frameHighlightBand = color;
	else if( !stricmp( key, "Frame.ShadowBandColor" ) )
		g_Scheme.frameShadowBand = color;

	// Buttons
	else if( !stricmp( key, "Button.TextColor" ) || !stricmp( key, "ControlFG" ) )
		g_Scheme.buttonTextColor = color;
	else if( !stricmp( key, "Button.BgColor" ) || !stricmp( key, "ControlBG" ) )
		g_Scheme.buttonBgColor = color;
	else if( !stricmp( key, "Button.ArmedTextColor" ) )
		g_Scheme.buttonArmedTextColor = color;
	else if( !stricmp( key, "Button.ArmedBgColor" ) )
		g_Scheme.buttonArmedBgColor = color;
	else if( !stricmp( key, "Button.DepressedTextColor" ) )
		g_Scheme.buttonDepressedTextColor = color;

	// Labels
	else if( !stricmp( key, "Label.TextColor" ) || !stricmp( key, "BaseText" ) )
		g_Scheme.labelTextColor = color;
	else if( !stricmp( key, "Label.TextBrightColor" ) || !stricmp( key, "BrightControlText" ) )
		g_Scheme.labelBrightColor = color;
	else if( !stricmp( key, "Label.TextDullColor" ) || !stricmp( key, "LabelDimText" ) )
		g_Scheme.labelDimColor = color;
	else if( !stricmp( key, "Label.DisabledFgColor1" ) || !stricmp( key, "DisabledFgColor1" ) )
		g_Scheme.labelDisabledFg1 = color;
	else if( !stricmp( key, "Label.DisabledFgColor2" ) || !stricmp( key, "DisabledFgColor2" ) )
		g_Scheme.labelDisabledFg2 = color;

	// ListPanel / Table
	else if( !stricmp( key, "ListPanel.TextColor" ) || !stricmp( key, "WindowFgColor" ) )
		g_Scheme.listTextColor = color;
	else if( !stricmp( key, "ListPanel.BgColor" ) || !stricmp( key, "ListBgColor" ) )
		g_Scheme.listBgColor = color;
	else if( !stricmp( key, "ListPanel.SelectedTextColor" ) || !stricmp( key, "ListSelectionFgColor" ) )
		g_Scheme.listSelectedTextColor = color;
	else if( !stricmp( key, "ListPanel.SelectedBgColor" ) )
		g_Scheme.listSelectedBgColor = color;
	else if( !stricmp( key, "SectionedListPanel.HeaderTextColor" ) || !stricmp( key, "SectionTextColor" ) )
		g_Scheme.listHeaderTextColor = color;

	// TextEntry / Field
	else if( !stricmp( key, "TextEntry.TextColor" ) )
		g_Scheme.fieldTextColor = color;
	else if( !stricmp( key, "TextEntry.BgColor" ) || !stricmp( key, "WindowBgColor" ) )
		g_Scheme.fieldBgColor = color;
	else if( !stricmp( key, "TextEntry.SelectedTextColor" ) || !stricmp( key, "SelectionFgColor" ) )
		g_Scheme.fieldSelectedTextColor = color;
	else if( !stricmp( key, "TextEntry.SelectedBgColor" ) || !stricmp( key, "SelectionBgColor" ) )
		g_Scheme.fieldSelectedBgColor = color;

	// PropertySheet / Tabs
	else if( !stricmp( key, "PropertySheet.TextColor" ) || !stricmp( key, "FgColorDim" ) )
		g_Scheme.tabTextColor = color;
	else if( !stricmp( key, "PropertySheet.SelectedTextColor" ) )
		g_Scheme.tabSelectedTextColor = color;
	else if( !stricmp( key, "PropertySheet.ActiveTabBgColor" ) )
		g_Scheme.tabActiveBgColor = color;
	else if( !stricmp( key, "PropertySheet.InactiveTabBgColor" ) )
		g_Scheme.tabInactiveBgColor = color;

	// Menu
	else if( !stricmp( key, "Menu.TextColor" ) )
		g_Scheme.menuTextColor = color;
	else if( !stricmp( key, "Menu.BgColor" ) )
		g_Scheme.menuBgColor = color;
	else if( !stricmp( key, "Menu.ArmedTextColor" ) )
		g_Scheme.menuArmedTextColor = color;
	else if( !stricmp( key, "Menu.ArmedBgColor" ) )
		g_Scheme.menuArmedBgColor = color;

	// Generic
	else if( !stricmp( key, "Panel.BgColor" ) )
		g_Scheme.bgColor = color;
	else if( !stricmp( key, "Panel.FgColor" ) || !stricmp( key, "FgColor" ) )
		g_Scheme.fgColor = color;
	// Content area behind a PropertySheet/TabView (read by TabView.cpp)
	else if( !stricmp( key, "PropertySheet.BgColor" ) )
		g_Scheme.windowBgColor = color;

	// Also apply to mainui global colors for legacy controls
	if( !stricmp( key, "Label.TextColor" ) || !stricmp( key, "BaseText" ) )
		uiPromptTextColor = color;
	else if( !stricmp( key, "Button.ArmedTextColor" ) || !stricmp( key, "BrightControlText" ) )
		uiPromptFocusColor = color;
	else if( !stricmp( key, "Frame.BgColor" ) || !stricmp( key, "BgColor" ) )
		uiPromptBgColor = color;
	else if( !stricmp( key, "TextEntry.TextColor" ) )
		uiInputTextColor = color;
	else if( !stricmp( key, "TextEntry.BgColor" ) )
		uiInputBgColor = color;
	else if( !stricmp( key, "Label.TextDullColor" ) || !stricmp( key, "LabelDimText" ) )
		uiColorHelp = color;
}

// Parse a scheme KeyValues buffer (on-disk file or built-in default) into
// g_Scheme and mirror the relevant colors into the legacy mainui globals.
// buffer must be a writable, null-terminated cursor for COM_ParseFile.
static void ParseSchemeBuffer( char *buffer )
{
	KVParser kv;
	kv.pFile = buffer;

	// Top-level: optional root key (e.g. "Scheme") followed by opening brace.
	if( !kv.NextToken() )
		return;
	if( kv.token[0] != '{' )
	{
		if( !kv.NextToken() || kv.token[0] != '{' )
		{
			Con_Printf( "TrackerScheme: parse error, expected '{' after root key\n" );
			return;
		}
	}

	while( kv.NextToken() )
	{
		if( kv.token[0] == '}' )
			break;

		if( !stricmp( kv.token, "Colors" ) )
		{
			ParseSection( kv, ColorsHandler );
		}
		else if( !stricmp( kv.token, "BaseSettings" ) )
		{
			ParseSection( kv, BaseSettingsHandler );
		}
		else if( !stricmp( kv.token, "Fonts" ) || !stricmp( kv.token, "Borders" ) || !stricmp( kv.token, "CustomFontFiles" ) )
		{
			// Handled elsewhere (FontManager / programmatic bevels) - skip.
			ParseSection( kv, NULL );
		}
		else
		{
			// Unknown section or key at root level - try to skip its block.
			if( kv.NextToken() && kv.token[0] == '{' )
			{
				int depth = 1;
				while( depth > 0 && kv.NextToken() )
				{
					if( kv.token[0] == '{' ) depth++;
					else if( kv.token[0] == '}' ) depth--;
				}
			}
		}
	}

	// Mirror parsed colors into the legacy mainui globals used by older controls.
	if( g_Scheme.labelTextColor )
		uiPromptTextColor = g_Scheme.labelTextColor;
	if( g_Scheme.buttonArmedTextColor )
		uiPromptFocusColor = g_Scheme.buttonArmedTextColor;
	if( g_Scheme.frameBgColor )
		uiPromptBgColor = g_Scheme.frameBgColor;
	if( g_Scheme.fieldTextColor )
		uiInputTextColor = g_Scheme.fieldTextColor;
	if( g_Scheme.fieldBgColor )
		uiInputBgColor = g_Scheme.fieldBgColor;
	if( g_Scheme.borderDark )
		uiInputFgColor = g_Scheme.borderDark;
	if( g_Scheme.labelDimColor )
		uiColorHelp = g_Scheme.labelDimColor;
}

void UI_LoadTrackerScheme( void )
{
	// Reset
	memset( &g_Scheme, 0, sizeof( g_Scheme ) );
	s_numColorDefs = 0;

	// Prefer an on-disk override, otherwise use the built-in canonical scheme.
	// Both are parsed through the same path, so the result is identical.
	const char *paths[] = {
		"resource/TrackerScheme.res",
		"gfx/shell/TrackerScheme.res",
		NULL
	};

	char *afile = NULL;
	for( int i = 0; paths[i]; i++ )
	{
		afile = (char *)EngFuncs::COM_LoadFile( paths[i], NULL );
		if( afile )
		{
			Con_Printf( "TrackerScheme: loaded from %s\n", paths[i] );
			break;
		}
	}

	if( afile )
	{
		ParseSchemeBuffer( afile );
		EngFuncs::COM_FreeFile( afile );
	}
	else
	{
		Con_Printf( "TrackerScheme: file not found, using built-in canonical scheme\n" );
		ParseSchemeBuffer( s_defaultScheme );
	}
}
