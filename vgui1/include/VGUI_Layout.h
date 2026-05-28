#ifndef VGUI_LAYOUT_H
#define VGUI_LAYOUT_H

#include <VGUI.h>

namespace vgui
{

class Panel;

class Layout
{
public:
	Layout();
public:
	virtual void performLayout(Panel* panel);
};

}

#endif
