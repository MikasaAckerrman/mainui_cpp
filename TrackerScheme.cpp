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

// ---------------------------------------------------------------------------
// Generic role table.
//
// Every BaseSettings key is stored verbatim, resolved lazily on query. The
// shipped CS 1.6 resource/TrackerScheme.res defines 147 roles; the named
// g_Scheme fields cover 34 of them. Storing all of them means a widget can ask
// for "ScrollBarSlider.NobDragColor" or "Tooltip.BgColor" without anyone
// touching this file, and re-skinning is a pure data change.
//
// 256 slots: 147 in the shipped scheme, plus room for the per-widget roles the
// Source-style variants add. Overflow is reported once instead of silently
// dropping roles the way the old 54-branch chain did.
#define MAX_SCHEME_ROLES 256

struct SchemeRole
{
	char name[64];
	char value[128];
};

static SchemeRole s_roles[MAX_SCHEME_ROLES];
static int        s_numRoles = 0;
static bool       s_roleOverflowWarned = false;

// Forward decls: ResolveColor lives below the built-in scheme text.
static unsigned int ResolveColor( const char *value, bool *ok );

static const SchemeRole *Scheme_FindRole( const char *role )
{
	int i;

	if( !role || !role[0] )
		return NULL;

	for( i = 0; i < s_numRoles; i++ )
	{
		if( !stricmp( s_roles[i].name, role ))
			return &s_roles[i];
	}

	return NULL;
}

// Store (or replace) one role. Later definitions win: CS schemes legitimately
// repeat a key (the shipped file sets FrameTitleBar.Font twice, UiBold then
// DefaultLarge) and Valve's own loader keeps the last one.
static void Scheme_StoreRole( const char *key, const char *value )
{
	int i;

	if( !key || !key[0] )
		return;

	for( i = 0; i < s_numRoles; i++ )
	{
		if( !stricmp( s_roles[i].name, key ))
		{
			Q_strncpy( s_roles[i].value, value ? value : "", sizeof( s_roles[i].value ));
			return;
		}
	}

	if( s_numRoles >= MAX_SCHEME_ROLES )
	{
		if( !s_roleOverflowWarned )
		{
			Con_Printf( "TrackerScheme: role table full (%d), ignoring \"%s\" and later roles\n",
				MAX_SCHEME_ROLES, key );
			s_roleOverflowWarned = true;
		}
		return;
	}

	Q_strncpy( s_roles[s_numRoles].name, key, sizeof( s_roles[0].name ));
	Q_strncpy( s_roles[s_numRoles].value, value ? value : "", sizeof( s_roles[0].value ));
	s_numRoles++;
}

bool Scheme_HasRole( const char *role )
{
	const SchemeRole *r = Scheme_FindRole( role );
	bool ok = false;

	if( !r )
		return false;

	// Present but unparsable counts as absent: the caller wants a usable colour.
	ResolveColor( r->value, &ok );
	return ok;
}

unsigned int Scheme_Role( const char *role, unsigned int fallback )
{
	const SchemeRole *r = Scheme_FindRole( role );
	bool ok = false;
	unsigned int color;

	if( !r )
		return fallback;

	color = ResolveColor( r->value, &ok );

	// ok, not color != 0: "Blank" resolves to 0 and MUST be honoured, otherwise
	// every transparent role in the canon scheme (21 of them) silently becomes
	// whatever the widget's fallback happens to be.
	return ok ? color : fallback;
}

bool Scheme_HasMetric( const char *role )
{
	const SchemeRole *r = Scheme_FindRole( role );

	if( !r || !r->value[0] )
		return false;

	// A metric is a bare number. Colours ("136 145 128 255") and names
	// ("Orange") must not answer a metric query.
	{
		const char *p = r->value;
		int digits = 0;

		if( *p == '-' || *p == '+' )
			p++;

		for( ; *p; p++ )
		{
			if( *p >= '0' && *p <= '9' )
			{
				digits++;
				continue;
			}
			if( *p == '.' )
				continue;
			return false;
		}

		return digits > 0;
	}
}

float Scheme_Metric( const char *role, float fallback )
{
	const SchemeRole *r;

	if( !Scheme_HasMetric( role ))
		return fallback;

	r = Scheme_FindRole( role );
	return (float)atof( r->value );
}

