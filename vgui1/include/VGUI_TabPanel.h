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
public:
	virtual void addTab(const char* text, Panel* panel);
	virtual void setSelectedTab(int index);
	virtual int getSelectedTab();
	virtual Panel* getSelectedPanel();
	virtual int getTabCount();
protected:
	virtual void paint();
	virtual void paintBackground();
	virtual void internalMousePressed(MouseCode code);
	virtual void performLayout();
private:
	struct Tab
	{
		char text[64];
		Panel* panel;
	};
	Dar<Tab*> _tabDar;
	int _selectedTab;
};

}

#endif
