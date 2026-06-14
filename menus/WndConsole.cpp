/*
WndConsole.cpp -- GoldSrc/CS 1.6-authentic console overlay.
Copyright (C) 2024 DragonSlayer Team

Visual design matches original Counter-Strike 1.6 console:
  - Full screen width, top ~60% of screen height (no title bar chrome).
  - Solid black background; faint separator above the input line.
  - Console text rendered bottom-up (newest at bottom), scrollable.
  - Input row: gold ">" prompt + CMenuField for text entry.
  - Green 1px accent line at the very bottom of the console panel.

Data source: reads engine.log via EngFuncs::COM_LoadFile (incremental,
throttled to 250ms). On Android, engine.log is written to the game's VFS.
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "controls/Frame.h"
#include "Field.h"
#include "Action.h"
#include "keydefs.h"
#include "Utils.h"
#include "TrackerScheme.h"

#define CON_MAX_LINES     512
#define CON_LINE_LEN      512
#define CON_HISTORY_SIZE  32
#define CON_READ_INTERVAL 250   // ms between log re-reads
#define CON_FONT_H        14    // logical line height (px before scale)
#define CON_INPUT_HEIGHT  20    // logical input row height
#define CON_PADDING       6     // logical inner pad

// CS 1.6 console palette
#define CON_BG_COLOR      0xFF000000  // solid black background
#define CON_BORDER_COLOR  0xFF00AA00  // bright green accent at bottom
#define CON_SEP_COLOR     0xFF004400  // dark green separator above input
#define CON_TEXT_COLOR    0xFF40FF40  // green for engine output
#define CON_CMD_COLOR     0xFFFFFF00  // yellow for echoed commands
#define CON_PROMPT_COLOR  0xFFCCBB44  // gold  ">" prompt
#define CON_TITLE_COLOR   0xFFCCBB44  // gold  "CONSOLE" header text
#define CON_HEADER_BG     0xFF0A0A0A  // near-black header band
#define CON_INPUT_BG      0xFF080808  // near-black input band

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

	// Helper: append a line to the ring buffer
	void AppendLine( const char *text, unsigned int color );

	CMenuField inputField;

	// Output ring buffer
	char         m_lines[CON_MAX_LINES][CON_LINE_LEN];
	unsigned int m_lineColors[CON_MAX_LINES];
	int          m_iLineHead;    // next-write position
	int          m_iLineCount;   // valid lines (max CON_MAX_LINES)

	// Scroll (0 = bottom/newest)
	int m_iScrollOffset;

	// Log file tracking
	int m_iLastReadTime;
	int m_iLastFileSize;

	// Command history
	char m_history[CON_HISTORY_SIZE][256];
	int  m_iHistoryHead;
	int  m_iHistoryCount;
	int  m_iHistoryIndex;       // -1 = not browsing
	char m_szSavedInput[256];   // saved input when browsing starts
};

static CMenuWndConsole *s_pWndConsole = NULL;

// ─── Constructor ─────────────────────────────────────────────────────────────

CMenuWndConsole::CMenuWndConsole() : CMenuFrame( "CONSOLE" )
{
	m_iLineHead   = 0;
	m_iLineCount  = 0;
	m_iScrollOffset = 0;
	m_iLastReadTime  = 0;
	m_iLastFileSize  = 0;
	m_iHistoryHead   = 0;
	m_iHistoryCount  = 0;
	m_iHistoryIndex  = -1;
	m_szSavedInput[0] = '\0';
	memset( m_lines,      0, sizeof( m_lines ) );
	memset( m_lineColors, 0, sizeof( m_lineColors ) );
}

// ─── Layout ──────────────────────────────────────────────────────────────────

void CMenuWndConsole::_Init()
{
	// CS 1.6 console: full width, top 60%, anchored at y=0.
	int w = uiStatic.width;
	int h = (int)(uiStatic.height * 0.60f);
	SetRect( 0, 0, w, h );

	inputField.iFlags    |= QMF_DISABLESCAILING;
	inputField.iMaxLength = 255;
	inputField.szName     = "";
	// Rect is recalculated every Draw(); give it a valid placeholder.
	inputField.SetRect( CON_PADDING, h - CON_INPUT_HEIGHT,
	                    w - CON_PADDING * 2, CON_INPUT_HEIGHT );
	AddItem( inputField );
}

void CMenuWndConsole::_VidInit()
{
	// Always re-dock to top regardless of m_bUserMoved;
	// a console is not a floating window.
	int w = uiStatic.width;
	int h = (int)(uiStatic.height * 0.60f);
	SetRect( 0, 0, w, h );
	m_bUserMoved = false; // prevent drift after resolution change
}

// ─── Show ─────────────────────────────────────────────────────────────────────

void CMenuWndConsole::Show()
{
	m_iLastReadTime = 0;  // force immediate log refresh on open
	CMenuFrame::Show();
	SetCursorToItem( inputField );
}

// ─── Log reading ─────────────────────────────────────────────────────────────

void CMenuWndConsole::AppendLine( const char *text, unsigned int color )
{
	if( !text ) return;
	Q_strncpy( m_lines[m_iLineHead], text, CON_LINE_LEN );
	m_lineColors[m_iLineHead] = color;
	m_iLineHead = (m_iLineHead + 1) % CON_MAX_LINES;
	// Guard: never exceed ring-buffer capacity.
	if( m_iLineCount < CON_MAX_LINES ) m_iLineCount++;
}

void CMenuWndConsole::ReadLog()
{
	if( uiStatic.realTime - m_iLastReadTime < CON_READ_INTERVAL )
		return;
	m_iLastReadTime = uiStatic.realTime;

	int length = 0;
	byte *pFile = EngFuncs::COM_LoadFile( "engine.log", &length );
	if( !pFile || length <= 0 )
	{
		if( pFile ) EngFuncs::COM_FreeFile( pFile );
		return;
	}

	if( length == m_iLastFileSize )
	{
		EngFuncs::COM_FreeFile( pFile );
		return;
	}

	const char *text     = (const char *)pFile;
	bool        truncated = (length < m_iLastFileSize);

	// If log was rotated/truncated, wipe buffer and re-parse from scratch.
	if( truncated )
	{
		m_iLineHead  = 0;
		m_iLineCount = 0;
	}

	// Parse: from last-known offset (or 0 on truncation) to end.
	int pos = truncated ? 0 : m_iLastFileSize;
	bool hasNew = false;

	while( pos < length )
	{
		int lineStart = pos;
		while( pos < length && text[pos] != '\n' && text[pos] != '\r' )
			pos++;

		int len = pos - lineStart;
		if( len > 0 )
		{
			// Clamp long lines to buffer length.
			if( len >= CON_LINE_LEN ) len = CON_LINE_LEN - 1;

			char buf[CON_LINE_LEN];
			memcpy( buf, text + lineStart, len );
			buf[len] = '\0';

			AppendLine( buf, CON_TEXT_COLOR );  // AppendLine guards m_iLineCount
			hasNew = true;
		}

		// Advance past newline(s)
		if( pos < length && text[pos] == '\r' ) pos++;
		if( pos < length && text[pos] == '\n' ) pos++;
	}

	if( hasNew )
		m_iScrollOffset = 0;  // auto-scroll to newest on new output

	m_iLastFileSize = length;
	EngFuncs::COM_FreeFile( pFile );
}

// ─── Draw ────────────────────────────────────────────────────────────────────
//
// Overrides CMenuFrame::Draw() completely.
// We do NOT call DrawBackground() / DrawTitleBar() / DrawBorder() from the
// parent — those draw the green menu-chrome which is wrong for a console.
// Instead we manually paint a black overlay.
//
void CMenuWndConsole::Draw()
{
	ReadLog();

	// Recalculate scaled metrics each frame (handles resolution changes).
	m_iTitleH  = (int)(FRAME_TITLE_HEIGHT * uiStatic.scaleY);
	m_iBorderW = (int)(FRAME_BORDER_WIDTH * uiStatic.scaleY);
	if( m_iBorderW < 1 ) m_iBorderW = 1;

	int pad    = (int)(CON_PADDING       * uiStatic.scaleX);
	int inputH = (int)(CON_INPUT_HEIGHT  * uiStatic.scaleY);
	int lineH  = (int)(CON_FONT_H        * uiStatic.scaleY);
	if( lineH < 8 ) lineH = 8;

	int cx = m_scPos.x, cy = m_scPos.y;
	int cw = m_scSize.w, ch = m_scSize.h;

	// ── 1. Solid black background ──
	UI_FillRect( cx, cy, cw, ch, CON_BG_COLOR );

	// ── 2. Compact header bar ("CONSOLE" title, gold on near-black) ──
	int headerH = m_iTitleH;
	UI_FillRect( cx, cy, cw, headerH, CON_HEADER_BG );

	// Title text centered in header
	if( uiStatic.hSmallFont )
	{
		const char *title = "CONSOLE";
		UI_DrawString( uiStatic.hSmallFont,
		               cx, cy, cw, headerH,
		               title, CON_TITLE_COLOR,
		               lineH, QM_CENTER, ETF_FORCECOL | ETF_NO_WRAP );
	}

	// 1px gold underline below header
	UI_FillRect( cx, cy + headerH, cw, 1, CON_PROMPT_COLOR );

	// ── 3. Text area metrics ──
	int textTop    = cy + headerH + 2;
	int inputY     = cy + ch - inputH;
	int separatorY = inputY - 2;
	int textBottom = separatorY;

	int maxVis = (textBottom - textTop) / lineH;
	if( maxVis < 1 ) maxVis = 1;

	// Clamp scroll
	int maxScroll = m_iLineCount - maxVis;
	if( maxScroll < 0 ) maxScroll = 0;
	if( m_iScrollOffset > maxScroll ) m_iScrollOffset = maxScroll;
	if( m_iScrollOffset < 0 )         m_iScrollOffset = 0;

	// ── 4. Console output lines (bottom-up, newest first) ──
	HFont drawFont = uiStatic.hConsoleFont ? uiStatic.hConsoleFont : uiStatic.hSmallFont;

	for( int row = 0; row < maxVis; row++ )
	{
		// row 0 = bottommost visible line (= newest minus scroll offset)
		int age    = row + m_iScrollOffset;
		if( age >= m_iLineCount ) break;

		int ringIdx = (m_iLineHead - 1 - age + CON_MAX_LINES) % CON_MAX_LINES;
		const char *lineText = m_lines[ringIdx];
		if( !lineText[0] ) continue;

		// Draw bottom-up: row 0 at the very bottom of text area
		int lineY = textBottom - (row + 1) * lineH;
		if( lineY < textTop ) break;

		UI_DrawString( drawFont,
		               cx + pad, lineY, cw - pad * 2, lineH,
		               lineText, m_lineColors[ringIdx],
		               lineH, QM_LEFT, ETF_FORCECOL | ETF_NO_WRAP );
	}

	// ── 5. Separator line above input ──
	UI_FillRect( cx, separatorY, cw, 1, CON_SEP_COLOR );
	UI_FillRect( cx, separatorY + 1, cw, 1, 0xFF001100 );  // subtle shadow

	// ── 6. Input row ──
	UI_FillRect( cx, inputY, cw, inputH, CON_INPUT_BG );

	// Draw gold ">" prompt on the left
	const char *prompt = ">";
	int promptW = (int)(10 * uiStatic.scaleX);  // approximate prompt width
	UI_DrawString( drawFont,
	               cx + pad, inputY, promptW + pad, inputH,
	               prompt, CON_PROMPT_COLOR,
	               lineH, QM_LEFT, ETF_FORCECOL | ETF_NO_WRAP );

	// Reposition CMenuField to sit right after the ">" prompt
	inputField.pos.x  = pad + promptW;
	inputField.pos.y  = (ch - headerH) - inputH;
	inputField.size.w = cw - pad * 2 - promptW;
	inputField.size.h = inputH;
	inputField.CalcPosition();
	inputField.CalcSizes();

	// ── 7. Bright green accent line at very bottom ──
	UI_FillRect( cx, cy + ch - 1, cw, 1, CON_BORDER_COLOR );

	// ── 8. Draw child items (the input CMenuField) ──
	CMenuItemsHolder::Draw();
}

// ─── Key handling ────────────────────────────────────────────────────────────

bool CMenuWndConsole::KeyDown( int key )
{
	if( UI::Key::IsEnter( key ) )
	{
		Submit();
		return true;
	}

	if( key == K_UPARROW )   { HistoryUp();   return true; }
	if( key == K_DOWNARROW ) { HistoryDown(); return true; }

	if( key == K_MWHEELUP )
	{
		m_iScrollOffset += 3;
		return true;
	}
	if( key == K_MWHEELDOWN )
	{
		m_iScrollOffset -= 3;
		if( m_iScrollOffset < 0 ) m_iScrollOffset = 0;
		return true;
	}

	// Tap on input field → activate soft keyboard on Android.
	if( UI::Key::IsLeftMouse( key ) )
	{
		Point iPos  = inputField.GetRenderPosition();
		Size  iSize = inputField.GetRenderSize();
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

// ─── Submit ──────────────────────────────────────────────────────────────────

void CMenuWndConsole::Submit()
{
	const char *cmd = inputField.GetBuffer();
	if( !cmd || !cmd[0] ) return;

	// Echo command in yellow ("] cmd" like classic console)
	char echo[CON_LINE_LEN];
	snprintf( echo, sizeof( echo ), "] %s", cmd );
	AppendLine( echo, CON_CMD_COLOR );

	// Add to history
	Q_strncpy( m_history[m_iHistoryHead], cmd, sizeof( m_history[0] ) );
	m_iHistoryHead  = (m_iHistoryHead + 1) % CON_HISTORY_SIZE;
	if( m_iHistoryCount < CON_HISTORY_SIZE ) m_iHistoryCount++;
	m_iHistoryIndex = -1;

	// Execute
	char cmdBuf[512];
	snprintf( cmdBuf, sizeof( cmdBuf ), "%s\n", cmd );
	EngFuncs::ClientCmd( false, cmdBuf );

	inputField.Clear();
	m_iScrollOffset  = 0;
	m_iLastReadTime  = 0;  // force log re-read for output
}

// ─── History navigation ───────────────────────────────────────────────────────

void CMenuWndConsole::HistoryUp()
{
	if( m_iHistoryCount == 0 ) return;

	if( m_iHistoryIndex == -1 )
	{
		Q_strncpy( m_szSavedInput, inputField.GetBuffer(), sizeof( m_szSavedInput ) );
		m_iHistoryIndex = 0;
	}
	else if( m_iHistoryIndex < m_iHistoryCount - 1 )
	{
		m_iHistoryIndex++;
	}
	else return;

	int idx = (m_iHistoryHead - 1 - m_iHistoryIndex + CON_HISTORY_SIZE) % CON_HISTORY_SIZE;
	inputField.SetBuffer( m_history[idx] );
}

void CMenuWndConsole::HistoryDown()
{
	if( m_iHistoryIndex < 0 ) return;

	m_iHistoryIndex--;
	if( m_iHistoryIndex < 0 )
	{
		inputField.SetBuffer( m_szSavedInput );
	}
	else
	{
		int idx = (m_iHistoryHead - 1 - m_iHistoryIndex + CON_HISTORY_SIZE) % CON_HISTORY_SIZE;
		inputField.SetBuffer( m_history[idx] );
	}
}

// ─── Public API (ADD_MENU4 registration) ─────────────────────────────────────

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
