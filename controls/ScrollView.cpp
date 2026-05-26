#include "extdll_menu.h"
#include "BaseMenu.h"
#include "ScrollView.h"
#include "Scissor.h"
#include "keydefs.h"
#include "TrackerScheme.h"

CMenuScrollView::CMenuScrollView() : CMenuItemsHolder (),
	m_bHoldingMouse1( false ),
	m_bGestureOnContent( false ),
	m_bScrollBarDragging( false ),
	m_iScrollBarDragStartY( 0 ),
	m_iScrollBarDragStartPos( 0 ),
	m_iScrollBarWidth( 0 )
{
}

void CMenuScrollView::VidInit()
{
	colorStroke.SetDefault( uiInputFgColor );

	BaseClass::VidInit();

	m_iMax = 0;
	m_iPos = 0;

	FOR_EACH_VEC( m_pItems, i )
	{
		Point pt = m_pItems[i]->pos;
		Size sz = m_pItems[i]->size;

		m_iMax += pt.y + sz.h;
	}
	m_bDisableScrolling = (m_iMax < size.h);

	m_iMax *= uiStatic.scaleX;

	// Compute scaled scrollbar width
	m_iScrollBarWidth = (int)(SCROLLBAR_WIDTH * uiStatic.scaleX);
}

bool CMenuScrollView::KeyDown( int key )
{
	// act when key is pressed or repeated
	if( !m_bDisableScrolling )
	{
		int newPos = m_iPos;
		if( UI::Key::IsUpArrow( key ))
			newPos -= 20;
		else if( UI::Key::IsDownArrow( key ))
			newPos += 20;
		else if( UI::Key::IsPageUp( key ))
			newPos -= 100;
		else if( UI::Key::IsPageDown( key ))
			newPos += 100;
		else if( key == K_MWHEELUP )
			newPos -= 40;
		else if( key == K_MWHEELDOWN )
			newPos += 40;
		else if( UI::Key::IsLeftMouse( key ))
		{
			// Check if click is on the scrollbar area
			int sbX = m_scPos.x + m_scSize.w - m_iScrollBarWidth;
			if( uiStatic.cursorX >= sbX && uiStatic.cursorX <= m_scPos.x + m_scSize.w &&
			    uiStatic.cursorY >= m_scPos.y && uiStatic.cursorY <= m_scPos.y + m_scSize.h )
			{
				int arrowBtnH = m_iScrollBarWidth; // square arrow buttons
				int trackTop = m_scPos.y + arrowBtnH;
				int trackBottom = m_scPos.y + m_scSize.h - arrowBtnH;
				int trackH = trackBottom - trackTop;

				// Click on top arrow button
				if( uiStatic.cursorY < trackTop )
				{
					newPos -= 20;
				}
				// Click on bottom arrow button
				else if( uiStatic.cursorY >= trackBottom )
				{
					newPos += 20;
				}
				else if( trackH > 0 && m_iMax > m_scSize.h )
				{
					// Compute thumb position/size
					int visibleH = m_scSize.h;
					int thumbH = (int)((float)visibleH / (float)m_iMax * trackH);
					if( thumbH < 16 ) thumbH = 16;

					int scrollRange = m_iMax - visibleH;
					int thumbRange = trackH - thumbH;
					int thumbY = trackTop;
					if( scrollRange > 0 && thumbRange > 0 )
						thumbY = trackTop + (int)((float)m_iPos / (float)scrollRange * thumbRange);

					// Click on thumb - start dragging
					if( uiStatic.cursorY >= thumbY && uiStatic.cursorY <= thumbY + thumbH )
					{
						m_bScrollBarDragging = true;
						m_iScrollBarDragStartY = uiStatic.cursorY;
						m_iScrollBarDragStartPos = m_iPos;
					}
					else
					{
						// Click on track above/below thumb - page scroll
						if( uiStatic.cursorY < thumbY )
							newPos -= visibleH / 2;
						else
							newPos += visibleH / 2;
					}
				}

				// Consume the click so children don't get it
				newPos = bound( 0, newPos, m_iMax - m_scSize.h );
				if( newPos != m_iPos )
				{
					m_iPos = newPos;
					FOR_EACH_VEC( m_pItems, i )
					{
						CMenuBaseItem *pItem = m_pItems[i];
						pItem->VidInit();
					}
					CMenuItemsHolder::MouseMove( uiStatic.cursorX, uiStatic.cursorY );
				}
				return true;
			}
		}

		// TODO: overscrolling
		newPos = bound( 0, newPos, m_iMax - m_scSize.h );

		// recalc
		if( newPos != m_iPos )
		{
			m_iPos = newPos;
			FOR_EACH_VEC( m_pItems, i )
			{
				CMenuBaseItem *pItem = m_pItems[i];

				pItem->VidInit();
			}
			CMenuItemsHolder::MouseMove( uiStatic.cursorX, uiStatic.cursorY );
		}
	}

	return CMenuItemsHolder::KeyDown( key );
}

