/*
Frame.cpp -- Source Engine-style framed window
Copyright (C) 2024 DragonSlayer Team
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "Frame.h"
#include "Utils.h"
#include "keydefs.h"

CMenuFrame::CMenuFrame( const char *title ) : BaseClass( title )
{
	m_szTitle = title;
	m_bDragging = false;
	bAllowDrag = true;
	m_iTitleH = FRAME_TITLE_HEIGHT;
	m_iBorderW = FRAME_BORDER_WIDTH;
}

void CMenuFrame::SetRect( int x, int y, int w, int h )
{
	pos.x = x;
	pos.y = y;
	size.w = w;
	size.h = h;
}

Point CMenuFrame::GetPositionOffset() const
{
	// Content starts below title bar
	Point pt = m_scPos;
	pt.y += m_iTitleH;
	return pt;
}

bool CMenuFrame::IsInTitleBar( int x, int y )
{
	return ( x >= m_scPos.x && x <= m_scPos.x + m_scSize.w &&
	         y >= m_scPos.y && y <= m_scPos.y + m_iTitleH );
}

bool CMenuFrame::IsOnCloseButton( int x, int y )
{
	// Close button is in top-right corner of title bar
	int btnSize = m_iTitleH - 4;
	int btnX = m_scPos.x + m_scSize.w - btnSize - 4;
	int btnY = m_scPos.y + 2;
	return ( x >= btnX && x <= btnX + btnSize &&
	         y >= btnY && y <= btnY + btnSize );
}

void CMenuFrame::DrawBackground()
{
	unsigned int bgColor = Scheme_GetColor( g_Scheme.frameBgColor, uiPromptBgColor );
	UI_FillRect( m_scPos.x, m_scPos.y + m_iTitleH, m_scSize.w, m_scSize.h - m_iTitleH, bgColor );
}

void CMenuFrame::DrawTitleBar()
{
	unsigned int titleBg = Scheme_GetColor( g_Scheme.frameTitleBarBg, 0xFF4A3520 );
	unsigned int titleFg = Scheme_GetColor( g_Scheme.frameTitleBarFg, 0xFFF0ECE0 );

	// Title bar background
	UI_FillRect( m_scPos.x, m_scPos.y, m_scSize.w, m_iTitleH, titleBg );

	// Title text
	if( m_szTitle && m_szTitle[0] )
	{
		UI_DrawString( uiStatic.hDefaultFont,
			m_scPos.x + 8, m_scPos.y, m_scSize.w - m_iTitleH - 8, m_iTitleH,
			m_szTitle, titleFg, m_iTitleH - 4, QM_LEFT, ETF_FORCECOL );
	}

	// Close button [X]
	int btnSize = m_iTitleH - 6;
	int btnX = m_scPos.x + m_scSize.w - btnSize - 4;
	int btnY = m_scPos.y + 3;

	unsigned int btnColor = titleFg;
	if( IsOnCloseButton( uiStatic.cursorX, uiStatic.cursorY ) )
		btnColor = 0xFFFF4040; // red on hover

	// Draw X as two lines using filled rects (diagonal approximation)
	UI_DrawString( uiStatic.hDefaultFont,
		btnX, btnY, btnSize, btnSize,
		"X", btnColor, btnSize, QM_CENTER, ETF_FORCECOL );
}

void CMenuFrame::DrawBorder()
{
	unsigned int bright = Scheme_GetColor( g_Scheme.borderBright, 0xC8808080 );
	unsigned int dark = Scheme_GetColor( g_Scheme.borderDark, 0xC4282828 );

	// Top border (bright)
	UI_FillRect( m_scPos.x, m_scPos.y - m_iBorderW, m_scSize.w, m_iBorderW, bright );
	// Left border (bright)
	UI_FillRect( m_scPos.x - m_iBorderW, m_scPos.y - m_iBorderW, m_iBorderW, m_scSize.h + m_iBorderW * 2, bright );
	// Bottom border (dark)
	UI_FillRect( m_scPos.x, m_scPos.y + m_scSize.h, m_scSize.w, m_iBorderW, dark );
	// Right border (dark)
	UI_FillRect( m_scPos.x + m_scSize.w, m_scPos.y - m_iBorderW, m_iBorderW, m_scSize.h + m_iBorderW * 2, dark );
}

void CMenuFrame::Draw()
{
	// Compute scaled title height and border
	m_iTitleH = (int)(FRAME_TITLE_HEIGHT * uiStatic.scaleY);
	m_iBorderW = (int)(FRAME_BORDER_WIDTH * uiStatic.scaleY);
	if( m_iBorderW < 1 ) m_iBorderW = 1;

	// Handle dragging
	if( m_bDragging )
	{
		m_scPos.x = uiStatic.cursorX - m_dragOffset.x;
		m_scPos.y = uiStatic.cursorY - m_dragOffset.y;

		// Clamp to screen
		if( m_scPos.x < 0 ) m_scPos.x = 0;
		if( m_scPos.y < 0 ) m_scPos.y = 0;
		if( m_scPos.x + m_scSize.w > ScreenWidth ) m_scPos.x = ScreenWidth - m_scSize.w;
		if( m_scPos.y + m_scSize.h > ScreenHeight ) m_scPos.y = ScreenHeight - m_scSize.h;

		CalcItemsPositions();
	}

	DrawBackground();
	DrawTitleBar();
	DrawBorder();

	// Draw child items
	CMenuItemsHolder::Draw();
}

bool CMenuFrame::KeyDown( int key )
{
	if( UI::Key::IsLeftMouse( key ) )
	{
		if( IsOnCloseButton( uiStatic.cursorX, uiStatic.cursorY ) )
		{
			return true; // consume, handle on KeyUp
		}

		if( IsInTitleBar( uiStatic.cursorX, uiStatic.cursorY ) )
		{
			m_bDragging = true;
			m_dragOffset.x = uiStatic.cursorX - m_scPos.x;
			m_dragOffset.y = uiStatic.cursorY - m_scPos.y;
			return true;
		}
	}

	if( UI::Key::IsEscape( key ) )
	{
		Hide();
		return true;
	}

	return BaseClass::KeyDown( key );
}

bool CMenuFrame::KeyUp( int key )
{
	if( UI::Key::IsLeftMouse( key ) )
	{
		if( m_bDragging )
		{
			m_bDragging = false;
			return true;
		}

		if( IsOnCloseButton( uiStatic.cursorX, uiStatic.cursorY ) )
		{
			Hide();
			return true;
		}
	}

	return BaseClass::KeyUp( key );
}

bool CMenuFrame::MouseMove( int x, int y )
{
	// Update positions during drag
	if( m_bDragging )
		return true;

	return BaseClass::MouseMove( x, y );
}
