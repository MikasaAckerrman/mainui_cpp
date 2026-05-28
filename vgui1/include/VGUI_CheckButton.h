#ifndef VGUI_CHECKBUTTON_H
#define VGUI_CHECKBUTTON_H

#include <VGUI.h>
#include <VGUI_Button.h>

namespace vgui
{

class CheckButton : public Button
{
public:
	CheckButton(const char* text, int x, int y);
	CheckButton(const char* text, int x, int y, int wide, int tall);
protected:
	virtual void paint();
	virtual void paintBackground();
	virtual void internalMousePressed(MouseCode code);
};

}

#endif
