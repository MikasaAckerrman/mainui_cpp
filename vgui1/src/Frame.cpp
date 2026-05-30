// Frame.cpp - GoldSrc CS 1.6 VGUI frame: pixel-perfect look + robust touch drag/resize.
//
// Drag/resize uses incremental-delta tracking: each cursorMoved computes the
// delta from the LAST seen cursor position, not from a stored anchor. This
// eliminates the "jumps to the right" bug caused by stale press-time anchors
// on Android (KEY_DOWN fires before cursor pos updates).

extern void UI_FillRect( int x, int y, int width, int height, const unsigned int color );
#include "TrackerScheme.h"

#include <VGUI_Log.h>
#include <VGUI_SchemeColors.h>
#include <VGUI_UIScale.h>
#include <VGUI_Frame.h>
#include <VGUI_App.h>
#include <VGUI_Button.h>
#include <VGUI_ActionSignal.h>
#include <VGUI_CvarBridge.h> // VGUI_GetScreenSize - authoritative drawable canvas
#include <string.h>

namespace vgui
{

// GoldSrc VGUI Frame layout constants (CS 1.6 reference @ 640x480, scaled via VS).
enum
{
	FRAME_CAPTION_HEIGHT       = 28,
	FRAME_CAPTION_HEIGHT_SMALL = 22,
	FRAME_BORDER               = 4,
	FRAME_BUTTON_SIZE          = 16,
	FRAME_BUTTON_INSET         = 3
};

static inline int Fcap()      { return VS(FRAME_CAPTION_HEIGHT); }
static inline int FcapSmall() { return VS(FRAME_CAPTION_HEIGHT_SMALL); }
static inline int Fborder()   { return VS(FRAME_BORDER); }
static inline int FbtnSz()    { return VS(FRAME_BUTTON_SIZE); }
static inline int FbtnIns()   { return VS(FRAME_BUTTON_INSET); }

// Resize zone bitmask
enum { RZ_N = 1, RZ_S = 2, RZ_W = 4, RZ_E = 8 };

// Edge grip thickness (clickable resize area along each side)
static inline int EdgeGrip()   { return VS(6); }
static inline int CornerGrip() { return VS(10); } // larger for corner hit-test
static inline int MinW()       { return VS(360); }
static inline int MinH()       { return VS(240); }

// Authoritative drawable canvas size (the coordinate space _pos/_size live in).
// Prefer the parent (root) panel size; fall back to the engine screen size, then
// to a sane default. Used to clamp drag/resize so the window can never grow
// larger than the screen nor be flung fully off it.
static void GetCanvasSize(Panel* frame, int& sw, int& sh)
{
	sw = 0; sh = 0;
	Panel* p = frame ? frame->getParent() : null;
	if (p) p->getSize(sw, sh);
	if (sw <= 0 || sh <= 0) VGUI_GetScreenSize(&sw, &sh);
	if (sw <= 0) sw = 640;
	if (sh <= 0) sh = 480;
}

// Hit-test resize zones. lx,ly are panel-local coords, w,h panel size.
// Returns 0 if cursor is not in any resize edge.
static int HitTestResize(int lx, int ly, int w, int h)
{
	int eg = EdgeGrip();
	int cg = CornerGrip();
	int zone = 0;

	// Corners take priority (use larger grip)
	if (lx < cg && ly < cg) return RZ_N | RZ_W;
	if (lx >= w - cg && ly < cg) return RZ_N | RZ_E;
	if (lx < cg && ly >= h - cg) return RZ_S | RZ_W;
	if (lx >= w - cg && ly >= h - cg) return RZ_S | RZ_E;

	// Edges
	if (ly < eg) zone |= RZ_N;
	else if (ly >= h - eg) zone |= RZ_S;
	if (lx < eg) zone |= RZ_W;
	else if (lx >= w - eg) zone |= RZ_E;
	return zone;
}

// Close button signal
class FrameCloseSignal : public ActionSignal
{
public:
	FrameCloseSignal(Frame* frame) : _frame(frame) {}
	virtual void actionPerformed(Panel* panel) { if (_frame) _frame->setVisible(false); }
private:
	Frame* _frame;
};

// GoldSrc close button: bevel border + thick X glyph
class FrameCloseGlyph : public Button
{
public:
	FrameCloseGlyph(int x, int y, int w, int h) : Button("", x, y, w, h) {}
protected:
	virtual void paintBackground()
	{
		int wide, tall;
		getSize(wide, tall);
		bool sunken = isDepressed() || isSelected();

		unsigned int bg = g_Scheme.buttonBgColor ? g_Scheme.buttonBgColor : 0xFF4C5844;
		schemeBgColor(this, bg);
		drawFilledRect(1, 1, wide - 1, tall - 1);

		// Phase 1-D: canonical CS 1.6 border = 1px ONLY.
		// Sunken (depressed/selected) = inset (BorderDark TL, BorderBright BR).
		// Raised = bevel (BorderBright TL, BorderDark BR). No inner second band.
		unsigned int bright = g_Scheme.borderBright ? g_Scheme.borderBright : 0xFF889180;
		unsigned int dark   = g_Scheme.borderDark   ? g_Scheme.borderDark   : 0xFF282E22;

		unsigned int tl = sunken ? dark   : bright;
		unsigned int br = sunken ? bright : dark;

		schemeBgColor(this, tl);
		drawFilledRect(0, 0, wide, 1);
		drawFilledRect(0, 0, 1, tall);
		schemeBgColor(this, br);
		drawFilledRect(0, tall - 1, wide, tall);
		drawFilledRect(wide - 1, 0, wide, tall);
	}