int Scheme_RoleCount( void )
{
	return s_numRoles;
}

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

// Last-resort values for the named colors CS 1.6 schemes use. These are ONLY
// consulted when the scheme's own "Colors" section does not define the name
// (ResolveColor checks Colors first), so a scheme file always wins.
//
// The canonical values are copied from the shipped resource/TrackerScheme.res.
// "Orange" in particular is NOT orange: it is the muted CS gold 142 137 35, and
// it is the selection colour for ListPanel, Menu, TextEntry, RichText,
// SectionedListPanel, Slider.NobFocus and Tooltip - a wrong value here washes
// out every selection highlight in the UI at once.
struct NamedColor
{
	const char   *name;
	unsigned int  argb;
};

static const NamedColor s_namedColors[] =
{
	{ "White",            0xFFFFFFFF },
	{ "OffWhite",         0xFFD8D8D8 },
	{ "DullWhite",        0xFFB6B6B6 },
	{ "Orange",           0xFF8E8923 },
	{ "TransparentBlack", 0x80000000 },
	{ "Black",            0xFF000000 },
	{ "Blank",            0x00000000 },
	{ "None",             0x00000000 },
	{ "ScrollBarGrey",    0xFF333333 },
	{ "ScrollBarHilight", 0xFF6E6E6E },
	{ "ScrollBarDark",    0xFF262626 },
};

