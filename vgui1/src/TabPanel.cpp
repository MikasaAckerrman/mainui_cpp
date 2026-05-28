#include <VGUI_TabPanel.h>
#include <VGUI_App.h>
#include <string.h>

namespace vgui
{

TabPanel::TabPanel(int x, int y, int wide, int tall) : Panel(x, y, wide, tall)
{
	_selectedTab = 0;
	setBgColor(192, 192, 192, 0);
}

void TabPanel::addTab(const char* text, Panel* panel)
{
	Tab* tab = new Tab;
	vgui_strcpy(tab->text, sizeof(tab->text), text ? text : "");
	tab->panel = panel;
	_tabDar.addElement(tab);

	if (panel)
	{
		addChild(panel);
		panel->setVisible(_tabDar.getCount() - 1 == _selectedTab);
	}

	invalidateLayout(true);
}

void TabPanel::setSelectedTab(int index)
{
	if (index < 0 || index >= _tabDar.getCount())
		return;

	// Hide old panel
	if (_selectedTab >= 0 && _selectedTab < _tabDar.getCount())
	{
		Tab* oldTab = _tabDar[_selectedTab];
		if (oldTab && oldTab->panel)
			oldTab->panel->setVisible(false);
	}

	_selectedTab = index;

	// Show new panel
	Tab* newTab = _tabDar[_selectedTab];
	if (newTab && newTab->panel)
		newTab->panel->setVisible(true);

	repaint();
}

int TabPanel::getSelectedTab()
{
	return _selectedTab;
}

Panel* TabPanel::getSelectedPanel()
{
	if (_selectedTab >= 0 && _selectedTab < _tabDar.getCount())
	{
		Tab* tab = _tabDar[_selectedTab];
		if (tab)
			return tab->panel;
	}
	return null;
}

int TabPanel::getTabCount()
{
	return _tabDar.getCount();
}

void TabPanel::performLayout()
{
	int wide, tall;
	getSize(wide, tall);

	int tabHeight = 28;

	// Position page panels below tabs
	for (int i = 0; i < _tabDar.getCount(); i++)
	{
		Tab* tab = _tabDar[i];
		if (tab && tab->panel)
		{
			tab->panel->setBounds(0, tabHeight, wide, tall - tabHeight);
		}
	}
}

void TabPanel::paintBackground()
{
	int wide, tall;
	getSize(wide, tall);

	// Fill background
	drawSetColor(192, 192, 192, 0);
	drawFilledRect(0, 0, wide, tall);
}

void TabPanel::paint()
{
	int wide, tall;
	getSize(wide, tall);

	int tabCount = _tabDar.getCount();
	if (tabCount == 0)
		return;

	int tabHeight = 28;
	int tabWidth = tabCount > 0 ? wide / tabCount : wide;
	if (tabWidth > 100) tabWidth = 100;
	if (tabWidth < 20) tabWidth = 20;

	for (int i = 0; i < tabCount; i++)
	{
		Tab* tab = _tabDar[i];
		if (!tab)
			continue;

		int tx = i * tabWidth;
		bool selected = (i == _selectedTab);

		if (selected)
		{
			// Selected tab - raised, connects to panel below
			drawSetColor(192, 192, 192, 0);
			drawFilledRect(tx + 1, 2, tx + tabWidth - 1, tabHeight);

			// Top-left highlight
			drawSetColor(255, 255, 255, 0);
			drawFilledRect(tx, 2, tx + 1, tabHeight);        // left
			drawFilledRect(tx, 1, tx + tabWidth, 2);         // top

			// Top-right shadow
			drawSetColor(64, 64, 64, 0);
			drawFilledRect(tx + tabWidth - 1, 2, tx + tabWidth, tabHeight); // right
		}
		else
		{
			// Unselected tab - smaller, recessed
			drawSetColor(180, 180, 180, 0);
			drawFilledRect(tx + 1, 4, tx + tabWidth - 1, tabHeight - 1);

			drawSetColor(255, 255, 255, 0);
			drawFilledRect(tx, 4, tx + 1, tabHeight - 1);
			drawFilledRect(tx, 3, tx + tabWidth, 4);

			drawSetColor(64, 64, 64, 0);
			drawFilledRect(tx + tabWidth - 1, 4, tx + tabWidth, tabHeight - 1);
		}

		// Tab text
		int textLen = (int)strlen(tab->text);
		if (textLen > 0)
		{
			drawSetTextColor(0, 0, 0, 0);
			drawSetTextFont(Scheme::sf_primary1);
			int textY = selected ? 7 : 9;
			drawPrintText(tx + 6, textY, tab->text, textLen);
		}
	}

	// Bottom line under tabs (except selected)
	drawSetColor(64, 64, 64, 0);
	int selTx = _selectedTab * tabWidth;
	drawFilledRect(0, tabHeight - 1, selTx, tabHeight);
	drawFilledRect(selTx + tabWidth, tabHeight - 1, wide, tabHeight);
}

void TabPanel::internalMousePressed(MouseCode code)
{
	if (code == MOUSE_LEFT)
	{
		App* app = App::getInstance();
		if (app)
		{
			int mx, my;
			app->getCursorPos(mx, my);
			screenToLocal(mx, my);

			int tabHeight = 28;
			if (my < tabHeight)
			{
				int tabCount = _tabDar.getCount();
				int wide, tall;
				getSize(wide, tall);
				int tabWidth = tabCount > 0 ? wide / tabCount : wide;
				if (tabWidth > 100) tabWidth = 100;
				if (tabWidth < 20) tabWidth = 20;

				int clickedTab = mx / tabWidth;
				if (clickedTab >= 0 && clickedTab < tabCount)
					setSelectedTab(clickedTab);
			}
		}
	}
	Panel::internalMousePressed(code);
}

}
