#ifndef VGUI_LINEBORDER_H
#define VGUI_LINEBORDER_H

#include <VGUI.h>
#include <VGUI_Border.h>
#include <VGUI_Color.h>

namespace vgui
{

class LineBorder : public Border
{
public:
	LineBorder();
	LineBorder(Color color);
	LineBorder(int thickness);
	LineBorder(Color color, int thickness);
protected:
	virtual void paint(Panel* panel);
private:
	Color _color;
	int _thickness;
};

}

#endif
