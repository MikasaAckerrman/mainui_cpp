// VguiConsole.cpp - Pixel-accurate CS 1.6 GoldSrc VGUI console window.
//
// Layout (matches the PC CS 1.6 "Консоль" window):
//   - Title bar: Steam icon + "Консоль" + close (X) button  (drawn by Frame)
//   - Main output area: scrollable, monospace-style text with per-line colors
//       * yellow  for system/bracketed messages
//       * white   for normal output
//       * green   for build info ("Build: ...")
//   - Right scrollbar: 16px wide, ▲ up button, ▼ down button, draggable thumb
//   - Bottom input bar: text field + "RU" language indicator + "Подтвердить"
//
// The window is freely resizable (Frame already supports drag-resize) and has
// no maximize button. All coordinates use VS() scaling from a 640x480 grid.

// Heavy mainui headers FIRST to avoid the `null` macro clash documented in
// VGUI_SchemeColors.h (TrackerScheme.h pulls in EventSystem.h which uses a
// `null` parameter name that collides with VGUI's `null` macro).
#include "BaseMenu.h"
#include "FontManager.h"
extern void UI_FillRect( int x, int y, int width, int height, const unsigned int color );
extern void UI_EnableTextInput( bool enable );
#include "TrackerScheme.h"

#include <VGUI_Log.h>
#include <VGUI_SchemeColors.h>
#include <VGUI_UIScale.h>
#include <VGUI_Frame.h>
#include <VGUI_Panel.h>
#include <VGUI_Label.h>
#include <VGUI_Button.h>
#include <VGUI_TextEntry.h>
#include <VGUI_App.h>
#include <VGUI_ActionSignal.h>
#include <VGUI_CvarBridge.h>
#include <VGUI_Scheme.h>
#include <VGUI_Console.h>
#include <string.h>
#include <stdio.h>

// Lazy-init entry point implemented in vgui_main.cpp (used by VGUI_ShowOptions
// too) - ensures the App/Scheme/root panel exist before we add the window.
extern "C" void VGUI_EnsureInitialized(int screenW, int screenH);

