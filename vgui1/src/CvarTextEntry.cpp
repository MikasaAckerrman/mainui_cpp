#include <VGUI_CvarTextEntry.h>
#include <VGUI_CvarBridge.h>
#include <string.h>

namespace vgui
{

CvarTextEntry::CvarTextEntry(const char* cvarName, int x, int y, int wide, int tall)
	: TextEntry("", x, y, wide, tall)
{
	_cvarName[0] = 0;
	if (cvarName)
		vgui_strcpy(_cvarName, sizeof(_cvarName), cvarName);
	reset();
}

void CvarTextEntry::reset()
{
	const char* val = VGUI_GetCvarString(_cvarName);
	if (val)
	{
		int len = (int)strlen(val);
		setText(val, len);
	}
	else
	{
		setText("", 0);
	}
}

void CvarTextEntry::apply()
{
	char buf[256];
	getText(0, buf, sizeof(buf));
	VGUI_SetCvarString(_cvarName, buf);
}

}
