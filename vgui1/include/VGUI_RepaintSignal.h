#ifndef VGUI_REPAINTSIGNAL_H
#define VGUI_REPAINTSIGNAL_H

#include <VGUI.h>

namespace vgui
{

class Panel;

class RepaintSignal
{
public:
	virtual void panelRepainted(Panel* panel) = 0;
};

}

#endif
