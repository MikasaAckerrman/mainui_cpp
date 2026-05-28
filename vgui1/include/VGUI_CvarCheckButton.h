#ifndef VGUI_CVARCHECKBUTTON_H
#define VGUI_CVARCHECKBUTTON_H

#include <VGUI.h>
#include <VGUI_CheckButton.h>

namespace vgui
{

class CvarCheckButton : public CheckButton
{
public:
	CvarCheckButton(const char* cvarName, const char* text, int x, int y, int wide, int tall);
public:
	virtual void reset();
	virtual void apply();
protected:
	virtual void internalMousePressed(MouseCode code);
private:
	char _cvarName[64];
};

}

#endif
