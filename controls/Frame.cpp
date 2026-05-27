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
	m_bUserMoved = false;
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
	// Close button: 30x30 logical px, scaled, positioned at right of title bar
	int btnSize = (int)(30 * uiStatic.scaleY);
	if( btnSize < 12 ) btnSize = 12;
	int btnX = m_scPos.x + m_scSize.w - btnSize - (int)(6 * uiStatic.scaleX);
	int btnY = m_scPos.y + (m_iTitleH - btnSize) / 2;
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
	unsigned int titleBg = Scheme_GetColor( g_Scheme.frameTitleBarBg, 0xFF4B4B4B );
	unsigned int titleFg = Scheme_GetColor( g_Scheme.frameTitleBarFg, 0xFFFFFFFF );
	unsigned int bright  = Scheme_GetColor( g_Scheme.borderBright, 0xFF707864 );
	unsigned int dark    = Scheme_GetColor( g_Scheme.borderDark, 0xFF3A4035 );

	// Title bar background - full width inside border
	UI_FillRect( m_scPos.x, m_scPos.y, m_scSize.w, m_iTitleH, titleBg );

	// 3D title bar border: top edge bright, bottom edge dark
	UI_FillRect( m_scPos.x, m_scPos.y, m_scSize.w, 1, bright );
	UI_FillRect( m_scPos.x, m_scPos.y + m_iTitleH - 1, m_scSize.w, 1, dark );

	// Title text - small font (Tahoma 11px feel), vertically centered.
	int textH = (int)(FRAME_TEXT_HEIGHT * uiStatic.scaleY);
	if( textH < 8 ) textH = 8;
	if( m_szTitle && m_szTitle[0] )
	{
		UI_DrawString( uiStatic.hSmallFont,
			m_scPos.x + 10, m_scPos.y, m_scSize.w - m_iTitleH - 10, m_iTitleH,
			m_szTitle, titleFg, textH, QM_LEFT, ETF_FORCECOL );
	}

	// Close button [X] - 30x30 raised bevel box
	int btnSize = (int)(30 * uiStatic.scaleY);
	if( btnSize < 12 ) btnSize = 12;
	int btnX = m_scPos.x + m_scSize.w - btnSize - (int)(6 * uiStatic.scaleX);
	int btnY = m_scPos.y + (m_iTitleH - btnSize) / 2;

	bool hovered = IsOnCloseButton( uiStatic.cursorX, uiStatic.cursorY );

	// Button background
	unsigned int btnBg = hovered ? 0xFF5A5A5A : 0xFF4A4A4A;
	UI_FillRect( btnX, btnY, btnSize, btnSize, btnBg );

	// Raised bevel border (bright top+left, dark bottom+right)
	UI_FillRect( btnX, btnY, btnSize, 1, bright );              // top
	UI_FillRect( btnX, btnY, 1, btnSize, bright );              // left
	UI_FillRect( btnX, btnY + btnSize - 1, btnSize, 1, dark );  // bottom
	UI_FillRect( btnX + btnSize - 1, btnY, 1, btnSize, dark );  // right

	// X glyph - 2px wide diagonal strokes for visibility
	unsigned int glyphColor = titleFg;
	int pad = (int)(4 * uiStatic.scaleY);
	if( pad < 3 ) pad = 3;
	int x0 = btnX + pad;
	int y0 = btnY + pad;
	int x1 = btnX + btnSize - pad - 1;
	int y1 = btnY + btnSize - pad - 1;
	int span = x1 - x0;
	if( span < 4 ) span = 4;

	for( int i = 0; i <= span; i++ )
	{
		int px = x0 + i;
		int py1 = y0 + (i * (y1 - y0)) / span;
		int py2 = y1 - (i * (y1 - y0)) / span;
		// 2px wide strokes
		UI_FillRect( px, py1, 1, 1, glyphColor );
		UI_FillRect( px, py1 + 1, 1, 1, glyphColor );
		UI_FillRect( px, py2, 1, 1, glyphColor );
		UI_FillRect( px, py2 - 1, 1, 1, glyphColor );
	}
}

