// Frame.cpp - GoldSrc CS 1.6 VGUI frame: pixel-perfect look + robust touch drag/resize.
//
// Drag/resize uses incremental-delta tracking: each cursorMoved computes the
// delta from the LAST seen cursor position, not from a stored anchor. This
// eliminates the "jumps to the right" bug caused by stale press-time anchors
// on Android (KEY_DOWN fires before cursor pos updates).

extern void UI_FillRect( int x, int y, int width, int height, const unsigned int color );
#include "TrackerScheme.h"
#include "BMPUtils.h" // tga_t - in-memory TGA for the cached grain texture

#include <VGUI_Log.h>
#include <VGUI_SchemeColors.h>
#include <VGUI_UIScale.h>
#include <VGUI_Frame.h>
#include <VGUI_App.h>
#include <VGUI_Button.h>
#include <VGUI_ActionSignal.h>
#include <VGUI_CvarBridge.h> // VGUI_GetScreenSize - authoritative drawable canvas
#include <VGUI_SurfaceBase.h>
#include <string.h>

namespace vgui
{

// Fallback scheme colors (used when the active scheme doesn't define one).
static const unsigned int FALLBACK_FRAME_BG      = 0xFF4C5844u; // olive body/button bg
static const unsigned int FALLBACK_BORDER_BRIGHT = 0xFF889180u; // raised bevel top-left
static const unsigned int FALLBACK_BORDER_DARK   = 0xFF282E22u; // raised bevel bottom-right
static const unsigned int FALLBACK_TEXT          = 0xFFD8DED3u; // idle glyph/text
static const unsigned int FALLBACK_TEXT_ARMED    = 0xFFFFFFFFu; // armed glyph/text
static const unsigned int FALLBACK_TITLE_FG      = 0xFFFFFFFFu; // title bar text
static const unsigned int FALLBACK_HI_BAND       = 0x40FFFFFFu; // top gradient band
static const unsigned int FALLBACK_LO_BAND       = 0x40000000u; // bottom gradient band

// Fade transition duration in seconds (full 0<->255 alpha sweep).
static const double FADE_DURATION = 0.12;

// Largest believable cursor delta per engine tick (px). Larger deltas are
// engine glitches and are dropped (see internalCursorMoved).
static const int MAX_CURSOR_DELTA = 500;

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

// 1px GoldSrc bevel border around the panel's full extent:
// top+left edges in `tl`, bottom+right edges in `br`.
// Raised look = (BorderBright, BorderDark); sunken = swapped.
static void DrawBevel(Panel* p, int wide, int tall, unsigned int tl, unsigned int br)
{
	SurfaceBase* sb = p ? p->getSurfaceBase() : null;
	if (!sb)
		return;
	schemeBgColor(p, tl);
	sb->drawFilledRect(0, 0, wide, 1);
	sb->drawFilledRect(0, 0, 1, tall);
	schemeBgColor(p, br);
	sb->drawFilledRect(0, tall - 1, wide, tall);
	sb->drawFilledRect(wide - 1, 0, wide, tall);
}

// ---------------------------------------------------------------------------
// Cached GoldSrc grain/noise texture.
//
// The canonical CS 1.6 window body has a visible noisy texture (reference
// luminance stddev ~15-25). We used to draw it as ~50K 1x1 drawFilledRect
// calls per repaint at 800x500 - a measurable hit on weak Android GPUs.
// Instead, build ONE small tileable RGBA texture (same position-hash pattern)
// and tile it over the body with a handful of PIC_DrawTrans calls.
//
// Texture space is base-640x480 pixels (cell step GRAIN_STEP), drawn scaled
// by VS() so the grain size tracks UI scale exactly like the old loop did.
// White dots lighten, black dots darken; alphas are tuned so the effective
// nudge over the olive body color matches the old +/-16 RGB offset.
// ---------------------------------------------------------------------------
static const int GRAIN_TEX_SIZE = 64; // base px, tileable
static const int GRAIN_STEP     = 3;  // base px between noise cells (old VS(3))

static HIMAGE GetGrainTexture()
{
	static HIMAGE s_grain = (HIMAGE)-1;
	if (s_grain != (HIMAGE)-1)
		return s_grain;

	const int bufSize = (int)sizeof(tga_t) + GRAIN_TEX_SIZE * GRAIN_TEX_SIZE * 4;
	unsigned char* buf = new unsigned char[bufSize];
	memset(buf, 0, bufSize);

	tga_t* hdr      = (tga_t*)buf;
	hdr->image_type = 2; // uncompressed true-color
	hdr->width      = GRAIN_TEX_SIZE;
	hdr->height     = GRAIN_TEX_SIZE;
	hdr->pixel_size = 32;
	hdr->attributes = 0x28; // 8-bit alpha, origin upper left

	unsigned char* pixels = buf + sizeof(tga_t);

	// Same position-hash as the old per-pixel loop: 25% bright, 25% dark.
	// Alpha tuned against the olive body (~0x58 luminance): white a=24 gives
	// ~+16, black a=46 gives ~-16 after alpha blending.
	for (int y = 0; y < GRAIN_TEX_SIZE; y += GRAIN_STEP)
	{
		for (int x = 0; x < GRAIN_TEX_SIZE; x += GRAIN_STEP)
		{
			unsigned int h = ((unsigned int)x * 2654435761u) ^ ((unsigned int)y * 340573321u);
			int q = h & 0xF;
			unsigned char lum = 0, a = 0;
			if (q < 4)        { lum = 255; a = 24; } // brighter dot
			else if (q >= 12) { lum = 0;   a = 46; } // darker dot
			else continue;
			int i = (y * GRAIN_TEX_SIZE + x) * 4;
			pixels[i + 0] = lum; // B
			pixels[i + 1] = lum; // G
			pixels[i + 2] = lum; // R
			pixels[i + 3] = a;   // A
		}
	}

	s_grain = EngFuncs::PIC_Load("#frame_grain.tga",
		buf, bufSize, PIC_NOMIPMAP | PIC_NEAREST | PIC_HAS_ALPHA);
	delete[] buf;

	if (!s_grain)
		Con_DPrintf("vgui1/Frame: failed to create #frame_grain.tga noise texture\n");
	return s_grain;
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

		unsigned int bg = g_Scheme.buttonBgColor ? g_Scheme.buttonBgColor : FALLBACK_FRAME_BG;
		schemeBgColor(this, bg);
		drawFilledRect(1, 1, wide - 1, tall - 1);

		// Phase 1-D: canonical CS 1.6 border = 1px ONLY.
		// Sunken (depressed/selected) = inset (BorderDark TL, BorderBright BR).
		// Raised = bevel (BorderBright TL, BorderDark BR). No inner second band.
		unsigned int bright = g_Scheme.borderBright ? g_Scheme.borderBright : FALLBACK_BORDER_BRIGHT;
		unsigned int dark   = g_Scheme.borderDark   ? g_Scheme.borderDark   : FALLBACK_BORDER_DARK;

		if (sunken)
			DrawBevel(this, wide, tall, dark, bright);
		else
			DrawBevel(this, wide, tall, bright, dark);
	}

