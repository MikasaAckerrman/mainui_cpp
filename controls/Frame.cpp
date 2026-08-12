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
	m_bDragPending = false;
	m_bResizePending = false;
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
	// Close button: 26x26 logical px, scaled, positioned at right of title bar
	int btnSize = (int)(26 * uiStatic.scaleY);
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
	unsigned int bgColor = Scheme_GetColor( g_Scheme.frameBgColor, 0xFF4C5844 );

	// A FLAT FILL, and that is the whole of it.
	//
	// This used to also scatter per-pixel "grain" over the panel: up to 4000
	// separate UI_FillRect( x, y, 1, 1 ) calls per window per frame. It was added
	// to imitate a texture in the CS 1.6 panels, and it imitates nothing -- the
	// real thing has no such texture. Checked against the game's own scheme:
	// resource/ClientScheme.res paints panels with a single flat role (ControlBG)
	// and gets all of its depth from the Borders section, which is 21 named
	// borders each built out of 1px lines with an inset and a per-side offset.
	// Grain on top of that reads as noise, and it cost thousands of draw calls a
	// frame on a phone.
	//
	// The depth belongs in DrawBorder (see the note there), not here.
	UI_FillRect( m_scPos.x, m_scPos.y + m_iTitleH, m_scSize.w, m_scSize.h - m_iTitleH, bgColor );
}

void CMenuFrame::DrawTitleBar()
{
	unsigned int bgColor = Scheme_GetColor( g_Scheme.frameBgColor, 0xFF4C5844 );
	unsigned int titleFg = Scheme_GetColor( g_Scheme.frameTitleBarFg, 0xFFFFFFFF );
	unsigned int bright  = Scheme_GetColor( g_Scheme.borderBright, 0xFF889180 );
	unsigned int dark    = Scheme_GetColor( g_Scheme.borderDark, 0xFF282E22 );

	// Title bar uses the SAME color as frame body so it blends seamlessly (CS 1.6 style)
	unsigned int titleTop = Scheme_GetColor( g_Scheme.frameTitleBarTop, 0xFF697259 );
	unsigned int titleBot = Scheme_GetColor( g_Scheme.frameTitleBarBottom, 0xFF4B543B );

	// Top edge - 1px subtle highlight
	UI_FillRect( m_scPos.x, m_scPos.y, m_scSize.w, 1, titleTop );
	// Main body - same as frame background
	UI_FillRect( m_scPos.x, m_scPos.y + 1, m_scSize.w, m_iTitleH - 2, bgColor );
	// Bottom edge - 1px subtle separator
	UI_FillRect( m_scPos.x, m_scPos.y + m_iTitleH - 1, m_scSize.w, 1, titleBot );

	// Title text - Tahoma 11px, padding-left 8px, vertically centered.
	int textH = (int)(FRAME_TEXT_HEIGHT * uiStatic.scaleY);
	if( textH < 8 ) textH = 8;
	int textPadLeft = (int)(8 * uiStatic.scaleX);
	if( m_szTitle && m_szTitle[0] )
	{
		UI_DrawString( uiStatic.hSmallFont,
			m_scPos.x + textPadLeft, m_scPos.y, m_scSize.w - m_iTitleH - textPadLeft, m_iTitleH,
			m_szTitle, titleFg, textH, QM_LEFT, ETF_FORCECOL );
	}

	// Close button [X] - 26x26 logical px with GoldSrc double border
	int btnSize = (int)(26 * uiStatic.scaleY);
	if( btnSize < 12 ) btnSize = 12;
	int btnX = m_scPos.x + m_scSize.w - btnSize - (int)(6 * uiStatic.scaleX);
	int btnY = m_scPos.y + (m_iTitleH - btnSize) / 2;

	bool hovered = IsOnCloseButton( uiStatic.cursorX, uiStatic.cursorY );

	// Button background
	unsigned int btnBg = hovered ? Scheme_GetColor( g_Scheme.buttonArmedBgColor, 0xFF5A5A5A ) : Scheme_GetColor( g_Scheme.buttonBgColor, 0xFF4A4A4A );
	UI_FillRect( btnX, btnY, btnSize, btnSize, btnBg );

	// Outer dark border (1px all around)
	UI_FillRect( btnX, btnY, btnSize, 1, dark );                 // top
	UI_FillRect( btnX, btnY + btnSize - 1, btnSize, 1, dark );   // bottom
	UI_FillRect( btnX, btnY, 1, btnSize, dark );                 // left
	UI_FillRect( btnX + btnSize - 1, btnY, 1, btnSize, dark );   // right

	// Inner raised bevel (bright top+left, dark bottom+right)
	UI_FillRect( btnX + 1, btnY + 1, btnSize - 2, 1, bright );              // top
	UI_FillRect( btnX + 1, btnY + 1, 1, btnSize - 2, bright );              // left
	UI_FillRect( btnX + 1, btnY + btnSize - 2, btnSize - 2, 1, dark );      // bottom
	UI_FillRect( btnX + btnSize - 2, btnY + 1, 1, btnSize - 2, dark );      // right

	// X glyph - 3px wide diagonal strokes for visibility
	unsigned int glyphColor = titleFg;
	int pad = (int)(5 * uiStatic.scaleY);
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
		// 3px wide strokes
		UI_FillRect( px, py1, 1, 1, glyphColor );
		UI_FillRect( px, py1 + 1, 1, 1, glyphColor );
		UI_FillRect( px, py1 + 2, 1, 1, glyphColor );
		UI_FillRect( px, py2, 1, 1, glyphColor );
		UI_FillRect( px, py2 - 1, 1, 1, glyphColor );
		UI_FillRect( px, py2 - 2, 1, 1, glyphColor );
	}
}

