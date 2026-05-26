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
#define CON_INPUT_HEIGHT  32   // logical pixels
#define CON_PADDING       8    // logical pixels

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

	bool newContent = (length > m_iLastFileSize);
	m_iLastFileSize = length;

	// Parse from end of file to extract last CON_MAX_LINES lines
	// Work backwards to find line boundaries
	const char *text = (const char *)pFile;
	int lineStarts[CON_MAX_LINES];
	int numLines = 0;

	// Start from end, find line boundaries
	int pos = length - 1;

	// Skip trailing newline
	while( pos >= 0 && (text[pos] == '\n' || text[pos] == '\r') )
		pos--;

	if( pos < 0 )
	{
		EngFuncs::COM_FreeFile( pFile );
		return;
	}

	// The end of the last content line
	int lineEnd = pos + 1;

	// Walk backwards finding line starts
	while( pos >= 0 && numLines < CON_MAX_LINES )
	{
		if( text[pos] == '\n' )
		{
			lineStarts[numLines] = pos + 1;
			numLines++;
			// Skip \r\n pairs
			if( pos > 0 && text[pos - 1] == '\r' )
				pos--;
			lineEnd = pos; // end of previous line
			pos--;
		}
		else
		{
			pos--;
		}
	}

	// If we didn't hit the beginning, add the first line
	if( pos < 0 && numLines < CON_MAX_LINES )
	{
		lineStarts[numLines] = 0;
		numLines++;
	}

	// Now lineStarts[0..numLines-1] are in reverse order (last line first)
	// Reset ring buffer and fill with the extracted lines
	m_iLineHead = 0;
	m_iLineCount = 0;

	for( int i = numLines - 1; i >= 0; i-- )
	{
		int start = lineStarts[i];
		int end;

		if( i == 0 )
		{
			// This is the last line in file (first in our reversed array)
			// Find its end
			end = length;
			// Trim trailing newlines
			while( end > start && (text[end - 1] == '\n' || text[end - 1] == '\r') )
				end--;
		}
		else
		{
			// End is just before the next line's start
			end = lineStarts[i - 1];
			// Trim newlines between lines
			while( end > start && (text[end - 1] == '\n' || text[end - 1] == '\r') )
				end--;
		}

		int len = end - start;
		if( len >= CON_LINE_LEN )
			len = CON_LINE_LEN - 1;
		if( len < 0 ) len = 0;

		memcpy( m_lines[m_iLineHead], text + start, len );
		m_lines[m_iLineHead][len] = '\0';
		m_lineColors[m_iLineHead] = CON_TEXT_COLOR;

		m_iLineHead = (m_iLineHead + 1) % CON_MAX_LINES;
		if( m_iLineCount < CON_MAX_LINES )
			m_iLineCount++;
	}

	EngFuncs::COM_FreeFile( pFile );

	// Auto-scroll to bottom when new content detected
	if( newContent )
		m_iScrollOffset = 0;
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

	// Handle resizing (from CMenuFrame)
	if( m_bResizing )
	{
		int dx = uiStatic.cursorX - m_resizeStartCursor.x;
		int dy = uiStatic.cursorY - m_resizeStartCursor.y;

		int minW = (int)(FRAME_MIN_W * uiStatic.scaleX);
		int minH = (int)(FRAME_MIN_H * uiStatic.scaleY);

		int newX = m_resizeStartPos.x;
		int newY = m_resizeStartPos.y;
		int newW = m_resizeStartSize.w;
		int newH = m_resizeStartSize.h;

		switch( m_iResizeEdge )
		{
		case RESIZE_RIGHT:       newW += dx; break;
		case RESIZE_LEFT:        newX += dx; newW -= dx; break;
		case RESIZE_BOTTOM:      newH += dy; break;
		case RESIZE_TOP:         newY += dy; newH -= dy; break;
		case RESIZE_BOTTOMRIGHT: newW += dx; newH += dy; break;
		case RESIZE_BOTTOMLEFT:  newX += dx; newW -= dx; newH += dy; break;
		case RESIZE_TOPRIGHT:    newW += dx; newY += dy; newH -= dy; break;
		case RESIZE_TOPLEFT:     newX += dx; newW -= dx; newY += dy; newH -= dy; break;
		}

		if( newW < minW )
		{
			if( m_iResizeEdge == RESIZE_LEFT || m_iResizeEdge == RESIZE_TOPLEFT || m_iResizeEdge == RESIZE_BOTTOMLEFT )
				newX = m_resizeStartPos.x + m_resizeStartSize.w - minW;
			newW = minW;
		}
		if( newH < minH )
		{
			if( m_iResizeEdge == RESIZE_TOP || m_iResizeEdge == RESIZE_TOPLEFT || m_iResizeEdge == RESIZE_TOPRIGHT )
				newY = m_resizeStartPos.y + m_resizeStartSize.h - minH;
			newH = minH;
		}

		if( newX < 0 ) { newW += newX; newX = 0; }
		if( newY < 0 ) { newH += newY; newY = 0; }
		if( newX + newW > ScreenWidth ) newW = (int)ScreenWidth - newX;
		if( newY + newH > ScreenHeight ) newH = (int)ScreenHeight - newY;
		if( newW < minW ) newW = minW;
		if( newH < minH ) newH = minH;

		m_scPos.x = newX;
		m_scPos.y = newY;
		m_scSize.w = newW;
		m_scSize.h = newH;

		CalcItemsPositions();
		CalcItemsSizes();
	}

	// Handle dragging
	if( m_bDragging )
	{
		m_scPos.x = uiStatic.cursorX - m_dragOffset.x;
		m_scPos.y = uiStatic.cursorY - m_dragOffset.y;

		if( m_scPos.x < 0 ) m_scPos.x = 0;
		if( m_scPos.y < 0 ) m_scPos.y = 0;
		if( m_scPos.x + m_scSize.w > ScreenWidth ) m_scPos.x = (int)ScreenWidth - m_scSize.w;
		if( m_scPos.y + m_scSize.h > ScreenHeight ) m_scPos.y = (int)ScreenHeight - m_scSize.h;

		CalcItemsPositions();
	}

	// --- Dynamic input field positioning ---
	// Because inputField has QMF_DISABLESCAILING, CalcPosition sets m_scPos = pos directly.
	// So we set pos/size to screen-space values and trigger recalculation.
	int pad = (int)(CON_PADDING * uiStatic.scaleX);
	int inputH = (int)(CON_INPUT_HEIGHT * uiStatic.scaleY);

	inputField.pos.x = m_scPos.x + pad;
	inputField.pos.y = m_scPos.y + m_scSize.h - inputH - pad;
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
	int separatorY = inputField.pos.y - pad;
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
	int newestIdx = (m_iLineHead - 1 + CON_MAX_LINES) % CON_MAX_LINES;

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
		UI_DrawString( uiStatic.hSmallFont, contentX, drawY, contentW, lineH,
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
		int maxScroll = m_iLineCount - 1;
		if( m_iScrollOffset > maxScroll )
			m_iScrollOffset = maxScroll;
		return true;
	}

	if( key == K_MWHEELDOWN )
	{
		m_iScrollOffset -= 3;
		if( m_iScrollOffset < 0 )
			m_iScrollOffset = 0;
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
