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

// GoldSrc VGUI Frame layout constants (pixel-perfect CS 1.6 reference @ 640x480).
// All scaled via VS() at runtime for HD/4K.
enum
{
	FRAME_CAPTION_HEIGHT       = 18,   // GoldSrc: compact 18px titlebar
	FRAME_CAPTION_HEIGHT_SMALL = 14,
	FRAME_BORDER               = 3,    // outer frame edge thickness
	FRAME_BUTTON_SIZE          = 14,   // close button = small square
	FRAME_BUTTON_INSET         = 2
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

// GoldSrc close button: vector X with double bevel border (raised look).
// Pressed state shifts glyph +1/+1 and inverts bevel. Matches PC CS 1.6.
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

		// Body fill
		unsigned int bg = g_Scheme.buttonBgColor ? g_Scheme.buttonBgColor : 0xFF5B6350;
		schemeBgColor(this, bg);
		drawFilledRect(1, 1, wide - 1, tall - 1);

		// Double bevel: outer 1px + inner 1px
		unsigned int bright = g_Scheme.borderBright ? g_Scheme.borderBright : 0xC87A8070;
		unsigned int dark   = g_Scheme.borderDark   ? g_Scheme.borderDark   : 0xC8282C24;
		unsigned int mid    = 0xC84A5040; // mid-tone for inner bevel

		if (sunken)
		{
			// Outer: dark top+left, bright bottom+right
			schemeBgColor(this, dark);
			drawFilledRect(0, 0, wide, 1);
			drawFilledRect(0, 0, 1, tall);
			schemeBgColor(this, bright);
			drawFilledRect(0, tall - 1, wide, tall);
			drawFilledRect(wide - 1, 0, wide, tall);
			// Inner
			schemeBgColor(this, mid);
			drawFilledRect(1, 1, wide - 1, 2);
			drawFilledRect(1, 1, 2, tall - 1);
		}
		else
		{
			// Outer: bright top+left, dark bottom+right
			schemeBgColor(this, bright);
			drawFilledRect(0, 0, wide, 1);
			drawFilledRect(0, 0, 1, tall);
			schemeBgColor(this, dark);
			drawFilledRect(0, tall - 1, wide, tall);
			drawFilledRect(wide - 1, 0, wide, tall);
			// Inner highlight/shadow
			schemeBgColor(this, 0xC8909880);
			drawFilledRect(1, 1, wide - 1, 2);
			drawFilledRect(1, 1, 2, tall - 1);
			schemeBgColor(this, mid);
			drawFilledRect(1, tall - 2, wide - 1, tall - 1);
			drawFilledRect(wide - 2, 1, wide - 1, tall - 1);
		}
	}

	virtual void paint()
	{
		int wide, tall;
		getSize(wide, tall);

		unsigned int argb;
		if (!isEnabled())
			argb = g_Scheme.labelDimColor ? g_Scheme.labelDimColor : 0xFFA0A0A0;
		else if (isArmed())
			argb = g_Scheme.buttonArmedTextColor ? g_Scheme.buttonArmedTextColor : 0xFFFFFFFF;
		else
			argb = g_Scheme.buttonTextColor ? g_Scheme.buttonTextColor : 0xFFE0E0E0;

		// Chunky X glyph: 2px brush, ~60% of button size
		int side = (wide < tall ? wide : tall);
		int extent = (side * 6) / 10;
		if (extent < 5) extent = 5;
		int sx = (wide - extent) / 2;
		int sy = (tall - extent) / 2;
		if (isDepressed())
		{
			sx += 1;
			sy += 1;
		}
		int brush = VS(2);
		if (brush < 1) brush = 1;

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
	_dragAnchorReady = false;
	_dragOrgPos[0] = 0;
	_dragOrgPos[1] = 0;
	_dragOrgCursor[0] = 0;
	_dragOrgCursor[1] = 0;
	_dragOrgSize[0] = 0;
	_dragOrgSize[1] = 0;

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

	int captionH = Fcap();
	int border = Fborder();
	_client = new Panel(border, captionH + border, wide - border * 2, tall - captionH - border * 2);
	addChild(_client);

	// _captionBar kept as null (ABI), drag is handled by Frame::internalMousePressed
	_captionBar = null;

	// Close button: beveled square with vector X
	int btnSize = FbtnSz();
	int btnInset = FbtnIns();
	_closeButton = new FrameCloseGlyph(wide - border - btnSize - btnInset,
		border + btnInset, btnSize, btnSize);
	_closeButton->addActionSignal(new FrameCloseSignal(this));
	addChild(_closeButton);
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
		_dragAnchorReady = false;
		App* app = App::getInstance();
		if (app)
			app->setMouseCapture(null);
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

	// Frame body fill - GoldSrc warm olive
	unsigned int frameBg = g_Scheme.frameBgColor ? g_Scheme.frameBgColor : 0xE6646E50;
	schemeBgColor(this, frameBg);
	drawFilledRect(0, 0, wide, tall);

	// GoldSrc double-bevel outer frame border:
	// Outer ring: bright top+left, dark bottom+right
	// Inner ring (1px inset): slightly dimmer highlight/shadow
	unsigned int bright = g_Scheme.borderBright ? g_Scheme.borderBright : 0xC87A8070;
	unsigned int dark   = g_Scheme.borderDark   ? g_Scheme.borderDark   : 0xC8282C24;

	// Outer bevel
	schemeBgColor(this, bright);
	drawFilledRect(0, 0, wide, 1);
	drawFilledRect(0, 0, 1, tall);
	schemeBgColor(this, dark);
	drawFilledRect(0, tall - 1, wide, tall);
	drawFilledRect(wide - 1, 0, wide, tall);

	// Inner bevel (1px inside)
	unsigned int innerBright = 0xC8909880;
	unsigned int innerDark   = 0xC83A3E30;
	schemeBgColor(this, innerBright);
	drawFilledRect(1, 1, wide - 1, 2);
	drawFilledRect(1, 1, 2, tall - 1);
	schemeBgColor(this, innerDark);
	drawFilledRect(1, tall - 2, wide - 1, tall - 1);
	drawFilledRect(wide - 2, 1, wide - 1, tall - 1);

	drawTitleBar(wide);

	// Bottom-right resize grip
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

	// Title bar background - slightly darker than frame body
	unsigned int titleBg = g_Scheme.frameTitleBarBg ? g_Scheme.frameTitleBarBg : 0xE6586248;
	schemeBgColor(this, titleBg);
	drawFilledRect(barX, barY, barX + barW, barY + barH);

	// Top highlight
	unsigned int topEdge = g_Scheme.frameTitleBarTop ? g_Scheme.frameTitleBarTop : 0xFF7A8268;
	schemeBgColor(this, topEdge);
	drawFilledRect(barX, barY, barX + barW, barY + 1);

	// Bottom separator (dark line between titlebar and client)
	unsigned int botEdge = g_Scheme.frameTitleBarBottom ? g_Scheme.frameTitleBarBottom : 0xFF3A4030;
	schemeBgColor(this, botEdge);
	drawFilledRect(barX, barY + barH - 1, barX + barW, barY + barH);

	// Steam logo icon
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

	// Title text - positioned 1px higher for GoldSrc look
	if (_title[0])
	{
		unsigned int titleFg = g_Scheme.frameTitleBarFg ? g_Scheme.frameTitleBarFg : 0xFFFFFFFF;
		schemeFgColor(this, titleFg);
		drawSetTextFont(Scheme::sf_primary1);
		int textY = barY + VS(2);
		drawPrintText(titleTextX, textY, _title, (int)strlen(_title));
	}
}

