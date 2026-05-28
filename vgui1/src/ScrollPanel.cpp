#include <VGUI_ScrollPanel.h>
#include <VGUI_ScrollBar.h>

namespace vgui
{

ScrollPanel::ScrollPanel(int x, int y, int wide, int tall) : Panel(x, y, wide, tall)
{
	int sbSize = 16;

	_client = new Panel(0, 0, wide - sbSize, tall - sbSize);
	addChild(_client);

	_verticalScrollBar = new ScrollBar(wide - sbSize, 0, sbSize, tall - sbSize, true);
	addChild(_verticalScrollBar);

	_horizontalScrollBar = new ScrollBar(0, tall - sbSize, wide - sbSize, sbSize, false);
	addChild(_horizontalScrollBar);

	_autoVisible[0] = true;
	_autoVisible[1] = true;
}

Panel* ScrollPanel::getClient()
{
	return _client;
}

ScrollBar* ScrollPanel::getHorizontalScrollBar()
{
	return _horizontalScrollBar;
}

ScrollBar* ScrollPanel::getVerticalScrollBar()
{
	return _verticalScrollBar;
}

void ScrollPanel::setScrollBarVisible(bool horizontal, bool vertical)
{
	_autoVisible[0] = horizontal;
	_autoVisible[1] = vertical;
	if (_horizontalScrollBar)
		_horizontalScrollBar->setVisible(horizontal);
	if (_verticalScrollBar)
		_verticalScrollBar->setVisible(vertical);
	invalidateLayout(true);
}

void ScrollPanel::validate()
{
	performLayout();
}

void ScrollPanel::performLayout()
{
	int wide, tall;
	getSize(wide, tall);
	int sbSize = 16;

	bool showH = _autoVisible[0];
	bool showV = _autoVisible[1];

	int clientW = showV ? wide - sbSize : wide;
	int clientH = showH ? tall - sbSize : tall;

	if (_client)
		_client->setBounds(0, 0, clientW, clientH);

	if (_verticalScrollBar)
	{
		_verticalScrollBar->setBounds(clientW, 0, sbSize, clientH);
		_verticalScrollBar->setVisible(showV);
	}

	if (_horizontalScrollBar)
	{
		_horizontalScrollBar->setBounds(0, clientH, clientW, sbSize);
		_horizontalScrollBar->setVisible(showH);
	}
}

void ScrollPanel::paintBackground()
{
	// Transparent - client draws itself
}

void ScrollPanel::paint()
{
	// Children handle painting
}

}
