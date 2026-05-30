// CS 1.6 PC canonical scheme. This file is a PLAIN MIRROR of the embedded
// default in TrackerScheme.cpp (s_defaultScheme). Both pass through the same
// parser, so on-disk and built-in are byte-equivalent. Values were verified
// pixel-by-pixel against original CS 1.6 screenshots
// (Documentation/reference/): ControlBG #4C5844 dominates >74% of every
// Options page, gold #ADA34D = BrightControlText 196 181 80 appears in the
// active tab text band.

"Scheme"
{
	"Colors"
	{
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

		// Tab strip
		"TabInactive"		"55 62 43 230"

		// Title bar
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
		// ===== Frame / Windows =====
		"Frame.BgColor"					"ControlBG"
		"Frame.OutOfFocusBgColor"		"ControlBG"
		"FrameTitleBar.BgColor"			"TitleBG"
		"FrameTitleBar.TextColor"		"BrightBaseText"
		"FrameTitleBar.TopEdgeColor"	"TitleTopEdge"
		"FrameTitleBar.BottomEdgeColor"	"TitleBottomEdge"

		// ===== Borders =====
		"Border.Bright"					"BorderBright"
		"Border.Dark"					"BorderDark"
		"Border.Selection"				"BorderSelection"
		"Border.InnerBright"			"104 113 96 200"
		"Border.InnerDark"				"50 56 42 200"

		// ===== Frame body gradient bands =====
		"Frame.HighlightBandColor"		"255 255 255 48"
		"Frame.ShadowBandColor"			"0 0 0 48"

		// ===== Buttons =====
		"Button.TextColor"				"BaseText"
		"Button.BgColor"				"ControlBG"
		"Button.ArmedTextColor"			"BrightBaseText"
		"Button.ArmedBgColor"			"ControlDarkBG"
		"Button.DepressedTextColor"		"DimBaseText"

		// ===== Labels =====
		"Label.TextColor"				"ControlText"
		"Label.TextBrightColor"			"BrightBaseText"
		"Label.TextDullColor"			"DimBaseText"
		"Label.DisabledFgColor1"		"DisabledText1"
		"Label.DisabledFgColor2"		"DisabledText2"

		// ===== List / Table =====
		"ListPanel.TextColor"			"BaseText"
		"ListPanel.BgColor"				"ListBG"
		"ListPanel.SelectedTextColor"	"BrightBaseText"
		"ListPanel.SelectedBgColor"		"SelectionBG"
		"ListPanel.HeaderTextColor"		"DimBaseText"
		"SectionedListPanel.HeaderTextColor" "DimBaseText"

		// ===== Text entry / Fields =====
		"TextEntry.TextColor"			"BaseText"
		"TextEntry.BgColor"				"WindowBG"
		"TextEntry.SelectedTextColor"	"BrightBaseText"
		"TextEntry.SelectedBgColor"		"SelectionBG"

		// ===== Tabs / PropertySheet =====
		"PropertySheet.TextColor"			"DimBaseText"
		"PropertySheet.SelectedTextColor"	"BrightControlText"
		"PropertySheet.ActiveTabBgColor"	"ControlBG"
		"PropertySheet.InactiveTabBgColor"	"TabInactive"
		"PropertySheet.BgColor"				"ControlBG"

		// ===== Menu =====
		"Menu.TextColor"				"BaseText"
		"Menu.BgColor"					"ControlBG"
		"Menu.ArmedTextColor"			"BrightBaseText"
		"Menu.ArmedBgColor"				"SelectionBG"

		// ===== Generic panel =====
		"Panel.FgColor"					"BaseText"
		"Panel.BgColor"					"ControlBG"

		// ===== Check button (canon check mark = gold) =====
		"CheckButtonCheck"				"BrightControlText"

		// ===== Slider (canon track = ControlDarkBG) =====
		"Slider.SliderBgColor"			"ControlDarkBG"
	}

	"Fonts"
	{
		// Tahoma matches CS 1.6 dialogs; mainui FontManager loads it.
		"Default"
		{
			"1"
			{
				"name"		"Tahoma"
				"tall"		"12"
				"weight"	"400"
			}
		}
		"Title"
		{
			"1"
			{
				"name"		"Tahoma"
				"tall"		"14"
				"weight"	"700"
			}
		}
	}
}
