#include <VGUI_CvarCheckButton.h>
#include <VGUI_CvarBridge.h>
#include <string.h>

namespace vgui
{

CvarCheckButton::CvarCheckButton(const char* cvarName, const char* text, int x, int y, int wide, int tall)
	: CheckButton(text, x, y, wide, tall)
{
	_cvarName[0] = 0;
	if (cvarName)
		vgui_strcpy(_cvarName, sizeof(_cvarName), cvarName);
	reset();
}

void CvarCheckButton::reset()
{
	float val = VGUI_GetCvarFloat(_cvarName);
	setSelected(val != 0.0f);
}

void CvarCheckButton::apply()
{
	VGUI_SetCvarFloat(_cvarName, isSelected() ? 1.0f : 0.0f);
}

void CvarCheckButton::internalMousePressed(MouseCode code)
{
	if (code == MOUSE_LEFT && isEnabled())
	{
		setSelected(!isSelected());
		VGUI_SetCvarFloat(_cvarName, isSelected() ? 1.0f : 0.0f);
		fireActionSignal();
	}
	Panel::internalMousePressed(code);
}

}
