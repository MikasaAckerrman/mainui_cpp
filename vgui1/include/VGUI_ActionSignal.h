#ifndef VGUI_ACTIONSIGNAL_H
#define VGUI_ACTIONSIGNAL_H

#include <VGUI.h>

namespace vgui
{

class Panel;

class ActionSignal
{
public:
	virtual ~ActionSignal() {}
	virtual void actionPerformed(Panel* panel) = 0;
};

}

#endif
