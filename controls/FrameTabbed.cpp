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

void CMenuFrameTabbed::DrawTabs()
{
	m_iTabH = (int)(TAB_HEIGHT * uiStatic.scaleY);

	int tabY = m_scPos.y + m_iTitleH;
	int tabW = ( m_iNumTabs > 0 ) ? m_scSize.w / m_iNumTabs : m_scSize.w;
	int lastTabW = ( m_iNumTabs > 1 ) ? m_scSize.w - tabW * (m_iNumTabs - 1) : tabW;

	// GoldSrc VGUI tab colors
	unsigned int activeBg   = Scheme_GetColor( g_Scheme.tabActiveBgColor, 0xFF6E754A );
	unsigned int inactiveBg = Scheme_GetColor( g_Scheme.tabInactiveBgColor, 0xFF58604F );
	unsigned int bright     = Scheme_GetColor( g_Scheme.borderBright, 0xFF767D6A );
	unsigned int dark       = Scheme_GetColor( g_Scheme.borderDark, 0xFF30342B );
	unsigned int tabText    = Scheme_GetColor( g_Scheme.tabTextColor, 0xFF9C9C9C );
	unsigned int tabSelText = Scheme_GetColor( g_Scheme.tabSelectedTextColor, 0xFFD1C469 );

	int hovered = TabAtCursor();

	for( int i = 0; i < m_iNumTabs; i++ )
	{
		int x = m_scPos.x + i * tabW;
		int tw = (i == m_iNumTabs - 1) ? lastTabW : tabW;
		int ty = tabY;
		int th = m_iTabH;
		unsigned int bg, fg;

		if( i == m_iActiveTab )
		{
			bg = activeBg;
			fg = tabSelText;
		}
		else
		{
			bg = inactiveBg;
			fg = ( i == hovered ) ? tabSelText : tabText;
		}

		// Tab background fill
		UI_FillRect( x, ty, tw, th, bg );

		// Border system: top bright, left bright, right dark
		UI_FillRect( x, ty, tw, 1, bright );        // top edge
		UI_FillRect( x, ty, 1, th, bright );           // left edge
		UI_FillRect( x + tw - 1, ty, 1, th, dark );  // right edge

		if( i != m_iActiveTab )
		{
			// Inactive tabs: bottom dark border
			UI_FillRect( x, ty + th - 1, tw, 1, dark );
		}
		// Active tab: NO bottom border (merges with content area below)

		// Tab label text - centered, using hSmallFont (Tahoma)
		UI_DrawString( uiStatic.hSmallFont, x, ty, tw, th,
			m_tabs[i].name, fg, (int)(FRAME_TEXT_HEIGHT * uiStatic.scaleY), QM_CENTER, ETF_FORCECOL );
	}
}

void CMenuFrameTabbed::Draw()
{
	// Compute scaled sizes
	m_iTitleH = (int)(FRAME_TITLE_HEIGHT * uiStatic.scaleY);
	m_iBorderW = (int)(FRAME_BORDER_WIDTH * uiStatic.scaleY);
	m_iTabH = (int)(TAB_HEIGHT * uiStatic.scaleY);
	if( m_iBorderW < 1 ) m_iBorderW = 1;

	ApplyResize();
	ApplyDrag();

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
		if( tab >= 0 && tab != m_iActiveTab )
		{
			m_bDragPending = false; // Clear pending drag set by Frame::KeyDown
			SetActiveTab( tab );
			PlayLocalSound( uiStatic.sounds[SND_LAUNCH] );
			return true;
		}
	}

	return BaseClass::KeyUp( key );
}
