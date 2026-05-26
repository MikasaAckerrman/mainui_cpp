/*
WndConsole.cpp -- Source Engine-style console window
Copyright (C) 2024 DragonSlayer Team

A draggable framed console window with output area and input field.
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "controls/Frame.h"
#include "Field.h"
#include "Action.h"
#include "keydefs.h"
#include "Utils.h"
#include "TrackerScheme.h"

#define CON_MAX_LINES 128
#define CON_LINE_LEN 256

class CMenuWndConsole : public CMenuFrame
{
public:
	CMenuWndConsole();

	bool IsRoot() const override { return false; }

private:
	void _Init() override;
	void _VidInit() override;
	void Draw() override;
	bool KeyDown( int key ) override;

	void Submit();
	void UpdateOutput();

	CMenuField inputField;

	// Console output buffer
	char m_lines[CON_MAX_LINES][CON_LINE_LEN];
	int m_iNumLines;
	int m_iScrollOffset;
};

static CMenuWndConsole *s_pWndConsole = NULL;

CMenuWndConsole::CMenuWndConsole() : CMenuFrame( "Console" )
{
	m_iNumLines = 0;
	m_iScrollOffset = 0;
}

void CMenuWndConsole::_Init()
{
	// Center the window, 70% of screen
	int w = (int)(uiStatic.width * 0.7f);
	int h = (int)(768 * 0.6f);
	int x = (uiStatic.width - w) / 2;
	int y = (768 - h) / 2;
	SetRect( x, y, w, h );

	// Input field at bottom of content area (content = h - titleH)
	int contentH = h - FRAME_TITLE_HEIGHT;
	inputField.iMaxLength = 256;
	inputField.szName = "";
	inputField.SetRect( 8, contentH - 40, w - 16, 32 );
	AddItem( inputField );
}

void CMenuWndConsole::_VidInit()
{
	// Recalc positions
	int w = (int)(uiStatic.width * 0.7f);
	int h = (int)(768 * 0.6f);
	int x = (uiStatic.width - w) / 2;
	int y = (768 - h) / 2;
	SetRect( x, y, w, h );

	int contentH = h - FRAME_TITLE_HEIGHT;
	inputField.SetRect( 8, contentH - 40, w - 16, 32 );
}

void CMenuWndConsole::UpdateOutput()
{
	// Grab console output via condump approach
	// For now, show a placeholder message
	if( m_iNumLines == 0 )
	{
		Q_strncpy( m_lines[0], "Type commands below. Press Enter to submit.", CON_LINE_LEN );
		m_iNumLines = 1;
	}
}

void CMenuWndConsole::Submit()
{
	const char *cmd = inputField.GetBuffer();
	if( !cmd || !cmd[0] )
		return;

	// Add to output
	if( m_iNumLines < CON_MAX_LINES )
	{
		snprintf( m_lines[m_iNumLines], CON_LINE_LEN, "] %s", cmd );
		m_iNumLines++;
	}

	// Execute (append newline so engine processes it)
	char cmdBuf[512];
	snprintf( cmdBuf, sizeof( cmdBuf ), "%s\n", cmd );
	EngFuncs::ClientCmd( false, cmdBuf );

	// Clear input and reset scroll
	inputField.Clear();
	m_iScrollOffset = 0;
}

void CMenuWndConsole::Draw()
{
	UpdateOutput();

	// Compute scaled sizes (same as CMenuFrame::Draw start)
	m_iTitleH = (int)(FRAME_TITLE_HEIGHT * uiStatic.scaleY);
	m_iBorderW = (int)(FRAME_BORDER_WIDTH * uiStatic.scaleY);
	if( m_iBorderW < 1 ) m_iBorderW = 1;

	// Handle dragging
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

	// Draw frame chrome
	DrawBackground();
	DrawTitleBar();

	// Draw console output text (between bg and child items)
	unsigned int textColor = Scheme_GetColor( g_Scheme.listTextColor, uiColorWhite );
	int lineH = (int)(14 * uiStatic.scaleY);
	int contentY = m_scPos.y + m_iTitleH + 4;
	int contentX = m_scPos.x + 8;
	int contentW = m_scSize.w - 16;
	int inputAreaH = (int)(48 * uiStatic.scaleY);
	int maxVisibleLines = (m_scSize.h - m_iTitleH - inputAreaH) / lineH;
	if( maxVisibleLines < 1 ) maxVisibleLines = 1;

	int startLine = m_iNumLines - maxVisibleLines - m_iScrollOffset;
	if( startLine < 0 ) startLine = 0;

	for( int i = startLine; i < m_iNumLines && (i - startLine) < maxVisibleLines; i++ )
	{
		int y = contentY + (i - startLine) * lineH;
		UI_DrawString( uiStatic.hSmallFont, contentX, y, contentW, lineH,
			m_lines[i], textColor, lineH, QM_LEFT, ETF_FORCECOL );
	}

	DrawBorder();

	// Draw child items (input field)
	CMenuItemsHolder::Draw();
}

bool CMenuWndConsole::KeyDown( int key )
{
	if( UI::Key::IsEnter( key ) )
	{
		Submit();
		return true;
	}

	if( key == K_MWHEELUP )
	{
		m_iScrollOffset++;
		if( m_iScrollOffset > m_iNumLines ) m_iScrollOffset = m_iNumLines;
		return true;
	}
	if( key == K_MWHEELDOWN )
	{
		m_iScrollOffset--;
		if( m_iScrollOffset < 0 ) m_iScrollOffset = 0;
		return true;
	}

	return CMenuFrame::KeyDown( key );
}

// ============= Public API =============

void WndConsole_Precache()
{
	s_pWndConsole = new CMenuWndConsole();
}

void WndConsole_Show()
{
	if( s_pWndConsole )
		s_pWndConsole->Show();
}

void WndConsole_Shutdown()
{
	delete s_pWndConsole;
	s_pWndConsole = NULL;
}

ADD_MENU4( menu_wndconsole, WndConsole_Precache, WndConsole_Show, WndConsole_Shutdown );