namespace vgui
{

// ====================================================================
// Circular output buffer (last 500 lines, 256 bytes each)
// ====================================================================
#define CON_MAX_LINES 500
#define CON_LINE_LEN  256

static char         s_conLines[CON_MAX_LINES][CON_LINE_LEN];
static unsigned int s_conColors[CON_MAX_LINES];
static int          s_conHead   = 0;   // index where the next line is written
static int          s_conCount  = 0;   // number of valid lines (<= CON_MAX_LINES)
static int          s_conScroll = 0;   // lines scrolled up from the bottom (0 = bottom)

// Default text colors
#define CON_COL_WHITE  0xFFD8DED3u
#define CON_COL_YELLOW 0xFFB8A010u
#define CON_COL_GREEN  0xFF40C040u

static ConsoleOutputPanel* s_outputPanel = 0;
static ConsoleScrollBar*   s_scrollPanel = 0;
static VguiConsole*        s_console     = 0;

static int Con_Count() { return s_conCount; }

// k = 0 returns the OLDEST stored line, k = count-1 the newest.
static const char* Con_Line(int k, unsigned int& color)
{
	int idx = (s_conHead - s_conCount + k) % CON_MAX_LINES;
	if (idx < 0) idx += CON_MAX_LINES;
	color = s_conColors[idx];
	return s_conLines[idx];
}

static void Con_AppendLine(const char* s, unsigned int color)
{
	char* dst = s_conLines[s_conHead];
	int i = 0;
	for (; s[i] && i < CON_LINE_LEN - 1; i++) dst[i] = s[i];
	dst[i] = 0;
	s_conColors[s_conHead] = color;
	s_conHead = (s_conHead + 1) % CON_MAX_LINES;
	if (s_conCount < CON_MAX_LINES) s_conCount++;
}

// Map a Quake-style color code digit to an ARGB color (0 = not a color code).
static unsigned int Con_CodeColor(char c)
{
	switch (c)
	{
	case '1': return 0xFFFF4040u; // red
	case '2': return 0xFF40C040u; // green
	case '3': return 0xFFB8A010u; // yellow
	case '4': return 0xFF4060FFu; // blue
	case '5': return 0xFF40E0E0u; // cyan
	case '6': return 0xFFE08020u; // orange
	}
	return 0;
}

// Strip ^N color codes from a single (newline-free) line, pick its color, and
// append it to the circular buffer.
static void Con_ProcessLine(const char* raw)
{
	char clean[CON_LINE_LEN];
	int o = 0;
	unsigned int codeColor = 0;
	bool haveCode = false;

	for (int i = 0; raw[i] && o < CON_LINE_LEN - 1; i++)
	{
		if (raw[i] == '^' && raw[i + 1])
		{
			unsigned int cc = Con_CodeColor(raw[i + 1]);
			if (cc)
			{
				if (!haveCode) { codeColor = cc; haveCode = true; }
				i++;        // skip the digit too
				continue;   // drop the 2-char code
			}
		}
		clean[o++] = raw[i];
	}
	clean[o] = 0;

	unsigned int color;
	if (haveCode)
	{
		color = codeColor;
	}
	else
	{
		const char* t = clean;
		while (*t == ' ') t++;
		if (strncmp(t, "Build:", 6) == 0)
			color = CON_COL_GREEN;          // build info = green
		else if (t[0] == '[')
			color = CON_COL_YELLOW;         // system/bracketed = yellow
		else
			color = CON_COL_WHITE;          // normal output = white
	}
	Con_AppendLine(clean, color);
}

// ====================================================================
// Shared metrics helpers (used by both output panel and scrollbar so
// their notion of "visible lines" stays in sync)
// ====================================================================
static int Con_LineHeight()
{
	int lh = VS(12);
	return lh < 8 ? 8 : lh;
}

static int Con_VisibleForHeight(int tall)
{
	int v = (tall - VS(3) * 2) / Con_LineHeight();
	return v < 1 ? 1 : v;
}

static void Con_ClampScroll(int visible)
{
	int total = Con_Count();
	int maxS = total - visible;
	if (maxS < 0) maxS = 0;
	if (s_conScroll > maxS) s_conScroll = maxS;
	if (s_conScroll < 0) s_conScroll = 0;
}

// Defined after the panel classes are complete (see below).
static void Con_RepaintAll();

// ====================================================================
// VGUI_Console_Output - parse text, split on newlines, append to buffer
// ====================================================================
void VGUI_Console_Output(const char* text)
{
	if (!text) return;

	char line[CON_LINE_LEN];
	int o = 0;
	for (int i = 0; ; i++)
	{
		char c = text[i];
		if (c == '\n' || c == 0)
		{
			line[o] = 0;
			if (!(c == 0 && o == 0))   // don't append a trailing empty segment
				Con_ProcessLine(line);
			o = 0;
			if (c == 0) break;
		}
		else if (c == '\r')
		{
			continue;                  // drop CR (CRLF -> LF)
		}
		else
		{
			if (o < CON_LINE_LEN - 1) line[o++] = c;
		}
	}

	s_conScroll = 0;                   // auto-scroll to bottom on new output
	Con_RepaintAll();
}

// ====================================================================
// ConsoleOutputPanel - flat field background + colored text lines
// ====================================================================
class ConsoleOutputPanel : public Panel
{
public:
	ConsoleOutputPanel(int x, int y, int w, int h) : Panel(x, y, w, h) {}
protected:
	// Output area draws everything in paintBackground: a FLAT field background
	// (no grain), a 1px sunken inset border, then the colored text lines.
	virtual void paintBackground()
	{
		int wide, tall;
		getSize(wide, tall);

		unsigned int fieldBg = g_Scheme.fieldBgColor ? g_Scheme.fieldBgColor : 0xFF3E4637;
		unsigned int bright  = g_Scheme.borderBright ? g_Scheme.borderBright : 0xFF889180;
		unsigned int dark    = g_Scheme.borderDark   ? g_Scheme.borderDark   : 0xFF282E22;

		// Flat background (no grain).
		schemeBgColor(this, fieldBg);
		drawFilledRect(0, 0, wide, tall);

		// Sunken 1px inset border (dark TL, bright BR).
		schemeBgColor(this, dark);
		drawFilledRect(0, 0, wide, 1);
		drawFilledRect(0, 0, 1, tall);
		schemeBgColor(this, bright);
		drawFilledRect(0, tall - 1, wide, tall);
		drawFilledRect(wide - 1, 0, wide, tall);

		// Colored text lines, bottom-anchored, offset upward by s_conScroll.
		int lh = Con_LineHeight();
		int pad = VS(3);
		int visible = Con_VisibleForHeight(tall);
		Con_ClampScroll(visible);

		int total = Con_Count();
		int first = total - visible - s_conScroll;
		if (first < 0) first = 0;

		drawSetTextFont(Scheme::sf_primary1);

		int y = pad + 1;
		int areaBot = tall - pad;
		for (int k = first; k < total; k++)
		{
			if (y + lh > areaBot) break;
			unsigned int col;
			const char* s = Con_Line(k, col);
			schemeFgColor(this, col);
			drawPrintText(pad + VS(2), y, s, (int)strlen(s));
			y += lh;
		}
	}
};

// ====================================================================
// ConsoleScrollBar - simple Panel with up/down buttons + draggable thumb
// ====================================================================
class ConsoleScrollBar : public Panel
{
public:
	ConsoleScrollBar(int x, int y, int w, int h)
		: Panel(x, y, w, h), _dragging(false), _dragStartY(0), _dragStartScroll(0) {}
protected:
	virtual void paintBackground()
	{
		int wide, tall;
		getSize(wide, tall);
		int barW = wide;

		unsigned int track  = g_Scheme.fieldBgColor  ? g_Scheme.fieldBgColor  : 0xFF3E4637;
		unsigned int btnBg  = g_Scheme.buttonBgColor  ? g_Scheme.buttonBgColor  : 0xFF4C5844;
		unsigned int bright = g_Scheme.borderBright   ? g_Scheme.borderBright   : 0xFF889180;
		unsigned int dark   = g_Scheme.borderDark     ? g_Scheme.borderDark     : 0xFF282E22;
		unsigned int glyph  = g_Scheme.buttonTextColor? g_Scheme.buttonTextColor: 0xFFD8DED3;

		// Track
		schemeBgColor(this, track);
		drawFilledRect(0, 0, wide, tall);

		// Up button (top square)
		bevelRaised(0, 0, wide, barW, btnBg, bright, dark);
		drawArrow(barW / 2, barW / 2, glyph, true);

		// Down button (bottom square)
		bevelRaised(0, tall - barW, wide, tall, btnBg, bright, dark);
		drawArrow(barW / 2, tall - barW + barW / 2, glyph, false);

		// Thumb
		int thumbY, thumbH;
		computeThumb(thumbY, thumbH);
		bevelRaised(0, thumbY, wide, thumbY + thumbH, btnBg, bright, dark);
	}