// Parse "R G B" / "R G B A" or a known colour name into packed ARGB.
//
// ok distinguishes "parsed to zero" from "could not parse". Both look like 0 to
// the caller otherwise, and the difference matters: "Blank" is a legitimate,
// widely used value in CS schemes (21 occurrences in the shipped file) meaning
// "draw nothing", while an unparsable value must leave the previous colour
// alone. Treating the two the same is why every Blank role used to be dropped.
static unsigned int ParseColorString( const char *str, bool *ok )
{
	int r = 255, g = 255, b = 255, a = 255;
	int count;
	size_t i;

	if( ok )
		*ok = false;

	if( !str || !str[0] )
		return 0;

	for( i = 0; i < sizeof( s_namedColors ) / sizeof( s_namedColors[0] ); i++ )
	{
		if( !stricmp( str, s_namedColors[i].name ))
		{
			if( ok )
				*ok = true;
			return s_namedColors[i].argb;
		}
	}

	count = sscanf( str, "%d %d %d %d", &r, &g, &b, &a );
	if( count < 3 )
		return 0;
	if( count < 4 )
		a = 255;

	if( ok )
		*ok = true;

	return (unsigned int)((a << 24) | (r << 16) | (g << 8) | b);
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

static unsigned int ResolveColor( const char *value, bool *ok )
{
	const char *resolved;

	if( ok )
		*ok = false;

	if( !value || !value[0] )
		return 0;

	// A name defined in the scheme's own Colors section wins over the built-in
	// fallback table, so a re-skin only has to edit Colors.
	resolved = LookupColorName( value );
	if( resolved )
		return ParseColorString( resolved, ok );

	// Otherwise it is either a literal "R G B A" or a well-known name.
	return ParseColorString( value, ok );
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
	bool ok = false;
	unsigned int color;

	// EVERY role is recorded first, whether or not a named g_Scheme field wants
	// it. The named fields below are just the hot path for existing widgets;
	// the generic table is what lets a widget ask for ScrollBarSlider.NobDragColor
	// without anyone editing this function.
	Scheme_StoreRole( key, value );

	color = ResolveColor( value, &ok );
	if( !ok )
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
// Parse the "Fonts" section to capture the menu's default font.
//
// Group naming differs between sources and this cost us the font entirely:
// our own built-in scheme calls the group "DefaultFont", but the CS 1.6 file
// shipped with the game names its groups "Default", "DefaultBold",
// "DefaultSmall", "UiBold", "MenuLarge"... - there is no "DefaultFont" at all.
// Matching only our own name meant menuFontName stayed empty for every real
// scheme file, and the engine silently kept its hardcoded font.
//
// Both names are accepted, with "DefaultFont" winning when a scheme defines it,
// so our built-in text and canon files behave the same.
//   Fonts {
//     Default { "1" { "name" "Tahoma" "tall" "16" "weight" "500" } }
//   }
// Only name/tall/weight are read; antialias/outline/dropshadow and the
// resolution buckets ("2", "3", ...) are the FontManager's business.
static void ParseFontsSection( KVParser &kv )
{
	char defaultFontName[64] = {0};
	int  defaultFontTall = 0, defaultFontWeight = 0;
	char fallbackName[64] = {0};
	int  fallbackTall = 0, fallbackWeight = 0;

	if( !kv.NextToken() || kv.token[0] != '{' )
		return;

	while( kv.NextToken() )
	{
		if( kv.token[0] == '}' )
			break;

		// Preference order, highest first.
		bool isPreferred = !stricmp( kv.token, "DefaultFont" );
		bool isFallback  = !stricmp( kv.token, "Default" );

		if( !kv.NextToken() || kv.token[0] != '{' )
			return;

		if( !isPreferred && !isFallback )
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

		// Inside the group: walk numbered buckets, keep the first readable one.
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

			if( !fName[0] )
				continue;

			if( isPreferred && !defaultFontName[0] )
			{
				Q_strncpy( defaultFontName, fName, sizeof( defaultFontName ));
				defaultFontTall = fTall;
				defaultFontWeight = fWeight;
			}
			else if( isFallback && !fallbackName[0] )
			{
				Q_strncpy( fallbackName, fName, sizeof( fallbackName ));
				fallbackTall = fTall;
				fallbackWeight = fWeight;
			}
		}
	}

	if( defaultFontName[0] )
	{
		Q_strncpy( g_Scheme.menuFontName, defaultFontName, sizeof( g_Scheme.menuFontName ));
		g_Scheme.menuFontTall = defaultFontTall;
		g_Scheme.menuFontWeight = defaultFontWeight;
	}
	else if( fallbackName[0] )
	{
		Q_strncpy( g_Scheme.menuFontName, fallbackName, sizeof( g_Scheme.menuFontName ));
		g_Scheme.menuFontTall = fallbackTall;
		g_Scheme.menuFontWeight = fallbackWeight;
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

// ---------------------------------------------------------------------------
// Scheme selection and live reload.
//
// The look of the menu is a FILE in the game directory, not a build artefact.
// CS 1.6 (and NextClient on top of it) ships resource/schemes/TrackerScheme_*.res
// and a "Colour scheme" dropdown that picks one; NextClient stores the choice in
// MiscellaneousSettings.vdf and requires a game restart to apply it.
//
// We keep the same file layout so schemes are interchangeable with the PC game,
// but the choice lives in an archived cvar (survives restart, editable from
// config.cfg or the console) and applying it does NOT need a restart - the
// scheme is data, so re-reading it and re-running VidInit is enough.
//
// Search order for the active scheme, first hit wins:
//   1. resource/schemes/<ui_scheme>      - the selected variant
//   2. resource/TrackerScheme.res        - the game's own base scheme
//   3. gfx/shell/TrackerScheme.res       - legacy location we shipped earlier
//   4. built-in canonical text           - identical parse path, never fails
#define SCHEME_DIR      "resource/schemes"
#define SCHEME_PATTERN  SCHEME_DIR "/TrackerScheme_*.res"
#define SCHEME_DEFAULT  "TrackerScheme_ClassicPlus.res"

static char s_activeSchemePath[256];

const char *Scheme_ActivePath( void )
{
	return s_activeSchemePath;
}

// Where did the active scheme come from? Empty cvar means "no variant selected",
// which is not an error - the base resource/TrackerScheme.res is then used.
static char *Scheme_LoadSelected( char *pathOut, size_t pathSize )
{
	const char *selected = EngFuncs::GetCvarString( "ui_scheme" );
	char        candidate[256];
	char       *afile;

	if( !selected || !selected[0] )
		return NULL;

	// Accept both "TrackerScheme_Source.res" and a full relative path, so a user
	// editing config.cfg by hand cannot end up with resource/schemes/resource/...
	if( strchr( selected, '/' ) || strchr( selected, '\\' ))
	{
		Q_strncpy( candidate, selected, sizeof( candidate ));
	}
	else
	{
		Q_strncpy( candidate, SCHEME_DIR "/", sizeof( candidate ));
		strncat( candidate, selected, sizeof( candidate ) - strlen( candidate ) - 1 );
	}

	afile = (char *)EngFuncs::COM_LoadFile( candidate, NULL );
	if( !afile )
	{
		Con_Printf( "TrackerScheme: ui_scheme \"%s\" not found (%s), falling back\n",
			selected, candidate );
		return NULL;
	}

	Q_strncpy( pathOut, candidate, pathSize );
	return afile;
}

void UI_LoadTrackerScheme( void )
{
	static const char *paths[] = {
		"resource/TrackerScheme.res",
		"gfx/shell/TrackerScheme.res",
		NULL
	};
	char *afile = NULL;
	int i;

	// Reset. s_numRoles too: without it a scheme switch would keep roles from
	// the previous file that the new one does not define.
	memset( &g_Scheme, 0, sizeof( g_Scheme ));
	s_numColorDefs = 0;
	s_numRoles = 0;
	s_roleOverflowWarned = false;
	s_activeSchemePath[0] = 0;

	afile = Scheme_LoadSelected( s_activeSchemePath, sizeof( s_activeSchemePath ));

	if( !afile )
	{
		for( i = 0; paths[i]; i++ )
		{
			afile = (char *)EngFuncs::COM_LoadFile( paths[i], NULL );
			if( afile )
			{
				Q_strncpy( s_activeSchemePath, paths[i], sizeof( s_activeSchemePath ));
				break;
			}
		}
	}

	if( afile )
	{
		ParseSchemeBuffer( afile );
		EngFuncs::COM_FreeFile( afile );
	}
	else
	{
		Q_strncpy( s_activeSchemePath, "(built-in)", sizeof( s_activeSchemePath ));
		ParseSchemeBuffer( s_defaultScheme );
	}

	Con_Printf( "TrackerScheme: %s, %d roles\n", s_activeSchemePath, s_numRoles );
}

// "ui_scheme_list" - print the schemes present in the game directory.
// Reading the directory (instead of a hardcoded list) is the point: dropping a
// new .res in resource/schemes/ makes it selectable with no code change.
static void UI_SchemeList_f( void )
{
	const char *active = EngFuncs::GetCvarString( "ui_scheme" );
	char      **files;
	int         count = 0, i;

	files = EngFuncs::GetFilesList( SCHEME_PATTERN, &count, true );

	Con_Printf( "Colour schemes in %s:\n", SCHEME_DIR );

	if( !files || count <= 0 )
	{
		Con_Printf( "  (none found - using %s or the built-in scheme)\n",
			"resource/TrackerScheme.res" );
	}

	for( i = 0; i < count; i++ )
	{
		const char *slash = strrchr( files[i], '/' );
		const char *name = slash ? slash + 1 : files[i];
		bool isActive = active && active[0] && !stricmp( active, name );

		Con_Printf( "  %s%s\n", name, isActive ? "  <-- active" : "" );
	}

	Con_Printf( "active file: %s\n", s_activeSchemePath[0] ? s_activeSchemePath : "(none)" );
	Con_Printf( "use: ui_scheme <file> ; ui_scheme_reload\n" );
}

// "ui_scheme_reload" - re-read the scheme and re-lay out the menu.
//
// UI_VidInit is what re-creates fonts and re-runs every _VidInit, so colours,
// the scheme font and any metric-derived geometry all pick up the new file. This
// is why a restart is not needed: nothing about a scheme is baked at build time.
static void UI_SchemeReload_f( void )
{
	UI_LoadTrackerScheme();
	UI_VidInit();
}

void UI_RegisterSchemeCommands( void )
{
	// FCVAR_ARCHIVE: the choice belongs in the player's config.cfg, same as any
	// other preference. Default empty rather than SCHEME_DEFAULT so a game whose
	// resource/TrackerScheme.res is already the desired look is not overridden
	// by a variant that happens to exist next to it.
	EngFuncs::CvarRegister( "ui_scheme", "", FCVAR_ARCHIVE );

	EngFuncs::Cmd_AddCommand( "ui_scheme_list", UI_SchemeList_f );
	EngFuncs::Cmd_AddCommand( "ui_scheme_reload", UI_SchemeReload_f );
}
