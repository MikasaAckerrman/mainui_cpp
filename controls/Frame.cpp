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
	m_bResizing = false;
	m_iResizeEdge = RESIZE_NONE;
	bAllowDrag = true;
	bAllowResize = true;
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
	pt.y += (int)(FRAME_TITLE_HEIGHT * uiStatic.scaleY);
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

int CMenuFrame::HitTestResize( int x, int y )
{
	if( !bAllowResize )
		return RESIZE_NONE;

	int grip = (int)(FRAME_RESIZE_GRIP * uiStatic.scaleX);
	int gripY = (int)(FRAME_RESIZE_GRIP * uiStatic.scaleY);

	// Check if cursor is inside the window at all
	if( x < m_scPos.x || x > m_scPos.x + m_scSize.w ||
	    y < m_scPos.y || y > m_scPos.y + m_scSize.h )
		return RESIZE_NONE;

	bool nearLeft   = ( x < m_scPos.x + grip );
	bool nearRight  = ( x > m_scPos.x + m_scSize.w - grip );
	bool nearBottom = ( y > m_scPos.y + m_scSize.h - gripY );

	// Only corners and bottom edge trigger resize
	if( nearBottom && nearLeft )  return RESIZE_BOTTOMLEFT;
	if( nearBottom && nearRight ) return RESIZE_BOTTOMRIGHT;
	if( nearBottom )             return RESIZE_BOTTOM;

	return RESIZE_NONE;
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

void CMenuFrame::ApplyResize()
{
	if( !m_bResizing )
		return;

	int dx = uiStatic.cursorX - m_resizeStartCursor.x;
	int dy = uiStatic.cursorY - m_resizeStartCursor.y;

	int minW = (int)(FRAME_MIN_W * uiStatic.scaleX);
	int minH = (int)(FRAME_MIN_H * uiStatic.scaleY);

	int newX = m_resizeStartPos.x;
	int newY = m_resizeStartPos.y;
	int newW = m_resizeStartSize.w;
	int newH = m_resizeStartSize.h;

	// Adjust based on edge being dragged
	// HitTestResize only returns BOTTOM, BOTTOMLEFT, BOTTOMRIGHT
	switch( m_iResizeEdge )
	{
	case RESIZE_BOTTOM:
		newH += dy;
		break;
	case RESIZE_BOTTOMRIGHT:
		newW += dx;
		newH += dy;
		break;
	case RESIZE_BOTTOMLEFT:
		newX += dx;
		newW -= dx;
		newH += dy;
		break;
	}

	// Enforce minimum size
	if( newW < minW )
	{
		if( m_iResizeEdge == RESIZE_BOTTOMLEFT )
			newX = m_resizeStartPos.x + m_resizeStartSize.w - minW;
		newW = minW;
	}
	if( newH < minH )
	{
		newH = minH;
	}

	// Clamp to screen
	if( newX < 0 ) { newW += newX; newX = 0; }
	if( newY < 0 ) { newH += newY; newY = 0; }
	if( newX + newW > ScreenWidth ) newW = ScreenWidth - newX;
	if( newY + newH > ScreenHeight ) newH = ScreenHeight - newY;

	// Re-enforce minimum after clamping
	if( newW < minW ) newW = minW;
	if( newH < minH ) newH = minH;

	m_scPos.x = newX;
	m_scPos.y = newY;
	m_scSize.w = newW;
	m_scSize.h = newH;

	CalcItemsPositions();
	CalcItemsSizes();

	// Sync logical coordinates so base class doesn't overwrite
	pos.x = (int)(m_scPos.x / uiStatic.scaleX);
	pos.y = (int)(m_scPos.y / uiStatic.scaleY);
	size.w = (int)(m_scSize.w / uiStatic.scaleX);
	size.h = (int)(m_scSize.h / uiStatic.scaleY);
}

void CMenuFrame::ApplyDrag()
{
	if( !m_bDragging )
		return;

	m_scPos.x = uiStatic.cursorX - m_dragOffset.x;
	m_scPos.y = uiStatic.cursorY - m_dragOffset.y;

	// Clamp to screen
	if( m_scPos.x < 0 ) m_scPos.x = 0;
	if( m_scPos.y < 0 ) m_scPos.y = 0;
	if( m_scPos.x + m_scSize.w > ScreenWidth ) m_scPos.x = ScreenWidth - m_scSize.w;
	if( m_scPos.y + m_scSize.h > ScreenHeight ) m_scPos.y = ScreenHeight - m_scSize.h;

	CalcItemsPositions();

	// Sync logical coordinates
	pos.x = (int)(m_scPos.x / uiStatic.scaleX);
	pos.y = (int)(m_scPos.y / uiStatic.scaleY);
}

void CMenuFrame::Draw()
{
	// Compute scaled title height and border
	m_iTitleH = (int)(FRAME_TITLE_HEIGHT * uiStatic.scaleY);
	m_iBorderW = (int)(FRAME_BORDER_WIDTH * uiStatic.scaleY);
	if( m_iBorderW < 1 ) m_iBorderW = 1;

	ApplyResize();
	ApplyDrag();

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

		// Check resize edges/corners before anything else
		int edge = HitTestResize( uiStatic.cursorX, uiStatic.cursorY );
		if( edge != RESIZE_NONE )
		{
			m_bResizing = true;
			m_iResizeEdge = edge;
			m_resizeStartCursor = Point( uiStatic.cursorX, uiStatic.cursorY );
			m_resizeStartPos = m_scPos;
			m_resizeStartSize = m_scSize;
			return true;
		}

		// Let child items handle the click first
		if( BaseClass::KeyDown( key ) )
			return true;

		// No child consumed it — start drag from anywhere in window
		if( uiStatic.cursorX >= m_scPos.x && uiStatic.cursorX <= m_scPos.x + m_scSize.w &&
		    uiStatic.cursorY >= m_scPos.y && uiStatic.cursorY <= m_scPos.y + m_scSize.h )
		{
			m_bDragging = true;
			m_dragOffset.x = uiStatic.cursorX - m_scPos.x;
			m_dragOffset.y = uiStatic.cursorY - m_scPos.y;
			return true;
		}

		return false;
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
		if( m_bResizing )
		{
			m_bResizing = false;
			m_iResizeEdge = RESIZE_NONE;
			return true;
		}

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
