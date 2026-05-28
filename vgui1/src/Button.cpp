#include <VGUI_Button.h>
#include <VGUI_ActionSignal.h>
#include <string.h>

namespace vgui
{

//-------------------------------------------------------------------
// ButtonGroup
//-------------------------------------------------------------------
ButtonGroup::ButtonGroup()
{
}

void ButtonGroup::addButton(Button* button)
{
	_buttonDar.addElement(button);
}

void ButtonGroup::setSelected(Button* button)
{
	for (int i = 0; i < _buttonDar.getCount(); i++)
	{
		Button* b = _buttonDar[i];
		if (b && b != button)
			b->setSelected(false);
	}
	if (button)
		button->setSelected(true);
}

//-------------------------------------------------------------------
// Button
//-------------------------------------------------------------------
Button::Button(const char* text, int x, int y) : Label(text, x, y, 72, 24)
{
	init();
}

Button::Button(const char* text, int x, int y, int wide, int tall) : Label(text, x, y, wide, tall)
{
	init();
}

void Button::init()
{
	_armed = false;
	_selected = false;
	_buttonGroup = null;
	memset(_mouseClickMask, 0, sizeof(_mouseClickMask));
	_mouseClickMask[MOUSE_LEFT] = true;
	setContentAlignment(a_center);
	setBgColor(192, 192, 192, 0);
	setFgColor(0, 0, 0, 0);
}

void Button::setArmed(bool state)
{
	_armed = state;
	repaint();
}

bool Button::isArmed()
{
	return _armed;
}

void Button::setMouseClickEnabled(MouseCode code, bool state)
{
	if (code >= 0 && code < MOUSE_LAST)
		_mouseClickMask[code] = state;
}

bool Button::isMouseClickEnabled(MouseCode code)
{
	if (code >= 0 && code < MOUSE_LAST)
		return _mouseClickMask[code];
	return false;
}

void Button::setSelected(bool state)
{
	_selected = state;
	repaint();
}

bool Button::isSelected()
{
	return _selected;
}

void Button::addActionSignal(ActionSignal* s)
{
	_actionSignalDar.addElement(s);
}

void Button::fireActionSignal()
{
	for (int i = 0; i < _actionSignalDar.getCount(); i++)
	{
		ActionSignal* s = _actionSignalDar[i];
		if (s)
			s->actionPerformed(this);
	}
}

Panel* Button::createPropertyPanel()
{
	return null;
}

void Button::setButtonGroup(ButtonGroup* buttonGroup)
{
	_buttonGroup = buttonGroup;
	if (_buttonGroup)
		_buttonGroup->addButton(this);
}

void Button::paintBackground()
{
	int wide, tall;
	getSize(wide, tall);

	if (_armed || _selected)
	{
		// Pressed/lowered look
		// Dark top-left
		drawSetColor(128, 128, 128, 0);
		drawFilledRect(0, 0, wide, 1);
		drawFilledRect(0, 0, 1, tall);
		// Light bottom-right
		drawSetColor(255, 255, 255, 0);
		drawFilledRect(0, tall - 1, wide, tall);
		drawFilledRect(wide - 1, 0, wide, tall);
		// Fill
		drawSetColor(184, 184, 184, 0);
		drawFilledRect(1, 1, wide - 1, tall - 1);
	}
	else
	{
		// Raised/normal look
		// Light top-left
		drawSetColor(255, 255, 255, 0);
		drawFilledRect(0, 0, wide, 1);
		drawFilledRect(0, 0, 1, tall);
		// Dark bottom-right
		drawSetColor(64, 64, 64, 0);
		drawFilledRect(0, tall - 1, wide, tall);
		drawFilledRect(wide - 1, 0, wide, tall);
		// Inner dark
		drawSetColor(128, 128, 128, 0);
		drawFilledRect(1, tall - 2, wide - 1, tall - 1);
		drawFilledRect(wide - 2, 1, wide - 1, tall - 1);
		// Fill
		drawSetColor(192, 192, 192, 0);
		drawFilledRect(2, 2, wide - 2, tall - 2);
	}
}

void Button::paint()
{
	Label::paint();
}

void Button::internalCursorEntered()
{
	if (isMouseDown(MOUSE_LEFT) && _mouseClickMask[MOUSE_LEFT])
		setArmed(true);
	Panel::internalCursorEntered();
}

void Button::internalCursorExited()
{
	setArmed(false);
	Panel::internalCursorExited();
}

void Button::internalMousePressed(MouseCode code)
{
	if (code >= 0 && code < MOUSE_LAST && _mouseClickMask[code])
	{
		setArmed(true);
	}
	Panel::internalMousePressed(code);
}

void Button::internalMouseReleased(MouseCode code)
{
	if (code >= 0 && code < MOUSE_LAST && _mouseClickMask[code])
	{
		if (_armed)
		{
			fireActionSignal();
			if (_buttonGroup)
				_buttonGroup->setSelected(this);
		}
		setArmed(false);
	}
	Panel::internalMouseReleased(code);
}

void Button::internalKeyPressed(KeyCode code)
{
	if (code == KEY_ENTER || code == KEY_SPACE)
	{
		fireActionSignal();
	}
	Panel::internalKeyPressed(code);
}

}
