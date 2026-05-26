"Scheme"
{
	"Colors"
	{
		// CS 1.6 Steam-era color palette
		"Orange"			"240 180 24 255"
		"OrangeDim"			"200 140 20 200"
		"White"				"240 236 224 255"
		"OffWhite"			"220 216 204 255"
		"DullWhite"			"160 156 144 255"
		"LightGray"			"160 160 160 255"
		"DarkGray"			"64 64 64 255"
		"DarkerGray"		"40 40 40 255"
		"Black"				"0 0 0 255"
		"TransBlack"		"0 0 0 128"
		"DarkBrown"			"74 53 32 255"
		"MedBrown"			"100 80 50 255"
		"ListBG"			"32 32 32 230"
		"FieldBG"			"24 24 24 230"
		"FrameBG"			"40 40 40 230"
		"SelectionBG"		"74 53 32 255"
		"Highlight"			"80 64 40 64"
		"Blank"				"0 0 0 0"
	}

	"BaseSettings"
	{
		// ===== Frame / Windows =====
		"Frame.BgColor"					"FrameBG"
		"Frame.OutOfFocusBgColor"		"FrameBG"
		"FrameTitleBar.BgColor"			"DarkBrown"
		"FrameTitleBar.TextColor"		"White"
		"FrameTitleBar.DisabledTextColor" "DullWhite"
		"FrameTitleBar.DisabledBgColor"	"DarkerGray"

		// ===== Borders =====
		"Border.Bright"					"LightGray"
		"Border.Dark"					"DarkerGray"
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
		"Label.TextColor"				"Orange"
		"Label.TextBrightColor"			"White"
		"Label.TextDullColor"			"LightGray"
		"Label.DisabledFgColor1"		"DarkGray"
		"Label.DisabledFgColor2"		"DarkerGray"

		// ===== List / Table =====
		"ListPanel.TextColor"			"White"
		"ListPanel.BgColor"				"ListBG"
		"ListPanel.SelectedTextColor"	"White"
		"ListPanel.SelectedBgColor"		"SelectionBG"
		"ListPanel.HeaderTextColor"		"LightGray"
		"ListPanel.EmptyListInfoTextColor" "DullWhite"

		"SectionedListPanel.HeaderTextColor" "LightGray"
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
		"TextEntry.CursorColor"			"Orange"
		"TextEntry.DisabledTextColor"	"DullWhite"
		"TextEntry.DisabledBgColor"		"DarkerGray"

		// ===== Tabs / PropertySheet =====
		"PropertySheet.TextColor"		"DullWhite"
		"PropertySheet.SelectedTextColor" "White"

		// ===== Menu =====
		"Menu.TextColor"				"White"
		"Menu.BgColor"					"FrameBG"
		"Menu.ArmedTextColor"			"White"
		"Menu.ArmedBgColor"				"SelectionBG"

		// ===== Scrollbar =====
		"ScrollBarButton.FgColor"		"DullWhite"
		"ScrollBarButton.BgColor"		"DarkerGray"
		"ScrollBarButton.ArmedFgColor"	"White"
		"ScrollBarButton.ArmedBgColor"	"DarkerGray"
		"ScrollBarSlider.FgColor"		"DarkGray"
		"ScrollBarSlider.BgColor"		"DarkerGray"

		// ===== Slider =====
		"Slider.NobColor"				"Orange"
		"Slider.TextColor"				"White"
		"Slider.TrackColor"				"DarkGray"

		// ===== Tooltip =====
		"Tooltip.TextColor"				"White"
		"Tooltip.BgColor"				"DarkBrown"

		// ===== Generic panel =====
		"Panel.FgColor"					"White"
		"Panel.BgColor"					"FrameBG"
	}

	"Fonts"
	{
		// Font definitions - mainui_cpp handles these via FontManager
		// These are here for documentation/reference
		"Default"
		{
			"1"
			{
				"name"		"Tahoma"
				"tall"		"16"
				"weight"	"500"
			}
		}
		"DefaultBold"
		{
			"1"
			{
				"name"		"Tahoma"
				"tall"		"16"
				"weight"	"700"
			}
		}
		"Title"
		{
			"1"
			{
				"name"		"Tahoma"
				"tall"		"20"
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
					"color"		"Border.Dark"
					"offset"	"0 1"
				}
			}
			"Right"
			{
				"1"
				{
					"color"		"Border.Bright"
					"offset"	"1 0"
				}
			}
			"Top"
			{
				"1"
				{
					"color"		"Border.Dark"
					"offset"	"0 0"
				}
			}
			"Bottom"
			{
				"1"
				{
					"color"		"Border.Bright"
					"offset"	"0 0"
				}
			}
		}
	}
}