	virtual void paint()
	{
		int wide, tall;
		getSize(wide, tall);

		unsigned int argb = isArmed()
			? (g_Scheme.buttonArmedTextColor ? g_Scheme.buttonArmedTextColor : FALLBACK_TEXT_ARMED)
			: (g_Scheme.buttonTextColor ? g_Scheme.buttonTextColor : FALLBACK_TEXT);

		int side = (wide < tall ? wide : tall);
		int extent = (side * 55) / 100;
		if (extent < 6) extent = 6;
		int sx = (wide - extent) / 2;
		int sy = (tall - extent) / 2;
		if (isDepressed()) { sx += 1; sy += 1; }
		// PC CS 1.6: close button uses an elegant thin 1px or 2px cross.
		// Scale brush dynamically but keep it thin (1px on small screens, 2px max on high-res)
		// to prevent it from rendering as a thick, blobby, ugly cross.
		int brush = (side >= 24) ? 2 : 1;

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
	_fadeAlpha = 255; _fadingIn = false; _fadingOut = false;
	_fadeStartTime = 0.0;

	_topGrip = null; _bottomGrip = null; _leftGrip = null; _rightGrip = null;
	_topLeftGrip = null; _topRightGrip = null;
	_bottomLeftGrip = null; _bottomRightGrip = null;
	_minimizeButton = null;
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

// Advance the fade state machine from real elapsed time. _fadeAlpha is
// always DERIVED from (now - _fadeStartTime) - never incremented per frame -
// so the animation is FPS-independent and self-healing: even if the engine
// stops repainting mid-fade (menu deactivated, resolution change, missed
// ticks), the very next call from ANY entry point (paintTraverse, setVisible,
// isWithinTraverse) resolves the fade to its correct final state. The window
// can therefore never be left stuck invisible or half-faded.
void Frame::updateFade()
{
	if (!_fadingIn && !_fadingOut)
		return;

	double elapsed = EngFuncs::DoubleTime() - _fadeStartTime;
	if (elapsed < 0.0) elapsed = 0.0; // clock anomaly safety

	int a = (int)(255.0 * (elapsed / FADE_DURATION));
	if (a > 255) a = 255;

	if (_fadingIn)
	{
		_fadeAlpha = a;
		if (a >= 255)
		{
			_fadeAlpha = 255;
			_fadingIn  = false;
		}
	}
	else // _fadingOut
	{
		_fadeAlpha = 255 - a;
		if (a >= 255)
		{
			_fadeAlpha = 0;
			_fadingOut = false;
			Panel::setVisible(false); // actually hide now
		}
	}
}

void Frame::setVisible(bool state)
{
	// Always stop drag/resize when visibility changes.
	if (_dragging || _resizing)
	{
		_dragging = false; _resizing = false;
		_resizeZone = 0;  _lastCursorValid = false;
		App* app = App::getInstance();
		if (app) app->setMouseCapture(null);
	}

	// Resolve any in-flight fade against real time first: if a fade-out
	// already ran to completion (even without repaints), this hides the
	// panel now so the branches below see the true state.
	updateFade();

	if (state)
	{
		if (!_visible)
		{
			// Fresh show: fade in from fully faded.
			_fadingIn  = true;
			_fadingOut = false;
			_fadeAlpha = 0;
			_fadeStartTime = EngFuncs::DoubleTime();
			Panel::setVisible(true);
			repaint();
		}
		else if (_fadingOut)
		{
			// Re-shown mid fade-out: cancel the fade-out and fade back in
			// FROM THE CURRENT alpha (no restart-from-black flash). Back-date
			// the start time so the derived alpha continues seamlessly.
			_fadingOut = false;
			_fadingIn  = true;
			_fadeStartTime = EngFuncs::DoubleTime()
				- FADE_DURATION * ((double)_fadeAlpha / 255.0);
			repaint();
		}
		// else: already fully visible or mid fade-in - nothing to do.
	}
	else
	{
		if (_visible && !_fadingOut)
		{
			// Fade out, then actually hide (in updateFade). If this interrupts
			// a fade-in, start from the current alpha, not from fully opaque.
			double doneFrac = _fadingIn ? (1.0 - (double)_fadeAlpha / 255.0) : 0.0;
			_fadingOut = true;
			_fadingIn  = false;
			_fadeStartTime = EngFuncs::DoubleTime() - FADE_DURATION * doneFrac;
			repaint();
		}
	}
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

	unsigned int frameBg = g_Scheme.frameBgColor ? g_Scheme.frameBgColor : FALLBACK_FRAME_BG;
	schemeBgColor(this, frameBg);
	drawFilledRect(0, 0, wide, tall);

	int captionH = _smallCaption ? FcapSmall() : Fcap();
	int border = Fborder();

	// Subtle GoldSrc noise/grain over the body region (canonical look - the
	// original CS 1.6 VGUI window has a visible noisy texture, not a flat
	// color). Tiled from ONE cached texture (see GetGrainTexture) instead of
	// the old ~50K per-pixel drawFilledRect calls per repaint.
	if (!_dragging && !_resizing)
	{
		int bx0 = border;
		int by0 = captionH + border;
		int bx1 = wide - border;
		int by1 = tall - border;
		if (bx1 - bx0 > 16 && by1 - by0 > 16)
		{
			HIMAGE grain = GetGrainTexture();
			if (grain)
			{
				int tile = VS(GRAIN_TEX_SIZE);
				if (tile < GRAIN_TEX_SIZE) tile = GRAIN_TEX_SIZE;

				int ox = 0, oy = 0;
				localToScreen(ox, oy);

				EngFuncs::PIC_Set(grain, 255, 255, 255, 255);
				for (int ty = by0; ty < by1; ty += tile)
				{
					int drawH = by1 - ty; if (drawH > tile) drawH = tile;
					for (int tx = bx0; tx < bx1; tx += tile)
					{
						int drawW = bx1 - tx; if (drawW > tile) drawW = tile;
						// Clip partial edge tiles in texture space so the
						// grain scale stays uniform at the borders.
						wrect_t rc;
						rc.left   = 0;
						rc.top    = 0;
						rc.right  = GRAIN_TEX_SIZE * drawW / tile;
						rc.bottom = GRAIN_TEX_SIZE * drawH / tile;
						if (rc.right < 1) rc.right = 1;
						if (rc.bottom < 1) rc.bottom = 1;
						EngFuncs::PIC_DrawTrans(ox + tx, oy + ty, drawW, drawH, &rc);
					}
				}
			}
		}
	}

	// Subtle gradient bands
	unsigned int hiBand = g_Scheme.frameHighlightBand ? g_Scheme.frameHighlightBand : FALLBACK_HI_BAND;
	unsigned int loBand = g_Scheme.frameShadowBand    ? g_Scheme.frameShadowBand    : FALLBACK_LO_BAND;
	schemeBgColor(this, hiBand);
	drawFilledRect(border, captionH + border, wide - border, captionH + border + 1);
	schemeBgColor(this, loBand);
	drawFilledRect(border, tall - border - 1, wide - border, tall - border);

	unsigned int bright = g_Scheme.borderBright ? g_Scheme.borderBright : FALLBACK_BORDER_BRIGHT;
	unsigned int dark   = g_Scheme.borderDark   ? g_Scheme.borderDark   : FALLBACK_BORDER_DARK;

	// Canonical CS 1.6 RaisedBorder = 1px only:
	// Top + Left = BorderBright; Bottom + Right = BorderDark.
	// No inner-bevel (we used to draw a second band - that was non-canonical).
	DrawBevel(this, wide, tall, bright, dark);

	drawTitleBar(wide);

	// Bottom-right resize grip dots
	// Bottom-right resize grip: canonical dotted right-triangle (Windows/GoldSrc
	// sizer). Only shown on resizable frames (the fixed Options window hides it).
	if (_sizeable)
	{
		int s    = VS(1); if (s < 1) s = 1;   // dot size
		int step = VS(3); if (step < 2) step = 2;
		int ox = wide - border - VS(2);       // anchor inside the bottom-right border
		int oy = tall - border - VS(2);
		for (int i = 0; i < 3; i++)           // column from the right edge
		{
			for (int j = 0; j < 3 - i; j++)   // dots up this column -> triangle
			{
				int dx = ox - i * step;
				int dy = oy - j * step;
				schemeBgColor(this, dark);    // shadow first (lower-right)
				drawFilledRect(dx + s, dy + s, dx + s + s, dy + s + s);
				schemeBgColor(this, bright);  // bright highlight on top
				drawFilledRect(dx, dy, dx + s, dy + s);
			}
		}
	}
}

void Frame::paintTraverse(bool repaintFlag)
{
	// Advance the fade BEFORE painting: may complete a fade-out and hide
	// the panel (Panel::paintTraverse below then draws nothing).
	updateFade();

	Panel::paintTraverse(repaintFlag);

	// Fade overlay: a semi-transparent black rectangle over the ENTIRE frame,
	// drawn AFTER the children so the whole window (buttons, tabs, client
	// area) fades as one unit - the old paint()-time overlay was painted
	// before the children and left them fully visible over the dimming.
	// fade-in : overlayAlpha goes 255->0 (window appears from black)
	// fade-out: overlayAlpha goes 0->255 (window disappears to black)
	if ((_fadingIn || _fadingOut) && _visible && _surfaceBase)
	{
		int overlayAlpha = 255 - _fadeAlpha;
		if (overlayAlpha > 0)
		{
			// Xash3D VGUI renders with GL_BLEND enabled, so alpha IS applied.
			_surfaceBase->pushMakeCurrent(this, true);
			schemeBgColor(this, ((unsigned int)overlayAlpha << 24) | 0x000000u);
			int wide, tall;
			getSize(wide, tall);
			drawFilledRect(0, 0, wide, tall);
			_surfaceBase->popMakeCurrent(this);
		}
		// Request another pass while animating. Harmless when the engine
		// already redraws every frame; with time-derived alpha a missed
		// repaint can no longer stall the animation.
		repaint();
	}
}

void Frame::drawTitleBar(int wide)
{
	int captionH = _smallCaption ? FcapSmall() : Fcap();
	int border = Fborder();
	int barX = border, barY = border;
	int barW = wide - border * 2, barH = captionH;

	unsigned int titleBg = g_Scheme.frameTitleBarBg ? g_Scheme.frameTitleBarBg : FALLBACK_FRAME_BG;
	schemeBgColor(this, titleBg);
	drawFilledRect(barX, barY, barX + barW, barY + barH);

	unsigned int topEdge = g_Scheme.frameTitleBarTop ? g_Scheme.frameTitleBarTop : FALLBACK_BORDER_BRIGHT;
	schemeBgColor(this, topEdge);
	drawFilledRect(barX, barY, barX + barW, barY + 1);

	unsigned int botEdge = g_Scheme.frameTitleBarBottom ? g_Scheme.frameTitleBarBottom : FALLBACK_BORDER_DARK;
	schemeBgColor(this, botEdge);
	drawFilledRect(barX, barY + barH - 1, barX + barW, barY + barH);

	static HIMAGE s_steamIcon = (HIMAGE)-1;
	if (s_steamIcon == (HIMAGE)-1)
	{
		s_steamIcon = EngFuncs::PIC_Load("gfx/vgui2/steam_logo.tga");
		if (!s_steamIcon)
			Con_DPrintf("vgui1/Frame: missing asset gfx/vgui2/steam_logo.tga - title bar icon disabled\n");
	}

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
		unsigned int titleFg = g_Scheme.frameTitleBarFg ? g_Scheme.frameTitleBarFg : FALLBACK_TITLE_FG;
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

	// Engine on Android emits spurious cursorMoved events with cursor=(0,0)
	// between real touch motion samples (likely a per-frame poll reading
	// stale state when no FINGER_MOTION fired this tick). Without this
	// filter, the (0,0) events seeded with huge negative deltas relative
	// to real touch positions and the next real sample applied an equally
	// huge positive delta - net algebraic round-trip, but resize/drag
	// clamps consume the negative excursion asymmetrically and the frame
	// drifts (typically right-and-down on this device). A finger on an
	// active dialog cannot legitimately be at exactly (0,0); drop them.
	if (x == 0 && y == 0)
	{
		_dragOrgSize[0]++;   // count spurious (0,0) events filtered this drag
		Panel::internalCursorMoved(x, y);
		return;
	}

	if (!_lastCursorValid)
	{
		_lastCursor[0] = x;
		_lastCursor[1] = y;
		_lastCursorValid = true;
		// The algorithm applies NO movement on the seed sample - the window
		// starts tracking from here, not from the press point. Re-anchor the
		// report's finger-start to this seed so finger-delta and window-delta
		// share the same origin; otherwise MISMATCH would carry a constant
		// (press - firstMove) offset that is not real drift.
		_dragOrgCursor[0] = x;
		_dragOrgCursor[1] = y;
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

	// Safety net for any other engine-side glitch that produces a delta
	// larger than physically possible in one engine tick (60 Hz). Finger
	// velocity tops out well under 500 px/tick on a 2800 px display; any
	// larger delta is spurious. Re-seed _lastCursor without applying the
	// move so the next real event measures from the new baseline rather
	// than chasing the bogus jump on its own.
	{
		int adx = dx < 0 ? -dx : dx;
		int ady = dy < 0 ? -dy : dy;
		if (adx > MAX_CURSOR_DELTA || ady > MAX_CURSOR_DELTA)
		{
			_dragOrgSize[1]++;   // count clamped huge-delta events this drag
			_lastCursor[0] = x;
			_lastCursor[1] = y;
			Panel::internalCursorMoved(x, y);
			return;
		}
	}

	_lastCursor[0] = x;
	_lastCursor[1] = y;

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
	// Resolve any in-flight fade first: a fade-out that already ran to
	// completion (even if painting stopped) must not keep eating input.
	updateFade();

	// During an active fade-out the window is closing - don't accept input.
	// This state is now guaranteed transient (time-derived, self-healing).
	if (!_visible || _fadingOut)
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
				_dragOrgCursor[0] = mx; _dragOrgCursor[1] = my;
				_dragOrgPos[0] = _pos[0]; _dragOrgPos[1] = _pos[1];
				_dragOrgSize[0] = 0; _dragOrgSize[1] = 0;
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
					// Report anchors (reuse legacy ABI fields):
					//   _dragOrgCursor = finger reference; seeded to the press
					//     point here as a fallback for taps with no motion,
					//     then refined to the first move sample (the actual
					//     tracking origin) in internalCursorMoved.
					//   _dragOrgPos    = window position at drag start
					//   _dragOrgSize   = [0] dropped spurious(0,0), [1] clamped-huge
					_dragOrgCursor[0] = mx; _dragOrgCursor[1] = my;
					_dragOrgPos[0] = _pos[0]; _dragOrgPos[1] = _pos[1];
					_dragOrgSize[0] = 0; _dragOrgSize[1] = 0;
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
		if (_dragging)
		{
			// By-the-numbers drag verification (no eyeballing):
			//   finger delta  = last real cursor - finger-down anchor
			//   window delta  = current pos       - window-start anchor
			//   expected end  = window-start + finger delta
			//   MISMATCH      = actual end - expected end
			// MISMATCH (0,0) means the window tracked the finger perfectly.
			// A non-zero MISMATCH with dropped/clamped == 0 is a pure
			// edge-clamp (window hit a screen border); with dropped/clamped
			// > 0 it quantifies drift the engine-glitch filter absorbed.
			int fsx = _dragOrgCursor[0], fsy = _dragOrgCursor[1];
			int fex = _lastCursorValid ? _lastCursor[0] : fsx;
			int fey = _lastCursorValid ? _lastCursor[1] : fsy;
			int fdx = fex - fsx, fdy = fey - fsy;
			int wsx = _dragOrgPos[0], wsy = _dragOrgPos[1];
			int wex = _pos[0], wey = _pos[1];
			int wdx = wex - wsx, wdy = wey - wsy;
			int eex = wsx + fdx, eey = wsy + fdy;
			int mmx = wdx - fdx, mmy = wdy - fdy;
			VLOG("Frame DRAG report: finger start(%d,%d) end(%d,%d) d(%+d,%+d) | "
				"window start(%d,%d) end(%d,%d) d(%+d,%+d) | expected end(%d,%d) | "
				"MISMATCH(%+d,%+d) | dropped(0,0)=%d clampedHuge=%d",
				fsx, fsy, fex, fey, fdx, fdy,
				wsx, wsy, wex, wey, wdx, wdy,
				eex, eey, mmx, mmy, _dragOrgSize[0], _dragOrgSize[1]);
		}
		else
		{
			VLOG("Frame RESIZE report: frame ended at (%d,%d %dx%d) | "
				"dropped(0,0)=%d clampedHuge=%d",
				_pos[0], _pos[1], getWide(), getTall(),
				_dragOrgSize[0], _dragOrgSize[1]);
		}
		_dragging = false;
		_resizing = false;
		_resizeZone = 0;
		_lastCursorValid = false;
		setAsMouseCapture(false);
	}
	Panel::internalMouseReleased(code);
}

}
