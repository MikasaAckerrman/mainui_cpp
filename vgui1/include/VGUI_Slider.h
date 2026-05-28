#ifndef VGUI_SLIDER_H
#define VGUI_SLIDER_H

#include <VGUI.h>
#include <VGUI_Panel.h>
#include <VGUI_Dar.h>

namespace vgui
{

class IntChangeSignal;

class Slider : public Panel
{
public:
	Slider(int x, int y, int wide, int tall, bool vertical);
public:
	virtual void setValue(int value);
	virtual int getValue();
	virtual void setRange(int min, int max);
	virtual void getRange(int& min, int& max);
	virtual void setRangeWindow(int rangeWindow);
	virtual void setRangeWindowEnabled(bool state);
	virtual void addIntChangeSignal(IntChangeSignal* s);
	virtual void fireIntChangeSignal();
	virtual bool isVertical();
	virtual void setSliderSize(int size);
	virtual int getSliderSize();
	virtual void setButtonOffset(int offset);
protected:
	virtual void paint();
	virtual void paintBackground();
	virtual void internalMousePressed(MouseCode code);
	virtual void internalMouseReleased(MouseCode code);
	virtual void internalCursorMoved(int x, int y);
private:
	void recomputeNobPosFromValue();
	void recomputeValueFromNobPos();
private:
	bool _vertical;
	int _value;
	int _range[2];
	int _rangeWindow;
	bool _rangeWindowEnabled;
	int _nobPos[2]; // x, y of the nob
	int _nobSize;
	int _nobDragStartPos[2];
	int _dragStartPos[2];
	bool _dragging;
	int _buttonOffset;
	Dar<IntChangeSignal*> _intChangeSignalDar;
};

}

#endif
