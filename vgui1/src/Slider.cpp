// Include heavy mainui headers BEFORE VGUI_*.h to avoid the `null` macro clash.
extern void UI_FillRect( int x, int y, int width, int height, const unsigned int color );
#include "TrackerScheme.h"

#include <VGUI_SchemeColors.h>
#include <VGUI_Slider.h>
#include <VGUI_App.h>
#include <VGUI_IntChangeSignal.h>

namespace vgui
{

Slider::Slider(int x, int y, int wide, int tall, bool vertical) : Panel(x, y, wide, tall)
{
	_vertical = vertical;
	_value = 0;
	_range[0] = 0;
	_range[1] = 100;
	_rangeWindow = 0;
	_rangeWindowEnabled = false;
	_nobPos[0] = 0;
	_nobPos[1] = 0;
	_nobSize = 8;
	_nobDragStartPos[0] = 0;
	_nobDragStartPos[1] = 0;
	_dragStartPos[0] = 0;
	_dragStartPos[1] = 0;
	_dragging = false;
	_buttonOffset = 0;

	// Visual colors driven by g_Scheme at draw time
}

void Slider::setValue(int value)
{
	if (value < _range[0]) value = _range[0];
	if (value > _range[1]) value = _range[1];
	_value = value;
	recomputeNobPosFromValue();
	repaint();
}

int Slider::getValue()
{
	return _value;
}

void Slider::setRange(int min, int max)
{
	_range[0] = min;
	_range[1] = max;
	if (_value < min) _value = min;
	if (_value > max) _value = max;
	recomputeNobPosFromValue();
	repaint();
}

void Slider::getRange(int& min, int& max)
{
	min = _range[0];
	max = _range[1];
}

void Slider::setRangeWindow(int rangeWindow)
{
	_rangeWindow = rangeWindow;
}

void Slider::setRangeWindowEnabled(bool state)
{
	_rangeWindowEnabled = state;
}

void Slider::addIntChangeSignal(IntChangeSignal* s)
{
	_intChangeSignalDar.addElement(s);
}

void Slider::fireIntChangeSignal()
{
	for (int i = 0; i < _intChangeSignalDar.getCount(); i++)
	{
		IntChangeSignal* s = _intChangeSignalDar[i];
		if (s)
			s->intChanged(_value, this);
	}
}

bool Slider::isVertical()
{
	return _vertical;
}

void Slider::setSliderSize(int size)
{
	_nobSize = size;
}

int Slider::getSliderSize()
{
	return _nobSize;
}

void Slider::setButtonOffset(int offset)
{
	_buttonOffset = offset;
}

void Slider::recomputeNobPosFromValue()
{
	int wide, tall;
	getSize(wide, tall);

	int range = _range[1] - _range[0];
	if (range <= 0) range = 1;

	if (_vertical)
	{
		int trackLen = tall - _buttonOffset * 2 - _nobSize;
		if (trackLen < 0) trackLen = 0; // safety: tiny slider
		_nobPos[1] = _buttonOffset + (int)((float)(_value - _range[0]) / (float)range * (float)trackLen);
		_nobPos[0] = 0;
	}
	else
	{
		int trackLen = wide - _buttonOffset * 2 - _nobSize;
		if (trackLen < 0) trackLen = 0; // safety: tiny slider
		_nobPos[0] = _buttonOffset + (int)((float)(_value - _range[0]) / (float)range * (float)trackLen);
		_nobPos[1] = 0;
	}
}

void Slider::recomputeValueFromNobPos()
{
	int wide, tall;
	getSize(wide, tall);

	int range = _range[1] - _range[0];
	if (range <= 0) range = 1;

	if (_vertical)
	{
		int trackLen = tall - _buttonOffset * 2 - _nobSize;
		if (trackLen <= 0) trackLen = 1;
		_value = _range[0] + (int)((float)(_nobPos[1] - _buttonOffset) / (float)trackLen * (float)range);
	}
	else
	{
		int trackLen = wide - _buttonOffset * 2 - _nobSize;
		if (trackLen <= 0) trackLen = 1;
		_value = _range[0] + (int)((float)(_nobPos[0] - _buttonOffset) / (float)trackLen * (float)range);
	}

	if (_value < _range[0]) _value = _range[0];
	if (_value > _range[1]) _value = _range[1];
}

void Slider::paintBackground()
{
	int wide, tall;
	getSize(wide, tall);

	unsigned int trackCol = g_Scheme.fieldBgColor ? g_Scheme.fieldBgColor : 0xE655604B;
	unsigned int bright   = g_Scheme.borderBright ? g_Scheme.borderBright : 0xC85F6558;
	unsigned int dark     = g_Scheme.borderDark   ? g_Scheme.borderDark   : 0xC8282C24;

	// Slider sits transparently on the parent panel; only draw the track groove.
	if (_vertical)
	{
		int cx = wide / 2;
		// 3px wide sunken groove
		schemeBgColor(this, trackCol);
		drawFilledRect(cx - 1, _buttonOffset, cx + 2, tall - _buttonOffset);
		schemeBgColor(this, dark);
		drawFilledRect(cx - 1, _buttonOffset, cx, tall - _buttonOffset);
		schemeBgColor(this, bright);
		drawFilledRect(cx + 1, _buttonOffset, cx + 2, tall - _buttonOffset);
	}
	else
	{
		int cy = tall / 2;
		schemeBgColor(this, trackCol);
		drawFilledRect(_buttonOffset, cy - 1, wide - _buttonOffset, cy + 2);
		schemeBgColor(this, dark);
		drawFilledRect(_buttonOffset, cy - 1, wide - _buttonOffset, cy);
		schemeBgColor(this, bright);
		drawFilledRect(_buttonOffset, cy + 1, wide - _buttonOffset, cy + 2);
	}
}

void Slider::paint()
{
	int wide, tall;
	getSize(wide, tall);

	unsigned int nobBg  = g_Scheme.buttonBgColor ? g_Scheme.buttonBgColor : 0xFF5B6350;
	unsigned int bright = g_Scheme.borderBright  ? g_Scheme.borderBright  : 0xC85F6558;
	unsigned int dark   = g_Scheme.borderDark    ? g_Scheme.borderDark    : 0xC8282C24;

	int nx, ny, nw, nh;
	if (_vertical)
	{
		nx = 0;
		ny = _nobPos[1];
		nw = wide;
		nh = _nobSize;
	}
	else
	{
		nx = _nobPos[0];
		ny = 0;
		nw = _nobSize;
		nh = tall;
	}

	// Raised olive nob
	schemeBgColor(this, nobBg);
	drawFilledRect(nx + 1, ny + 1, nx + nw - 1, ny + nh - 1);

	schemeBgColor(this, bright);
	drawFilledRect(nx, ny, nx + nw, ny + 1);
	drawFilledRect(nx, ny, nx + 1, ny + nh);

	schemeBgColor(this, dark);
	drawFilledRect(nx, ny + nh - 1, nx + nw, ny + nh);
	drawFilledRect(nx + nw - 1, ny, nx + nw, ny + nh);
}

void Slider::internalMousePressed(MouseCode code)
{
	if (code == MOUSE_LEFT)
	{
		App* app = App::getInstance();
		if (app)
		{
			int mx, my;
			app->getCursorPos(mx, my);
			screenToLocal(mx, my);

			_dragging = true;
			_dragStartPos[0] = mx;
			_dragStartPos[1] = my;
			_nobDragStartPos[0] = _nobPos[0];
			_nobDragStartPos[1] = _nobPos[1];
			setAsMouseCapture(true);
		}
	}
	Panel::internalMousePressed(code);
}

void Slider::internalMouseReleased(MouseCode code)
{
	if (code == MOUSE_LEFT && _dragging)
	{
		_dragging = false;
		setAsMouseCapture(false);
		fireIntChangeSignal();
	}
	Panel::internalMouseReleased(code);
}

void Slider::internalCursorMoved(int x, int y)
{
	if (_dragging)
	{
		int wide, tall;
		getSize(wide, tall);

		int lx = x, ly = y;
		screenToLocal(lx, ly);

		if (_vertical)
		{
			int dy = ly - _dragStartPos[1];
			_nobPos[1] = _nobDragStartPos[1] + dy;
			int maxPos = tall - _buttonOffset - _nobSize;
			if (_nobPos[1] < _buttonOffset) _nobPos[1] = _buttonOffset;
			if (_nobPos[1] > maxPos) _nobPos[1] = maxPos;
		}
		else
		{
			int dx = lx - _dragStartPos[0];
			_nobPos[0] = _nobDragStartPos[0] + dx;
			int maxPos = wide - _buttonOffset - _nobSize;
			if (_nobPos[0] < _buttonOffset) _nobPos[0] = _buttonOffset;
			if (_nobPos[0] > maxPos) _nobPos[0] = maxPos;
		}

		recomputeValueFromNobPos();
		repaint();
	}
	Panel::internalCursorMoved(x, y);
}

}