void CMenuFrame::DrawBorder()
{
	unsigned int bright = Scheme_GetColor( g_Scheme.borderBright, 0xFF6B745E );
	unsigned int dark   = Scheme_GetColor( g_Scheme.borderDark,   0xFF2F342C );

	int x = m_scPos.x;
	int y = m_scPos.y;
	int w = m_scSize.w;
	int h = m_scSize.h;

	// Layer 1: outer dark outline (1px, all around)
	UI_FillRect( x - 2, y - 2, w + 4, 1, dark );     // top
	UI_FillRect( x - 2, y + h + 1, w + 4, 1, dark );  // bottom
	UI_FillRect( x - 2, y - 2, 1, h + 4, dark );      // left
	UI_FillRect( x + w + 1, y - 2, 1, h + 4, dark );  // right

	// Layer 2: inner raised bevel (1px, bright top+left, dark bottom+right)
	UI_FillRect( x - 1, y - 1, w + 2, 1, bright );    // top (bright)
	UI_FillRect( x - 1, y - 1, 1, h + 2, bright );    // left (bright)
	UI_FillRect( x - 1, y + h, w + 2, 1, dark );      // bottom (dark)
	UI_FillRect( x + w, y - 1, 1, h + 2, dark );      // right (dark)
}

void CMenuFrame::DrawResizeGrip()
{
	if( !bAllowResize )
		return;

	unsigned int bright = Scheme_GetColor( g_Scheme.borderBright, 0xFFC8C8C8 );
	unsigned int dark   = Scheme_GetColor( g_Scheme.borderDark,   0xFF282828 );

	// Draw 3 diagonal groove lines at bottom-right corner.
	// Each line is a pair of bright+dark 1px diagonal strokes for a grooved look.
	// Lines go from bottom-right toward upper-left at offsets (4,4), (8,8), (12,12).
	int baseX = m_scPos.x + m_scSize.w;
	int baseY = m_scPos.y + m_scSize.h;

	// 3 groove lines at different offsets from the corner
	for( int line = 0; line < 3; line++ )
	{
		int offset = 4 + line * 4; // 4, 8, 12 pixels from corner
		int len = offset - 1;      // length of each diagonal stroke

		for( int i = 0; i < len; i++ )
		{
			// Dark stroke (shadow - offset by 1px down-right from bright)
			int dx = baseX - offset + i;
			int dy = baseY - 1 - i;
			UI_FillRect( dx, dy, 1, 1, dark );

			// Bright stroke (highlight - 1px up-left from dark)
			UI_FillRect( dx - 1, dy - 1, 1, 1, bright );
		}
	}
}

// ─── Drag/Resize math ────────────────────────────────────────────────────────

void CMenuFrame::VidInit()
{
	_VidInit();
	if( !m_bUserMoved )
	{
		CalcPosition();
		CalcSizes();
	}
	VidInitItems();
}

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
	DrawResizeGrip();

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
			m_bUserMoved = true;
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

		// No child claimed it — start drag from the title bar only.
		if( IsInTitleBar( uiStatic.cursorX, uiStatic.cursorY ) )
		{
			m_bDragging = true;
			m_bUserMoved = true;
			m_actionStartCursor.x = uiStatic.cursorX;
			m_actionStartCursor.y = uiStatic.cursorY;
			m_actionStartPos = m_scPos;
			m_actionStartSize = m_scSize;
			return true;
		}

		// Click was inside the window body but not on anything interactive.
		// Don't start a drag — just consume the event.
		if( uiStatic.cursorX >= m_scPos.x && uiStatic.cursorX <= m_scPos.x + m_scSize.w &&
		    uiStatic.cursorY >= m_scPos.y && uiStatic.cursorY <= m_scPos.y + m_scSize.h )
		{
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
		// Close button takes priority over drag/resize
		if( !m_bDragging && !m_bResizing && IsOnCloseButton( uiStatic.cursorX, uiStatic.cursorY ) )
		{
			Hide();
			return true;
		}

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
	}

	return BaseClass::KeyUp( key );
}

bool CMenuFrame::MouseMove( int x, int y )
{
	// Drag/resize updates are driven HERE — synchronously with the cursor event.
	// This gives real-time visual feedback as the user drags.
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
