/*
Slider.h - slider
Copyright (C) 2010 Uncle Mike
Copyright (C) 2017 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "Slider.h"
#include "Utils.h"
#include "Frame.h"

CMenuSlider::CMenuSlider() : BaseClass(), m_flMinValue(), m_flMaxValue(), m_flCurValue(),
	m_flDrawStep(), m_iNumSteps(), m_flRange(), m_iKeepSlider()
{
	m_iSliderOutlineWidth = 6;

	size.w = 200;
	size.h = 2 + m_iSliderOutlineWidth * 2;

	m_flRange = 1.0f;

	eFocusAnimation = QM_HIGHLIGHTIFFOCUS;

	SetCharSize( QM_DEFAULTFONT );

	imgSlider = UI_SLIDER_MAIN;

	iFlags |= QMF_DROPSHADOW;
}

/*
=================
CMenuSlider::Init
=================
*/
void CMenuSlider::VidInit(  )
{
	if( m_flRange < 0.05f )
		m_flRange = 0.05f;

	colorBase.SetDefault( uiColorWhite );
	colorFocus.SetDefault( uiColorWhite );

	BaseClass::VidInit();

	// scale the center box
	m_scCenterBox.w = 40 * uiStatic.scaleX;
	m_scCenterBox.h = m_scSize.h - m_iSliderOutlineWidth * 2;

	m_iNumSteps = (m_flMaxValue - m_flMinValue) / m_flRange + 1;
	m_flDrawStep = (float)(m_scSize.w - m_iSliderOutlineWidth - m_scCenterBox.w) / (float)m_iNumSteps;
}

bool CMenuSlider::KeyUp( int key )
{
	if( m_iKeepSlider )
	{
		// tell menu about changes
		SetCvarValue( m_flCurValue );
		_Event( QM_CHANGED );
		m_iKeepSlider = false; // button released
	}
	return true;

}

/*
=================
CMenuSlider::Key
=================
*/
bool CMenuSlider::KeyDown( int key )
{
	if( UI::Key::IsLeftMouse( key ))
	{
		if( !UI_CursorInRect( m_scPos, m_scSize ) )
		{
			m_iKeepSlider = false;
			return true;
		}

		m_iKeepSlider = true;

		// immediately move slider into specified place
		int	dist, numSteps;

		dist = uiStatic.cursorX - (m_scPos.x + m_iSliderOutlineWidth + m_scCenterBox.w);
		numSteps = floor(dist / m_flDrawStep);
		m_flCurValue = bound( m_flMinValue, numSteps * m_flRange + m_flMinValue, m_flMaxValue );

		// tell menu about changes
		SetCvarValue( m_flCurValue );
		_Event( QM_CHANGED );

		return true;
	}
	else if( UI::Key::IsLeftArrow( key ))
	{
		m_flCurValue -= m_flRange;

		if( m_flCurValue < m_flMinValue )
		{
			m_flCurValue = m_flMinValue;
			PlayLocalSound( uiStatic.sounds[SND_BUZZ] );
			return true;
		}

		// tell menu about changes
		SetCvarValue( m_flCurValue );
		_Event( QM_CHANGED );

		PlayLocalSound( uiStatic.sounds[SND_KEY] );
		return true;
	}
	else if( UI::Key::IsRightArrow( key ))
	{
		m_flCurValue += m_flRange;

		if( m_flCurValue > m_flMaxValue )
		{
			m_flCurValue = m_flMaxValue;
			PlayLocalSound( uiStatic.sounds[SND_BUZZ] );
			return true;
		}

		// tell menu about changes
		SetCvarValue( m_flCurValue );
		_Event( QM_CHANGED );
		PlayLocalSound( uiStatic.sounds[SND_KEY] );
		return true;
	}

	return false;
}