	virtual void paint()
	{
		int wide, tall;
		getSize(wide, tall);

		unsigned int argb = isArmed()
			? (g_Scheme.buttonArmedTextColor ? g_Scheme.buttonArmedTextColor : 0xFFFFFFFF)
			: (g_Scheme.buttonTextColor ? g_Scheme.buttonTextColor : 0xFFD8DED3);

		int side = (wide < tall ? wide : tall);
		int extent = (side * 55) / 100;
		if (extent < 6) extent = 6;
		int sx = (wide - extent) / 2;
		int sy = (tall - extent) / 2;
		if (isDepressed()) { sx += 1; sy += 1; }
		int brush = VS(3);
		if (brush < 2) brush = 2;

		schemeBgColor(this, argb);
		for (int i = 0; i < extent; i++)
		{
			drawFilledRect(sx + i, sy + i, sx + i + brush, sy + i + brush);
			drawFilledRect(sx + i, sy + extent - i - brush, sx + i + brush, sy + extent - i);
		}
	}
};

Frame::Frame(int x, int y, int wide, int tall) : Panel(x, y, wide, tall)
{
	VLOG("Frame ctor: pos(%d,%d) size(%dx%d)", x, y, wide, tall);
	_title[0] = 0;
	_moveable = true;
	_sizeable = true;
	_internal = false;
	_smallCaption = false;
	_dragging = false;
	_resizing = false;
	_lastCursorValid = false;
	_lastCursor[0] = 0;
	_lastCursor[1] = 0;
	_resizeZone = 0;
	_dragOrgPos[0] = 0; _dragOrgPos[1] = 0;
	_dragOrgCursor[0] = 0; _dragOrgCursor[1] = 0;
	_dragOrgSize[0] = 0; _dragOrgSize[1] = 0;
	_dragAnchorReady = false;

	_topGrip = null; _bottomGrip = null; _leftGrip = null; _rightGrip = null;
	_topLeftGrip = null; _topRightGrip = null;
	_bottomLeftGrip = null; _bottomRightGrip = null;
	_minimizeButton = null; _maximizeButton = null;
	_captionBar = null;

	int captionH = Fcap();
	int border = Fborder();
	_client = new Panel(border, captionH + border, wide - border * 2, tall - captionH - border * 2);
	addChild(_client);

	int btnSize = FbtnSz();
	int btnInset = FbtnIns();
	_closeButton = new FrameCloseGlyph(wide - border - btnSize - btnInset,
		border + btnInset, btnSize, btnSize);
	_closeButton->addActionSignal(new FrameCloseSignal(this));
	addChild(_closeButton);
}

void Frame::setTitle(const char* title)
{
	if (title) vgui_strcpy(_title, sizeof(_title), title);
	else _title[0] = 0;
}

void Frame::getTitle(char* buf, int bufLen)
{
	if (buf && bufLen > 0) vgui_strcpy(buf, bufLen, _title);
}

void Frame::setMoveable(bool state) { _moveable = state; }
bool Frame::isMoveable() { return _moveable; }
void Frame::setSizeable(bool state) { _sizeable = state; }
bool Frame::isSizeable() { return _sizeable; }

void Frame::setVisible(bool state)
{
	if (!state && (_dragging || _resizing))
	{
		_dragging = false;
		_resizing = false;
		_resizeZone = 0;
		_lastCursorValid = false;
		App* app = App::getInstance();
		if (app) app->setMouseCapture(null);
	}
	Panel::setVisible(state);
}

Panel* Frame::getClient() { return _client; }
void Frame::setInternal(bool state) { _internal = state; }

void Frame::setSmallCaption(bool state)
{
	_smallCaption = state;
	int captionH = state ? FcapSmall() : Fcap();
	int border = Fborder();
	int wide, tall;
	getSize(wide, tall);
	if (_client)
		_client->setBounds(border, captionH + border, wide - border * 2, tall - captionH - border * 2);
}

void Frame::setSize(int wide, int tall)
{
	Panel::setSize(wide, tall);
	int captionH = _smallCaption ? FcapSmall() : Fcap();
	int border = Fborder();
	if (_client)
		_client->setBounds(border, captionH + border, wide - border * 2, tall - captionH - border * 2);
	if (_closeButton)
	{
		int btnSize = FbtnSz();
		int btnInset = FbtnIns();
		_closeButton->setBounds(wide - border - btnSize - btnInset,
			border + btnInset, btnSize, btnSize);
	}
}

void Frame::paintBackground()
{
	int wide, tall;
	getSize(wide, tall);

	unsigned int frameBg = g_Scheme.frameBgColor ? g_Scheme.frameBgColor : 0xFF4C5844;
	schemeBgColor(this, frameBg);
	drawFilledRect(0, 0, wide, tall);

	int captionH = _smallCaption ? FcapSmall() : Fcap();
	int border = Fborder();

	// Subtle GoldSrc noise/grain over the body region (canonical look - the
	// original CS 1.6 VGUI window has a visible noisy texture, not a flat
	// color; pixel audit shows reference body luminance stddev ~15-25 vs
	// our flat ~1). Position-based hash picks sparse pixels and nudges them
	// brighter/darker. Tighter step (VS(3)) + larger amplitude bring stddev
	// up while staying cheap (~50K pixels at 800x500).
	{
		int bx0 = border;
		int by0 = captionH + border;
		int bx1 = wide - border;
		int by1 = tall - border;
		int areaW = bx1 - bx0;
		int areaH = by1 - by0;
		if (areaW > 16 && areaH > 16)
		{
			int step = VS(3);
			if (step < 2) step = 2;
			unsigned int br = (frameBg >> 16) & 0xFF;
			unsigned int bg = (frameBg >> 8) & 0xFF;
			unsigned int bb = frameBg & 0xFF;
			unsigned int ba = (frameBg >> 24) & 0xFF;
			for (int py = by0; py < by1; py += step)
			{
				for (int px = bx0; px < bx1; px += step)
				{
					unsigned int h = ((unsigned int)px * 2654435761u) ^ ((unsigned int)py * 340573321u);
					int q = h & 0xF;
					int d = 0;
					if (q < 4)        d =  16;  // 25% brighter
					else if (q >= 12) d = -16;  // 25% darker
					if (d == 0) continue;
					int nr = (int)br + d; if (nr < 0) nr = 0; if (nr > 255) nr = 255;
					int ng = (int)bg + d; if (ng < 0) ng = 0; if (ng > 255) ng = 255;
					int nb = (int)bb + d; if (nb < 0) nb = 0; if (nb > 255) nb = 255;
					schemeBgColor(this, (ba << 24) | (nr << 16) | (ng << 8) | nb);
					drawFilledRect(px, py, px + 1, py + 1);
				}
			}
		}
	}

	// Subtle gradient bands
	unsigned int hiBand = g_Scheme.frameHighlightBand ? g_Scheme.frameHighlightBand : 0x40FFFFFF;
	unsigned int loBand = g_Scheme.frameShadowBand    ? g_Scheme.frameShadowBand    : 0x40000000;
	schemeBgColor(this, hiBand);
	drawFilledRect(border, captionH + border, wide - border, captionH + border + 1);
	schemeBgColor(this, loBand);
	drawFilledRect(border, tall - border - 1, wide - border, tall - border);

	unsigned int bright = g_Scheme.borderBright ? g_Scheme.borderBright : 0xFF889180;
	unsigned int dark   = g_Scheme.borderDark   ? g_Scheme.borderDark   : 0xFF282E22;

	// Canonical CS 1.6 RaisedBorder = 1px only:
	// Top + Left = BorderBright; Bottom + Right = BorderDark.
	// No inner-bevel (we used to draw a second band - that was non-canonical).
	schemeBgColor(this, bright);
	drawFilledRect(0, 0, wide, 1);
	drawFilledRect(0, 0, 1, tall);
	schemeBgColor(this, dark);
	drawFilledRect(0, tall - 1, wide, tall);
	drawFilledRect(wide - 1, 0, wide, tall);

	drawTitleBar(wide);

	// Bottom-right resize grip dots
	if (_sizeable)
	{
		schemeBgColor(this, bright);
		int gx = wide - VS(4);
		int gy = tall - VS(4);
		for (int i = 0; i < 3; i++)
		{
			int off = i * VS(3);
			drawFilledRect(gx - off, gy, gx - off + VS(1), gy + VS(1));
			drawFilledRect(gx, gy - off, gx + VS(1), gy - off + VS(1));
		}
	}
}

void Frame::paint() {}

void Frame::drawTitleBar(int wide)
{
	int captionH = _smallCaption ? FcapSmall() : Fcap();
	int border = Fborder();
	int barX = border, barY = border;
	int barW = wide - border * 2, barH = captionH;

	unsigned int titleBg = g_Scheme.frameTitleBarBg ? g_Scheme.frameTitleBarBg : 0xFF4C5844;
	schemeBgColor(this, titleBg);
	drawFilledRect(barX, barY, barX + barW, barY + barH);

	unsigned int topEdge = g_Scheme.frameTitleBarTop ? g_Scheme.frameTitleBarTop : 0xFF889180;
	schemeBgColor(this, topEdge);
	drawFilledRect(barX, barY, barX + barW, barY + 1);

	unsigned int botEdge = g_Scheme.frameTitleBarBottom ? g_Scheme.frameTitleBarBottom : 0xFF282E22;
	schemeBgColor(this, botEdge);
	drawFilledRect(barX, barY + barH - 1, barX + barW, barY + barH);

	static HIMAGE s_steamIcon = (HIMAGE)-1;
	if (s_steamIcon == (HIMAGE)-1)
		s_steamIcon = EngFuncs::PIC_Load("gfx/vgui2/steam_logo.tga");

	int titleTextX = border + VS(4);
	if (s_steamIcon)
	{
		int iconH = barH - VS(4);
		if (iconH < VS(6)) iconH = VS(6);
		int iconW = iconH;
		int iconX = barX + VS(3);
		int iconY = barY + (barH - iconH) / 2;
		int sx = iconX, sy = iconY;
		localToScreen(sx, sy);
		EngFuncs::PIC_Set(s_steamIcon, 255, 255, 255, 255);
		EngFuncs::PIC_DrawTrans(sx, sy, iconW, iconH);
		titleTextX = iconX + iconW + VS(3);
	}

	if (_title[0])
	{
		unsigned int titleFg = g_Scheme.frameTitleBarFg ? g_Scheme.frameTitleBarFg : 0xFFFFFFFF;
		schemeFgColor(this, titleFg);
		drawSetTextFont(Scheme::sf_primary1);
		int textY = barY + (barH - VS(13)) / 2;
		if (textY < barY + 2) textY = barY + 2;
		drawPrintText(titleTextX, textY, _title, (int)strlen(_title));
	}
}

// Robust drag/resize via incremental delta in cursorMoved events.
//
// App routes cursorMoved to the mouse-capture target (set on press), so the
// Frame keeps receiving moves even after the cursor leaves its bounds. The
// delta is measured from the LAST cursor pos (re-synced every move), never
// from a fixed press-time anchor - immune to the Android press/cursor desync
// that made the dialog jump sideways, and it tracks the finger every frame
// (move events fire continuously during a touch drag).
//
//   PRESS:    set flag, _lastCursorValid=false, capture mouse.
//   1st move: seed _lastCursor (no movement).
//   Nth move: delta = cur - _lastCursor; apply; re-sync _lastCursor.
//   RELEASE:  clear flags, release capture.
void Frame::internalCursorMoved(int x, int y)
{
	if (!_dragging && !_resizing)
	{
		Panel::internalCursorMoved(x, y);
		return;
	}

	if (!_lastCursorValid)
	{
		_lastCursor[0] = x;
		_lastCursor[1] = y;
		_lastCursorValid = true;
		VLOG("Frame move: seed cursor (%d,%d), no delta applied yet", x, y);
		Panel::internalCursorMoved(x, y);
		return;
	}

	int dx = x - _lastCursor[0];
	int dy = y - _lastCursor[1];
	if (dx == 0 && dy == 0)
	{
		Panel::internalCursorMoved(x, y);
		return;
	}
	_lastCursor[0] = x;
	_lastCursor[1] = y;
	VLOG("Frame move: cursor=(%d,%d) d=(%+d,%+d) %s",
		x, y, dx, dy, _dragging ? "drag" : "resize");

	if (_dragging && _moveable)
	{
		int newX = _pos[0] + dx;
		int newY = _pos[1] + dy;

		int wide, tall;
		getSize(wide, tall);
		int sw, sh;
		GetCanvasSize(this, sw, sh);

		int captionH = _smallCaption ? FcapSmall() : Fcap();
		int margin = VS(24);
		if (margin > wide) margin = wide;

		// Horizontal: the window may slide partly off the left/right edges, but
		// at least `margin` px must stay on-screen so it can always be grabbed
		// back. newX is the window's left edge in canvas space.
		int minX = margin - wide;   // right edge keeps `margin` visible at left
		int maxX = sw - margin;     // left edge keeps `margin` visible at right
		if (newX < minX) newX = minX;
		if (newX > maxX) newX = maxX;

		// Vertical: the caption bar must stay fully reachable - never hide it
		// above the top, and never push it past the bottom edge.
		int maxY = sh - captionH;
		if (maxY < 0) maxY = 0;
		if (newY < 0) newY = 0;
		if (newY > maxY) newY = maxY;

		setPos(newX, newY);
	}
	else if (_resizing && _sizeable && _resizeZone)
	{
		int wide, tall;
		getSize(wide, tall);
		int sw, sh;
		GetCanvasSize(this, sw, sh);

		int minW = MinW(), minH = MinH();
		int maxW = sw, maxH = sh;            // window can never exceed the screen
		if (minW > maxW) minW = maxW;
		if (minH > maxH) minH = maxH;

		// Work in absolute edge coordinates. Only the grabbed edge(s) move; the
		// opposite edge stays anchored. Each moving edge is clamped so the
		// resulting size is within [min,max] AND the edge stays on-screen -
		// this single formula prevents over-stretch, oversized windows, and
		// pushing the window off the canvas.
		int left   = _pos[0];
		int top    = _pos[1];
		int right  = left + wide;
		int bottom = top  + tall;

		if (_resizeZone & RZ_W)
		{
			left += dx;
			int lo = right - maxW; if (lo < 0) lo = 0; // on-screen + max width
			int hi = right - minW;                     // min width
			if (left < lo) left = lo;
			if (left > hi) left = hi;
		}
		if (_resizeZone & RZ_E)
		{
			right += dx;
			int lo = left + minW;
			int hi = left + maxW; if (hi > sw) hi = sw; // on-screen + max width
			if (right < lo) right = lo;
			if (right > hi) right = hi;
		}
		if (_resizeZone & RZ_N)
		{
			top += dy;
			int lo = bottom - maxH; if (lo < 0) lo = 0;
			int hi = bottom - minH;
			if (top < lo) top = lo;
			if (top > hi) top = hi;
		}
		if (_resizeZone & RZ_S)
		{
			bottom += dy;
			int lo = top + minH;
			int hi = top + maxH; if (hi > sh) hi = sh;
			if (bottom < lo) bottom = lo;
			if (bottom > hi) bottom = hi;
		}

		int newX = left, newY = top;
		int newW = right - left, newH = bottom - top;
		if (newW < minW) newW = minW;        // final safety on tiny canvases
		if (newH < minH) newH = minH;

		// Safety net: if the window was ALREADY larger than the screen or
		// off-screen before this resize (e.g. created oversized, or the
		// resolution shrank), force it back within bounds. For a normal
		// in-screen window this is a no-op (the per-edge clamps already hold).
		if (newW > sw) newW = sw;
		if (newH > sh) newH = sh;
		if (newX + newW > sw) newX = sw - newW;
		if (newY + newH > sh) newY = sh - newH;
		if (newX < 0) newX = 0;
		if (newY < 0) newY = 0;

		if (newX != _pos[0] || newY != _pos[1])
			setPos(newX, newY);
		if (newW != wide || newH != tall)
			setSize(newW, newH);
	}

	Panel::internalCursorMoved(x, y);
}

// Resize edges/corners and the caption bar must win the hit-test over inner
// children (TabPanel, buttons), otherwise a press on a resize grip that
// overlaps the client area would land on a child and Frame would never start
// resizing. We claim those zones here; everywhere else, normal child
// traversal applies (so buttons/tabs/fields stay clickable). The close
// button is checked first so it keeps working.
Panel* Frame::isWithinTraverse(int x, int y)
{
	if (!_visible)
		return null;

	// Close button (a child) keeps priority in its own rect.
	if (_closeButton && _closeButton->isVisible())
	{
		Panel* hit = _closeButton->isWithinTraverse(x, y);
		if (hit)
			return hit;
	}

	int ax = 0, ay = 0;
	localToScreen(ax, ay);
	int lx = x - ax;
	int ly = y - ay;
	int wide = getWide();
	int tall = getTall();

	// Inside the frame at all?
	if (lx >= 0 && lx < wide && ly >= 0 && ly < tall)
	{
		// Resize grips win over children.
		if (_sizeable && HitTestResize(lx, ly, wide, tall) != 0)
			return this;

		// Caption bar (drag zone) wins over children.
		int captionH = _smallCaption ? FcapSmall() : Fcap();
		int border = Fborder();
		if (_moveable && ly >= border && ly < border + captionH &&
			lx >= border && lx < wide - border)
			return this;
	}

	// Otherwise normal front-to-back child traversal.
	return Panel::isWithinTraverse(x, y);
}

void Frame::internalMousePressed(MouseCode code)
{
	if (code == MOUSE_LEFT)
	{
		App* app = App::getInstance();
		if (app)
		{
			int mx, my;
			app->getCursorPos(mx, my);

			int ax = 0, ay = 0;
			localToScreen(ax, ay);

			int lx = mx - ax;
			int ly = my - ay;
			int wide = getWide();
			int tall = getTall();

			// Resize hit-test (priority over titlebar drag)
			int zone = _sizeable ? HitTestResize(lx, ly, wide, tall) : 0;
			if (zone)
			{
				_resizing = true;
				_resizeZone = zone;
				// First cursorMoved seeds _lastCursor; no movement happens
				// until the finger actually moves -> no press-time jump.
				_lastCursorValid = false;
				setAsMouseCapture(true);
				VLOG("Frame press: resize zone=%d at screen(%d,%d) local(%d,%d) frame(%d,%d %dx%d)",
					zone, mx, my, lx, ly, _pos[0], _pos[1], wide, tall);
			}
			else if (_moveable)
			{
				int captionH = _smallCaption ? FcapSmall() : Fcap();
				int border = Fborder();
				if (ly >= border && ly < border + captionH && lx >= border && lx < wide - border)
				{
					_dragging = true;
					_lastCursorValid = false;
					setAsMouseCapture(true);
					VLOG("Frame press: drag at screen(%d,%d) local(%d,%d) frame(%d,%d %dx%d)",
						mx, my, lx, ly, _pos[0], _pos[1], wide, tall);
				}
				else
				{
					VLOG("Frame press: no-action local(%d,%d) caption=[%d..%d]",
						lx, ly, border, border + captionH);
				}
			}
		}
	}
	Panel::internalMousePressed(code);
}

void Frame::internalMouseReleased(MouseCode code)
{
	if (code == MOUSE_LEFT && (_dragging || _resizing))
	{
		VLOG("Frame release: was %s, frame ended at (%d,%d %dx%d)",
			_dragging ? "drag" : "resize",
			_pos[0], _pos[1], getWide(), getTall());
		_dragging = false;
		_resizing = false;
		_resizeZone = 0;
		_lastCursorValid = false;
		setAsMouseCapture(false);
	}
	Panel::internalMouseReleased(code);
}

}
