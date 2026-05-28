#ifndef VGUI_FOCUSNAVGROUP_H
#define VGUI_FOCUSNAVGROUP_H

#include <VGUI.h>
#include <VGUI_Dar.h>

namespace vgui
{

class Panel;

class FocusNavGroup
{
public:
	FocusNavGroup();
public:
	virtual void addPanel(Panel* panel);
	virtual void requestFocusPrev();
	virtual void requestFocusNext();
	virtual void setCurrentPanel(Panel* panel);
private:
	Dar<Panel*> _panelDar;
	int _currentIndex;
};

}

#endif
