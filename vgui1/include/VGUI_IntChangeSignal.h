#ifndef VGUI_INTCHANGESIGNAL_H
#define VGUI_INTCHANGESIGNAL_H

#include <VGUI.h>

namespace vgui
{

class Panel;

class IntChangeSignal
{
public:
	virtual void intChanged(int value, Panel* panel) = 0;
};

}

#endif
