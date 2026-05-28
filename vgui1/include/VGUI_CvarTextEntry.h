#ifndef VGUI_CVARTEXTENTRY_H
#define VGUI_CVARTEXTENTRY_H

#include <VGUI.h>
#include <VGUI_TextEntry.h>

namespace vgui
{

class CvarTextEntry : public TextEntry
{
public:
	CvarTextEntry(const char* cvarName, int x, int y, int wide, int tall);
public:
	virtual void reset();
	virtual void apply();
private:
	char _cvarName[64];
};

}

#endif