	virtual void internalMousePressed(MouseCode code)
	{
		if (code == MOUSE_LEFT)
		{
			App* app = App::getInstance();
			if (app)
			{
				int mx, my;
				app->getCursorPos(mx, my);
				screenToLocal(mx, my);
				int wide, tall;
				getSize(wide, tall);
				int barW = wide;

				if (my < barW)
				{
					Con_Scroll(+1);              // view older lines
				}
				else if (my >= tall - barW)
				{
					Con_Scroll(-1);              // view newer lines
				}
				else
				{
					_dragging = true;
					_dragStartY = my;
					_dragStartScroll = s_conScroll;
					setAsMouseCapture(true);
				}
				Con_RepaintAll();
			}
		}
		Panel::internalMousePressed(code);
	}

	virtual void internalCursorMoved(int x, int y)
	{
		if (_dragging)
		{
			App* app = App::getInstance();
			if (app)
			{
				int mx, my;
				app->getCursorPos(mx, my);
				screenToLocal(mx, my);
				int wide, tall;
				getSize(wide, tall);
				int barW = wide;

				int trackH = (tall - barW) - barW;
				if (trackH < VS(8)) trackH = VS(8);
				int visible = Con_VisibleForHeight(tall);
				int total = Con_Count();
				int maxScroll = total - visible;
				if (maxScroll < 0) maxScroll = 0;

				int thumbH = (total <= visible) ? trackH : (trackH * visible / total);
				int minTh = VS(16);
				if (thumbH < minTh) thumbH = minTh;
				if (thumbH > trackH) thumbH = trackH;
				int avail = trackH - thumbH;
				if (avail < 1) avail = 1;

				int dy = my - _dragStartY;
				// Drag down -> toward bottom -> smaller scroll value.
				int ns = _dragStartScroll - (dy * maxScroll) / avail;
				if (ns < 0) ns = 0;
				if (ns > maxScroll) ns = maxScroll;
				s_conScroll = ns;
				Con_RepaintAll();
			}
		}
		Panel::internalCursorMoved(x, y);
	}

