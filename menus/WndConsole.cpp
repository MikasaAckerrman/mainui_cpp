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

	inputField.iMaxLength = 256;
	inputField.szName = "";
	inputField.SetRect( 8, h - 40 - m_iTitleH, w - 16, 32 );
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
	inputField.SetRect( 8, h - 40 - FRAME_TITLE_HEIGHT, w - 16, 32 );
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

	// Execute
	EngFuncs::ClientCmd( false, cmd );
	EngFuncs::ClientCmd( false, "\n" );

	// Clear input
	inputField.Clear();
}

void CMenuWndConsole::Draw()
{
	UpdateOutput();

	// Draw frame (bg, title, border)
	CMenuFrame::Draw();

	// Draw console output text
	unsigned int textColor = Scheme_GetColor( g_Scheme.listTextColor, uiColorWhite );
	int lineH = (int)(14 * uiStatic.scaleY);
	int contentY = m_scPos.y + m_iTitleH + 4;
	int contentX = m_scPos.x + 8;
	int contentW = m_scSize.w - 16;
	int maxVisibleLines = (m_scSize.h - m_iTitleH - 50) / lineH;

	int startLine = m_iNumLines - maxVisibleLines - m_iScrollOffset;
	if( startLine < 0 ) startLine = 0;

	for( int i = startLine; i < m_iNumLines && (i - startLine) < maxVisibleLines; i++ )
	{
		int y = contentY + (i - startLine) * lineH;
		UI_DrawString( uiStatic.hSmallFont, contentX, y, contentW, lineH,
			m_lines[i], textColor, lineH, QM_LEFT, ETF_FORCECOL );
	}
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
