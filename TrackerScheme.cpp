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
		// CS 1.6 PC canonical palette - mirrors SmileyAG TrackerScheme.res
		// (reference verified pixel-by-pixel against original screenshots in
		//  Documentation/reference/: ControlBG 76 88 68 dominates 74-77% of
		//  every Options page; gold #ADA34D == BrightControlText 196 181 80
		//  appears in the active tab text band).
		"White"				"255 255 255 255"
		"Black"				"0 0 0 255"
		"Blank"				"0 0 0 0"

		// Text colors
		"BaseText"			"216 222 211 255"
		"BrightBaseText"	"255 255 255 255"
		"DimBaseText"		"160 170 149 255"
		"ControlText"		"216 222 211 255"
		"BrightControlText"	"196 181 80 255"
		"DimListText"		"117 134 102 255"
		"DisabledText1"		"117 128 111 255"
		"DisabledText2"		"40 46 34 255"

		// Backgrounds
		"ControlBG"			"76 88 68 255"
		"ControlDarkBG"		"90 106 80 255"
		"WindowBG"			"62 70 55 255"
		"ListBG"			"62 70 55 230"
		"SelectionBG"		"149 136 49 255"
		"FieldBG"			"62 70 55 230"

		// Tab strip: inactive tabs share ControlBG with the body (canon CS 1.6).
		// They are distinguished from the active tab purely by 1px bevel and
		// the 2px top shrink, NOT by background color. Using a darker fill
		// here was a non-canonical artefact of an early scheme draft.
		"TabInactive"		"76 88 68 255"

		// Title bar (canonical TitleBG is transparent; we keep ControlBG for
		// VGUI1 which expects an opaque title since it does not composite)
		"TitleBG"			"76 88 68 255"
		"TitleTopEdge"		"136 145 128 255"
		"TitleBottomEdge"	"40 46 34 255"

		// Borders
		"BorderBright"		"136 145 128 255"
		"BorderDark"		"40 46 34 255"
		"BorderSelection"	"0 0 0 255"
	}

	"BaseSettings"
	{
		"Frame.BgColor"					"ControlBG"
		"Frame.OutOfFocusBgColor"		"ControlBG"
		"FrameTitleBar.BgColor"			"TitleBG"
		"FrameTitleBar.TextColor"		"BrightBaseText"
		"FrameTitleBar.TopEdgeColor"	"TitleTopEdge"
		"FrameTitleBar.BottomEdgeColor"	"TitleBottomEdge"

		"Border.Bright"					"BorderBright"
		"Border.Dark"					"BorderDark"
		"Border.Selection"				"BorderSelection"
		// Phase 1 inner-bevel (we draw a second 1px line inside the outer
		// border for GoldSrc 3D depth). Canonical CS 1.6 borders are 1px
		// only, but removing this is Phase D - for now we just dim it so it
		// reads as a subtle highlight rather than a second band.
		"Border.InnerBright"			"104 113 96 200"
		"Border.InnerDark"				"50 56 42 200"

		"Frame.HighlightBandColor"		"255 255 255 48"
		"Frame.ShadowBandColor"			"0 0 0 48"

		// Buttons
		"Button.TextColor"				"BaseText"
		"Button.BgColor"				"ControlBG"
		"Button.ArmedTextColor"			"BrightBaseText"
		"Button.ArmedBgColor"			"ControlDarkBG"
		"Button.DepressedTextColor"		"DimBaseText"

		// Labels
		"Label.TextColor"				"ControlText"
		"Label.TextBrightColor"			"BrightBaseText"
		"Label.TextDullColor"			"DimBaseText"
		"Label.DisabledFgColor1"		"DisabledText1"
		"Label.DisabledFgColor2"		"DisabledText2"

		// Lists
		"ListPanel.TextColor"			"BaseText"
		"ListPanel.BgColor"				"ListBG"
		"ListPanel.SelectedTextColor"	"BrightBaseText"
		"ListPanel.SelectedBgColor"		"SelectionBG"
		"ListPanel.HeaderTextColor"		"DimBaseText"
		"SectionedListPanel.HeaderTextColor" "DimBaseText"

		// Text fields
		"TextEntry.TextColor"			"BaseText"
		"TextEntry.BgColor"				"WindowBG"
		"TextEntry.SelectedTextColor"	"BrightBaseText"
		"TextEntry.SelectedBgColor"		"SelectionBG"

		// Property sheet (tabs)
		"PropertySheet.TextColor"			"DimBaseText"
		"PropertySheet.SelectedTextColor"	"BrightControlText"
		"PropertySheet.ActiveTabBgColor"	"ControlBG"
		"PropertySheet.InactiveTabBgColor"	"TabInactive"
		"PropertySheet.BgColor"				"ControlBG"

		// Menus
		"Menu.TextColor"				"BaseText"
		"Menu.BgColor"					"ControlBG"
		"Menu.ArmedTextColor"			"BrightBaseText"
		"Menu.ArmedBgColor"				"SelectionBG"

		// Generic Panel
		"Panel.FgColor"					"BaseText"
		"Panel.BgColor"					"ControlBG"

		// Check button glyph (canon = gold)
		"CheckButtonCheck"				"BrightControlText"

		// Slider track behind handle (canon: ControlDarkBG)
		"Slider.SliderBgColor"			"ControlDarkBG"
	}

	"Fonts"
	{
		// Default menu font. CS 1.6 PC menu canonically uses Tahoma.
		// Override here (and ship a matching TTF at gfx/fonts/<name>.ttf)
		// to re-skin the engine font without rebuilding mainui.
		"DefaultFont"
		{
			"1"
			{
				"name"		"Tahoma"
				"tall"		"10"
				"weight"	"500"
			}
		}
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

	// Check button glyph color (gold in canon CS 1.6)
	else if( !stricmp( key, "CheckButtonCheck" ) )
		g_Scheme.checkMarkColor = color;

	// Slider track (area behind handle - canonical ControlDarkBG)
	else if( !stricmp( key, "Slider.SliderBgColor" ) || !stricmp( key, "ScrollBarSlider.ScrollBarSliderBgColor" ) )
		g_Scheme.sliderBgColor = color;

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

// Parse the "Fonts" section to capture the DefaultFont entry (name + tall).
// CS 1.6 / Tracker scheme structure:
//   Fonts {
//     DefaultFont {
//       "1" { "name" "Tahoma" "tall" "10" "weight" "500" }
//     }
//     ...other named font groups we ignore...
//   }
// We grab only DefaultFont/1/{name,tall,weight}; the FontManager resolves
// the rest on its own. Anything we don't understand is skipped, including
// any extra resolution-buckets ("2", "3", ...) - the CS 1.6 menu uses a
// single bucket in practice and DPI scaling is already handled by VS().
static void ParseFontsSection( KVParser &kv )
{
	if( !kv.NextToken() || kv.token[0] != '{' )
		return;

	while( kv.NextToken() )
	{
		if( kv.token[0] == '}' )
			return;

		bool isDefault = !stricmp( kv.token, "DefaultFont" );

		if( !kv.NextToken() || kv.token[0] != '{' )
			return;

		if( !isDefault )
		{
			// Skip the whole named-font group.
			int depth = 1;
			while( depth > 0 && kv.NextToken() )
			{
				if( kv.token[0] == '{' ) depth++;
				else if( kv.token[0] == '}' ) depth--;
			}
			continue;
		}

		// Inside DefaultFont: walk numbered buckets and grab the first one
		// we can read fully.
		while( kv.NextToken() )
		{
			if( kv.token[0] == '}' )
				break;

			if( !kv.NextToken() || kv.token[0] != '{' )
				return;

			char fName[64] = {0};
			int fTall = 0, fWeight = 0;
			while( kv.NextToken() )
			{
				if( kv.token[0] == '}' )
					break;
				char key[64];
				Q_strncpy( key, kv.token, sizeof( key ) );
				if( !kv.NextToken() )
					return;
				if( !stricmp( key, "name" ) )
					Q_strncpy( fName, kv.token, sizeof( fName ) );
				else if( !stricmp( key, "tall" ) )
					fTall = atoi( kv.token );
				else if( !stricmp( key, "weight" ) )
					fWeight = atoi( kv.token );
			}

			if( !g_Scheme.menuFontName[0] && fName[0] )
			{
				Q_strncpy( g_Scheme.menuFontName, fName, sizeof( g_Scheme.menuFontName ) );
				g_Scheme.menuFontTall = fTall;
				g_Scheme.menuFontWeight = fWeight;
			}
		}
	}
}


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
		else if( !stricmp( kv.token, "Fonts" ) )
		{
			ParseFontsSection( kv );
		}
		else if( !stricmp( kv.token, "Borders" ) || !stricmp( kv.token, "CustomFontFiles" ) )
		{
			// Handled elsewhere (programmatic bevels) - skip.
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
