/*
WndConsole.cpp -- Source Engine-style console window
Copyright (C) 2024 DragonSlayer Team

A draggable, resizable console window that reads engine.log for output,
supports command input with soft keyboard, command history, scrolling,
and dynamic content layout that adjusts on resize.
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "controls/Frame.h"
#include "Field.h"
#include "Action.h"
#include "keydefs.h"
#include "Utils.h"
#include "TrackerScheme.h"

#define CON_MAX_LINES    512
#define CON_LINE_LEN     512
#define CON_HISTORY_SIZE 32
#define CON_READ_INTERVAL 300  // milliseconds between file re-reads
#define CON_INPUT_HEIGHT  20   // logical pixels
#define CON_PADDING       6    // logical pixels

// Colors
#define CON_TEXT_COLOR    0xFF40FF40  // green for regular output
#define CON_CMD_COLOR     0xFFFFFF00  // yellow for command echo
#define CON_SEPARATOR_COLOR 0xFF606060

class CMenuWndConsole : public CMenuFrame
{
public:
	CMenuWndConsole();

	bool IsRoot() const override { return false; }
	void Show() override;

private:
	void _Init() override;
	void _VidInit() override;
	void Draw() override;
	bool KeyDown( int key ) override;

	void Submit();
	void ReadLog();
	void HistoryUp();
	void HistoryDown();

	CMenuField inputField;

	// Console output ring buffer
	char m_lines[CON_MAX_LINES][CON_LINE_LEN];
	unsigned int m_lineColors[CON_MAX_LINES];
	int m_iLineHead;    // next write position in ring buffer
	int m_iLineCount;   // total lines stored (up to CON_MAX_LINES)

	// Scroll state
	int m_iScrollOffset; // 0 = bottom (most recent), positive = scrolled up

	// File reading state
	int m_iLastReadTime;   // uiStatic.realTime of last read
	int m_iLastFileSize;   // last known file size

	// Command history
	char m_history[CON_HISTORY_SIZE][256];
	int m_iHistoryHead;    // next write position
	int m_iHistoryCount;   // total stored
	int m_iHistoryIndex;   // current browse position (-1 = not browsing)
	char m_szSavedInput[256]; // input saved when starting history browse
};

static CMenuWndConsole *s_pWndConsole = NULL;

CMenuWndConsole::CMenuWndConsole() : CMenuFrame( "Console" )
{
	m_iLineHead = 0;
	m_iLineCount = 0;
	m_iScrollOffset = 0;
	m_iLastReadTime = 0;
	m_iLastFileSize = 0;
	m_iHistoryHead = 0;
	m_iHistoryCount = 0;
	m_iHistoryIndex = -1;
	m_szSavedInput[0] = '\0';
}

void CMenuWndConsole::Show()
{
	CMenuFrame::Show();
	SetCursorToItem( inputField );
}

void CMenuWndConsole::_Init()
{
	// Center the window, 70% of screen width, 60% of height
	int w = (int)(uiStatic.width * 0.7f);
	int h = (int)(768 * 0.6f);
	int x = (uiStatic.width - w) / 2;
	int y = (768 - h) / 2;
	SetRect( x, y, w, h );

	// Input field - disable scaling so we can position it manually in Draw()
	inputField.iFlags |= QMF_DISABLESCAILING;
	inputField.iMaxLength = 255;
	inputField.szName = "";
	inputField.SetRect( 0, 0, 100, CON_INPUT_HEIGHT ); // placeholder, updated in Draw()
	AddItem( inputField );
}

void CMenuWndConsole::_VidInit()
{
	// Recompute window position for current resolution
	int w = (int)(uiStatic.width * 0.7f);
	int h = (int)(768 * 0.6f);
	int x = (uiStatic.width - w) / 2;
	int y = (768 - h) / 2;
	SetRect( x, y, w, h );
}

void CMenuWndConsole::ReadLog()
{
	// Throttle reads
	if( uiStatic.realTime - m_iLastReadTime < CON_READ_INTERVAL )
		return;

	m_iLastReadTime = uiStatic.realTime;

	int length = 0;
	byte *pFile = EngFuncs::COM_LoadFile( "engine.log", &length );
	if( !pFile || length <= 0 )
	{
		if( pFile )
			EngFuncs::COM_FreeFile( pFile );
		return;
	}

	// If file size has not changed, skip processing
	if( length == m_iLastFileSize )
	{
		EngFuncs::COM_FreeFile( pFile );
		return;
	}

	const char *text = (const char *)pFile;
	bool truncated = (length < m_iLastFileSize);

	if( truncated )
	{
		// File was truncated/rotated - reset and parse the whole file
		m_iLineHead = 0;
		m_iLineCount = 0;

		// Parse entire file forward; ring buffer wraps naturally to retain most recent lines
		int pos = 0;
		while( pos < length )
		{
			int lineStart = pos;
			// Find end of line
			while( pos < length && text[pos] != '\n' && text[pos] != '\r' )
				pos++;

			int len = pos - lineStart;
			if( len >= CON_LINE_LEN )
				len = CON_LINE_LEN - 1;

			memcpy( m_lines[m_iLineHead], text + lineStart, len );
			m_lines[m_iLineHead][len] = '\0';
			m_lineColors[m_iLineHead] = CON_TEXT_COLOR;

			m_iLineHead = (m_iLineHead + 1) % CON_MAX_LINES;
			m_iLineCount++;

			// Skip newline characters
			if( pos < length && text[pos] == '\r' ) pos++;
			if( pos < length && text[pos] == '\n' ) pos++;
		}

		if( m_iLineCount > CON_MAX_LINES )
			m_iLineCount = CON_MAX_LINES;
	}
	else
	{
		// Incremental: only parse new bytes from m_iLastFileSize onward
		int pos = m_iLastFileSize;
		bool hasNew = false;

		while( pos < length )
		{
			int lineStart = pos;
			// Find end of line
			while( pos < length && text[pos] != '\n' && text[pos] != '\r' )
				pos++;

			// Only add complete lines (terminated by newline) or content at EOF
			int len = pos - lineStart;
			if( len > 0 || pos < length )
			{
				if( len >= CON_LINE_LEN )
					len = CON_LINE_LEN - 1;
				if( len < 0 ) len = 0;

				memcpy( m_lines[m_iLineHead], text + lineStart, len );
				m_lines[m_iLineHead][len] = '\0';
				m_lineColors[m_iLineHead] = CON_TEXT_COLOR;

				m_iLineHead = (m_iLineHead + 1) % CON_MAX_LINES;
				if( m_iLineCount < CON_MAX_LINES )
					m_iLineCount++;

				hasNew = true;
			}

			// Skip newline characters
			if( pos < length && text[pos] == '\r' ) pos++;
			if( pos < length && text[pos] == '\n' ) pos++;
		}

		// Auto-scroll to bottom when new content detected
		if( hasNew )
			m_iScrollOffset = 0;
	}

	m_iLastFileSize = length;
	EngFuncs::COM_FreeFile( pFile );
}

void CMenuWndConsole::Submit()
{
	const char *cmd = inputField.GetBuffer();
	if( !cmd || !cmd[0] )
		return;

	// Save to history
	Q_strncpy( m_history[m_iHistoryHead], cmd, sizeof( m_history[0] ) );
	m_iHistoryHead = (m_iHistoryHead + 1) % CON_HISTORY_SIZE;
	if( m_iHistoryCount < CON_HISTORY_SIZE )
		m_iHistoryCount++;
	m_iHistoryIndex = -1;

	// Add command echo to output
	char echo[CON_LINE_LEN];
	snprintf( echo, sizeof( echo ), "] %s", cmd );

	Q_strncpy( m_lines[m_iLineHead], echo, CON_LINE_LEN );
	m_lineColors[m_iLineHead] = CON_CMD_COLOR;
	m_iLineHead = (m_iLineHead + 1) % CON_MAX_LINES;
	if( m_iLineCount < CON_MAX_LINES )
		m_iLineCount++;

	// Execute command (append newline so engine processes it)
	char cmdBuf[512];
	snprintf( cmdBuf, sizeof( cmdBuf ), "%s\n", cmd );
	EngFuncs::ClientCmd( false, cmdBuf );

	// Clear input and scroll to bottom
	inputField.Clear();
	m_iScrollOffset = 0;

	// Force re-read on next frame to pick up command output
	m_iLastReadTime = 0;
}

void CMenuWndConsole::HistoryUp()
{
	if( m_iHistoryCount == 0 )
		return;

	if( m_iHistoryIndex == -1 )
	{
		// Save current input before browsing
		Q_strncpy( m_szSavedInput, inputField.GetBuffer(), sizeof( m_szSavedInput ) );
		// Start at most recent entry
		m_iHistoryIndex = 0;
	}
	else if( m_iHistoryIndex < m_iHistoryCount - 1 )
	{
		m_iHistoryIndex++;
	}
	else
	{
		return; // already at oldest
	}

	// History is stored with head pointing past the most recent entry
	// Index 0 = most recent, 1 = second most recent, etc.
	int idx = (m_iHistoryHead - 1 - m_iHistoryIndex + CON_HISTORY_SIZE) % CON_HISTORY_SIZE;
	inputField.SetBuffer( m_history[idx] );
}

void CMenuWndConsole::HistoryDown()
{
	if( m_iHistoryIndex < 0 )
		return;

	m_iHistoryIndex--;

	if( m_iHistoryIndex < 0 )
	{
		// Restore saved input
		inputField.SetBuffer( m_szSavedInput );
	}
	else
	{
		int idx = (m_iHistoryHead - 1 - m_iHistoryIndex + CON_HISTORY_SIZE) % CON_HISTORY_SIZE;
		inputField.SetBuffer( m_history[idx] );
	}
}

void CMenuWndConsole::Draw()
{
	// Read console log periodically
	ReadLog();

	// Compute scaled title height and border (same as CMenuFrame::Draw)
	m_iTitleH = (int)(FRAME_TITLE_HEIGHT * uiStatic.scaleY);
	m_iBorderW = (int)(FRAME_BORDER_WIDTH * uiStatic.scaleY);
	if( m_iBorderW < 1 ) m_iBorderW = 1;

	ApplyResize();
	ApplyDrag();

	// --- Dynamic input field positioning ---
	// Because inputField has QMF_DISABLESCAILING, CalcPosition sets m_scPos = pos directly.
	// So we set pos/size to screen-space values and trigger recalculation.
	int pad = (int)(CON_PADDING * uiStatic.scaleX);
	int inputH = (int)(CON_INPUT_HEIGHT * uiStatic.scaleY);

	inputField.pos.x = pad;
	inputField.pos.y = (m_scSize.h - m_iTitleH) - inputH - pad;
	inputField.size.w = m_scSize.w - pad * 2;
	inputField.size.h = inputH;
	inputField.CalcPosition();
	inputField.CalcSizes();

	// Draw frame chrome
	DrawBackground();
	DrawTitleBar();

	// --- Draw console output text ---
	int lineH = (int)(14 * uiStatic.scaleY);
	if( lineH < 8 ) lineH = 8;

	int contentTop = m_scPos.y + m_iTitleH + 4;
	int separatorY = inputField.GetRenderPosition().y - pad;
	int contentBottom = separatorY;
	int contentX = m_scPos.x + pad;
	int contentW = m_scSize.w - pad * 2;
	int visibleHeight = contentBottom - contentTop;
	int maxVisibleLines = visibleHeight / lineH;
	if( maxVisibleLines < 1 ) maxVisibleLines = 1;

	// Clamp scroll offset
	int maxScroll = m_iLineCount - maxVisibleLines;
	if( maxScroll < 0 ) maxScroll = 0;
	if( m_iScrollOffset > maxScroll ) m_iScrollOffset = maxScroll;
	if( m_iScrollOffset < 0 ) m_iScrollOffset = 0;

	// Draw lines from ring buffer
	// The ring buffer: oldest line is at index (m_iLineHead - m_iLineCount + CON_MAX_LINES) % CON_MAX_LINES
	// We want to show lines from (newest - maxVisibleLines - scrollOffset) to (newest - scrollOffset)

	// Start from the bottom-most visible line (which is the newest minus scroll offset)
	int firstVisibleFromBottom = m_iScrollOffset;
	int lastVisibleFromBottom = firstVisibleFromBottom + maxVisibleLines - 1;
	if( lastVisibleFromBottom >= m_iLineCount )
		lastVisibleFromBottom = m_iLineCount - 1;

	for( int i = lastVisibleFromBottom; i >= firstVisibleFromBottom; i-- )
	{
		// i=0 is the newest line, i=1 is one before that, etc.
		int ringIdx = (m_iLineHead - 1 - i + CON_MAX_LINES) % CON_MAX_LINES;
		int screenLine = (lastVisibleFromBottom - i); // 0 = top of visible area
		int drawY = contentTop + screenLine * lineH;

		if( drawY + lineH > contentBottom )
			break;

		unsigned int color = m_lineColors[ringIdx];
		UI_DrawString( uiStatic.hConsoleFont, contentX, drawY, contentW, lineH,
			m_lines[ringIdx], color, lineH, QM_LEFT, ETF_FORCECOL | ETF_NO_WRAP );
	}

	// Draw separator line between output and input
	UI_FillRect( m_scPos.x + pad, separatorY, m_scSize.w - pad * 2, 1, CON_SEPARATOR_COLOR );

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

	if( key == K_UPARROW )
	{
		HistoryUp();
		return true;
	}

	if( key == K_DOWNARROW )
	{
		HistoryDown();
		return true;
	}

	if( key == K_MWHEELUP )
	{
		m_iScrollOffset += 3;
		return true;
	}

	if( key == K_MWHEELDOWN )
	{
		m_iScrollOffset -= 3;
		if( m_iScrollOffset < 0 )
			m_iScrollOffset = 0;
		return true;
	}

	// Check if tap is on the input field - activate keyboard
	if( UI::Key::IsLeftMouse( key ) )
	{
		Point iPos = inputField.GetRenderPosition();
		Size iSize = inputField.GetRenderSize();
		if( uiStatic.cursorX >= iPos.x &&
		    uiStatic.cursorX <= iPos.x + iSize.w &&
		    uiStatic.cursorY >= iPos.y &&
		    uiStatic.cursorY <= iPos.y + iSize.h )
		{
			SetCursorToItem( inputField );
			UI_EnableTextInput( true );
			return true;
		}
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
