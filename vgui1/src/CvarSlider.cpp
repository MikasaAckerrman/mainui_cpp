#include <VGUI_CvarSlider.h>
#include <VGUI_CvarBridge.h>
#include <string.h>

namespace vgui
{

CvarSlider::CvarSlider(const char* cvarName, int x, int y, int wide, int tall, int minVal, int maxVal)
	: Slider(x, y, wide, tall, false)
{
	_cvarName[0] = 0;
	if (cvarName)
		vgui_strcpy(_cvarName, sizeof(_cvarName), cvarName);
	setRange(minVal, maxVal);
	reset();
}

void CvarSlider::reset()
{
	float val = VGUI_GetCvarFloat(_cvarName);
	int intVal = (int)val;
	int minVal, maxVal;
	getRange(minVal, maxVal);
	if (intVal < minVal) intVal = minVal;
	if (intVal > maxVal) intVal = maxVal;
	setValue(intVal);
}

void CvarSlider::apply()
{
	VGUI_SetCvarFloat(_cvarName, (float)getValue());
}

void CvarSlider::fireIntChangeSignal()
{
	Slider::fireIntChangeSignal();
	VGUI_SetCvarFloat(_cvarName, (float)getValue());
}

}
