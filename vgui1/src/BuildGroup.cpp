#include <VGUI_BuildGroup.h>
#include <VGUI_Panel.h>
#include <VGUI_Cursor.h>

namespace vgui
{

BuildGroup::BuildGroup()
{
	_enabled = false;
	_snapX = 4;
	_snapY = 4;
	_cursor_sizenwse = null;
	_cursor_sizenesw = null;
	_cursor_sizewe = null;
	_cursor_sizens = null;
	_cursor_sizeall = null;
	_dragging = false;
	_dragMouseCode = MOUSE_LEFT;
	_dragStartPanelPos[0] = 0;
	_dragStartPanelPos[1] = 0;
	_dragStartCursorPos[0] = 0;
	_dragStartCursorPos[1] = 0;
	_currentPanel = null;
}

void BuildGroup::setEnabled(bool state)
{
	_enabled = state;
}

bool BuildGroup::isEnabled()
{
	return _enabled;
}

void BuildGroup::addCurrentPanelChangeSignal(ChangeSignal* s)
{
	_currentPanelChangeSignalDar.addElement(s);
}

Panel* BuildGroup::getCurrentPanel()
{
	return _currentPanel;
}

void BuildGroup::copyPropertiesToClipboard()
{
	// Stub - build mode not used at runtime
}

void BuildGroup::applySnap(Panel* panel)
{
	// Stub
}

void BuildGroup::fireCurrentPanelChangeSignal()
{
	// Stub
}

void BuildGroup::panelAdded(Panel* panel, const char* panelName)
{
	_panelDar.addElement(panel);
}

void BuildGroup::cursorMoved(int x, int y, Panel* panel)
{
	// Stub
}

void BuildGroup::mousePressed(MouseCode code, Panel* panel)
{
	// Stub
}

void BuildGroup::mouseReleased(MouseCode code, Panel* panel)
{
	// Stub
}

void BuildGroup::mouseDoublePressed(MouseCode code, Panel* panel)
{
	// Stub
}

void BuildGroup::keyTyped(KeyCode code, Panel* panel)
{
	// Stub
}

Cursor* BuildGroup::getCursor(Panel* panel)
{
	return null;
}

}
