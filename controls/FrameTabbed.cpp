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

	if( uiStatic.cursorY < tabY || uiStatic.cursorY > tabY + tabH )
		return -1;

	int idx = ( uiStatic.cursorX - m_scPos.x ) / tabW;
	if( idx < 0 || idx >= m_iNumTabs )
		return -1;

	return idx;
}

void CMenuFrameTabbed::DrawTabs()
{
	m_iTabH = (int)(TAB_HEIGHT * uiStatic.scaleY);

	int tabY = m_scPos.y + m_iTitleH;
	int tabW = ( m_iNumTabs > 0 ) ? m_scSize.w / m_iNumTabs : m_scSize.w;

	// CS 1.6 PC style: darker tab strip background, bright underline on active
	unsigned int tabStripBg = 0xFF2D2D2D; // dark strip bg
	unsigned int tabSelBg = Scheme_GetColor( g_Scheme.frameBgColor, 0xE6282828 ); // blends with content
	unsigned int tabText = Scheme_GetColor( g_Scheme.tabTextColor, 0xFF9C9080 );
	unsigned int tabSelText = Scheme_GetColor( g_Scheme.tabSelectedTextColor, 0xFFF0ECE0 );
	unsigned int accentColor = Scheme_GetColor( g_Scheme.frameTitleBarBg, 0xFF4A3520 );
	unsigned int borderColor = Scheme_GetColor( g_Scheme.borderDark, 0xC4282828 );

	int hovered = TabAtCursor();

	// Fill tab strip background
	UI_FillRect( m_scPos.x, tabY, m_scSize.w, m_iTabH, tabStripBg );

	for( int i = 0; i < m_iNumTabs; i++ )
	{
		int x = m_scPos.x + i * tabW;
		unsigned int fg;

		if( i == m_iActiveTab )
		{
			// Active tab: blend with content bg, highlight text
			UI_FillRect( x, tabY, tabW, m_iTabH, tabSelBg );
			fg = tabSelText;
			// Accent underline (2px) on active tab
			UI_FillRect( x + 2, tabY + m_iTabH - 2, tabW - 4, 2, accentColor );
		}
		else if( i == hovered )
		{
			fg = tabSelText;
		}
		else
		{
			fg = tabText;
		}

		UI_DrawString( uiStatic.hSmallFont, x, tabY, tabW, m_iTabH,
			m_tabs[i].name, fg, (int)(FRAME_TEXT_HEIGHT * uiStatic.scaleY), QM_CENTER, ETF_FORCECOL );

		// Separator between tabs
		if( i < m_iNumTabs - 1 )
		{
			UI_FillRect( x + tabW - 1, tabY + 4, 1, m_iTabH - 8, borderColor );
		}
	}

	// Thin line below entire tab strip
	UI_FillRect( m_scPos.x, tabY + m_iTabH, m_scSize.w, 1, borderColor );
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
			SetActiveTab( tab );
			PlayLocalSound( uiStatic.sounds[SND_LAUNCH] );
			return true;
		}
	}

	return BaseClass::KeyUp( key );
}
