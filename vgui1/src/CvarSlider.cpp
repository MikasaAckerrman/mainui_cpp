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
	_cvarMin = 0.0f;
	_cvarMax = 0.0f;
	_scaled = false;
	setRange(minVal, maxVal);
	reset();
}

CvarSlider::CvarSlider(const char* cvarName, int x, int y, int wide, int tall, int minVal, int maxVal, float cvarMin, float cvarMax)
	: Slider(x, y, wide, tall, false)
{
	_cvarName[0] = 0;
	if (cvarName)
		vgui_strcpy(_cvarName, sizeof(_cvarName), cvarName);
	_cvarMin = cvarMin;
	_cvarMax = cvarMax;
	_scaled = true;
	setRange(minVal, maxVal);
	reset();
}

float CvarSlider::sliderToFloat()
{
	int minVal, maxVal;
	getRange(minVal, maxVal);
	int range = maxVal - minVal;
	if (range <= 0) range = 1;
	float t = (float)(getValue() - minVal) / (float)range;
	return _cvarMin + t * (_cvarMax - _cvarMin);
}

int CvarSlider::floatToSlider(float val)
{
	float range = _cvarMax - _cvarMin;
	if (range <= 0.0f) range = 1.0f;
	float t = (val - _cvarMin) / range;
	if (t < 0.0f) t = 0.0f;
	if (t > 1.0f) t = 1.0f;
	int minVal, maxVal;
	getRange(minVal, maxVal);
	return minVal + (int)(t * (float)(maxVal - minVal) + 0.5f);
}

void CvarSlider::reset()
{
	float val = VGUI_GetCvarFloat(_cvarName);
	int intVal;
	if (_scaled)
	{
		intVal = floatToSlider(val);
	}
	else
	{
		intVal = (int)val;
		int minVal, maxVal;
		getRange(minVal, maxVal);
		if (intVal < minVal) intVal = minVal;
		if (intVal > maxVal) intVal = maxVal;
	}
	setValue(intVal);
}

void CvarSlider::apply()
{
	if (_scaled)
		VGUI_SetCvarFloat(_cvarName, sliderToFloat());
	else
		VGUI_SetCvarFloat(_cvarName, (float)getValue());
}

void CvarSlider::fireIntChangeSignal()
{
	Slider::fireIntChangeSignal();
	if (_scaled)
		VGUI_SetCvarFloat(_cvarName, sliderToFloat());
	else
		VGUI_SetCvarFloat(_cvarName, (float)getValue());
}

}
