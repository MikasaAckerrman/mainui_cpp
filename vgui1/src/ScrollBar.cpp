#include <VGUI_ScrollBar.h>
#include <VGUI_Button.h>
#include <VGUI_Slider.h>
#include <VGUI_IntChangeSignal.h>

namespace vgui
{

ScrollBar::ScrollBar(int x, int y, int wide, int tall, bool vertical) : Panel(x, y, wide, tall)
{
	_vertical = vertical;
	_button[0] = null;
	_button[1] = null;
	_slider = null;

	int btnSize = vertical ? wide : tall;

	// Create default slider
	if (vertical)
		_slider = new Slider(0, btnSize, wide, tall - btnSize * 2, true);
	else
		_slider = new Slider(btnSize, 0, wide - btnSize * 2, tall, false);

	addChild(_slider);
}

void ScrollBar::setValue(int value)
{
	if (_slider)
		_slider->setValue(value);
}

int ScrollBar::getValue()
{
	if (_slider)
		return _slider->getValue();
	return 0;
}

void ScrollBar::setRange(int min, int max)
{
	if (_slider)
		_slider->setRange(min, max);
}

void ScrollBar::setRangeWindow(int rangeWindow)
{
	if (_slider)
		_slider->setRangeWindow(rangeWindow);
}

void ScrollBar::setRangeWindowEnabled(bool state)
{
	if (_slider)
		_slider->setRangeWindowEnabled(state);
}

void ScrollBar::addIntChangeSignal(IntChangeSignal* s)
{
	_intChangeSignalDar.addElement(s);
}

void ScrollBar::fireIntChangeSignal()
{
	for (int i = 0; i < _intChangeSignalDar.getCount(); i++)
	{
		IntChangeSignal* s = _intChangeSignalDar[i];
		if (s)
			s->intChanged(getValue(), this);
	}
}

bool ScrollBar::isVertical()
{
	return _vertical;
}

void ScrollBar::setButton(Button* button, int index)
{
	if (index >= 0 && index < 2)
	{
		_button[index] = button;
		if (button)
			addChild(button);
	}
}

Button* ScrollBar::getButton(int index)
{
	if (index >= 0 && index < 2)
		return _button[index];
	return null;
}

void ScrollBar::setSlider(Slider* slider)
{
	_slider = slider;
	if (_slider)
		addChild(_slider);
}

Slider* ScrollBar::getSlider()
{
	return _slider;
}

}
