/*
FrameButton.cpp -- CS 1.6 / Source Engine style flat button with bevel border
Copyright (C) 2024 DragonSlayer Team
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "FrameButton.h"
#include "Frame.h"
#include "Utils.h"
#include "keydefs.h"

CMenuFrameButton::CMenuFrameButton() : BaseClass()
{
	eTextAlignment = QM_CENTER;
}

void CMenuFrameButton::VidInit()
{
	BaseClass::VidInit();
}

bool CMenuFrameButton::KeyDown( int key )
{
	if( UI::Key::IsEnter( key ) && !(iFlags & QMF_MOUSEONLY) )
	{
		_Event( QM_PRESSED );
		return true;
	}
	if( UI::Key::IsLeftMouse( key ) && ( iFlags & QMF_HASMOUSEFOCUS ) )
	{
		_Event( QM_PRESSED );
		return true;
	}
	return false;
}

bool CMenuFrameButton::KeyUp( int key )
{
	bool handled = false;

	if( UI::Key::IsEnter( key ) && !(iFlags & QMF_MOUSEONLY) )
		handled = true;
	else if( UI::Key::IsLeftMouse( key ) && ( iFlags & QMF_HASMOUSEFOCUS ) )
		handled = true;

	if( handled )
	{
		_Event( QM_RELEASED );
		PlayLocalSound( uiStatic.sounds[SND_LAUNCH] );
	}

	return handled;
}

void CMenuFrameButton::Draw()
{
	bool focused = ( m_pParent && this == m_pParent->ItemAtCursor() );
	bool pressed = m_bPressed;

	// Colors - GoldSrc VGUI style button, customizable via TrackerScheme
	unsigned int schemeBg = g_Scheme.buttonBgColor;
	unsigned int bgNormal  = schemeBg ? schemeBg : 0xFF5B6350;
	unsigned int bgHover   = g_Scheme.buttonArmedBgColor ? g_Scheme.buttonArmedBgColor : 0xFF6B7360;
	unsigned int bgPressed = 0xFF353535;
	unsigned int bright    = Scheme_GetColor( g_Scheme.borderBright, 0xFF757D69 );
	unsigned int dark      = Scheme_GetColor( g_Scheme.borderDark, 0xFF2F342B );
	unsigned int textNorm  = Scheme_GetColor( g_Scheme.buttonTextColor, 0xFFFFFFFF );
	unsigned int textFocus = Scheme_GetColor( g_Scheme.buttonArmedTextColor, 0xFFFFFFFF );

	// Background fill
	unsigned int bg = bgNormal;
	if( pressed )
		bg = bgPressed;
	else if( focused )
		bg = bgHover;

	UI_FillRect( m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h, bg );

	// Double-layer border: outer dark outline + inner raised/sunken bevel
	// Outer dark outline (1px all around) - reuses 'dark' from above
	UI_FillRect( m_scPos.x, m_scPos.y, m_scSize.w, 1, dark );
	UI_FillRect( m_scPos.x, m_scPos.y + m_scSize.h - 1, m_scSize.w, 1, dark );
	UI_FillRect( m_scPos.x, m_scPos.y, 1, m_scSize.h, dark );
	UI_FillRect( m_scPos.x + m_scSize.w - 1, m_scPos.y, 1, m_scSize.h, dark );

	// Inner bevel (1px, inside the outer dark outline)
	unsigned int topLeft = pressed ? dark : bright;
	unsigned int bottomRight = pressed ? bright : dark;

	UI_FillRect( m_scPos.x + 1, m_scPos.y + 1, m_scSize.w - 2, 1, topLeft );     // top
	UI_FillRect( m_scPos.x + 1, m_scPos.y + 1, 1, m_scSize.h - 2, topLeft );     // left
	UI_FillRect( m_scPos.x + 1, m_scPos.y + m_scSize.h - 2, m_scSize.w - 2, 1, bottomRight ); // bottom
	UI_FillRect( m_scPos.x + m_scSize.w - 2, m_scPos.y + 1, 1, m_scSize.h - 2, bottomRight ); // right

	// Text — small font (Tahoma 11px feel) for PC parity.
	unsigned int textColor = focused ? textFocus : textNorm;
	if( iFlags & QMF_GRAYED )
		textColor = Scheme_GetColor( g_Scheme.labelDisabledFg1, 0xFF505050 );

	int textH = (int)(FRAME_TEXT_HEIGHT * uiStatic.scaleY);
	if( textH < 8 ) textH = 8;

	// When pressed, offset text 1px right and 1px down for "pushed" feel
	int textX = m_scPos.x + ( pressed ? 1 : 0 );
	int textY = m_scPos.y + ( pressed ? 1 : 0 );
	UI_DrawString( uiStatic.hSmallFont,
		textX, textY, m_scSize.w, m_scSize.h,
		szName, textColor, textH, QM_CENTER, ETF_FORCECOL );
}
