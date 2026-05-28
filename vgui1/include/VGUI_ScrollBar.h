#ifndef VGUI_SCROLLBAR_H
#define VGUI_SCROLLBAR_H

#include <VGUI.h>
#include <VGUI_Panel.h>
#include <VGUI_Dar.h>

namespace vgui
{

class IntChangeSignal;
class Button;
class Slider;

class ScrollBar : public Panel
{
public:
	ScrollBar(int x, int y, int wide, int tall, bool vertical);
public:
	virtual void setValue(int value);
	virtual int getValue();
	virtual void setRange(int min, int max);
	virtual void setRangeWindow(int rangeWindow);
	virtual void setRangeWindowEnabled(bool state);
	virtual void addIntChangeSignal(IntChangeSignal* s);
	virtual void fireIntChangeSignal();
	virtual bool isVertical();
	virtual void setButton(Button* button, int index);
	virtual Button* getButton(int index);
	virtual void setSlider(Slider* slider);
	virtual Slider* getSlider();
private:
	bool _vertical;
	Button* _button[2];
	Slider* _slider;
	Dar<IntChangeSignal*> _intChangeSignalDar;
};

}

#endif