bool CMenuScrollView::KeyUp( int key )
{
	if( UI::Key::IsLeftMouse( key ) )
	{
		m_bScrollBarDragging = false;
	}
	return CMenuItemsHolder::KeyUp( key );
}

Point CMenuScrollView::GetPositionOffset() const
{
	return Point( 0, -m_iPos ) + BaseClass::GetPositionOffset();
}

bool CMenuScrollView::MouseMove( int x, int y )
{
	// Handle scrollbar thumb dragging
	if( m_bScrollBarDragging )
	{
		int arrowBtnH = m_iScrollBarWidth;
		int trackTop = m_scPos.y + arrowBtnH;
		int trackBottom = m_scPos.y + m_scSize.h - arrowBtnH;
		int trackH = trackBottom - trackTop;

		int visibleH = m_scSize.h;
		int thumbH = (int)((float)visibleH / (float)m_iMax * trackH);
		if( thumbH < 16 ) thumbH = 16;
		int thumbRange = trackH - thumbH;

		if( thumbRange > 0 )
		{
			int scrollRange = m_iMax - visibleH;
			int dy = y - m_iScrollBarDragStartY;
			int newPos = m_iScrollBarDragStartPos + (int)((float)dy / (float)thumbRange * scrollRange);
			newPos = bound( 0, newPos, scrollRange );

			if( newPos != m_iPos )
			{
				m_iPos = newPos;
				FOR_EACH_VEC( m_pItems, i )
				{
					CMenuBaseItem *pItem = m_pItems[i];
					pItem->VidInit();
				}
			}
		}
		return true;
	}

	return CMenuItemsHolder::MouseMove( x, y );
}

bool CMenuScrollView::IsRectVisible(Point pt, Size sz)
{
	bool x = isrange( m_scPos.x, pt.x, m_scPos.x + m_scSize.w ) ||
			 isrange( pt.x, m_scPos.x, pt.x + sz.w );

	bool y = isrange( m_scPos.y, pt.y, m_scPos.y + m_scSize.h ) ||
			 isrange( pt.y, m_scPos.y, pt.y + sz.h );

	return x && y;
}

void CMenuScrollView::DrawScrollBar()
{
	if( m_bDisableScrolling )
		return;

	unsigned int bright = Scheme_GetColor( g_Scheme.borderBright, 0xFFC8C8C8 );
	unsigned int dark   = Scheme_GetColor( g_Scheme.borderDark, 0xFF282828 );
	unsigned int trackFill = Scheme_GetColor( g_Scheme.fieldBgColor, 0xFF3C3C3C );
	unsigned int thumbFill = Scheme_GetColor( g_Scheme.frameBgColor, 0xFF5A5A5A );
	unsigned int arrowGlyph = Scheme_GetColor( g_Scheme.buttonTextColor, 0xFFFFFFFF );

	int sbW = m_iScrollBarWidth;
	int sbX = m_scPos.x + m_scSize.w - sbW;
	int sbY = m_scPos.y;
	int sbH = m_scSize.h;

	int arrowBtnH = sbW; // square buttons

	// --- Draw sunken track background ---
	// Sunken bevel: dark on top+left, bright on bottom+right
	UI_FillRect( sbX, sbY, sbW, sbH, trackFill );
	// Top edge (dark)
	UI_FillRect( sbX, sbY, sbW, 1, dark );
	// Left edge (dark)
	UI_FillRect( sbX, sbY, 1, sbH, dark );
	// Bottom edge (bright)
	UI_FillRect( sbX, sbY + sbH - 1, sbW, 1, bright );
	// Right edge (bright)
	UI_FillRect( sbX + sbW - 1, sbY, 1, sbH, bright );

	// --- Top arrow button (raised bevel) ---
	int btnX = sbX;
	int btnY = sbY;
	// Fill
	UI_FillRect( btnX, btnY, sbW, arrowBtnH, thumbFill );
	// Raised bevel
	UI_FillRect( btnX, btnY, sbW, 1, bright );
	UI_FillRect( btnX, btnY, 1, arrowBtnH, bright );
	UI_FillRect( btnX, btnY + arrowBtnH - 1, sbW, 1, dark );
	UI_FillRect( btnX + sbW - 1, btnY, 1, arrowBtnH, dark );
	// Up triangle glyph (3-4px)
	{
		int cx = btnX + sbW / 2;
		int cy = btnY + arrowBtnH / 2;
		int triH = 4;
		for( int row = 0; row < triH; row++ )
		{
			int lineW = row * 2 + 1;
			int lx = cx - row;
			int ly = cy - triH / 2 + row;
			UI_FillRect( lx, ly, lineW, 1, arrowGlyph );
		}
	}

	// --- Bottom arrow button (raised bevel) ---
	btnY = sbY + sbH - arrowBtnH;
	// Fill
	UI_FillRect( btnX, btnY, sbW, arrowBtnH, thumbFill );
	// Raised bevel
	UI_FillRect( btnX, btnY, sbW, 1, bright );
	UI_FillRect( btnX, btnY, 1, arrowBtnH, bright );
	UI_FillRect( btnX, btnY + arrowBtnH - 1, sbW, 1, dark );
	UI_FillRect( btnX + sbW - 1, btnY, 1, arrowBtnH, dark );
	// Down triangle glyph (3-4px)
	{
		int cx = btnX + sbW / 2;
		int cy = btnY + arrowBtnH / 2;
		int triH = 4;
		for( int row = 0; row < triH; row++ )
		{
			int lineW = (triH - 1 - row) * 2 + 1;
			int lx = cx - (triH - 1 - row);
			int ly = cy - triH / 2 + row;
			UI_FillRect( lx, ly, lineW, 1, arrowGlyph );
		}
	}

	// --- Proportional thumb ---
	int trackTop = sbY + arrowBtnH;
	int trackBottom = sbY + sbH - arrowBtnH;
	int trackH = trackBottom - trackTop;

	if( trackH <= 0 || m_iMax <= m_scSize.h )
		return;

	int visibleH = m_scSize.h;
	int thumbH = (int)((float)visibleH / (float)m_iMax * trackH);
	if( thumbH < 16 ) thumbH = 16;
	if( thumbH > trackH ) thumbH = trackH;

	int scrollRange = m_iMax - visibleH;
	int thumbRange = trackH - thumbH;
	int thumbY = trackTop;
	if( scrollRange > 0 && thumbRange > 0 )
		thumbY = trackTop + (int)((float)m_iPos / (float)scrollRange * thumbRange);

	// Thumb fill
	UI_FillRect( sbX + 1, thumbY, sbW - 2, thumbH, thumbFill );
	// Raised bevel on thumb
	UI_FillRect( sbX + 1, thumbY, sbW - 2, 1, bright );
	UI_FillRect( sbX + 1, thumbY, 1, thumbH, bright );
	UI_FillRect( sbX + 1, thumbY + thumbH - 1, sbW - 2, 1, dark );
	UI_FillRect( sbX + sbW - 2, thumbY, 1, thumbH, dark );
}

