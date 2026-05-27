"Scheme"
{
	"Colors"
	{
		// CS 1.6 olive/military green palette
		"White"				"255 255 255 255"
		"OffWhite"			"220 220 220 255"
		"LightGray"			"200 200 200 255"
		"DullWhite"			"160 160 160 255"
		"MedOlive"			"75 80 50 255"
		"DarkOlive"			"55 58 35 255"
		"DarkerOlive"		"35 38 22 255"
		"TitleOlive"		"60 60 40 255"
		"Black"				"0 0 0 255"
		"TransBlack"		"0 0 0 128"
		"ListBG"			"45 50 35 230"
		"FieldBG"			"50 55 38 230"
		"FrameBG"			"75 75 50 230"
		"SelectionBG"		"90 90 50 255"
		"Highlight"			"80 85 50 64"
		"ActiveTabGreen"	"90 120 50 255"
		"Blank"				"0 0 0 0"
	}

	"BaseSettings"
	{
		// ===== Frame / Windows =====
		"Frame.BgColor"					"FrameBG"
		"Frame.OutOfFocusBgColor"		"FrameBG"
		"FrameTitleBar.BgColor"			"TitleOlive"
		"FrameTitleBar.TextColor"		"White"
		"FrameTitleBar.DisabledTextColor" "DullWhite"
		"FrameTitleBar.DisabledBgColor"	"DarkerOlive"

		// ===== Borders =====
		"Border.Bright"					"LightGray"
		"Border.Dark"					"DarkerOlive"
		"Border.Selection"				"Black"

		// ===== Buttons =====
		"Button.TextColor"				"White"
		"Button.BgColor"				"Blank"
		"Button.ArmedTextColor"			"White"
		"Button.ArmedBgColor"			"Highlight"
		"Button.DepressedTextColor"		"DullWhite"
		"Button.DepressedBgColor"		"Blank"

		// ===== Check buttons =====
		"CheckButton.TextColor"			"OffWhite"
		"CheckButton.SelectedTextColor"	"White"
		"CheckButton.BgColor"			"TransBlack"

		// ===== Labels =====
		"Label.TextColor"				"LightGray"
		"Label.TextBrightColor"			"White"
		"Label.TextDullColor"			"DullWhite"
		"Label.DisabledFgColor1"		"DarkOlive"
		"Label.DisabledFgColor2"		"DarkerOlive"

		// ===== List / Table =====
		"ListPanel.TextColor"			"White"
		"ListPanel.BgColor"				"ListBG"
		"ListPanel.SelectedTextColor"	"White"
		"ListPanel.SelectedBgColor"		"SelectionBG"
		"ListPanel.HeaderTextColor"		"DullWhite"
		"ListPanel.EmptyListInfoTextColor" "DullWhite"

		"SectionedListPanel.HeaderTextColor" "DullWhite"
		"SectionedListPanel.HeaderBgColor"	"ListBG"
		"SectionedListPanel.TextColor"		"White"
		"SectionedListPanel.BrightTextColor" "White"
		"SectionedListPanel.BgColor"		"ListBG"
		"SectionedListPanel.SelectedTextColor" "White"
		"SectionedListPanel.SelectedBgColor" "SelectionBG"

		// ===== Text entry / Fields =====
		"TextEntry.TextColor"			"White"
		"TextEntry.BgColor"				"FieldBG"
		"TextEntry.SelectedTextColor"	"White"
		"TextEntry.SelectedBgColor"		"SelectionBG"
		"TextEntry.CursorColor"			"White"
		"TextEntry.DisabledTextColor"	"DullWhite"
		"TextEntry.DisabledBgColor"		"DarkerOlive"

		// ===== Tabs / PropertySheet =====
		"PropertySheet.TextColor"		"DullWhite"
		"PropertySheet.SelectedTextColor" "White"
		"PropertySheet.ActiveTabBgColor" "ActiveTabGreen"

		// ===== Menu =====
		"Menu.TextColor"				"White"
		"Menu.BgColor"					"FrameBG"
		"Menu.ArmedTextColor"			"White"
		"Menu.ArmedBgColor"				"SelectionBG"

		// ===== Scrollbar =====
		"ScrollBarButton.FgColor"		"DullWhite"
		"ScrollBarButton.BgColor"		"DarkerOlive"
		"ScrollBarButton.ArmedFgColor"	"White"
		"ScrollBarButton.ArmedBgColor"	"DarkerOlive"
		"ScrollBarSlider.FgColor"		"DarkOlive"
		"ScrollBarSlider.BgColor"		"DarkerOlive"

		// ===== Slider =====
		"Slider.NobColor"				"LightGray"
		"Slider.TextColor"				"White"
		"Slider.TrackColor"				"DarkOlive"

		// ===== Tooltip =====
		"Tooltip.TextColor"				"White"
		"Tooltip.BgColor"				"TitleOlive"

		// ===== Generic panel =====
		"Panel.FgColor"					"White"
		"Panel.BgColor"					"FrameBG"
	}

	"Fonts"
	{
		// Font definitions - mainui_cpp handles these via FontManager
		// Tahoma 11px feel (CS 1.6 PC reference)
		"Default"
		{
			"1"
			{
				"name"		"Tahoma"
				"tall"		"12"
				"weight"	"500"
			}
		}
		"DefaultBold"
		{
			"1"
			{
				"name"		"Tahoma"
				"tall"		"12"
				"weight"	"700"
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

	"Borders"
	{
		// Border definitions - drawn programmatically in mainui_cpp
		"Default"
		{
			"inset"		"0 0 1 1"
			"Left"
			{
				"1"
				{
					"color"		"Border.Bright"
					"offset"	"0 1"
				}
			}
			"Right"
			{
				"1"
				{
					"color"		"Border.Dark"
					"offset"	"1 0"
				}
			}
			"Top"
			{
				"1"
				{
					"color"		"Border.Bright"
					"offset"	"0 0"
				}
			}
			"Bottom"
			{
				"1"
				{
					"color"		"Border.Dark"
					"offset"	"0 0"
				}
			}
		}
	}
}
