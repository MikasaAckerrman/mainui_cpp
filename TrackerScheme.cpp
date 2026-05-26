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
	if( !stricmp( str, "Orange" ) ) return 0xFFF0B418;

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

	// Borders
	else if( !stricmp( key, "Border.Bright" ) || !stricmp( key, "BorderBright" ) )
		g_Scheme.borderBright = color;
	else if( !stricmp( key, "Border.Dark" ) || !stricmp( key, "BorderDark" ) )
		g_Scheme.borderDark = color;
	else if( !stricmp( key, "Border.Selection" ) || !stricmp( key, "BorderSelection" ) )
		g_Scheme.borderSelection = color;

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

void UI_LoadTrackerScheme( void )
{
	// Reset
	memset( &g_Scheme, 0, sizeof( g_Scheme ) );
	s_numColorDefs = 0;

	// Try loading from resource/TrackerScheme.res first, then gfx/shell/TrackerScheme.res
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

	if( !afile )
	{
		Con_Printf( "TrackerScheme: not found, applying built-in CS 1.6 defaults\n" );

		// Apply CS 1.6 Steam default colors
		g_Scheme.frameBgColor        = 0xE6282828; // 40,40,40,230
		g_Scheme.frameTitleBarBg     = 0xFF4A3520; // 74,53,32,255 (dark orange-brown)
		g_Scheme.frameTitleBarFg     = 0xFFF0ECE0; // 240,236,224,255
		g_Scheme.frameBorderColor    = 0xFF5C5C5C; // 92,92,92,255

		g_Scheme.borderBright        = 0xC8C8C8C8; // 200,200,200,200
		g_Scheme.borderDark          = 0xC4282828; // 40,40,40,196
		g_Scheme.borderSelection     = 0xC4000000; // 0,0,0,196

		g_Scheme.buttonTextColor     = 0xFFF0ECE0; // light cream
		g_Scheme.buttonBgColor       = 0x00000000; // transparent
		g_Scheme.buttonArmedTextColor= 0xFFFFFFFF; // white on hover
		g_Scheme.buttonArmedBgColor  = 0x40505040; // subtle highlight
		g_Scheme.buttonDepressedTextColor = 0xFF9C9080; // dimmed

		g_Scheme.labelTextColor      = 0xFFF0B418; // 240,180,24  CS 1.6 orange
		g_Scheme.labelBrightColor    = 0xFFFFFFFF; // white
		g_Scheme.labelDimColor       = 0xFFA0A0A0; // gray
		g_Scheme.labelDisabledFg1    = 0xFF505050;
		g_Scheme.labelDisabledFg2    = 0xFF404040;

		g_Scheme.listTextColor       = 0xFFF0ECE0;
		g_Scheme.listBgColor         = 0xE6202020; // 32,32,32,230
		g_Scheme.listSelectedTextColor = 0xFFFFFFFF;
		g_Scheme.listSelectedBgColor = 0xFF4A3520; // dark orange highlight
		g_Scheme.listHeaderTextColor = 0xFFA0A0A0;

		g_Scheme.fieldTextColor      = 0xFFF0ECE0;
		g_Scheme.fieldBgColor        = 0xE6181818; // 24,24,24,230
		g_Scheme.fieldSelectedTextColor = 0xFFFFFFFF;
		g_Scheme.fieldSelectedBgColor = 0xFF4A3520;

		g_Scheme.tabTextColor        = 0xFF9C9080;
		g_Scheme.tabSelectedTextColor= 0xFFF0ECE0;

		g_Scheme.menuTextColor       = 0xFFF0ECE0;
		g_Scheme.menuBgColor         = 0xE6282828;
		g_Scheme.menuArmedTextColor  = 0xFFFFFFFF;
		g_Scheme.menuArmedBgColor    = 0xFF4A3520;

		g_Scheme.bgColor             = 0xE6282828;
		g_Scheme.fgColor             = 0xFFF0ECE0;
		g_Scheme.windowBgColor       = 0xE6202020;
		g_Scheme.windowFgColor       = 0xFFF0ECE0;

		// Also update legacy mainui globals
		uiPromptTextColor  = g_Scheme.labelTextColor;
		uiPromptFocusColor = g_Scheme.buttonArmedTextColor;
		uiPromptBgColor    = g_Scheme.frameBgColor;
		uiInputTextColor   = g_Scheme.fieldTextColor;
		uiInputBgColor     = g_Scheme.fieldBgColor;
		uiInputFgColor     = g_Scheme.borderDark;
		uiColorHelp        = g_Scheme.labelDimColor;

		return;
	}

	// Parse the file
	KVParser kv;
	kv.pFile = afile;

	// Top-level: expect "Scheme" or similar root key, then brace
	// First token is the root key name (e.g. "Scheme"), skip it
	// Then expect opening brace
	if( !kv.NextToken() )
	{
		EngFuncs::COM_FreeFile( afile );
		return;
	}
	// If first token is already '{', we're in a brace-only file
	if( kv.token[0] != '{' )
	{
		// Read next token which should be '{'
		if( !kv.NextToken() || kv.token[0] != '{' )
		{
			Con_Printf( "TrackerScheme: parse error, expected '{' after root key\n" );
			EngFuncs::COM_FreeFile( afile );
			return;
		}
	}

	// Now parse sections
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
			// Skip these sections for now (font sizes handled by FontManager)
			ParseSection( kv, NULL );
		}
		else
		{
			// Unknown section or key at root level - try to skip
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

	// Ensure legacy globals are updated from whatever we parsed
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

	EngFuncs::COM_FreeFile( afile );
}
