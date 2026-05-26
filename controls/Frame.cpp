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

	// We implement our own drag/resize end-to-end. Tell the base class to keep
	// its m_bHolding system disabled so it doesn't interfere on KeyDown/KeyUp
	// (parent toggles m_bHolding inside DragDrop() when bAllowDrag is true).
	bAllowDrag = false;
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
	// Close button glyph rect (matches DrawTitleBar layout); +2px padding for touch.
	int btnSize = m_iTitleH - 6;
	int btnX = m_scPos.x + m_scSize.w - btnSize - 4;
	int btnY = m_scPos.y + 3;
	int pad = 2;
	return ( x >= btnX - pad && x <= btnX + btnSize + pad &&
	         y >= btnY - pad && y <= btnY + btnSize + pad );
}

int CMenuFrame::HitTestResize( int x, int y )
{
	if( !bAllowResize )
		return RESIZE_NONE;

	int grip = (int)(FRAME_RESIZE_GRIP * uiStatic.scaleX);
	int gripY = (int)(FRAME_RESIZE_GRIP * uiStatic.scaleY);

	// Inside the window?
	if( x < m_scPos.x || x > m_scPos.x + m_scSize.w ||
	    y < m_scPos.y || y > m_scPos.y + m_scSize.h )
		return RESIZE_NONE;

	bool nearLeft   = ( x < m_scPos.x + grip );
	bool nearRight  = ( x > m_scPos.x + m_scSize.w - grip );
	bool nearBottom = ( y > m_scPos.y + m_scSize.h - gripY );

	// Only bottom edge / bottom corners trigger resize (PC reference behavior).
	if( nearBottom && nearLeft )  return RESIZE_BOTTOMLEFT;
	if( nearBottom && nearRight ) return RESIZE_BOTTOMRIGHT;
	if( nearBottom )              return RESIZE_BOTTOM;

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
	unsigned int sepColor = Scheme_GetColor( g_Scheme.borderDark, 0xFF282828 );

	// Title bar background
	UI_FillRect( m_scPos.x, m_scPos.y, m_scSize.w, m_iTitleH, titleBg );

	// 1px separator line at bottom of title bar
	UI_FillRect( m_scPos.x, m_scPos.y + m_iTitleH - 1, m_scSize.w, 1, sepColor );

	// Title text — small font (Tahoma 11px feel), vertically centered.
	int textH = (int)(FRAME_TEXT_HEIGHT * uiStatic.scaleY);
	if( textH < 8 ) textH = 8;
	if( m_szTitle && m_szTitle[0] )
	{
		UI_DrawString( uiStatic.hSmallFont,
			m_scPos.x + 6, m_scPos.y, m_scSize.w - m_iTitleH - 6, m_iTitleH,
			m_szTitle, titleFg, textH, QM_LEFT, ETF_FORCECOL );
	}

	// Close button [X] — sharp 1px diagonal lines (PC CS 1.6 reference)
	int btnSize = m_iTitleH - 6;
	int btnX = m_scPos.x + m_scSize.w - btnSize - 4;
	int btnY = m_scPos.y + 3;

	bool hovered = IsOnCloseButton( uiStatic.cursorX, uiStatic.cursorY );
	unsigned int btnColor = hovered ? 0xFFFF4040 : titleFg;

	int pad = 3;
	int x0 = btnX + pad;
	int y0 = btnY + pad;
	int x1 = btnX + btnSize - pad;
	int y1 = btnY + btnSize - pad;
	int span = x1 - x0;
	if( span < 4 ) span = 4;

	// Two thin diagonals (1px squares stepped along longer axis)
	for( int i = 0; i <= span; i++ )
	{
		int px = x0 + i;
		int py1 = y0 + i;            // top-left → bottom-right
		int py2 = y1 - i;            // bottom-left → top-right
		UI_FillRect( px, py1, 1, 1, btnColor );
		UI_FillRect( px, py2, 1, 1, btnColor );
	}
}

void CMenuFrame::DrawBorder()
{
	unsigned int bright = Scheme_GetColor( g_Scheme.borderBright, 0xFFA0A0A0 );
	unsigned int dark   = Scheme_GetColor( g_Scheme.borderDark,   0xFF282828 );

	// Double bevel: outer ring = dark, inner ring = bright
	int x = m_scPos.x;
	int y = m_scPos.y;
	int w = m_scSize.w;
	int h = m_scSize.h;

	// Outer ring (dark, 1px)
	UI_FillRect( x - 2, y - 2,     w + 4, 1,     dark );  // top
	UI_FillRect( x - 2, y + h + 1, w + 4, 1,     dark );  // bottom
	UI_FillRect( x - 2, y - 2,     1,     h + 4, dark );  // left
	UI_FillRect( x + w + 1, y - 2, 1,     h + 4, dark );  // right

	// Inner ring (bright, 1px)
	UI_FillRect( x - 1, y - 1, w + 2, 1,     bright );    // top
	UI_FillRect( x - 1, y + h, w + 2, 1,     bright );    // bottom
	UI_FillRect( x - 1, y - 1, 1,     h + 2, bright );    // left
	UI_FillRect( x + w, y - 1, 1,     h + 2, bright );    // right
}

// ─── Drag/Resize math ────────────────────────────────────────────────────────

