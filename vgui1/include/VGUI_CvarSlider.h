#ifndef VGUI_CVARSLIDER_H
#define VGUI_CVARSLIDER_H

#include <VGUI.h>
#include <VGUI_Slider.h>

namespace vgui
{

class CvarSlider : public Slider
{
public:
	CvarSlider(const char* cvarName, int x, int y, int wide, int tall, int minVal, int maxVal);
public:
	virtual void reset();
	virtual void apply();
	virtual void fireIntChangeSignal();
private:
	char _cvarName[64];
};

}

#endif