void Frame::internalCursorMoved(int x, int y)
{
	if (_dragging || _resizing)
	{
		if (!_dragAnchorReady)
		{
			_dragOrgCursor[0] = x;
			_dragOrgCursor[1] = y;
			_dragOrgPos[0] = _pos[0];
			_dragOrgPos[1] = _pos[1];
			int wide, tall;
			getSize(wide, tall);
			_dragOrgSize[0] = wide;
			_dragOrgSize[1] = tall;
			_dragAnchorReady = true;
			Panel::internalCursorMoved(x, y);
			return;
		}
	}

	if (_dragging && _moveable)
	{
		int dx = x - _dragOrgCursor[0];
		int dy = y - _dragOrgCursor[1];
		int newX = _dragOrgPos[0] + dx;
		int newY = _dragOrgPos[1] + dy;

		int wide, tall;
		getSize(wide, tall);
		Panel* p = getParent();
		int rootW = 0, rootH = 0;
		if (p) p->getSize(rootW, rootH);
		if (rootW > 0 && rootH > 0)
		{
			int margin = VS(24);
			if (newX < -wide + margin) newX = -wide + margin;
			if (newX > rootW - margin) newX = rootW - margin;
			if (newY < 0) newY = 0;
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

			int grip = VS(14);
			if (_sizeable && lx >= wide - grip && lx < wide && ly >= tall - grip && ly < tall)
			{
				_resizing = true;
				_dragAnchorReady = false;
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
					_dragAnchorReady = false;
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
		_dragAnchorReady = false;
		setAsMouseCapture(false);
	}

	Panel::internalMouseReleased(code);
}

}