/*
=================
CMenuSlider::Draw
=================
*/
void CMenuSlider::Draw( void )
{
	int	textHeight, sliderX;
	uint textflags = ( iFlags & QMF_DROPSHADOW ) ? ETF_SHADOW : 0;

	if( szStatusText && iFlags & QMF_NOTIFY )
	{
		Point coord;

		coord.x = m_scPos.x + 16 * uiStatic.scaleX;
		coord.y = m_scPos.y + m_scSize.h / 2 - EngFuncs::ConsoleCharacterHeight() / 2;

		int	r, g, b;

		UnpackRGB( r, g, b, uiColorHelp );
		EngFuncs::DrawSetTextColor( r, g, b );
		EngFuncs::DrawConsoleString( coord, szStatusText );
	}

	if( m_iKeepSlider )
	{
		if( !UI_CursorInRect( m_scPos.x, m_scPos.y - 40, m_scSize.w, m_scSize.h + 80 ) )
			m_iKeepSlider = false;
		else
		{
			int	dist, numSteps;

			// move slider follow the holded mouse button
			dist = uiStatic.cursorX - m_scPos.x - m_iSliderOutlineWidth - (m_scCenterBox.w/2);
			numSteps = floor(dist / m_flDrawStep);
			m_flCurValue = bound( m_flMinValue, numSteps * m_flRange + m_flMinValue, m_flMaxValue );

			// tell menu about changes
			SetCvarValue( m_flCurValue );
			_Event( QM_CHANGED );
		}
	}

	// keep value in range
	m_flCurValue = bound( m_flMinValue, m_flCurValue, m_flMaxValue );

	// calc slider position
	sliderX = m_scPos.x + (m_iSliderOutlineWidth/2) // start
		+ ( ( m_flCurValue - m_flMinValue ) / ( m_flMaxValue - m_flMinValue ) )  // calc fractional part
		* ( m_scSize.w - m_iSliderOutlineWidth - (m_scCenterBox.w) );

	// Frame-style programmatic drawing: sunken groove track + raised bevel thumb
	if( m_pParent && m_pParent->IsFrame() )
	{
		unsigned int bright = Scheme_GetColor( g_Scheme.borderBright, 0xFFC8C8C8 );
		unsigned int dark   = Scheme_GetColor( g_Scheme.borderDark,   0xFF282828 );

		// Track: thin 3px sunken groove centered vertically
		int trackH = 3;
		int trackY = m_scPos.y + m_scSize.h / 2 - trackH / 2;
		int trackX = m_scPos.x + m_iSliderOutlineWidth / 2;
		int trackW = m_scSize.w - m_iSliderOutlineWidth;

		// Track fill
		UI_FillRect( trackX + 1, trackY + 1, trackW - 2, trackH - 2, dark );

		// Track sunken bevel: dark top+left, bright bottom+right
		UI_FillRect( trackX, trackY, trackW, 1, dark );            // top
		UI_FillRect( trackX, trackY, 1, trackH, dark );            // left
		UI_FillRect( trackX, trackY + trackH - 1, trackW, 1, bright ); // bottom
		UI_FillRect( trackX + trackW - 1, trackY, 1, trackH, bright ); // right

		// Thumb: ~11x20 raised bevel rectangle
		int thumbW = (int)(11 * uiStatic.scaleX);
		int thumbH = (int)(20 * uiStatic.scaleY);
		if( thumbW < 7 ) thumbW = 7;
		if( thumbH < 12 ) thumbH = 12;
		int thumbX = sliderX;
		int thumbY = m_scPos.y + m_scSize.h / 2 - thumbH / 2;

		// Thumb fill (medium grey)
		// Use a slightly lighter grey for the thumb face
		unsigned int thumbFace = 0xFFC0C0C0;
		UI_FillRect( thumbX + 1, thumbY + 1, thumbW - 2, thumbH - 2, thumbFace );

		// Thumb raised bevel: bright top+left, dark bottom+right
		UI_FillRect( thumbX, thumbY, thumbW, 1, bright );            // top
		UI_FillRect( thumbX, thumbY, 1, thumbH, bright );            // left
		UI_FillRect( thumbX, thumbY + thumbH - 1, thumbW, 1, dark ); // bottom
		UI_FillRect( thumbX + thumbW - 1, thumbY, 1, thumbH, dark ); // right

		// Decorative: 3 thin vertical groove lines on the thumb center
		int grooveCenterX = thumbX + thumbW / 2;
		int grooveTop = thumbY + thumbH / 4;
		int grooveBot = thumbY + thumbH - thumbH / 4;
		int grooveH = grooveBot - grooveTop;

		for( int g = -1; g <= 1; g++ )
		{
			int gx = grooveCenterX + g * 2;
			if( gx > thumbX + 1 && gx < thumbX + thumbW - 2 )
			{
				UI_FillRect( gx, grooveTop, 1, grooveH, dark );
				UI_FillRect( gx + 1, grooveTop + 1, 1, grooveH, bright );
			}
		}
	}
	else
	{
		// Legacy bitmap-based drawing for non-frame contexts
		UI_DrawRectangleExt( m_scPos.x + m_iSliderOutlineWidth / 2, m_scPos.y + m_iSliderOutlineWidth, m_scSize.w - m_iSliderOutlineWidth, m_scCenterBox.h, uiInputBgColor, m_iSliderOutlineWidth );
		if( eFocusAnimation == QM_HIGHLIGHTIFFOCUS && this == m_pParent->ItemAtCursor())
			UI_DrawPic( sliderX, m_scPos.y, m_scCenterBox.w, m_scSize.h, uiColorHelp, imgSlider );
		else
			UI_DrawPic( sliderX, m_scPos.y, m_scCenterBox.w, m_scSize.h, uiColorWhite, imgSlider );
	}

	textHeight = m_scPos.y - (m_scChSize * 1.5f);
	UI_DrawString( font, m_scPos.x, textHeight, m_scSize.w, m_scChSize, szName, uiColorHelp, m_scChSize, eTextAlignment, textflags | ETF_FORCECOL );
}

void CMenuSlider::UpdateEditable()
{
	float flValue = CvarValue();

	m_flCurValue = flValue;
}
