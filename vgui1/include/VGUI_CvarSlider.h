#ifndef VGUI_CVARSLIDER_H
#define VGUI_CVARSLIDER_H

#include <VGUI.h>
#include <VGUI_Slider.h>

namespace vgui
{

class CvarSlider : public Slider
{
public:
	// Integer-only slider (cvar value = slider integer value directly)
	CvarSlider(const char* cvarName, int x, int y, int wide, int tall, int minVal, int maxVal);
	// Scaled float slider (slider 0..maxVal maps linearly to cvarMin..cvarMax)
	CvarSlider(const char* cvarName, int x, int y, int wide, int tall, int minVal, int maxVal, float cvarMin, float cvarMax);
public:
	virtual void reset();
	virtual void apply();
	virtual void fireIntChangeSignal();
private:
	float sliderToFloat();
	int floatToSlider(float val);
private:
	char _cvarName[64];
	float _cvarMin;
	float _cvarMax;
	bool _scaled;
};

}

#endif
