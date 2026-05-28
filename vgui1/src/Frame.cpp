// Include heavy mainui headers BEFORE VGUI_*.h to avoid the `null` macro clash.
extern void UI_FillRect( int x, int y, int width, int height, const unsigned int color );
#include "TrackerScheme.h"

#include <VGUI_Log.h>
#include <VGUI_SchemeColors.h>
#include <VGUI_UIScale.h>
#include <VGUI_Frame.h>
#include <VGUI_App.h>
#include <VGUI_Button.h>
#include <VGUI_ActionSignal.h>
#include <string.h>

namespace vgui
{

// Base GoldSrc Frame layout constants. Scaled at runtime via VS() so the
// dialog stays readable on HD/4K Android screens.
enum
{
	FRAME_CAPTION_HEIGHT = 22,
	FRAME_CAPTION_HEIGHT_SMALL = 18,
	FRAME_BORDER = 3,
	FRAME_BUTTON_SIZE = 16,
	FRAME_BUTTON_INSET = 3
};

static inline int Fcap()      { return VS(FRAME_CAPTION_HEIGHT); }
static inline int FcapSmall() { return VS(FRAME_CAPTION_HEIGHT_SMALL); }
static inline int Fborder()   { return VS(FRAME_BORDER); }
static inline int FbtnSz()    { return VS(FRAME_BUTTON_SIZE); }
static inline int FbtnIns()   { return VS(FRAME_BUTTON_INSET); }

// Private close action signal
class FrameCloseSignal : public ActionSignal
{
public:
	FrameCloseSignal(Frame* frame) : _frame(frame) {}
	virtual void actionPerformed(Panel* panel)
	{
		if (_frame)
			_frame->setVisible(false);
	}
private:
	Frame* _frame;
};

// Title-bar close button -- draws a vector X (two diagonal strokes) instead
// of using a font glyph. Matches PC CS 1.6 close-button look. Inherits all
// hover / pressed feedback from Button.
class FrameCloseGlyph : public Button
{
public:
	FrameCloseGlyph(int x, int y, int w, int h) : Button("", x, y, w, h) {}
protected:
	virtual void paint()
	{
		int wide, tall;
		getSize(wide, tall);

		// Same color logic as Button::paint, no text drawn.
		unsigned int argb;
		if (!isEnabled())
			argb = g_Scheme.labelDimColor ? g_Scheme.labelDimColor : 0xFFA0A0A0;
		else if (isArmed())
			argb = g_Scheme.buttonArmedTextColor ? g_Scheme.buttonArmedTextColor : 0xFFFFFFFF;
		else
			argb = g_Scheme.buttonTextColor ? g_Scheme.buttonTextColor : 0xFFFFFFFF;

		// Diagonal extent: roughly 60% of button size, centered. Stroke is a
		// 2px square brush stepped along the diagonal -- gives a chunky pixel
		// X that matches GoldSrc bitmap close icon at any scale.
		int side = (wide < tall ? wide : tall);
		int extent = (side * 6) / 10;
		if (extent < 6) extent = 6;
		int sx = (wide - extent) / 2;
		int sy = (tall - extent) / 2;
		int brush = VS(2);
		if (brush < 2) brush = 2;

		schemeBgColor(this, argb);
		for (int i = 0; i < extent; i++)
		{
			// '\\' stroke: top-left -> bottom-right
			drawFilledRect(sx + i, sy + i, sx + i + brush, sy + i + brush);
			// '/' stroke: bottom-left -> top-right
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
	_dragOrgPos[0] = 0;
	_dragOrgPos[1] = 0;
	_dragOrgCursor[0] = 0;
	_dragOrgCursor[1] = 0;
	_dragOrgSize[0] = 0;
	_dragOrgSize[1] = 0;

	// Create grips (null for now - drag handled directly)
	_topGrip = null;
	_bottomGrip = null;
	_leftGrip = null;
	_rightGrip = null;
	_topLeftGrip = null;
	_topRightGrip = null;
	_bottomLeftGrip = null;
	_bottomRightGrip = null;
	_minimizeButton = null;
	_maximizeButton = null;

	// Create client area panel
	int captionH = Fcap();
	int border = Fborder();
	_client = new Panel(border, captionH + border, wide - border * 2, tall - captionH - border * 2);
	addChild(_client);

	// Caption bar panel removed: as a child Panel covering the title-bar
	// area, it stole hit-test ownership from Frame (App::updateMouseFocus
	// picks the deepest child), so Frame::internalMousePressed never ran
	// and drag could not start. Frame::internalMousePressed already does
	// its own caption-zone hit-test, so the helper panel is unnecessary.
	_captionBar = null;

	// Close button: vector X glyph instead of font character (PC CS 1.6 look)
	int btnSize = FbtnSz();
	int btnInset = FbtnIns();
	_closeButton = new FrameCloseGlyph(wide - border - btnSize - btnInset,
		border + btnInset, btnSize, btnSize);
	_closeButton->addActionSignal(new FrameCloseSignal(this));
	addChild(_closeButton);

	// CS 1.6 frame has a flat panel + thin scheme-driven border drawn in
	// paintBackground; no chunky RaisedBorder around the frame itself.
}

void Frame::setTitle(const char* title)
{
	if (title)
		vgui_strcpy(_title, sizeof(_title), title);
	else
		_title[0] = 0;
}

void Frame::getTitle(char* buf, int bufLen)
{
	if (buf && bufLen > 0)
		vgui_strcpy(buf, bufLen, _title);
}

void Frame::setMoveable(bool state)
{
	_moveable = state;
}

bool Frame::isMoveable()
{
	return _moveable;
}

void Frame::setSizeable(bool state)
{
	_sizeable = state;
}

bool Frame::isSizeable()
{
	return _sizeable;
}

void Frame::setVisible(bool state)
{
	if (!state && (_dragging || _resizing))
	{
		// Drop any active drag/resize so a hidden frame does not leak its
		// state into the next show. Without this, _dragging stays true
		// after closing mid-drag and the next mousemove on reopen (e.g.
		// tapping a button) would teleport the dialog.
		_dragging = false;
		_resizing = false;
		App* app = App::getInstance();
		if (app)
			app->setMouseCapture(null);
	}
	Panel::setVisible(state);
}

Panel* Frame::getClient()
{
	return _client;
}

void Frame::setInternal(bool state)
{
	_internal = state;
}

void Frame::setSmallCaption(bool state)
{
	_smallCaption = state;
	int captionH = state ? FcapSmall() : Fcap();
	int border = Fborder();
	int wide, tall;
	getSize(wide, tall);
	if (_client)
		_client->setBounds(border, captionH + border, wide - border * 2, tall - captionH - border * 2);
	if (_captionBar)
		_captionBar->setBounds(border, border, wide - border * 2, captionH);
}

void Frame::setSize(int wide, int tall)
{
	Panel::setSize(wide, tall);

	int captionH = _smallCaption ? FcapSmall() : Fcap();
	int border = Fborder();

	if (_client)
		_client->setBounds(border, captionH + border, wide - border * 2, tall - captionH - border * 2);
	if (_captionBar)
		_captionBar->setBounds(border, border, wide - border * 2, captionH);
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

	// Frame body - olive panel from CS 1.6 scheme
	schemeBgColor(this, g_Scheme.frameBgColor ? g_Scheme.frameBgColor : 0xE65F684E);
	drawFilledRect(0, 0, wide, tall);

	// Subtle dark border around the frame (1px on right + bottom = drop shadow look)
	unsigned int borderDark = g_Scheme.borderDark ? g_Scheme.borderDark : 0xC8282C24;
	schemeBgColor(this, borderDark);
	drawFilledRect(0, 0, wide, 1);          // top
	drawFilledRect(0, 0, 1, tall);          // left
	drawFilledRect(0, tall - 1, wide, tall); // bottom
	drawFilledRect(wide - 1, 0, wide, tall); // right

	drawTitleBar(wide);

	// Bottom-right resize grip: 3 diagonal lines (CS 1.6 PC look)
	if (_sizeable)
	{
		unsigned int bright = g_Scheme.borderBright ? g_Scheme.borderBright : 0xC85F6558;
		schemeBgColor(this, bright);
		int gx = wide - VS(3);
		int gy = tall - VS(3);
		for (int i = 0; i < 3; i++)
		{
			int off = i * VS(4);
			drawFilledRect(gx - off, gy, gx - off + VS(2), gy + VS(2));
			drawFilledRect(gx, gy - off, gx + VS(2), gy - off + VS(2));
		}
	}
}

void Frame::paint()
{
}

void Frame::drawTitleBar(int wide)
{
	int captionH = _smallCaption ? FcapSmall() : Fcap();
	int border = Fborder();

	int barX = border;
	int barY = border;
	int barW = wide - border * 2;
	int barH = captionH;

	// Title bar background - dark olive/gray from scheme
	schemeBgColor(this, g_Scheme.frameTitleBarBg ? g_Scheme.frameTitleBarBg : 0xE65F684F);
	drawFilledRect(barX, barY, barX + barW, barY + barH);

	// Top highlight edge (subtle 1px lighter line at the very top of title bar)
	if (g_Scheme.frameTitleBarTop)
	{
		schemeBgColor(this, g_Scheme.frameTitleBarTop);
		drawFilledRect(barX, barY, barX + barW, barY + 1);
	}

	// Bottom separator edge between title bar and panel body
	if (g_Scheme.frameTitleBarBottom)
	{
		schemeBgColor(this, g_Scheme.frameTitleBarBottom);
		drawFilledRect(barX, barY + barH - 1, barX + barW, barY + barH);
	}

	// Title text
	if (_title[0])
	{
		schemeFgColor(this, g_Scheme.frameTitleBarFg ? g_Scheme.frameTitleBarFg : 0xFFFFFFFF);
		drawSetTextFont(Scheme::sf_primary1);
		drawPrintText(border + VS(6), border + VS(4), _title, (int)strlen(_title));
	}
}

void Frame::internalCursorMoved(int x, int y)
{
	if (_dragging && _moveable)
	{
		int dx = x - _dragOrgCursor[0];
		int dy = y - _dragOrgCursor[1];
		int newX = _dragOrgPos[0] + dx;
		int newY = _dragOrgPos[1] + dy;

		// Clamp so the title bar stays grabbable: never let the dialog leave
		// the screen entirely. Allows partial off-screen on left/right/bottom
		// for monitor-edge use, but keeps the caption row visible at top.
		int wide, tall;
		getSize(wide, tall);
		Panel* p = getParent();
		int rootW = 0, rootH = 0;
		if (p) p->getSize(rootW, rootH);
		if (rootW > 0 && rootH > 0)
		{
			int margin = VS(24); // keep at least this many px of caption visible (scaled for HD)
			if (newX < -wide + margin) newX = -wide + margin;
			if (newX > rootW - margin) newX = rootW - margin;
			if (newY < 0) newY = 0;                    // never above top
			if (newY > rootH - margin) newY = rootH - margin;
		}

		setPos(newX, newY);
	}
	else if (_resizing && _sizeable)
	{
		int dx = x - _dragOrgCursor[0];
		int dy = y - _dragOrgCursor[1];
		int newW = _dragOrgSize[0] + dx;
		int newH = _dragOrgSize[1] + dy;
		// Keep dialog bigger than its bottom button row + tab strip
		int minW = VS(360);
		int minH = VS(240);
		if (newW < minW) newW = minW;
		if (newH < minH) newH = minH;
		setSize(newW, newH);
	}

	Panel::internalCursorMoved(x, y);
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

			// Bottom-right resize grip area takes priority over body clicks.
			int grip = VS(14);
			if (_sizeable && lx >= wide - grip && lx < wide && ly >= tall - grip && ly < tall)
			{
				_resizing = true;
				_dragOrgSize[0] = wide;
				_dragOrgSize[1] = tall;
				_dragOrgCursor[0] = mx;
				_dragOrgCursor[1] = my;
				setAsMouseCapture(true);
			}
			else if (_moveable)
			{
				int captionH = _smallCaption ? FcapSmall() : Fcap();
				int border = Fborder();

				if (ly >= border && ly < border + captionH && lx >= border && lx < wide - border)
				{
					_dragging = true;
					_dragOrgPos[0] = _pos[0];
					_dragOrgPos[1] = _pos[1];
					_dragOrgCursor[0] = mx;
					_dragOrgCursor[1] = my;
					setAsMouseCapture(true);
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
		_dragging = false;
		_resizing = false;
		setAsMouseCapture(false);
	}

	Panel::internalMouseReleased(code);
}

}
