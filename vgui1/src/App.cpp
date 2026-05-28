#include <VGUI_App.h>
#include <VGUI_Panel.h>
#include <VGUI_SurfaceBase.h>
#include <VGUI_TickSignal.h>
#include <VGUI_Scheme.h>
#include <VGUI_Cursor.h>
#include <string.h>

namespace vgui
{

App* App::_instance = null;

App::App()
{
	_instance = this;
	_externalMain = false;
	_running = false;
	_keyFocus = null;
	_oldMouseFocus = null;
	_mouseFocus = null;
	_mouseCapture = null;
	_wantedKeyFocus = null;
	_scheme = null;
	_buildMode = false;
	_wantedBuildMode = false;
	_mouseArenaPanel = null;
	_cursorOveride = null;
	_nextTickMillis = 0;
	_minimumTickMillisInterval = 0;
	memset(_mousePressed, 0, sizeof(_mousePressed));
	memset(_mouseDoublePressed, 0, sizeof(_mouseDoublePressed));
	memset(_mouseDown, 0, sizeof(_mouseDown));
	memset(_mouseReleased, 0, sizeof(_mouseReleased));
	memset(_keyPressed, 0, sizeof(_keyPressed));
	memset(_keyTyped, 0, sizeof(_keyTyped));
	memset(_keyDown, 0, sizeof(_keyDown));
	memset(_keyReleased, 0, sizeof(_keyReleased));
	memset(_cursor, 0, sizeof(_cursor));
	init();
}

App::App(bool externalMain)
{
	_instance = this;
	_externalMain = externalMain;
	_running = false;
	_keyFocus = null;
	_oldMouseFocus = null;
	_mouseFocus = null;
	_mouseCapture = null;
	_wantedKeyFocus = null;
	_scheme = null;
	_buildMode = false;
	_wantedBuildMode = false;
	_mouseArenaPanel = null;
	_cursorOveride = null;
	_nextTickMillis = 0;
	_minimumTickMillisInterval = 0;
	memset(_mousePressed, 0, sizeof(_mousePressed));
	memset(_mouseDoublePressed, 0, sizeof(_mouseDoublePressed));
	memset(_mouseDown, 0, sizeof(_mouseDown));
	memset(_mouseReleased, 0, sizeof(_mouseReleased));
	memset(_keyPressed, 0, sizeof(_keyPressed));
	memset(_keyTyped, 0, sizeof(_keyTyped));
	memset(_keyDown, 0, sizeof(_keyDown));
	memset(_keyReleased, 0, sizeof(_keyReleased));
	memset(_cursor, 0, sizeof(_cursor));
	init();
}

App* App::getInstance()
{
	return _instance;
}

void App::start()
{
	_running = true;
}

void App::stop()
{
	_running = false;
}

void App::externalTick()
{
	internalTick();
	platTick();
}

bool App::wasMousePressed(MouseCode code, Panel* panel)
{
	if (code < 0 || code >= MOUSE_LAST)
		return false;
	return _mousePressed[code];
}

bool App::wasMouseDoublePressed(MouseCode code, Panel* panel)
{
	if (code < 0 || code >= MOUSE_LAST)
		return false;
	return _mouseDoublePressed[code];
}

bool App::isMouseDown(MouseCode code, Panel* panel)
{
	if (code < 0 || code >= MOUSE_LAST)
		return false;
	return _mouseDown[code];
}

bool App::wasMouseReleased(MouseCode code, Panel* panel)
{
	if (code < 0 || code >= MOUSE_LAST)
		return false;
	return _mouseReleased[code];
}

bool App::wasKeyPressed(KeyCode code, Panel* panel)
{
	if (code < 0 || code >= KEY_LAST)
		return false;
	return _keyPressed[code];
}

bool App::isKeyDown(KeyCode code, Panel* panel)
{
	if (code < 0 || code >= KEY_LAST)
		return false;
	return _keyDown[code];
}

bool App::wasKeyTyped(KeyCode code, Panel* panel)
{
	if (code < 0 || code >= KEY_LAST)
		return false;
	return _keyTyped[code];
}

bool App::wasKeyReleased(KeyCode code, Panel* panel)
{
	if (code < 0 || code >= KEY_LAST)
		return false;
	return _keyReleased[code];
}

void App::addTickSignal(TickSignal* s)
{
	_tickSignalDar.addElement(s);
}

void App::setCursorPos(int x, int y)
{
	if (_surfaceBaseDar.getCount() > 0)
	{
		SurfaceBase* sb = _surfaceBaseDar[0];
		if (sb)
			sb->setEmulatedCursorPos(x, y);
	}
}

void App::getCursorPos(int& x, int& y)
{
	x = 0;
	y = 0;
	if (_surfaceBaseDar.getCount() > 0)
	{
		SurfaceBase* sb = _surfaceBaseDar[0];
		if (sb)
			sb->GetMousePos(x, y);
	}
}

void App::setMouseCapture(Panel* panel)
{
	_mouseCapture = panel;
}

void App::setMouseArena(int x0, int y0, int x1, int y1, bool enabled)
{
	internalSetMouseArena(x0, y0, x1, y1, enabled);
}

void App::setMouseArena(Panel* panel)
{
	_mouseArenaPanel = panel;
}

void App::requestFocus(Panel* panel)
{
	if (_keyFocus != panel)
	{
		if (_keyFocus)
			_keyFocus->internalFocusChanged(true);
		_keyFocus = panel;
		if (_keyFocus)
			_keyFocus->internalFocusChanged(false);
	}
	_wantedKeyFocus = null;
}

Panel* App::getFocus()
{
	return _keyFocus;
}

void App::repaintAll()
{
	for (int i = 0; i < _surfaceBaseDar.getCount(); i++)
	{
		SurfaceBase* sb = _surfaceBaseDar[i];
		if (sb)
		{
			Panel* panel = sb->getPanel();
			if (panel)
				panel->repaint();
		}
	}
}

void App::setScheme(Scheme* scheme)
{
	_scheme = scheme;
}

Scheme* App::getScheme()
{
	return _scheme;
}

void App::enableBuildMode()
{
	_wantedBuildMode = true;
}

long App::getTimeMillis()
{
	return 0;
}

char App::getKeyCodeChar(KeyCode code, bool shifted)
{
	if (code >= KEY_0 && code <= KEY_9)
	{
		if (shifted)
		{
			static const char shiftedDigits[] = ")!@#$%^&*(";
			return shiftedDigits[code - KEY_0];
		}
		return '0' + (code - KEY_0);
	}
	if (code >= KEY_A && code <= KEY_Z)
	{
		if (shifted)
			return 'A' + (code - KEY_A);
		return 'a' + (code - KEY_A);
	}
	switch (code)
	{
	case KEY_SPACE:     return ' ';
	case KEY_ENTER:     return '\n';
	case KEY_TAB:       return '\t';
	case KEY_SEMICOLON: return shifted ? ':' : ';';
	case KEY_APOSTROPHE: return shifted ? '"' : '\'';
	case KEY_BACKQUOTE:  return shifted ? '~' : '`';
	case KEY_COMMA:     return shifted ? '<' : ',';
	case KEY_PERIOD:    return shifted ? '>' : '.';
	case KEY_SLASH:     return shifted ? '?' : '/';
	case KEY_BACKSLASH: return shifted ? '|' : '\\';
	case KEY_MINUS:     return shifted ? '_' : '-';
	case KEY_EQUAL:     return shifted ? '+' : '=';
	case KEY_LBRACKET:  return shifted ? '{' : '[';
	case KEY_RBRACKET:  return shifted ? '}' : ']';
	default: return 0;
	}
}

void App::getKeyCodeText(KeyCode code, char* buf, int buflen)
{
	if (!buf || buflen <= 0)
		return;
	buf[0] = 0;
	char ch = getKeyCodeChar(code, false);
	if (ch)
	{
		if (buflen > 1)
		{
			buf[0] = ch;
			buf[1] = 0;
		}
		return;
	}
	const char* name = "";
	switch (code)
	{
	case KEY_ESCAPE:    name = "ESCAPE"; break;
	case KEY_BACKSPACE: name = "BACKSPACE"; break;
	case KEY_TAB:       name = "TAB"; break;
	case KEY_ENTER:     name = "ENTER"; break;
	case KEY_SPACE:     name = "SPACE"; break;
	case KEY_INSERT:    name = "INS"; break;
	case KEY_DELETE:    name = "DEL"; break;
	case KEY_HOME:      name = "HOME"; break;
	case KEY_END:       name = "END"; break;
	case KEY_PAGEUP:    name = "PGUP"; break;
	case KEY_PAGEDOWN:  name = "PGDN"; break;
	case KEY_UP:        name = "UP"; break;
	case KEY_DOWN:      name = "DOWN"; break;
	case KEY_LEFT:      name = "LEFT"; break;
	case KEY_RIGHT:     name = "RIGHT"; break;
	case KEY_F1:        name = "F1"; break;
	case KEY_F2:        name = "F2"; break;
	case KEY_F3:        name = "F3"; break;
	case KEY_F4:        name = "F4"; break;
	case KEY_F5:        name = "F5"; break;
	case KEY_F6:        name = "F6"; break;
	case KEY_F7:        name = "F7"; break;
	case KEY_F8:        name = "F8"; break;
	case KEY_F9:        name = "F9"; break;
	case KEY_F10:       name = "F10"; break;
	case KEY_F11:       name = "F11"; break;
	case KEY_F12:       name = "F12"; break;
	case KEY_LSHIFT:    name = "SHIFT"; break;
	case KEY_RSHIFT:    name = "SHIFT"; break;
	case KEY_LALT:      name = "ALT"; break;
	case KEY_RALT:      name = "ALT"; break;
	case KEY_LCONTROL:  name = "CTRL"; break;
	case KEY_RCONTROL:  name = "CTRL"; break;
	default:            name = ""; break;
	}
	vgui_strcpy(buf, buflen, name);
}

int App::getClipboardTextCount()
{
	return 0;
}

void App::setClipboardText(const char* text, int textLen)
{
	// Stub
}

int App::getClipboardText(int offset, char* buf, int bufLen)
{
	if (buf && bufLen > 0)
		buf[0] = 0;
	return 0;
}

void App::reset()
{
	memset(_mousePressed, 0, sizeof(_mousePressed));
	memset(_mouseDoublePressed, 0, sizeof(_mouseDoublePressed));
	memset(_mouseDown, 0, sizeof(_mouseDown));
	memset(_mouseReleased, 0, sizeof(_mouseReleased));
	memset(_keyPressed, 0, sizeof(_keyPressed));
	memset(_keyTyped, 0, sizeof(_keyTyped));
	memset(_keyDown, 0, sizeof(_keyDown));
	memset(_keyReleased, 0, sizeof(_keyReleased));
}

void App::internalSetMouseArena(int x0, int y0, int x1, int y1, bool enabled)
{
	// Stub
}

bool App::setRegistryString(const char* key, const char* value)
{
	return false;
}

bool App::getRegistryString(const char* key, char* value, int valueLen)
{
	if (value && valueLen > 0)
		value[0] = 0;
	return false;
}

bool App::setRegistryInteger(const char* key, int value)
{
	return false;
}

bool App::getRegistryInteger(const char* key, int& value)
{
	value = 0;
	return false;
}

void App::setCursorOveride(Cursor* cursor)
{
	_cursorOveride = cursor;
}

Cursor* App::getCursorOveride()
{
	return _cursorOveride;
}

void App::setMinimumTickMillisInterval(int interval)
{
	_minimumTickMillisInterval = interval;
}

void App::run()
{
	start();
	while (_running)
	{
		externalTick();
	}
}

void App::internalCursorMoved(int x, int y, SurfaceBase* surfaceBase)
{
	updateMouseFocus(x, y, surfaceBase);
	if (_mouseFocus)
	{
		_mouseFocus->internalCursorMoved(x, y);
	}
}

void App::internalMousePressed(MouseCode code, SurfaceBase* surfaceBase)
{
	if (code >= 0 && code < MOUSE_LAST)
	{
		_mousePressed[code] = true;
		_mouseDown[code] = true;
	}
	if (_mouseFocus)
	{
		_mouseFocus->internalMousePressed(code);
	}
}

void App::internalMouseDoublePressed(MouseCode code, SurfaceBase* surfaceBase)
{
	if (code >= 0 && code < MOUSE_LAST)
	{
		_mouseDoublePressed[code] = true;
		_mouseDown[code] = true;
	}
	if (_mouseFocus)
	{
		_mouseFocus->internalMouseDoublePressed(code);
	}
}

void App::internalMouseReleased(MouseCode code, SurfaceBase* surfaceBase)
{
	if (code >= 0 && code < MOUSE_LAST)
	{
		_mouseReleased[code] = true;
		_mouseDown[code] = false;
	}
	if (_mouseFocus)
	{
		_mouseFocus->internalMouseReleased(code);
	}
}

void App::internalMouseWheeled(int delta, SurfaceBase* surfaceBase)
{
	if (_mouseFocus)
	{
		_mouseFocus->internalMouseWheeled(delta);
	}
}

void App::internalKeyPressed(KeyCode code, SurfaceBase* surfaceBase)
{
	if (code >= 0 && code < KEY_LAST)
	{
		_keyPressed[code] = true;
		_keyDown[code] = true;
	}
	if (_keyFocus)
	{
		_keyFocus->internalKeyPressed(code);
	}
}

void App::internalKeyTyped(KeyCode code, SurfaceBase* surfaceBase)
{
	if (code >= 0 && code < KEY_LAST)
	{
		_keyTyped[code] = true;
	}
	if (_keyFocus)
	{
		_keyFocus->internalKeyTyped(code);
	}
}

void App::internalKeyReleased(KeyCode code, SurfaceBase* surfaceBase)
{
	if (code >= 0 && code < KEY_LAST)
	{
		_keyReleased[code] = true;
		_keyDown[code] = false;
	}
	if (_keyFocus)
	{
		_keyFocus->internalKeyReleased(code);
	}
}

void App::init()
{
	// Empty
}

void App::updateMouseFocus(int x, int y, SurfaceBase* surfaceBase)
{
	Panel* newMouseFocus = null;

	if (_mouseCapture)
	{
		newMouseFocus = _mouseCapture;
	}
	else
	{
		if (surfaceBase)
		{
			Panel* embeddedPanel = surfaceBase->getPanel();
			if (embeddedPanel)
			{
				newMouseFocus = embeddedPanel->isWithinTraverse(x, y);
			}
		}
	}

	if (newMouseFocus != _mouseFocus)
	{
		setMouseFocus(newMouseFocus);
	}
}

void App::setMouseFocus(Panel* newMouseFocus)
{
	if (newMouseFocus == _mouseFocus)
		return;

	_oldMouseFocus = _mouseFocus;

	if (_mouseFocus)
		_mouseFocus->internalCursorExited();

	_mouseFocus = newMouseFocus;

	if (_mouseFocus)
		_mouseFocus->internalCursorEntered();
}

void App::surfaceBaseCreated(SurfaceBase* surfaceBase)
{
	_surfaceBaseDar.addElement(surfaceBase);
}

void App::surfaceBaseDeleted(SurfaceBase* surfaceBase)
{
	_surfaceBaseDar.removeElement(surfaceBase);
}

void App::platTick()
{
	// Empty - no platform tick needed for embedded use
}

void App::internalTick()
{
	// Process wanted key focus
	if (_wantedKeyFocus)
	{
		if (_keyFocus != _wantedKeyFocus)
		{
			if (_keyFocus)
				_keyFocus->internalFocusChanged(true);
			_keyFocus = _wantedKeyFocus;
			if (_keyFocus)
				_keyFocus->internalFocusChanged(false);
		}
		_wantedKeyFocus = null;
	}

	// Fire tick signals
	for (int i = 0; i < _tickSignalDar.getCount(); i++)
	{
		TickSignal* s = _tickSignalDar[i];
		if (s)
			s->ticked();
	}

	// Reset per-frame state (NOT down arrays)
	memset(_mousePressed, 0, sizeof(_mousePressed));
	memset(_mouseDoublePressed, 0, sizeof(_mouseDoublePressed));
	memset(_mouseReleased, 0, sizeof(_mouseReleased));
	memset(_keyPressed, 0, sizeof(_keyPressed));
	memset(_keyTyped, 0, sizeof(_keyTyped));
	memset(_keyReleased, 0, sizeof(_keyReleased));
}

}
