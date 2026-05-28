#ifndef VGUI_ETCHEDBORDER_H
#define VGUI_ETCHEDBORDER_H

#include <VGUI.h>
#include <VGUI_Border.h>

namespace vgui
{

class EtchedBorder : public Border
{
public:
	EtchedBorder();
protected:
	virtual void paint(Panel* panel);
};

}

#endif
