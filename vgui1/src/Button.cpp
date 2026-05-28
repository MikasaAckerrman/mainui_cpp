// Include heavy mainui headers BEFORE VGUI_*.h to avoid the `null` macro clash.
extern void UI_FillRect( int x, int y, int width, int height, const unsigned int color );
#include "TrackerScheme.h"

#include <VGUI_SchemeColors.h>
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
	_depressed = false;
	_selected = false;
	_buttonGroup = null;
	memset(_mouseClickMask, 0, sizeof(_mouseClickMask));
	_mouseClickMask[MOUSE_LEFT] = true;
	setContentAlignment(a_center);
	// Visual colors driven by g_Scheme in paintBackground/paint at draw time.
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

bool Button::isDepressed()
{
	return _depressed;
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

	bool enabled = isEnabled();

	// Color selection from scheme; disabled buttons render with a dimmer fill
	unsigned int bg;
	if (!enabled)
		bg = g_Scheme.tabInactiveBgColor ? g_Scheme.tabInactiveBgColor : 0xE64E5643;
	else if (_armed)
		bg = g_Scheme.buttonArmedBgColor ? g_Scheme.buttonArmedBgColor : 0xFF6B7360;
	else
		bg = g_Scheme.buttonBgColor ? g_Scheme.buttonBgColor : 0xFF5B6350;

	unsigned int bright = g_Scheme.borderBright ? g_Scheme.borderBright : 0xC85F6558;
	unsigned int dark   = g_Scheme.borderDark   ? g_Scheme.borderDark   : 0xC8282C24;

	// Selected (latched) buttons render sunken; momentary press also inverts
	// the bevel for the duration of mouse-down (PC CS 1.6 depressed look).
	bool sunken = _selected || _depressed;

	// Body fill
	schemeBgColor(this, bg);
	drawFilledRect(1, 1, wide - 1, tall - 1);

	// 1px raised/sunken bevel. When depressed (transient press) the outer
	// contour is suppressed so the click-feedback looks "pushed in" and the
	// surrounding dark frame disappears -- exactly matches the user's
	// "когда нажал, чёрный контур пропадает" observation.
	if (sunken)
	{
		// Shadow now on top+left, highlight on bottom+right (inverted bevel)
		schemeBgColor(this, dark);
		drawFilledRect(0, 0, wide, 1);
		drawFilledRect(0, 0, 1, tall);
		schemeBgColor(this, bright);
		drawFilledRect(0, tall - 1, wide, tall);
		drawFilledRect(wide - 1, 0, wide, tall);
	}
	else
	{
		schemeBgColor(this, bright);
		drawFilledRect(0, 0, wide, 1);
		drawFilledRect(0, 0, 1, tall);
		schemeBgColor(this, dark);
		drawFilledRect(0, tall - 1, wide, tall);
		drawFilledRect(wide - 1, 0, wide, tall);
	}
}

void Button::paint()
{
	// Pick text color from scheme based on enabled/armed state
	unsigned int argb;
	if (!isEnabled())
		argb = g_Scheme.labelDimColor ? g_Scheme.labelDimColor : 0xFFA0A0A0;
	else if (_armed)
		argb = g_Scheme.buttonArmedTextColor ? g_Scheme.buttonArmedTextColor : 0xFFFFFFFF;
	else
		argb = g_Scheme.buttonTextColor ? g_Scheme.buttonTextColor : 0xFFFFFFFF;

	int a = (argb >> 24) & 0xFF;
	int r = (argb >> 16) & 0xFF;
	int g = (argb >> 8) & 0xFF;
	int b = argb & 0xFF;
	setFgColor(r, g, b, 255 - a); // VGUI inverted alpha
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
	if (_depressed)
	{
		// If the user drags off the button while still holding mouse-down,
		// release the depressed visual so the bevel doesn't stay inverted
		// after a stray release that we won't receive.
		_depressed = false;
		repaint();
	}
	Panel::internalCursorExited();
}

void Button::internalMouseReleased(MouseCode code)
{
	if (code >= 0 && code < MOUSE_LAST && _mouseClickMask[code])
	{
		if (_armed && isEnabled())
		{
			fireActionSignal();
			if (_buttonGroup)
				_buttonGroup->setSelected(this);
		}
		setArmed(false);
	}
	if (_depressed)
	{
		_depressed = false;
		repaint();
	}
	Panel::internalMouseReleased(code);
}

void Button::internalMousePressed(MouseCode code)
{
	if (!isEnabled())
	{
		Panel::internalMousePressed(code);
		return;
	}
	if (code >= 0 && code < MOUSE_LAST && _mouseClickMask[code])
	{
		setArmed(true);
		_depressed = true; // CS 1.6 PC: bevel inverts and outer contour disappears
		repaint();
	}
	Panel::internalMousePressed(code);
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
