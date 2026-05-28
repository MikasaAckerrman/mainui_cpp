#ifndef VGUI_LISTPANEL_H
#define VGUI_LISTPANEL_H

#include <VGUI.h>
#include <VGUI_Panel.h>
#include <VGUI_Dar.h>

namespace vgui
{

class ScrollBar;

class ListPanel : public Panel
{
public:
	ListPanel(int x, int y, int wide, int tall);
public:
	virtual void setPixelScroll(int value);
	virtual void addString(const char* str);
	virtual void addItem(Panel* panel);
	virtual int getItemCount();
	virtual Panel* getItem(int index);
	virtual void removeItem(int index);
	virtual void setSelectedIndex(int index);
	virtual int getSelectedIndex();
protected:
	virtual void paint();
	virtual void paintBackground();
	virtual void performLayout();
	virtual void internalMousePressed(MouseCode code);
	virtual void internalMouseWheeled(int delta);
private:
	Dar<Panel*> _itemDar;
	ScrollBar* _vscrollBar;
	int _pixelScroll;
	int _selectedIndex;
};

}

#endif