	virtual void internalMouseReleased(MouseCode code)
	{
		if (code == MOUSE_LEFT && _dragging)
		{
			_dragging = false;
			setAsMouseCapture(false);
		}
		Panel::internalMouseReleased(code);
	}
private:
	void bevelRaised(int x0, int y0, int x1, int y1,
	                 unsigned int bg, unsigned int tl, unsigned int br)
	{
		schemeBgColor(this, bg);
		drawFilledRect(x0 + 1, y0 + 1, x1 - 1, y1 - 1);
		schemeBgColor(this, tl);
		drawFilledRect(x0, y0, x1, y0 + 1);
		drawFilledRect(x0, y0, x0 + 1, y1);
		schemeBgColor(this, br);
		drawFilledRect(x0, y1 - 1, x1, y1);
		drawFilledRect(x1 - 1, y0, x1, y1);
	}

	// Triangle glyph centered at (cx, cy). up=true draws ▲, else ▼.
	void drawArrow(int cx, int cy, unsigned int col, bool up)
	{
		int t = VS(1);
		if (t < 1) t = 1;
		int top = cy - (3 * t) / 2;
		schemeBgColor(this, col);
		if (up)
		{
			drawFilledRect(cx - VS(1), top,         cx + VS(1), top + t);
			drawFilledRect(cx - VS(2), top + t,     cx + VS(2), top + 2 * t);
			drawFilledRect(cx - VS(3), top + 2 * t, cx + VS(3), top + 3 * t);
		}
		else
		{
			drawFilledRect(cx - VS(3), top,         cx + VS(3), top + t);
			drawFilledRect(cx - VS(2), top + t,     cx + VS(2), top + 2 * t);
			drawFilledRect(cx - VS(1), top + 2 * t, cx + VS(1), top + 3 * t);
		}
	}

	void computeThumb(int& thumbY, int& thumbH)
	{
		int wide, tall;
		getSize(wide, tall);
		int barW = wide;
		int trackY0 = barW;
		int trackY1 = tall - barW;
		int trackH = trackY1 - trackY0;
		if (trackH < VS(8)) trackH = VS(8);

		int visible = Con_VisibleForHeight(tall);
		int total = Con_Count();
		Con_ClampScroll(visible);

		if (total <= visible)
		{
			thumbH = trackH;
			thumbY = trackY0;
			return;
		}

		thumbH = trackH * visible / total;
		int minTh = VS(16);
		if (thumbH < minTh) thumbH = minTh;
		if (thumbH > trackH) thumbH = trackH;

		int maxScroll = total - visible;
		int avail = trackH - thumbH;
		// scroll==0 -> bottom; scroll==maxScroll -> top.
		int fromTop = (maxScroll > 0) ? (avail * (maxScroll - s_conScroll) / maxScroll) : 0;
		thumbY = trackY0 + fromTop;
	}

