#ifndef VGUI_BUTTON_H
#define VGUI_BUTTON_H

#include <VGUI.h>
#include <VGUI_Label.h>
#include <VGUI_Dar.h>

namespace vgui
{

class ActionSignal;
class ButtonGroup;

class Button : public Label
{
public:
	Button(const char* text, int x, int y);
	Button(const char* text, int x, int y, int wide, int tall);
public:
	virtual void setArmed(bool state);
	virtual bool isArmed();
	virtual void setMouseClickEnabled(MouseCode code, bool state);
	virtual bool isMouseClickEnabled(MouseCode code);
	virtual void setSelected(bool state);
	virtual bool isSelected();
	virtual void addActionSignal(ActionSignal* s);
	virtual void fireActionSignal();
	virtual Panel* createPropertyPanel();
	virtual void setButtonGroup(ButtonGroup* buttonGroup);
protected:
	virtual void paint();
	virtual void paintBackground();
	virtual void internalCursorEntered();
	virtual void internalCursorExited();
	virtual void internalMousePressed(MouseCode code);
	virtual void internalMouseReleased(MouseCode code);
	virtual void internalKeyPressed(KeyCode code);
private:
	void init();
private:
	bool _armed;
	bool _selected;
	bool _mouseClickMask[MOUSE_LAST];
	Dar<ActionSignal*> _actionSignalDar;
	ButtonGroup* _buttonGroup;
};

class ButtonGroup
{
public:
	ButtonGroup();
public:
	virtual void addButton(Button* button);
	virtual void setSelected(Button* button);
private:
	Dar<Button*> _buttonDar;
};

}

#endif
