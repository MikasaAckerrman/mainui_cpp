/*
FrameTabbed.cpp -- Source Engine-style framed window with tabs
Copyright (C) 2024 DragonSlayer Team
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "FrameTabbed.h"
#include "Utils.h"
#include "keydefs.h"

#define TAB_HEIGHT FRAME_TAB_HEIGHT // use the public constant from header

CMenuFrameTabbed::CMenuFrameTabbed( const char *title ) : BaseClass( title )
{
	m_iNumTabs = 0;
	m_iActiveTab = 0;
	m_iTabH = TAB_HEIGHT;
	memset( m_tabs, 0, sizeof( m_tabs ) );
}

Point CMenuFrameTabbed::GetPositionOffset() const
{
	Point pt = m_scPos;
	pt.y += (int)(FRAME_TITLE_HEIGHT * uiStatic.scaleY) + (int)(TAB_HEIGHT * uiStatic.scaleY);
	return pt;
}

void CMenuFrameTabbed::AddTab( const char *name )
{
	if( m_iNumTabs >= MAX_FRAME_TABS )
		return;

	Tab &t = m_tabs[m_iNumTabs];
	t.name = name;
	t.firstItem = m_pItems.Count(); // next item added will be first for this tab
	t.lastItem = -1;
	m_iNumTabs++;
}

void CMenuFrameTabbed::AddItem( CMenuBaseItem &item )
{
	BaseClass::AddItem( item );

	// Associate with current tab
	if( m_iNumTabs > 0 )
	{
		m_tabs[m_iNumTabs - 1].lastItem = m_pItems.Count() - 1;
	}
}

void CMenuFrameTabbed::SetActiveTab( int idx )
{
	if( idx < 0 || idx >= m_iNumTabs )
		return;

	m_iActiveTab = idx;

	// Show/hide items based on tab
	for( int t = 0; t < m_iNumTabs; t++ )
	{
		bool visible = ( t == m_iActiveTab );
		for( int i = m_tabs[t].firstItem; i <= m_tabs[t].lastItem && i < m_pItems.Count(); i++ )
		{
			if( visible )
				m_pItems[i]->iFlags &= ~QMF_HIDDEN;
			else
				m_pItems[i]->iFlags |= QMF_HIDDEN;
		}
	}
}

int CMenuFrameTabbed::TabAtCursor()
{
	if( m_iNumTabs == 0 )
		return -1;

	int titleH = (int)(FRAME_TITLE_HEIGHT * uiStatic.scaleY);
	int tabH = (int)(TAB_HEIGHT * uiStatic.scaleY);
	int tabY = m_scPos.y + titleH;
	int tabW = m_scSize.w / m_iNumTabs;
	if( tabW <= 0 ) return -1;

	if( uiStatic.cursorY < tabY || uiStatic.cursorY > tabY + tabH )
		return -1;

	int relX = uiStatic.cursorX - m_scPos.x;
	if( relX < 0 || relX >= m_scSize.w )
		return -1;

	// Last tab extends to the right edge to absorb remainder pixels
	int idx = relX / tabW;
	if( idx >= m_iNumTabs )
		idx = m_iNumTabs - 1;

	return idx;
}

bool CMenuFrameTabbed::IsInTabBar( int x, int y )
{
	if( m_iNumTabs == 0 )
		return false;

	int titleH = (int)(FRAME_TITLE_HEIGHT * uiStatic.scaleY);
	int tabH = (int)(TAB_HEIGHT * uiStatic.scaleY);
	int tabY = m_scPos.y + titleH;

	return ( x >= m_scPos.x && x <= m_scPos.x + m_scSize.w &&
	         y >= tabY && y <= tabY + tabH );
}

// Draw a single pixel at (px, py) only if inside the tab strip clip region.
static inline void TabPixel( int px, int py, unsigned int color )
{
	UI_FillRect( px, py, 1, 1, color );
}

void CMenuFrameTabbed::DrawTabs()
{
	m_iTabH = (int)(TAB_HEIGHT * uiStatic.scaleY);

	int tabY = m_scPos.y + m_iTitleH;
	int tabW = ( m_iNumTabs > 0 ) ? m_scSize.w / m_iNumTabs : m_scSize.w;
	int lastTabW = ( m_iNumTabs > 1 ) ? m_scSize.w - tabW * (m_iNumTabs - 1) : tabW;

	// GoldSrc VGUI tab colors
	unsigned int frameBg    = Scheme_GetColor( g_Scheme.frameBgColor, 0xFF4C5844 );
	unsigned int inactiveBg = Scheme_GetColor( g_Scheme.tabInactiveBgColor, 0xE64E5643 );
	unsigned int bright     = Scheme_GetColor( g_Scheme.borderBright, 0xFF889180 );
	unsigned int dark       = Scheme_GetColor( g_Scheme.borderDark, 0xFF282E22 );
	unsigned int tabText    = Scheme_GetColor( g_Scheme.tabTextColor, 0xFFDCDCDC );
	unsigned int tabSelText = Scheme_GetColor( g_Scheme.tabSelectedTextColor, 0xFFBFB85E );

	// Color that exists "outside" the tab corners -- the dark frame border
	unsigned int outerBg = Scheme_GetColor( g_Scheme.frameBorderColor, dark );

	// Corner radius in screen pixels (2 logical pixels scaled, min 1)
	int cr = (int)(2.0f * uiStatic.scaleX + 0.5f);
	if( cr < 1 ) cr = 1;

	int hovered = TabAtCursor();

	for( int i = 0; i < m_iNumTabs; i++ )
	{
		int x  = m_scPos.x + i * tabW;
		int tw = (i == m_iNumTabs - 1) ? lastTabW : tabW;
		int ty = tabY;
		int th = m_iTabH;
		unsigned int bg, fg;

		if( i == m_iActiveTab )
		{
			bg = frameBg;
			fg = tabSelText;
		}
		else
		{
			bg = inactiveBg;
			fg = ( i == hovered ) ? tabSelText : tabText;
		}

		// 1. Full tab background fill
		UI_FillRect( x, ty, tw, th, bg );

		// 2. Cut top-left and top-right corners with outer background
		//    This creates the rounded-corner illusion without extra rendering overhead.
		UI_FillRect( x,            ty, cr, cr, outerBg ); // top-left cut
		UI_FillRect( x + tw - cr,  ty, cr, cr, outerBg ); // top-right cut

		// 3. Diagonal bevel pixels at the cut corners (1 bright/dark pixel on each diagonal)
		//    Top-left: bright diagonal
		TabPixel( x + cr - 1, ty,      bright ); // horizontal entry of top border
		TabPixel( x,      ty + cr - 1, bright ); // vertical entry of left border
		//    Top-right: dark diagonal
		TabPixel( x + tw - cr, ty,     dark   ); // horizontal entry of top border
		TabPixel( x + tw - 1, ty + cr - 1, dark ); // vertical entry of right border

		// 4. Borders (inset from corners)
		//    Top bright (between corner radii)
		UI_FillRect( x + cr, ty, tw - 2*cr, 1, bright );
		//    Left bright (below corner radius)
		UI_FillRect( x, ty + cr, 1, th - cr, bright );
		//    Right dark (below corner radius)
		UI_FillRect( x + tw - 1, ty + cr, 1, th - cr, dark );

		if( i != m_iActiveTab )
		{
			// Inactive tabs: bottom dark border
			UI_FillRect( x, ty + th - 1, tw, 1, dark );
		}
		// Active tab: NO bottom border (merges seamlessly with content area)

		// 5. Tab label text - vertically centered, horizontally centered
		UI_DrawString( uiStatic.hSmallFont, x, ty, tw, th,
			m_tabs[i].name, fg, (int)(FRAME_TEXT_HEIGHT * uiStatic.scaleY), QM_CENTER, ETF_FORCECOL );
	}

	// 1px separator line below entire tab strip
	unsigned int sepColor = Scheme_GetColor( g_Scheme.borderBright, 0xC8757B69 );
	UI_FillRect( m_scPos.x, tabY + m_iTabH, m_scSize.w, 1, sepColor );
}

void CMenuFrameTabbed::Draw()
{
	// Compute scaled sizes
	m_iTitleH = (int)(FRAME_TITLE_HEIGHT * uiStatic.scaleY);
	m_iBorderW = (int)(FRAME_BORDER_WIDTH * uiStatic.scaleY);
	m_iTabH = (int)(TAB_HEIGHT * uiStatic.scaleY);
	if( m_iBorderW < 1 ) m_iBorderW = 1;

	DrawBackground();
	DrawTitleBar();
	DrawTabs();
	DrawBorder();

	// Draw child items (only active tab's items are visible)
	CMenuItemsHolder::Draw();
}

bool CMenuFrameTabbed::KeyUp( int key )
{
	if( UI::Key::IsLeftMouse( key ) )
	{
		int tab = TabAtCursor();
		if( tab >= 0 )
		{
			if( tab != m_iActiveTab )
			{
				m_bDragPending = false;
				SetActiveTab( tab );
				PlayLocalSound( uiStatic.sounds[SND_LAUNCH] );
				return true;
			}
			else
			{
				// Tapped the already-active tab without dragging - just cancel pending
				if( m_bDragPending )
				{
					m_bDragPending = false;
					return true;
				}
			}
		}
	}

	return BaseClass::KeyUp( key );
}
