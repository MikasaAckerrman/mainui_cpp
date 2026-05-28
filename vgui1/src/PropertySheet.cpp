#include <VGUI_PropertySheet.h>
#include <VGUI_TabPanel.h>

namespace vgui
{

PropertySheet::PropertySheet(int x, int y, int wide, int tall) : Panel(x, y, wide, tall)
{
	_tabPanel = new TabPanel(0, 0, wide, tall);
	addChild(_tabPanel);
}

void PropertySheet::addPage(Panel* page, const char* title)
{
	if (_tabPanel)
		_tabPanel->addTab(title, page);
}

void PropertySheet::setActivePage(Panel* page)
{
	if (!_tabPanel || !page)
		return;

	for (int i = 0; i < _tabPanel->getTabCount(); i++)
	{
		_tabPanel->setSelectedTab(i);
		if (_tabPanel->getSelectedPanel() == page)
			return;
	}
}

void PropertySheet::setActiveTab(int index)
{
	if (_tabPanel)
		_tabPanel->setSelectedTab(index);
}

TabPanel* PropertySheet::getTabPanel()
{
	return _tabPanel;
}

void PropertySheet::performLayout()
{
	int wide, tall;
	getSize(wide, tall);

	if (_tabPanel)
		_tabPanel->setBounds(0, 0, wide, tall);
}

}
