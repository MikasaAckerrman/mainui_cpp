#ifndef VGUI_SCROLLPANEL_H
#define VGUI_SCROLLPANEL_H

#include <VGUI.h>
#include <VGUI_Panel.h>

namespace vgui
{

class ScrollBar;

class ScrollPanel : public Panel
{
public:
	ScrollPanel(int x, int y, int wide, int tall);
public:
	virtual Panel* getClient();
	virtual ScrollBar* getHorizontalScrollBar();
	virtual ScrollBar* getVerticalScrollBar();
	virtual void setScrollBarVisible(bool horizontal, bool vertical);
	virtual void validate();
protected:
	virtual void paint();
	virtual void paintBackground();
	virtual void performLayout();
private:
	Panel* _client;
	ScrollBar* _horizontalScrollBar;
	ScrollBar* _verticalScrollBar;
	bool _autoVisible[2]; // h, v
};

}

#endif
