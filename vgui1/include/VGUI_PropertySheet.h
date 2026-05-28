#ifndef VGUI_PROPERTYSHEET_H
#define VGUI_PROPERTYSHEET_H

#include <VGUI.h>
#include <VGUI_Panel.h>

namespace vgui
{

class TabPanel;

class PropertySheet : public Panel
{
public:
	PropertySheet(int x, int y, int wide, int tall);
public:
	virtual void addPage(Panel* page, const char* title);
	virtual void setActivePage(Panel* page);
	virtual void setActiveTab(int index);
	virtual TabPanel* getTabPanel();
protected:
	virtual void performLayout();
private:
	TabPanel* _tabPanel;
};

}

#endif
