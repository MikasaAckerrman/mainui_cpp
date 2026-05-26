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

	// Colors — CS 1.6 style grey button, customizable via TrackerScheme
	unsigned int schemeBg = g_Scheme.buttonBgColor;
	unsigned int bgNormal  = schemeBg ? schemeBg : 0xFF4A4A4A;
	unsigned int bgHover   = g_Scheme.buttonArmedBgColor ? g_Scheme.buttonArmedBgColor : 0xFF585858;
	unsigned int bgPressed = 0xFF353535;
	unsigned int bright    = Scheme_GetColor( g_Scheme.borderBright, 0xFFC8C8C8 );
	unsigned int dark      = Scheme_GetColor( g_Scheme.borderDark, 0xFF282828 );
	unsigned int textNorm  = Scheme_GetColor( g_Scheme.buttonTextColor, 0xFFFFFFFF );
	unsigned int textFocus = Scheme_GetColor( g_Scheme.buttonArmedTextColor, 0xFFFFFFFF );

	// Background fill
	unsigned int bg = bgNormal;
	if( pressed )
		bg = bgPressed;
	else if( focused )
		bg = bgHover;

	UI_FillRect( m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h, bg );

	// Bevel border (swap bright/dark when pressed for "pushed" effect)
	unsigned int topLeft = pressed ? dark : bright;
	unsigned int bottomRight = pressed ? bright : dark;

	// Top edge
	UI_FillRect( m_scPos.x, m_scPos.y, m_scSize.w, 1, topLeft );
	// Left edge
	UI_FillRect( m_scPos.x, m_scPos.y, 1, m_scSize.h, topLeft );
	// Bottom edge
	UI_FillRect( m_scPos.x, m_scPos.y + m_scSize.h - 1, m_scSize.w, 1, bottomRight );
	// Right edge
	UI_FillRect( m_scPos.x + m_scSize.w - 1, m_scPos.y, 1, m_scSize.h, bottomRight );

	// Text — small font (Tahoma 11px feel) for PC parity.
	unsigned int textColor = focused ? textFocus : textNorm;
	if( iFlags & QMF_GRAYED )
		textColor = Scheme_GetColor( g_Scheme.labelDisabledFg1, 0xFF505050 );

	int textH = (int)(FRAME_TEXT_HEIGHT * uiStatic.scaleY);
	if( textH < 8 ) textH = 8;
	UI_DrawString( uiStatic.hSmallFont,
		m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h,
		szName, textColor, textH, QM_CENTER, ETF_FORCECOL );
}
