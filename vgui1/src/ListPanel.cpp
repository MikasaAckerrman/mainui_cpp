#include <VGUI_ListPanel.h>
#include <VGUI_ScrollBar.h>
#include <VGUI_Label.h>
#include <VGUI_App.h>

namespace vgui
{

ListPanel::ListPanel(int x, int y, int wide, int tall) : Panel(x, y, wide, tall)
{
	_pixelScroll = 0;
	_selectedIndex = -1;

	// Create vertical scrollbar on right side
	int sbWidth = 16;
	_vscrollBar = new ScrollBar(wide - sbWidth, 0, sbWidth, tall, true);
	addChild(_vscrollBar);

	setBgColor(255, 255, 255, 0);
}

void ListPanel::setPixelScroll(int value)
{
	_pixelScroll = value;
	repaint();
}

void ListPanel::addString(const char* str)
{
	int wide, tall;
	getSize(wide, tall);
	Label* label = new Label(str, 0, 0, wide - 16, 16);
	label->setFgColor(0, 0, 0, 0);
	addItem(label);
}

void ListPanel::addItem(Panel* panel)
{
	if (panel)
	{
		_itemDar.addElement(panel);
		addChild(panel);
		invalidateLayout(true);
	}
}

int ListPanel::getItemCount()
{
	return _itemDar.getCount();
}

Panel* ListPanel::getItem(int index)
{
	if (index >= 0 && index < _itemDar.getCount())
		return _itemDar[index];
	return null;
}

void ListPanel::removeItem(int index)
{
	if (index >= 0 && index < _itemDar.getCount())
	{
		Panel* item = _itemDar[index];
		_itemDar.removeElementAt(index);
		if (item)
		{
			removeChild(item);
		}
		invalidateLayout(true);
	}
}

void ListPanel::setSelectedIndex(int index)
{
	_selectedIndex = index;
	repaint();
}

int ListPanel::getSelectedIndex()
{
	return _selectedIndex;
}

void ListPanel::performLayout()
{
	int wide, tall;
	getSize(wide, tall);
	int sbWidth = 16;
	int itemWide = wide - sbWidth;

	int yPos = -_pixelScroll;
	for (int i = 0; i < _itemDar.getCount(); i++)
	{
		Panel* item = _itemDar[i];
		if (item)
		{
			int iw, ih;
			item->getSize(iw, ih);
			item->setBounds(0, yPos, itemWide, ih);
			yPos += ih;
		}
	}

	// Update scrollbar range
	if (_vscrollBar)
	{
		_vscrollBar->setBounds(wide - sbWidth, 0, sbWidth, tall);
		_vscrollBar->setRange(0, yPos + _pixelScroll > tall ? yPos + _pixelScroll - tall : 0);
	}
}

void ListPanel::paintBackground()
{
	int wide, tall;
	getSize(wide, tall);

	int r, g, b, a;
	getBgColor(r, g, b, a);
	drawSetColor(r, g, b, a);
	drawFilledRect(0, 0, wide - 16, tall);

	// Draw selection highlight
	if (_selectedIndex >= 0 && _selectedIndex < _itemDar.getCount())
	{
		Panel* item = _itemDar[_selectedIndex];
		if (item)
		{
			int ix, iy, iw, ih;
			item->getBounds(ix, iy, iw, ih);
			drawSetColor(0, 0, 128, 0);
			drawFilledRect(ix, iy, ix + iw, iy + ih);
		}
	}
}

void ListPanel::paint()
{
	// Items paint themselves
}

void ListPanel::internalMousePressed(MouseCode code)
{
	if (code == MOUSE_LEFT)
	{
		App* app = App::getInstance();
		if (app)
		{
			int mx, my;
			app->getCursorPos(mx, my);
			screenToLocal(mx, my);

			// Find which item was clicked
			for (int i = 0; i < _itemDar.getCount(); i++)
			{
				Panel* item = _itemDar[i];
				if (item)
				{
					int ix, iy, iw, ih;
					item->getBounds(ix, iy, iw, ih);
					if (my >= iy && my < iy + ih && mx >= ix && mx < ix + iw)
					{
						_selectedIndex = i;
						repaint();
						break;
					}
				}
			}
		}
	}
	Panel::internalMousePressed(code);
}

void ListPanel::internalMouseWheeled(int delta)
{
	_pixelScroll -= delta * 16;
	if (_pixelScroll < 0) _pixelScroll = 0;
	invalidateLayout(true);
	repaint();
	Panel::internalMouseWheeled(delta);
}

}
