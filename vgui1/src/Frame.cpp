#include <VGUI_Frame.h>
#include <VGUI_App.h>
#include <VGUI_Button.h>
#include <VGUI_ActionSignal.h>
#include <VGUI_RaisedBorder.h>
#include <string.h>

namespace vgui
{

// GoldSrc Frame layout constants
enum
{
	FRAME_CAPTION_HEIGHT = 20,
	FRAME_CAPTION_HEIGHT_SMALL = 16,
	FRAME_BORDER = 3,
	FRAME_BUTTON_SIZE = 16,
	FRAME_BUTTON_INSET = 2
};

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

Frame::Frame(int x, int y, int wide, int tall) : Panel(x, y, wide, tall)
{
	_title[0] = 0;
	_moveable = true;
	_sizeable = true;
	_internal = false;
	_smallCaption = false;
	_dragging = false;
	_dragOrgPos[0] = 0;
	_dragOrgPos[1] = 0;
	_dragOrgCursor[0] = 0;
	_dragOrgCursor[1] = 0;

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
	int captionH = FRAME_CAPTION_HEIGHT;
	int border = FRAME_BORDER;
	_client = new Panel(border, captionH + border, wide - border * 2, tall - captionH - border * 2);
	addChild(_client);

	// Caption bar panel (invisible, used for hit-testing)
	_captionBar = new Panel(border, border, wide - border * 2, captionH);
	addChild(_captionBar);

	// Close button
	int btnSize = FRAME_BUTTON_SIZE;
	_closeButton = new Button("X", wide - border - btnSize - FRAME_BUTTON_INSET, border + FRAME_BUTTON_INSET, btnSize, btnSize);
	_closeButton->addActionSignal(new FrameCloseSignal(this));
	addChild(_closeButton);

	// Set raised border for the frame
	setBorder(new RaisedBorder());
	setBgColor(192, 192, 192, 0);
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
	// Relayout client
	int captionH = state ? FRAME_CAPTION_HEIGHT_SMALL : FRAME_CAPTION_HEIGHT;
	int border = FRAME_BORDER;
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

	// Reposition/resize internal child panels for the new dialog size
	int captionH = _smallCaption ? FRAME_CAPTION_HEIGHT_SMALL : FRAME_CAPTION_HEIGHT;
	int border = FRAME_BORDER;

	if (_client)
		_client->setBounds(border, captionH + border, wide - border * 2, tall - captionH - border * 2);
	if (_captionBar)
		_captionBar->setBounds(border, border, wide - border * 2, captionH);
	if (_closeButton)
	{
		int btnSize = FRAME_BUTTON_SIZE;
		_closeButton->setBounds(wide - border - btnSize - FRAME_BUTTON_INSET,
			border + FRAME_BUTTON_INSET, btnSize, btnSize);
	}
}

void Frame::paintBackground()
{
	int wide, tall;
	getSize(wide, tall);

	// Fill frame background (GoldSrc gray)
	drawSetColor(192, 192, 192, 0);
	drawFilledRect(0, 0, wide, tall);

	// Draw title bar
	drawTitleBar(wide);
}

void Frame::paint()
{
}

void Frame::drawTitleBar(int wide)
{
	int captionH = _smallCaption ? FRAME_CAPTION_HEIGHT_SMALL : FRAME_CAPTION_HEIGHT;
	int border = FRAME_BORDER;

	// Title bar background - GoldSrc dark blue when active
	drawSetColor(0, 0, 128, 0);
	drawFilledRect(border, border, wide - border, border + captionH);

	// Title text - white on blue
	if (_title[0])
	{
		drawSetTextColor(255, 255, 255, 0);
		drawSetTextFont(Scheme::sf_primary1);
		drawPrintText(border + 4, border + 2, _title, (int)strlen(_title));
	}
}

void Frame::internalCursorMoved(int x, int y)
{
	if (_dragging && _moveable)
	{
		int dx = x - _dragOrgCursor[0];
		int dy = y - _dragOrgCursor[1];
		setPos(_dragOrgPos[0] + dx, _dragOrgPos[1] + dy);
	}

	Panel::internalCursorMoved(x, y);
}

void Frame::internalMousePressed(MouseCode code)
{
	if (code == MOUSE_LEFT && _moveable)
	{
		App* app = App::getInstance();
		if (app)
		{
			int mx, my;
			app->getCursorPos(mx, my);

			// Get absolute position of this frame
			int ax = 0, ay = 0;
			localToScreen(ax, ay);

			// Convert cursor to local frame coords
			int lx = mx - ax;
			int ly = my - ay;

			int captionH = _smallCaption ? FRAME_CAPTION_HEIGHT_SMALL : FRAME_CAPTION_HEIGHT;
			int border = FRAME_BORDER;

			if (ly >= border && ly < border + captionH && lx >= border && lx < getWide() - border)
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

	Panel::internalMousePressed(code);
}

void Frame::internalMouseReleased(MouseCode code)
{
	if (code == MOUSE_LEFT && _dragging)
	{
		_dragging = false;
		setAsMouseCapture(false);
	}

	Panel::internalMouseReleased(code);
}

}
