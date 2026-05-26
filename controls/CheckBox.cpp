/*
CheckBox.h - checkbox
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
#include "CheckBox.h"
#include "Utils.h"
#include "Frame.h"

CMenuCheckBox::CMenuCheckBox() : BaseClass()
{
	SetCharSize( QM_DEFAULTFONT );
	SetSize( 32, 32 );
	SetPicture( UI_CHECKBOX_EMPTY,
		UI_CHECKBOX_FOCUS,
		UI_CHECKBOX_PRESSED,
		UI_CHECKBOX_ENABLED,
		UI_CHECKBOX_GRAYED );
	bChecked = false;
	eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
	iFlags |= QMF_DROPSHADOW;
	bChangeOnPressed = false;
	colorBase = uiColorWhite;
	colorFocus = uiColorWhite;
	iMask = 0;
	bInvertMask = false;
}

/*
=================
CMenuCheckBox::Init
=================
*/
void CMenuCheckBox::VidInit( void )
{
	colorText.SetDefault( uiColorHelp );

	// When inside a Frame, use compact 13x13 checkbox style
	if( m_pParent && m_pParent->IsFrame() )
	{
		SetSize( 13, 13 );
	}

	BaseClass::VidInit();

	if( m_pParent && m_pParent->IsFrame() )
	{
		// Position text closer to the compact box
		m_scTextPos.x = m_scPos.x + (int)(16 * uiStatic.scaleX);
		m_scTextPos.y = m_scPos.y;
	}
	else
	{
		m_scTextPos.x = m_scPos.x + ( m_scSize.w * 1.25f );
		m_scTextPos.y = m_scPos.y;
	}

	m_scTextSize.w = g_FontMgr->GetTextWideScaled( font, szName, m_scChSize );
	m_scTextSize.h = m_scChSize;
}

bool CMenuCheckBox::KeyUp( int key )
{
	const char	*sound = 0;

	if( UI::Key::IsLeftMouse( key ) && FBitSet( iFlags, QMF_HASMOUSEFOCUS ))
		sound = uiStatic.sounds[SND_GLOW];
	else if( UI::Key::IsEnter( key ) && !FBitSet( iFlags, QMF_MOUSEONLY ))
		sound = uiStatic.sounds[SND_GLOW];

	if( sound )
	{
		_Event( QM_RELEASED );
		if( !bChangeOnPressed )
		{
			bChecked = !bChecked;	// apply on release
			SetCvarValue( bChecked );
			_Event( QM_CHANGED );
		}
		PlayLocalSound( sound );
	}

	return sound != NULL;
}

bool CMenuCheckBox::KeyDown( int key )
{
	const char	*sound = 0;

	if( UI::Key::IsLeftMouse( key ) && FBitSet( iFlags, QMF_HASMOUSEFOCUS ))
		sound = uiStatic.sounds[SND_GLOW];
	else if( UI::Key::IsEnter( key ) && !FBitSet( iFlags, QMF_MOUSEONLY ))
		sound = uiStatic.sounds[SND_GLOW];

	if( sound )
	{
		_Event( QM_PRESSED );
		if( bChangeOnPressed )
		{
			bChecked = !bChecked;	// apply on release
			SetCvarValue( bChecked );
			_Event( QM_CHANGED );
		}
		PlayLocalSound( sound );
	}

	return sound != NULL;
}


