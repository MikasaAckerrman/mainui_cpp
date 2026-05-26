/*
FrameTabbed.cpp -- Source Engine-style framed window with tabs
Copyright (C) 2024 DragonSlayer Team
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "FrameTabbed.h"
#include "Utils.h"
#include "keydefs.h"

#define TAB_HEIGHT 22 // logical pixels

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

	unsigned int tabBg = Scheme_GetColor( g_Scheme.frameBgColor, uiPromptBgColor );
	unsigned int tabSelBg = Scheme_GetColor( g_Scheme.listSelectedBgColor, 0xFF4A3520 );
	unsigned int tabText = Scheme_GetColor( g_Scheme.tabTextColor, 0xFF9C9080 );
	unsigned int tabSelText = Scheme_GetColor( g_Scheme.tabSelectedTextColor, 0xFFF0ECE0 );
	unsigned int borderColor = Scheme_GetColor( g_Scheme.borderDark, 0xC4282828 );

	int hovered = TabAtCursor();

	for( int i = 0; i < m_iNumTabs; i++ )
	{
		int x = m_scPos.x + i * tabW;
		unsigned int bg, fg;

		if( i == m_iActiveTab )
		{
			bg = tabSelBg;
			fg = tabSelText;
		}
		else if( i == hovered )
		{
			bg = tabBg;
			fg = tabSelText;
		}
		else
		{
			bg = tabBg;
			fg = tabText;
		}

		UI_FillRect( x, tabY, tabW, m_iTabH, bg );
		UI_DrawString( uiStatic.hDefaultFont, x, tabY, tabW, m_iTabH,
			m_tabs[i].name, fg, m_iTabH - 4, QM_CENTER, ETF_FORCECOL );

		// Separator between tabs
		if( i < m_iNumTabs - 1 )
		{
			UI_FillRect( x + tabW - 1, tabY + 2, 1, m_iTabH - 4, borderColor );
		}
	}

	// Line below tabs
	UI_FillRect( m_scPos.x, tabY + m_iTabH, m_scSize.w, 1, borderColor );
}

void CMenuFrameTabbed::Draw()
{
	// Compute scaled sizes
	m_iTitleH = (int)(FRAME_TITLE_HEIGHT * uiStatic.scaleY);
	m_iBorderW = (int)(FRAME_BORDER_WIDTH * uiStatic.scaleY);
	m_iTabH = (int)(TAB_HEIGHT * uiStatic.scaleY);
	if( m_iBorderW < 1 ) m_iBorderW = 1;

	// Handle dragging (from base class)
	if( m_bDragging )
	{
		m_scPos.x = uiStatic.cursorX - m_dragOffset.x;
		m_scPos.y = uiStatic.cursorY - m_dragOffset.y;

		if( m_scPos.x < 0 ) m_scPos.x = 0;
		if( m_scPos.y < 0 ) m_scPos.y = 0;
		if( m_scPos.x + m_scSize.w > ScreenWidth ) m_scPos.x = ScreenWidth - m_scSize.w;
		if( m_scPos.y + m_scSize.h > ScreenHeight ) m_scPos.y = ScreenHeight - m_scSize.h;

		CalcItemsPositions();
	}

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