void CMenuScrollView::Draw()
{
	// Handle scrollbar drag release (safety net fallback)
	if( m_bScrollBarDragging && !EngFuncs::KEY_IsDown( K_MOUSE1 ) )
	{
		m_bScrollBarDragging = false;
	}

	if( EngFuncs::KEY_IsDown( K_MOUSE1 ) )
	{
		if( !m_bHoldingMouse1 )
		{
			m_bHoldingMouse1 = true;
			m_HoldingPoint = Point( uiStatic.cursorX, uiStatic.cursorY );

			// Record whether gesture started in content or scrollbar
			int sbX = m_scPos.x + m_scSize.w - m_iScrollBarWidth;
			m_bGestureOnContent = ( uiStatic.cursorX < sbX );
		}
	}
	else
	{
		if( m_bHoldingMouse1 ) m_bHoldingMouse1 = false;
	}

	if( m_bHoldingMouse1 && !m_bDisableScrolling && !m_bScrollBarDragging )
	{
		// Only do touch-drag if gesture started in content area
		if( m_bGestureOnContent )
		{
			int newPos = m_iPos;

			newPos -= ( uiStatic.cursorY - m_HoldingPoint.y ) / 2;

			// TODO: overscrolling
			newPos = bound( 0, newPos, m_iMax - m_scSize.h );

			// recalc
			if( newPos != m_iPos )
			{
				m_iPos = newPos;
				FOR_EACH_VEC( m_pItems, i )
				{
					CMenuBaseItem *pItem = m_pItems[i];

					pItem->VidInit();
				}
			}
		}
		m_HoldingPoint = Point( uiStatic.cursorX, uiStatic.cursorY );
	}

	if( bDrawStroke )
	{
		UI_DrawRectangleExt( m_scPos, m_scSize, colorStroke, iStrokeWidth );
	}

	int drawn = 0, skipped = 0;
	FOR_EACH_VEC( m_pItems, i )
	{
		if( !IsRectVisible( m_pItems[i]->GetRenderPosition(), m_pItems[i]->GetRenderSize() ) )
		{
			m_pItems[i]->iFlags |= QMF_HIDDENBYPARENT;
			skipped++;
		}
		else
		{
			m_pItems[i]->iFlags &= ~QMF_HIDDENBYPARENT;
			drawn++;
		}
	}

	// Scissor content area reduced by scrollbar width
	Size scissorSize = m_scSize;
	if( !m_bDisableScrolling )
		scissorSize.w -= m_iScrollBarWidth;

	UI::Scissor::PushScissor( m_scPos, scissorSize );
		CMenuItemsHolder::Draw();
	UI::Scissor::PopScissor();

	// Draw scrollbar after content (on top)
	DrawScrollBar();
}