void CMenuFrame::UpdateDrag( int x, int y )
{
	int dx = x - m_actionStartCursor.x;
	int dy = y - m_actionStartCursor.y;

	int newX = m_actionStartPos.x + dx;
	int newY = m_actionStartPos.y + dy;

	// Always keep at least 60px of title bar visible so user can drag back.
	int minVisible = (int)(60 * uiStatic.scaleX);
	if( minVisible < 30 ) minVisible = 30;

	if( newX < -m_scSize.w + minVisible ) newX = -m_scSize.w + minVisible;
	if( newX > ScreenWidth - minVisible )  newX = ScreenWidth - minVisible;
	if( newY < 0 ) newY = 0;
	if( newY > ScreenHeight - m_iTitleH ) newY = ScreenHeight - m_iTitleH;

	m_scPos.x = newX;
	m_scPos.y = newY;

	// Sync logical coords so any later VidInit doesn't snap us back
	pos.x = (int)(m_scPos.x / uiStatic.scaleX);
	pos.y = (int)(m_scPos.y / uiStatic.scaleY);

	CalcItemsPositions();
}

void CMenuFrame::UpdateResize( int x, int y )
{
	int dx = x - m_actionStartCursor.x;
	int dy = y - m_actionStartCursor.y;

	int minW = (int)(FRAME_MIN_W * uiStatic.scaleX);
	int minH = (int)(FRAME_MIN_H * uiStatic.scaleY);

	int newX = m_actionStartPos.x;
	int newY = m_actionStartPos.y;
	int newW = m_actionStartSize.w;
	int newH = m_actionStartSize.h;

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

	// Min size — anchor BOTTOMLEFT to the right edge so window doesn't jump
	if( newW < minW )
	{
		if( m_iResizeEdge == RESIZE_BOTTOMLEFT )
			newX = m_actionStartPos.x + m_actionStartSize.w - minW;
		newW = minW;
	}
	if( newH < minH )
		newH = minH;

	// Screen clamp — done AFTER min-size enforcement.
	// For BOTTOMLEFT, if newX went negative, shift right and shrink the gain.
	if( newX < 0 )
	{
		if( m_iResizeEdge == RESIZE_BOTTOMLEFT )
		{
			newW += newX;        // newX is negative, so this shrinks newW
			if( newW < minW )
				newW = minW;
		}
		newX = 0;
	}
	if( newY < 0 ) newY = 0;
	if( newX + newW > ScreenWidth )  newW = ScreenWidth  - newX;
	if( newY + newH > ScreenHeight ) newH = ScreenHeight - newY;
	if( newW < minW ) newW = minW;
	if( newH < minH ) newH = minH;

	m_scPos.x = newX;
	m_scPos.y = newY;
	m_scSize.w = newW;
	m_scSize.h = newH;

	pos.x = (int)(m_scPos.x / uiStatic.scaleX);
	pos.y = (int)(m_scPos.y / uiStatic.scaleY);
	size.w = (int)(m_scSize.w / uiStatic.scaleX);
	size.h = (int)(m_scSize.h / uiStatic.scaleY);

	CalcItemsPositions();
	CalcItemsSizes();
}

void CMenuFrame::ApplyResize()
{
	// Idempotent fallback for subclasses that override Draw.
	if( m_bResizing )
		UpdateResize( uiStatic.cursorX, uiStatic.cursorY );
}

void CMenuFrame::ApplyDrag()
{
	if( m_bDragging )
		UpdateDrag( uiStatic.cursorX, uiStatic.cursorY );
}

// ─── Render ──────────────────────────────────────────────────────────────────

void CMenuFrame::Draw()
{
	m_iTitleH = (int)(FRAME_TITLE_HEIGHT * uiStatic.scaleY);
	m_iBorderW = (int)(FRAME_BORDER_WIDTH * uiStatic.scaleY);
	if( m_iBorderW < 1 ) m_iBorderW = 1;

	// Safety net: if MouseMove dispatch was missed for any reason, reapply now.
	ApplyResize();
	ApplyDrag();

	DrawBackground();
	DrawTitleBar();
	DrawBorder();

	CMenuItemsHolder::Draw();
}

// ─── Input ───────────────────────────────────────────────────────────────────

bool CMenuFrame::KeyDown( int key )
{
	if( UI::Key::IsLeftMouse( key ) )
	{
		// Close button — consume here, action fires on KeyUp.
		if( IsOnCloseButton( uiStatic.cursorX, uiStatic.cursorY ) )
			return true;

		// Resize edges/corners take priority over child clicks.
		int edge = HitTestResize( uiStatic.cursorX, uiStatic.cursorY );
		if( edge != RESIZE_NONE )
		{
			m_bResizing = true;
			m_iResizeEdge = edge;
			m_actionStartCursor.x = uiStatic.cursorX;
			m_actionStartCursor.y = uiStatic.cursorY;
			m_actionStartPos = m_scPos;
			m_actionStartSize = m_scSize;
			return true;
		}

		// Let child items handle the click (button, slider, checkbox, …)
		if( BaseClass::KeyDown( key ) )
			return true;

		// No child claimed it — start drag from anywhere inside the window.
		if( uiStatic.cursorX >= m_scPos.x && uiStatic.cursorX <= m_scPos.x + m_scSize.w &&
		    uiStatic.cursorY >= m_scPos.y && uiStatic.cursorY <= m_scPos.y + m_scSize.h )
		{
			m_bDragging = true;
			m_actionStartCursor.x = uiStatic.cursorX;
			m_actionStartCursor.y = uiStatic.cursorY;
			m_actionStartPos = m_scPos;
			m_actionStartSize = m_scSize;
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
	// Drag/resize updates are driven HERE — synchronously with the cursor event,
	// not deferred to Draw(). This is the key fix for touch-screen reliability.
	if( m_bResizing )
	{
		UpdateResize( x, y );
		return true;
	}

	if( m_bDragging )
	{
		UpdateDrag( x, y );
		return true;
	}

	return BaseClass::MouseMove( x, y );
}
