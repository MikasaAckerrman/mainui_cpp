#include <VGUI_FocusNavGroup.h>
#include <VGUI_Panel.h>

namespace vgui
{

FocusNavGroup::FocusNavGroup()
{
	_currentIndex = 0;
}

void FocusNavGroup::addPanel(Panel* panel)
{
	_panelDar.addElement(panel);
}

void FocusNavGroup::requestFocusPrev()
{
	if (_panelDar.getCount() == 0)
		return;
	_currentIndex--;
	if (_currentIndex < 0)
		_currentIndex = _panelDar.getCount() - 1;
	Panel* panel = _panelDar[_currentIndex];
	if (panel)
		panel->requestFocus();
}

void FocusNavGroup::requestFocusNext()
{
	if (_panelDar.getCount() == 0)
		return;
	_currentIndex++;
	if (_currentIndex >= _panelDar.getCount())
		_currentIndex = 0;
	Panel* panel = _panelDar[_currentIndex];
	if (panel)
		panel->requestFocus();
}

void FocusNavGroup::setCurrentPanel(Panel* panel)
{
	for (int i = 0; i < _panelDar.getCount(); i++)
	{
		if (_panelDar[i] == panel)
		{
			_currentIndex = i;
			return;
		}
	}
}

}
