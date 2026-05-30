// Include heavy mainui headers BEFORE VGUI_*.h to avoid the `null` macro clash.
extern void UI_FillRect( int x, int y, int width, int height, const unsigned int color );
#include "TrackerScheme.h"

#include <VGUI_SchemeColors.h>
#include <VGUI_UIScale.h>
#include <VGUI_Button.h>
#include <VGUI_ActionSignal.h>
#include <string.h>

namespace vgui
{

//-------------------------------------------------------------------
// ButtonGroup
//-------------------------------------------------------------------
ButtonGroup::ButtonGroup() {}

void ButtonGroup::addButton(Button* button) { _buttonDar.addElement(button); }

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
}

void Button::setArmed(bool state) { _armed = state; repaint(); }
bool Button::isArmed() { return _armed; }
bool Button::isDepressed() { return _depressed; }

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

void Button::setSelected(bool state) { _selected = state; repaint(); }
bool Button::isSelected() { return _selected; }

void Button::addActionSignal(ActionSignal* s) { _actionSignalDar.addElement(s); }

void Button::fireActionSignal()
{
	for (int i = 0; i < _actionSignalDar.getCount(); i++)
	{
		ActionSignal* s = _actionSignalDar[i];
		if (s) s->actionPerformed(this);
	}
}

Panel* Button::createPropertyPanel() { return null; }

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
	bool sunken = _selected || _depressed;

	// Body fill color
	unsigned int bg;
	if (!enabled)
		bg = g_Scheme.tabInactiveBgColor ? g_Scheme.tabInactiveBgColor : 0xE6373E2B;
	else if (_armed)
		bg = g_Scheme.buttonArmedBgColor ? g_Scheme.buttonArmedBgColor : 0xFF5A6A50;
	else
		bg = g_Scheme.buttonBgColor ? g_Scheme.buttonBgColor : 0xFF4C5844;

	unsigned int bright = g_Scheme.borderBright ? g_Scheme.borderBright : 0xFF889180;
	unsigned int dark   = g_Scheme.borderDark   ? g_Scheme.borderDark   : 0xFF282E22;

	// Canonical CS 1.6: 1px border only.
	// Raised  (default): TL=Bright, BR=Dark.
	// Sunken (depressed): TL=Dark, BR=Bright (inset).
	schemeBgColor(this, bg);
	drawFilledRect(2, 2, wide - 2, tall - 2);

	unsigned int tl = sunken ? dark   : bright;
	unsigned int br = sunken ? bright : dark;
	schemeBgColor(this, tl);
	drawFilledRect(0, 0, wide, 1);
	drawFilledRect(0, 0, 1, tall);
	schemeBgColor(this, br);
	drawFilledRect(0, tall - 1, wide, tall);
	drawFilledRect(wide - 1, 0, wide, tall);
}

void Button::paint()
{
	// CS 1.6 canon: depressed buttons shift text +1,+1 px (TitleButtonDepressedBorder
	// inset "1 1 1 1" vs normal "0 0 1 1"). We replicate the visual shift here
	// without changing borders. Drawing manually instead of Label::paint() so
	// we control the offset.
	int pwide, ptall;
	getPaintSize(pwide, ptall);

	int textLen = (int)strlen(_text);
	if (textLen == 0)
		return;

	int twide, ttall;
	getContentSize(twide, ttall);

	int tx, ty;
	computeAlignment(tx, ty, twide, ttall, pwide, ptall);
	if (_depressed)
	{
		tx += 1;
		ty += 1;
	}

	unsigned int argb;
	if (!isEnabled())
		argb = g_Scheme.labelDimColor ? g_Scheme.labelDimColor : 0xFFA0AA95;
	else if (_armed)
		argb = g_Scheme.buttonArmedTextColor ? g_Scheme.buttonArmedTextColor : 0xFFFFFFFF;
	else
		argb = g_Scheme.buttonTextColor ? g_Scheme.buttonTextColor : 0xFFD8DED3;

	int a = (argb >> 24) & 0xFF;
	int r = (argb >> 16) & 0xFF;
	int g = (argb >> 8) & 0xFF;
	int b = argb & 0xFF;
	drawSetTextColor(r, g, b, 255 - a); // VGUI inverted alpha
	if (_font)
		drawSetTextFont(_font);
	else
		drawSetTextFont(_schemeFont);
	drawPrintText(tx, ty, _text, textLen);
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
		_depressed = false;
		repaint();
	}
	Panel::internalCursorExited();
}

void Button::internalMouseReleased(MouseCode code)
{
	if (code >= 0 && code < MOUSE_LAST && _mouseClickMask[code])
	{
		// Release with mouse capture: fire signal ONLY if cursor is still
		// inside this button. Without this, press A -> drag finger to B ->
		// release would fire B (wrong) because B would receive the up event.
		// With capture, A always receives the up event; we gate firing on
		// the actual cursor position at release time.
		bool inside = false;
		App* app = App::getInstance();
		if (app && isEnabled())
		{
			int mx, my;
			app->getCursorPos(mx, my);
			inside = isWithin(mx, my);
		}
		setAsMouseCapture(false);
		if (inside)
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
		_depressed = true;
		// Grab the mouse so the matching release ALWAYS comes back to this
		// button (see internalMouseReleased for the inside-check).
		setAsMouseCapture(true);
		repaint();
	}
	Panel::internalMousePressed(code);
}

// While the mouse is captured by this button, refresh armed/depressed state
// from the live cursor position so the bevel and text-shift track the finger
// (depressed when over the button, raised when dragged off).
void Button::internalCursorMoved(int x, int y)
{
	App* app = App::getInstance();
	if (app && app->isMouseDown(MOUSE_LEFT) && _mouseClickMask[MOUSE_LEFT] && isEnabled())
	{
		bool inside = isWithin(x, y);
		bool changed = false;
		if (_armed != inside)     { _armed = inside; changed = true; }
		if (_depressed != inside) { _depressed = inside; changed = true; }
		if (changed) repaint();
	}
	Panel::internalCursorMoved(x, y);
}

void Button::internalKeyPressed(KeyCode code)
{
	if (code == KEY_ENTER || code == KEY_SPACE)
		fireActionSignal();
	Panel::internalKeyPressed(code);
}

}