/*
=================
CMenuCheckBox::Draw
=================
*/
void CMenuCheckBox::Draw( void )
{
	uint textflags = ( iFlags & QMF_DROPSHADOW ? ETF_SHADOW : 0 ) | ETF_NOSIZELIMIT | ETF_FORCECOL;

	UI_DrawString( font, m_scTextPos, m_scTextSize, szName, colorText, m_scChSize, eTextAlignment, textflags );

	if( szStatusText && iFlags & QMF_NOTIFY )
	{
		Point coord;

		if( szName[0] )
			coord.x = ( uiStatic.buttons_draw_size.w + 40 ) * uiStatic.scaleX;
		else
			coord.x = m_scSize.w + 16 * uiStatic.scaleX;
		coord.x += m_scPos.x;
		coord.y = m_scPos.y + m_scSize.h / 2 - EngFuncs::ConsoleCharacterHeight() / 2;

		int	r, g, b;

		UnpackRGB( r, g, b, uiColorHelp );
		EngFuncs::DrawSetTextColor( r, g, b );
		EngFuncs::DrawConsoleString( coord, szStatusText );
	}

	// Frame-style programmatic drawing: 13x13 sunken bevel box + checkmark glyph
	if( m_pParent && m_pParent->IsFrame() )
	{
		unsigned int bright = Scheme_GetColor( g_Scheme.borderBright, 0xFFC8C8C8 );
		unsigned int dark   = Scheme_GetColor( g_Scheme.borderDark,   0xFF282828 );
		unsigned int fieldBg = Scheme_GetColor( g_Scheme.fieldBgColor, 0xE6323232 );

		int x = m_scPos.x;
		int y = m_scPos.y;
		int w = m_scSize.w;
		int h = m_scSize.h;

		// Fill inside (field background - white/light for checkbox well)
		UI_FillRect( x + 1, y + 1, w - 2, h - 2, fieldBg );

		// Sunken bevel: dark on top+left, bright on bottom+right (opposite of raised)
		// Top edge (dark)
		UI_FillRect( x, y, w, 1, dark );
		// Left edge (dark)
		UI_FillRect( x, y, 1, h, dark );
		// Bottom edge (bright)
		UI_FillRect( x, y + h - 1, w, 1, bright );
		// Right edge (bright)
		UI_FillRect( x + w - 1, y, 1, h, bright );

		// Draw checkmark when checked
		if( bChecked )
		{
			unsigned int checkColor = Scheme_GetColor( g_Scheme.labelTextColor, 0xFFC8C8C8 );

			// Draw a V-shaped checkmark inside the box
			// The check starts from left-center, goes down to bottom-center,
			// then up to top-right corner area.
			int pad = 2;
			int innerW = w - pad * 2;
			int innerH = h - pad * 2;

			// Short descending stroke (left part of V): from top-left area to bottom-center
			int midX = pad + innerW / 3;      // x pivot at ~1/3 width
			int startY = pad + innerH / 3;    // start ~1/3 down
			int midY = pad + innerH - 2;      // bottom of V

			// Descending stroke
			int steps1 = midX - pad;
			if( steps1 > 0 )
			{
				for( int i = 0; i <= steps1; i++ )
				{
					int px = x + pad + i;
					int py = y + startY + (int)((float)i * (midY - startY) / steps1);
					UI_FillRect( px, py, 1, 1, checkColor );
					UI_FillRect( px, py + 1, 1, 1, checkColor ); // 2px thick
				}
			}

			// Ascending stroke (right part of V): from bottom-center to top-right
			int endX = pad + innerW - 1;
			int endY = pad + 1;
			int steps2 = endX - midX;
			if( steps2 > 0 )
			{
				for( int i = 0; i <= steps2; i++ )
				{
					int px = x + midX + i;
					int py = y + midY - (int)((float)i * (midY - endY) / steps2);
					UI_FillRect( px, py, 1, 1, checkColor );
					UI_FillRect( px, py + 1, 1, 1, checkColor ); // 2px thick
				}
			}
		}

		return;
	}

	// Legacy bitmap-based drawing for non-frame contexts
	if( iFlags & QMF_GRAYED )
	{
		UI_DrawPic( m_scPos, m_scSize, uiColorWhite, szGrayedPic );
		return; // grayed
	}

	if((( iFlags & QMF_MOUSEONLY ) && !( iFlags & QMF_HASMOUSEFOCUS ))
	   || ( this != m_pParent->ItemAtCursor() ) )
	{
		if( !bChecked )
			UI_DrawPic( m_scPos, m_scSize, colorBase, szEmptyPic );
		else UI_DrawPic( m_scPos, m_scSize, colorBase, szCheckPic );
		return; // no focus
	}

	if( m_bPressed )
	{
		UI_DrawPic( m_scPos, m_scSize, colorBase, szPressPic );
	}
	else if( eFocusAnimation == QM_HIGHLIGHTIFFOCUS )
	{
		if( bChecked )
		{
			// use two textures for it. Second is just focus texture, slightly orange. Looks pretty.
			UI_DrawPic( m_scPos, m_scSize, colorBase, szPressPic );
			UI_DrawPic( m_scPos, m_scSize, uiInputTextColor, szFocusPic, QM_DRAWADDITIVE );
		}
		else
		{
			UI_DrawPic( m_scPos, m_scSize, colorFocus, szFocusPic );
		}
	}
	else if( bChecked )
	{
		UI_DrawPic( m_scPos, m_scSize, colorBase, szCheckPic );
	}
	else
	{
		UI_DrawPic( m_scPos, m_scSize, colorBase, szEmptyPic );
	}
}

void CMenuCheckBox::UpdateEditable()
{
	bChecked = !!CvarValue();
}
