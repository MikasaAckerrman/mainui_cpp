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

	setBgColor(192, 192, 192, 0);
	setFgColor(0, 0, 0, 0);
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
		_nobPos[1] = _buttonOffset + (int)((float)(_value - _range[0]) / (float)range * (float)trackLen);
		_nobPos[0] = 0;
	}
	else
	{
		int trackLen = wide - _buttonOffset * 2 - _nobSize;
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

	// Background
	drawSetColor(192, 192, 192, 0);
	drawFilledRect(0, 0, wide, tall);

	// Track groove (sunken line)
	if (_vertical)
	{
		int cx = wide / 2;
		drawSetColor(128, 128, 128, 0);
		drawFilledRect(cx - 1, _buttonOffset, cx, tall - _buttonOffset);
		drawSetColor(255, 255, 255, 0);
		drawFilledRect(cx, _buttonOffset, cx + 1, tall - _buttonOffset);
	}
	else
	{
		int cy = tall / 2;
		drawSetColor(128, 128, 128, 0);
		drawFilledRect(_buttonOffset, cy - 1, wide - _buttonOffset, cy);
		drawSetColor(255, 255, 255, 0);
		drawFilledRect(_buttonOffset, cy, wide - _buttonOffset, cy + 1);
	}
}

void Slider::paint()
{
	int wide, tall;
	getSize(wide, tall);

	// Draw the nob (raised look)
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

	// Raised nob
	drawSetColor(192, 192, 192, 0);
	drawFilledRect(nx, ny, nx + nw, ny + nh);
	drawSetColor(255, 255, 255, 0);
	drawFilledRect(nx, ny, nx + nw, ny + 1);
	drawFilledRect(nx, ny, nx + 1, ny + nh);
	drawSetColor(64, 64, 64, 0);
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