	static void Con_Scroll(int delta)
	{
		s_conScroll += delta;
		if (s_conScroll < 0) s_conScroll = 0;
		int total = Con_Count();
		if (s_conScroll > total) s_conScroll = total; // precise clamp happens in paint
	}

	bool _dragging;
	int  _dragStartY;
	int  _dragStartScroll;
};

// Now that ConsoleOutputPanel and ConsoleScrollBar are complete types, repaint
// them through their public base interface (single inheritance, offset 0).
static void Con_RepaintAll()
{
	if (s_outputPanel) s_outputPanel->repaint();
	if (s_scrollPanel) s_scrollPanel->repaint();
}

// ====================================================================
// Input field + submit wiring
// ====================================================================
class ConsoleSubmitSignal : public ActionSignal
{
public:
	ConsoleSubmitSignal(VguiConsole* con) : _con(con) {}
	virtual void actionPerformed(Panel* /*p*/) { if (_con) _con->submitCommand(); }
private:
	VguiConsole* _con;
};

// TextEntry subclass that submits the command on Enter (the base TextEntry
// fires its action signal on every keystroke, so we intercept ENTER here
// instead of relying on the signal to know "the user pressed Enter").
class ConsoleInput : public TextEntry
{
public:
	ConsoleInput(VguiConsole* con, int x, int y, int w, int h)
		: TextEntry("", x, y, w, h), _con(con) {}
protected:
	virtual void internalKeyPressed(KeyCode code)
	{
		if (code == KEY_ENTER)
		{
			if (_con) _con->submitCommand();
			return; // swallow ENTER (don't double-fire via base handler)
		}
		TextEntry::internalKeyPressed(code);
	}
private:
	VguiConsole* _con;
};

// ====================================================================
// VguiConsole
// ====================================================================
VguiConsole::VguiConsole(int screenW, int screenH)
	: Frame(0, 0, VS(480), VS(260))
{
	VLOG("VguiConsole ctor: screen=%dx%d", screenW, screenH);
	_output = 0;
	_scrollbar = 0;
	_input = 0;
	_ruLabel = 0;
	_confirmBtn = 0;

	int dlgW = VS(480);
	int dlgH = VS(260);
	if (dlgW > screenW - VS(8)) dlgW = screenW - VS(8);
	if (dlgH > screenH - VS(8)) dlgH = screenH - VS(8);

	setPos((screenW - dlgW) / 2, (screenH - dlgH) / 2);
	setSize(dlgW, dlgH);
	setSizeable(true);                 // user can drag edges/corners to resize
	setMinimumSize(VS(300), VS(160));
	setTitle("\xD0\x9A\xD0\xBE\xD0\xBD\xD1\x81\xD0\xBE\xD0\xBB\xD1\x8C"); // Консоль
	setVisible(false);

	Panel* client = getClient();
	if (!client) { VLOG("VguiConsole: getClient() null -- abort"); return; }

	int clientW, clientH;
	client->getSize(clientW, clientH);

	_output = new ConsoleOutputPanel(0, 0, 10, 10);
	client->addChild(_output);
	s_outputPanel = _output;

	_scrollbar = new ConsoleScrollBar(0, 0, 10, 10);
	client->addChild(_scrollbar);
	s_scrollPanel = _scrollbar;

	_input = new ConsoleInput(this, 0, 0, 10, 10);
	client->addChild(_input);

	_ruLabel = new Label("RU", 0, 0, 10, 10);
	_ruLabel->setContentAlignment(Label::a_center);
	client->addChild(_ruLabel);

	_confirmBtn = new Button(
		"\xD0\x9F\xD0\xBE\xD0\xB4\xD1\x82\xD0\xB2\xD0\xB5\xD1\x80\xD0\xB4\xD0\xB8\xD1\x82\xD1\x8C", // Подтвердить
		0, 0, 10, 10);
	_confirmBtn->addActionSignal(new ConsoleSubmitSignal(this));
	client->addChild(_confirmBtn);

	layoutChildren();
}

void VguiConsole::layoutChildren()
{
	Panel* client = getClient();
	if (!client) return;

	int clientW, clientH;
	client->getSize(clientW, clientH);

	int margin     = VS(4);
	int inputH     = VS(22);
	int scrollbarW = VS(16);

	int inputY     = clientH - inputH - margin;
	int outX       = margin;
	int outY       = margin;
	int outW       = clientW - margin * 2 - scrollbarW;
	int outH       = inputY - outY - margin;
	if (outW < VS(16)) outW = VS(16);
	if (outH < VS(16)) outH = VS(16);

	if (_output)    _output->setBounds(outX, outY, outW, outH);
	if (_scrollbar) _scrollbar->setBounds(outX + outW, outY, scrollbarW, outH);

	int confirmW = VS(90);
	int ruW      = VS(26);
	int gap      = VS(4);

	int confirmX = clientW - margin - confirmW;
	int ruX      = confirmX - gap - ruW;
	int inX      = margin;
	int inW      = ruX - gap - inX;
	if (inW < VS(40)) inW = VS(40);

	if (_input)      _input->setBounds(inX, inputY, inW, inputH);
	if (_ruLabel)    _ruLabel->setBounds(ruX, inputY, ruW, inputH);
	if (_confirmBtn) _confirmBtn->setBounds(confirmX, inputY, confirmW, inputH);
}

void VguiConsole::setSize(int wide, int tall)
{
	Frame::setSize(wide, tall);
	layoutChildren();
}

void VguiConsole::submitCommand()
{
	if (!_input) return;

	char buf[CON_LINE_LEN];
	_input->getText(0, buf, sizeof(buf));
	if (buf[0] == 0) return;

	// Echo the typed command into the output (CS 1.6 prefixes it with "] ").
	char echo[CON_LINE_LEN];
	snprintf(echo, sizeof(echo), "] %s", buf);
	VGUI_Console_Output(echo);

	// Run it through the engine command bridge.
	char cmd[CON_LINE_LEN];
	snprintf(cmd, sizeof(cmd), "%s\n", buf);
	VGUI_ClientCmd(cmd);

	_input->setText("", 0);
	_input->requestFocus();
}

// ====================================================================
// Lifecycle helpers
// ====================================================================
void VGUI_Console_Show(bool show)
{
	if (show)
	{
		int sw = 0, sh = 0;
		VGUI_GetScreenSize(&sw, &sh);
		if (sw <= 0) sw = 640;
		if (sh <= 0) sh = 480;

		VGUI_EnsureInitialized(sw, sh);
		Panel* root = VGUI_GetRootPanel();
		if (!root) return;
		root->setSize(sw, sh);

		if (!s_console)
		{
			VLOG("VGUI_Console_Show: creating console window");
			s_console = new VguiConsole(sw, sh);
			root->addChild(s_console);
		}
		s_console->setVisible(true);
		VLOG("VGUI_Console_Show: visible");
	}
	else
	{
		if (s_console)
		{
			s_console->setVisible(false);
			VLOG("VGUI_Console_Show: hidden");
		}
	}
}

bool VGUI_Console_IsVisible()
{
	return s_console && s_console->isVisible();
}

} // namespace vgui

// Called from VGUI_Shutdown (vgui_main.cpp) before the root panel tree is
// destroyed, so our cached pointers never dangle.
void VGUI_ConsoleShutdown(void)
{
	vgui::s_console     = 0;
	vgui::s_outputPanel = 0;
	vgui::s_scrollPanel = 0;
}
