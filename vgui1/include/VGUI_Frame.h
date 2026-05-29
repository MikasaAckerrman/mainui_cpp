#ifndef VGUI_FRAME_H
#define VGUI_FRAME_H

#include <VGUI.h>
#include <VGUI_Panel.h>

namespace vgui
{

class Button;

class Frame : public Panel
{
public:
	Frame(int x, int y, int wide, int tall);
public:
	virtual void setTitle(const char* title);
	virtual void getTitle(char* buf, int bufLen);
	virtual void setMoveable(bool state);
	virtual bool isMoveable();
	virtual void setSizeable(bool state);
	virtual bool isSizeable();
	virtual void setVisible(bool state);
	virtual Panel* getClient();
	virtual void setInternal(bool state);
	virtual void setSmallCaption(bool state);
	virtual void setSize(int wide, int tall);
protected:
	virtual void paintBackground();
	virtual void paint();
	virtual void internalCursorMoved(int x, int y);
	virtual void internalMousePressed(MouseCode code);
	virtual void internalMouseReleased(MouseCode code);
private:
	void drawTitleBar(int wide);
private:
	char _title[128];
	Panel* _topGrip;
	Panel* _bottomGrip;
	Panel* _leftGrip;
	Panel* _rightGrip;
	Panel* _topLeftGrip;
	Panel* _topRightGrip;
	Panel* _bottomLeftGrip;
	Panel* _bottomRightGrip;
	Panel* _captionBar;
	Panel* _client;
	Button* _closeButton;
	Button* _minimizeButton;
	Button* _maximizeButton;
	bool _moveable;
	bool _sizeable;
	bool _internal;
	bool _smallCaption;
	bool _dragging;
	bool _resizing;
	bool _lastCursorValid;
	int _lastCursor[2];
	int _resizeZone;
	// Legacy fields kept for ABI compatibility (unused)
	int _dragOrgPos[2];
	int _dragOrgCursor[2];
	int _dragOrgSize[2];
	bool _dragAnchorReady;
};

}

#endif