void CMenuFrame::DrawBorder()
{
	// FrameBorder, as the game's own scheme defines it.
	//
	// resource/ClientScheme.res, section Borders:
	//
	//     FrameBorder
	//     {
	//         "inset" "0 0 1 1"
	//         Left   { "1" { "color" "ControlBG" "offset" "0 1" } }
	//         Right  { "1" { "color" "ControlBG" "offset" "0 0" } }
	//         Top    { "1" { "color" "ControlBG" "offset" "0 1" } }
	//         Bottom { "1" { "color" "ControlBG" "offset" "0 0" } }
	//     }
	//
	// So a CS 1.6 frame border is FOUR 1px lines in the panel's own background
	// colour, each with its own start offset -- not an outline in a contrasting
	// colour. That is why the real windows look flat-edged and slightly recessed
	// rather than outlined. A dark outline (what this drew before) is what made
	// our frames read as "a rectangle with a border drawn round it".
	//
	// The offsets are what the inset/offset pair means in that format: a side's
	// line starts `offset` pixels in from the corner, which leaves the corner
	// pixel to the perpendicular side and produces the mitred look.
	unsigned int edge = Scheme_GetColor( g_Scheme.frameBorderColor, 0 );

	if( !edge )
		edge = Scheme_GetColor( g_Scheme.frameBgColor, 0xFF4C5844 );

	int x = m_scPos.x;
	int y = m_scPos.y;
	int w = m_scSize.w;
	int h = m_scSize.h;

	// Top: offset "0 1" -- starts one pixel in.
	UI_FillRect( x + 1, y - 1, w, 1, edge );
	// Bottom: offset "0 0".
	UI_FillRect( x, y + h, w, 1, edge );
	// Left: offset "0 1".
	UI_FillRect( x - 1, y + 1, 1, h, edge );
	// Right: offset "0 0".
	UI_FillRect( x + w, y, 1, h, edge );
}

void CMenuFrame::DrawResizeGrip()
{
	if( !bAllowResize )
		return;

	unsigned int bright = Scheme_GetColor( g_Scheme.borderBright, 0xFF889180 );
	unsigned int dark   = Scheme_GetColor( g_Scheme.borderDark,   0xFF282E22 );

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
			m_bResizePending = true;
			m_bUserMoved = true;
			m_iResizeEdge = edge;
			m_actionStartCursor.x = uiStatic.cursorX;
			m_actionStartCursor.y = uiStatic.cursorY;
			return true;
		}

		// Let child items handle the click (button, slider, checkbox, …)
		if( BaseClass::KeyDown( key ) )
			return true;

		// No child claimed it -- start drag from title bar OR tab bar area.
		// The expanded zone (title + tabs) provides a larger touch target on Android.
		// Do NOT allow drag from window body - on Android, stale cursorX/Y in
		// KeyDown causes false positive drag activation on any body touch.
		if( IsInTitleBar( uiStatic.cursorX, uiStatic.cursorY ) )
		{
			m_bDragPending = true;
			m_bUserMoved = true;
			m_actionStartCursor.x = uiStatic.cursorX;
			m_actionStartCursor.y = uiStatic.cursorY;
			return true;
		}

		// Tab bar also allows drag, but ONLY consumed here to prevent click falling
		// through to nothing. The actual drag vs tab-switch decision happens in
		// MouseMove (drag) vs KeyUp (tab switch in FrameTabbed::KeyUp).
		if( IsInTabBar( uiStatic.cursorX, uiStatic.cursorY ) )
		{
			m_bDragPending = true;
			m_bUserMoved = true;
			m_actionStartCursor.x = uiStatic.cursorX;
			m_actionStartCursor.y = uiStatic.cursorY;
			return true;
		}

		// Click landed inside the window but no child claimed it and it's not
		// on the title bar or tab bar. Just consume it (prevents click-through
		// to underlying windows) but do NOT start a drag. On Android, stale
		// cursorX/Y in KeyDown would cause the window to fly to a corner.
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
		if( !m_bDragging && !m_bResizing && !m_bDragPending && !m_bResizePending &&
		    IsOnCloseButton( uiStatic.cursorX, uiStatic.cursorY ) )
		{
			Hide();
			return true;
		}

		if( m_bResizePending )
		{
			m_bResizePending = false;
			m_iResizeEdge = RESIZE_NONE;
			return true;
		}

		if( m_bResizing )
		{
			m_bResizing = false;
			m_iResizeEdge = RESIZE_NONE;
			return true;
		}

		if( m_bDragPending )
		{
			m_bDragPending = false;
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
	// Deferred drag activation: KeyDown set pending=true and captured cursor position.
	// The first MouseMove carries the REAL touch position on Android, so we activate
	// here using its (x,y) as the anchor. No deadzone check needed - the deferred
	// pattern already prevents jumps because the anchor is set from MouseMove's coords.
	if( m_bDragPending )
	{
		m_bDragPending = false;
		m_bDragging = true;
		m_actionStartCursor.x = x;
		m_actionStartCursor.y = y;
		m_actionStartPos = m_scPos;
	}

	if( m_bResizePending )
	{
		m_bResizePending = false;
		m_bResizing = true;
		m_actionStartCursor.x = x;
		m_actionStartCursor.y = y;
		m_actionStartPos = m_scPos;
		m_actionStartSize = m_scSize;
	}

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
