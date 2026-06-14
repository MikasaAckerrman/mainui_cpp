#ifndef VGUI_TABPANEL_H
#define VGUI_TABPANEL_H

#include <VGUI.h>
#include <VGUI_Panel.h>
#include <VGUI_Dar.h>

namespace vgui
{

class TabPanel : public Panel
{
public:
	TabPanel(int x, int y, int wide, int tall);
	virtual ~TabPanel();
public:
	virtual void addTab(const char* text, Panel* panel);
	virtual void setSelectedTab(int index);
	virtual int getSelectedTab();
	virtual Panel* getSelectedPanel();
	virtual int getTabCount();
	virtual void setSize(int wide, int tall); // override - relayout pages
protected:
	virtual void paint();
	virtual void paintBackground();
	virtual void internalMousePressed(MouseCode code);
	virtual void internalMouseReleased(MouseCode code);
	virtual void performLayout();
private:
	struct Tab
	{
		char text[64];
		Panel* panel;
		int naturalW;
	};
	// Compute per-tab x/width so the tab row spans the full strip width
	// (stretched proportionally to each tab's natural text width). Shared by
	// paint() and hit-testing so they never disagree. xs/ws must hold >=count.
	void layoutTabs(int wide, int* xs, int* ws, int maxOut);
	Dar<Tab*> _tabDar;
	int _selectedTab;
	int _pressedTab;  // tab index currently held down (-1 = none)
};

}

#endif
