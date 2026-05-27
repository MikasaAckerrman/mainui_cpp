#include <VGUI_Panel.h>
#include <VGUI_App.h>
#include <VGUI_SurfaceBase.h>
#include <VGUI_InputSignal.h>
#include <VGUI_Layout.h>
#include <VGUI_FocusNavGroup.h>
#include <VGUI_FocusChangeSignal.h>
#include <VGUI_Border.h>
#include <VGUI_BuildGroup.h>
#include <VGUI_Cursor.h>
#include <VGUI_Font.h>
#include <VGUI_LayoutInfo.h>
#include <VGUI_Scheme.h>
#include <string.h>

namespace vgui
{

Panel::Panel()
{
	init(0, 0, 64, 64);
}

Panel::Panel(int x, int y, int wide, int tall)
{
	init(x, y, wide, tall);
}

void Panel::init(int x, int y, int wide, int tall)
{
	_pos[0] = x;
	_pos[1] = y;
	_size[0] = wide;
	_size[1] = tall;
	_loc[0] = 0;
	_loc[1] = 0;
	_minimumSize[0] = 0;
	_minimumSize[1] = 0;
	_preferredSize[0] = 0;
	_preferredSize[1] = 0;
	_clipRect[0] = 0;
	_clipRect[1] = 0;
	_clipRect[2] = 0;
	_clipRect[3] = 0;
	_parent = null;
	_surfaceBase = null;
	_cursor = null;
	_schemeCursor = Scheme::scu_arrow;
	_visible = true;
	_layout = null;
	_needsLayout = false;
	_focusNavGroup = null;
	_autoFocusNavEnabled = false;
	_border = null;
	_needsRepaint = true;
	_enabled = true;
	_buildGroup = null;
	_layoutInfo = null;
	_paintBorderEnabled = true;
	_paintBackgroundEnabled = true;
	_paintEnabled = true;
}

void Panel::setPos(int x, int y)
{
	_pos[0] = x;
	_pos[1] = y;
}

void Panel::getPos(int& x, int& y)
{
	x = _pos[0];
	y = _pos[1];
}

void Panel::setSize(int wide, int tall)
{
	_size[0] = wide;
	_size[1] = tall;
}

void Panel::getSize(int& wide, int& tall)
{
	wide = _size[0];
	tall = _size[1];
}

void Panel::setBounds(int x, int y, int wide, int tall)
{
	setPos(x, y);
	setSize(wide, tall);
}

void Panel::getBounds(int& x, int& y, int& wide, int& tall)
{
	getPos(x, y);
	getSize(wide, tall);
}

int Panel::getWide()
{
	return _size[0];
}

int Panel::getTall()
{
	return _size[1];
}

Panel* Panel::getParent()
{
	return _parent;
}

void Panel::setVisible(bool state)
{
	_visible = state;
}

bool Panel::isVisible()
{
	return _visible;
}

bool Panel::isVisibleUp()
{
	if (!_visible)
		return false;
	if (_parent)
		return _parent->isVisibleUp();
	return true;
}

void Panel::repaint()
{
	_needsRepaint = true;
	for (int i = 0; i < _repaintSignalDar.getCount(); i++)
	{
		RepaintSignal* s = _repaintSignalDar[i];
		if (s)
			s->panelRepainted(this);
	}
}

void Panel::repaintAll()
{
	repaint();
	for (int i = 0; i < _childDar.getCount(); i++)
	{
		Panel* child = _childDar[i];
		if (child)
			child->repaintAll();
	}
}

void Panel::getAbsExtents(int& x0, int& y0, int& x1, int& y1)
{
	x0 = 0;
	y0 = 0;
	Panel* p = this;
	while (p)
	{
		x0 += p->_pos[0];
		y0 += p->_pos[1];
		p = p->_parent;
	}
	x1 = x0 + _size[0];
	y1 = y0 + _size[1];
}

void Panel::getClipRect(int& x0, int& y0, int& x1, int& y1)
{
	getAbsExtents(x0, y0, x1, y1);
	if (_parent)
	{
		int px0, py0, px1, py1;
		_parent->getClipRect(px0, py0, px1, py1);
		if (x0 < px0) x0 = px0;
		if (y0 < py0) y0 = py0;
		if (x1 > px1) x1 = px1;
		if (y1 > py1) y1 = py1;
	}
}

void Panel::setParent(Panel* newParent)
{
	if (_parent)
		_parent->_childDar.removeElement(this);
	_parent = newParent;
	if (_parent)
	{
		_surfaceBase = _parent->_surfaceBase;
	}
}

void Panel::addChild(Panel* child)
{
	if (child)
	{
		child->setParent(this);
		_childDar.putElement(child);
	}
}

void Panel::insertChildAt(Panel* child, int index)
{
	if (child)
	{
		child->setParent(this);
		_childDar.insertElementAt(child, index);
	}
}

void Panel::removeChild(Panel* child)
{
	if (child)
	{
		_childDar.removeElement(child);
		child->_parent = null;
		child->_surfaceBase = null;
	}
}

bool Panel::wasMousePressed(MouseCode code)
{
	App* app = App::getInstance();
	if (app)
		return app->wasMousePressed(code, this);
	return false;
}

bool Panel::wasMouseDoublePressed(MouseCode code)
{
	App* app = App::getInstance();
	if (app)
		return app->wasMouseDoublePressed(code, this);
	return false;
}

bool Panel::isMouseDown(MouseCode code)
{
	App* app = App::getInstance();
	if (app)
		return app->isMouseDown(code, this);
	return false;
}

bool Panel::wasMouseReleased(MouseCode code)
{
	App* app = App::getInstance();
	if (app)
		return app->wasMouseReleased(code, this);
	return false;
}

bool Panel::wasKeyPressed(KeyCode code)
{
	App* app = App::getInstance();
	if (app)
		return app->wasKeyPressed(code, this);
	return false;
}

bool Panel::isKeyDown(KeyCode code)
{
	App* app = App::getInstance();
	if (app)
		return app->isKeyDown(code, this);
	return false;
}

bool Panel::wasKeyTyped(KeyCode code)
{
	App* app = App::getInstance();
	if (app)
		return app->wasKeyTyped(code, this);
	return false;
}

bool Panel::wasKeyReleased(KeyCode code)
{
	App* app = App::getInstance();
	if (app)
		return app->wasKeyReleased(code, this);
	return false;
}

void Panel::addInputSignal(InputSignal* s)
{
	_inputSignalDar.addElement(s);
}

void Panel::removeInputSignal(InputSignal* s)
{
	_inputSignalDar.removeElement(s);
}

void Panel::addRepaintSignal(RepaintSignal* s)
{
	_repaintSignalDar.addElement(s);
}

void Panel::removeRepaintSignal(RepaintSignal* s)
{
	_repaintSignalDar.removeElement(s);
}

bool Panel::isWithin(int x, int y)
{
	int x0, y0, x1, y1;
	getAbsExtents(x0, y0, x1, y1);
	return (x >= x0 && x < x1 && y >= y0 && y < y1);
}

Panel* Panel::isWithinTraverse(int x, int y)
{
	if (!_visible)
		return null;
	// Check children in reverse order (front to back)
	for (int i = _childDar.getCount() - 1; i >= 0; i--)
	{
		Panel* child = _childDar[i];
		if (child)
		{
			Panel* result = child->isWithinTraverse(x, y);
			if (result)
				return result;
		}
	}
	if (isWithin(x, y))
		return this;
	return null;
}

void Panel::localToScreen(int& x, int& y)
{
	Panel* p = this;
	while (p)
	{
		x += p->_pos[0];
		y += p->_pos[1];
		p = p->_parent;
	}
}

void Panel::screenToLocal(int& x, int& y)
{
	Panel* p = this;
	while (p)
	{
		x -= p->_pos[0];
		y -= p->_pos[1];
		p = p->_parent;
	}
}

void Panel::setCursor(Cursor* cursor)
{
	_cursor = cursor;
}

void Panel::setCursor(Scheme::SchemeCursor scu)
{
	_schemeCursor = scu;
}

Cursor* Panel::getCursor()
{
	return _cursor;
}

void Panel::setMinimumSize(int wide, int tall)
{
	_minimumSize[0] = wide;
	_minimumSize[1] = tall;
}

void Panel::getMinimumSize(int& wide, int& tall)
{
	wide = _minimumSize[0];
	tall = _minimumSize[1];
}

void Panel::requestFocus()
{
	App* app = App::getInstance();
	if (app)
		app->requestFocus(this);
}

bool Panel::hasFocus()
{
	App* app = App::getInstance();
	if (app)
		return (app->getFocus() == this);
	return false;
}

int Panel::getChildCount()
{
	return _childDar.getCount();
}

Panel* Panel::getChild(int index)
{
	return _childDar[index];
}

void Panel::setLayout(Layout* layout)
{
	_layout = layout;
}

void Panel::invalidateLayout(bool layoutNow)
{
	_needsLayout = true;
	if (layoutNow)
		internalPerformLayout();
}

void Panel::setFocusNavGroup(FocusNavGroup* focusNavGroup)
{
	_focusNavGroup = focusNavGroup;
	if (_focusNavGroup)
		_focusNavGroup->addPanel(this);
}

void Panel::requestFocusPrev()
{
	if (_focusNavGroup)
		_focusNavGroup->requestFocusPrev();
}

void Panel::requestFocusNext()
{
	if (_focusNavGroup)
		_focusNavGroup->requestFocusNext();
}

void Panel::addFocusChangeSignal(FocusChangeSignal* s)
{
	_focusChangeSignalDar.addElement(s);
}

bool Panel::isAutoFocusNavEnabled()
{
	return _autoFocusNavEnabled;
}

void Panel::setAutoFocusNavEnabled(bool state)
{
	_autoFocusNavEnabled = state;
}

void Panel::setBorder(Border* border)
{
	_border = border;
}

void Panel::setPaintBorderEnabled(bool state)
{
	_paintBorderEnabled = state;
}

void Panel::setPaintBackgroundEnabled(bool state)
{
	_paintBackgroundEnabled = state;
}

void Panel::setPaintEnabled(bool state)
{
	_paintEnabled = state;
}

void Panel::getInset(int& left, int& top, int& right, int& bottom)
{
	if (_border)
		_border->getInset(left, top, right, bottom);
	else
	{
		left = 0;
		top = 0;
		right = 0;
		bottom = 0;
	}
}

void Panel::getPaintSize(int& wide, int& tall)
{
	getSize(wide, tall);
	int left, top, right, bottom;
	getInset(left, top, right, bottom);
	wide -= (left + right);
	tall -= (top + bottom);
	if (wide < 0) wide = 0;
	if (tall < 0) tall = 0;
}

void Panel::setPreferredSize(int wide, int tall)
{
	_preferredSize[0] = wide;
	_preferredSize[1] = tall;
}

void Panel::getPreferredSize(int& wide, int& tall)
{
	wide = _preferredSize[0];
	tall = _preferredSize[1];
}

SurfaceBase* Panel::getSurfaceBase()
{
	return _surfaceBase;
}

bool Panel::isEnabled()
{
	return _enabled;
}

void Panel::setEnabled(bool state)
{
	_enabled = state;
}

void Panel::setBuildGroup(BuildGroup* buildGroup, const char* panelPersistanceName)
{
	_buildGroup = buildGroup;
	if (_buildGroup)
		_buildGroup->panelAdded(this, panelPersistanceName);
}

bool Panel::isBuildGroupEnabled()
{
	if (_buildGroup)
		return _buildGroup->isEnabled();
	return false;
}

void Panel::removeAllChildren()
{
	for (int i = _childDar.getCount() - 1; i >= 0; i--)
	{
		Panel* child = _childDar[i];
		if (child)
		{
			child->_parent = null;
			child->_surfaceBase = null;
		}
	}
	_childDar.removeAll();
}

void Panel::repaintParent()
{
	if (_parent)
		_parent->repaint();
}

Panel* Panel::createPropertyPanel()
{
	return null;
}

void Panel::getPersistanceText(char* buf, int bufLen)
{
	if (buf && bufLen > 0)
		buf[0] = 0;
}

void Panel::applyPersistanceText(const char* buf)
{
	// Stub
}

void Panel::setFgColor(Scheme::SchemeColor sc)
{
	_fgColor.setColor(sc);
}

void Panel::setBgColor(Scheme::SchemeColor sc)
{
	_bgColor.setColor(sc);
}

void Panel::setFgColor(int r, int g, int b, int a)
{
	_fgColor.setColor(r, g, b, a);
}

void Panel::setBgColor(int r, int g, int b, int a)
{
	_bgColor.setColor(r, g, b, a);
}

void Panel::getFgColor(int& r, int& g, int& b, int& a)
{
	_fgColor.getColor(r, g, b, a);
}

void Panel::getBgColor(int& r, int& g, int& b, int& a)
{
	_bgColor.getColor(r, g, b, a);
}

void Panel::setBgColor(Color color)
{
	_bgColor = color;
}

void Panel::setFgColor(Color color)
{
	_fgColor = color;
}

void Panel::getBgColor(Color& color)
{
	color = _bgColor;
}

void Panel::getFgColor(Color& color)
{
	color = _fgColor;
}

void Panel::setAsMouseCapture(bool state)
{
	App* app = App::getInstance();
	if (app)
		app->setMouseCapture(state ? this : null);
}

void Panel::setAsMouseArena(bool state)
{
	App* app = App::getInstance();
	if (app)
		app->setMouseArena(state ? this : null);
}

App* Panel::getApp()
{
	return App::getInstance();
}

void Panel::getVirtualSize(int& wide, int& tall)
{
	getSize(wide, tall);
}

void Panel::setLayoutInfo(LayoutInfo* layoutInfo)
{
	_layoutInfo = layoutInfo;
}

LayoutInfo* Panel::getLayoutInfo()
{
	return _layoutInfo;
}

bool Panel::isCursorNone()
{
	return (_schemeCursor == Scheme::scu_none);
}

void Panel::solveTraverse()
{
	solve();
	for (int i = 0; i < _childDar.getCount(); i++)
	{
		Panel* child = _childDar[i];
		if (child)
			child->solveTraverse();
	}
}

void Panel::paintTraverse()
{
	paintTraverse(true);
}

void Panel::setSurfaceBaseTraverse(SurfaceBase* surfaceBase)
{
	_surfaceBase = surfaceBase;
	for (int i = 0; i < _childDar.getCount(); i++)
	{
		Panel* child = _childDar[i];
		if (child)
			child->setSurfaceBaseTraverse(surfaceBase);
	}
}

void Panel::performLayout()
{
	// Base implementation - empty
}

void Panel::internalPerformLayout()
{
	performLayout();
	if (_layout)
		_layout->performLayout(this);
	_needsLayout = false;
}

void Panel::drawSetColor(Scheme::SchemeColor sc)
{
	if (_surfaceBase)
	{
		Scheme* scheme = App::getInstance() ? App::getInstance()->getScheme() : null;
		if (scheme)
		{
			int r, g, b, a;
			scheme->getColor(sc, r, g, b, a);
			_surfaceBase->drawSetColor(r, g, b, a);
		}
	}
}

void Panel::drawSetColor(int r, int g, int b, int a)
{
	if (_surfaceBase)
		_surfaceBase->drawSetColor(r, g, b, a);
}

void Panel::drawFilledRect(int x0, int y0, int x1, int y1)
{
	if (_surfaceBase)
		_surfaceBase->drawFilledRect(x0, y0, x1, y1);
}

void Panel::drawOutlinedRect(int x0, int y0, int x1, int y1)
{
	if (_surfaceBase)
		_surfaceBase->drawOutlinedRect(x0, y0, x1, y1);
}

void Panel::drawSetTextFont(Scheme::SchemeFont sf)
{
	if (_surfaceBase)
	{
		Scheme* scheme = App::getInstance() ? App::getInstance()->getScheme() : null;
		if (scheme)
		{
			Font* font = scheme->getFont(sf);
			if (font)
				_surfaceBase->drawSetTextFont(font);
		}
	}
}

void Panel::drawSetTextFont(Font* font)
{
	if (_surfaceBase)
		_surfaceBase->drawSetTextFont(font);
}

void Panel::drawSetTextColor(Scheme::SchemeColor sc)
{
	if (_surfaceBase)
	{
		Scheme* scheme = App::getInstance() ? App::getInstance()->getScheme() : null;
		if (scheme)
		{
			int r, g, b, a;
			scheme->getColor(sc, r, g, b, a);
			_surfaceBase->drawSetTextColor(r, g, b, a);
		}
	}
}

void Panel::drawSetTextColor(int r, int g, int b, int a)
{
	if (_surfaceBase)
		_surfaceBase->drawSetTextColor(r, g, b, a);
}

void Panel::drawSetTextPos(int x, int y)
{
	if (_surfaceBase)
		_surfaceBase->drawSetTextPos(x, y);
}

void Panel::drawPrintText(const char* str, int strlen)
{
	if (_surfaceBase)
		_surfaceBase->drawPrintText(str, strlen);
}

void Panel::drawPrintText(int x, int y, const char* str, int strlen)
{
	if (_surfaceBase)
	{
		_surfaceBase->drawSetTextPos(x, y);
		_surfaceBase->drawPrintText(str, strlen);
	}
}

void Panel::drawPrintChar(char ch)
{
	if (_surfaceBase)
		_surfaceBase->drawPrintText(&ch, 1);
}

void Panel::drawPrintChar(int x, int y, char ch)
{
	if (_surfaceBase)
	{
		_surfaceBase->drawSetTextPos(x, y);
		_surfaceBase->drawPrintText(&ch, 1);
	}
}

void Panel::drawSetTextureRGBA(int id, const char* rgba, int wide, int tall)
{
	if (_surfaceBase)
		_surfaceBase->drawSetTextureRGBA(id, rgba, wide, tall);
}

void Panel::drawSetTexture(int id)
{
	if (_surfaceBase)
		_surfaceBase->drawSetTexture(id);
}

void Panel::drawTexturedRect(int x0, int y0, int x1, int y1)
{
	if (_surfaceBase)
		_surfaceBase->drawTexturedRect(x0, y0, x1, y1);
}

void Panel::solve()
{
	_loc[0] = _pos[0];
	_loc[1] = _pos[1];

	int x0, y0, x1, y1;
	getAbsExtents(x0, y0, x1, y1);
	_clipRect[0] = x0;
	_clipRect[1] = y0;
	_clipRect[2] = x1;
	_clipRect[3] = y1;

	if (_parent)
	{
		int px0, py0, px1, py1;
		_parent->getClipRect(px0, py0, px1, py1);
		if (_clipRect[0] < px0) _clipRect[0] = px0;
		if (_clipRect[1] < py0) _clipRect[1] = py0;
		if (_clipRect[2] > px1) _clipRect[2] = px1;
		if (_clipRect[3] > py1) _clipRect[3] = py1;
	}

	if (_needsLayout)
		internalPerformLayout();
}

void Panel::paintTraverse(bool repaint)
{
	if (!_visible)
		return;

	if (_surfaceBase)
	{
		_surfaceBase->pushMakeCurrent(this, true);

		if (_paintBackgroundEnabled)
			paintBackground();

		if (_paintEnabled)
			paint();

		paintBuildOverlay();

		for (int i = 0; i < _childDar.getCount(); i++)
		{
			Panel* child = _childDar[i];
			if (child)
				child->paintTraverse(repaint);
		}

		if (_border && _paintBorderEnabled)
			_border->doPaint(this);

		_surfaceBase->popMakeCurrent(this);
	}

	_needsRepaint = false;
}

void Panel::paintBackground()
{
	// Base implementation - empty
}

void Panel::paint()
{
	// Base implementation - empty
}

void Panel::paintBuildOverlay()
{
	// Base implementation - empty
}

void Panel::internalCursorMoved(int x, int y)
{
	for (int i = 0; i < _inputSignalDar.getCount(); i++)
	{
		InputSignal* s = _inputSignalDar[i];
		if (s)
			s->cursorMoved(x, y, this);
	}
}

void Panel::internalCursorEntered()
{
	for (int i = 0; i < _inputSignalDar.getCount(); i++)
	{
		InputSignal* s = _inputSignalDar[i];
		if (s)
			s->cursorEntered(this);
	}
}

void Panel::internalCursorExited()
{
	for (int i = 0; i < _inputSignalDar.getCount(); i++)
	{
		InputSignal* s = _inputSignalDar[i];
		if (s)
			s->cursorExited(this);
	}
}

void Panel::internalMousePressed(MouseCode code)
{
	for (int i = 0; i < _inputSignalDar.getCount(); i++)
	{
		InputSignal* s = _inputSignalDar[i];
		if (s)
			s->mousePressed(code, this);
	}
}

void Panel::internalMouseDoublePressed(MouseCode code)
{
	for (int i = 0; i < _inputSignalDar.getCount(); i++)
	{
		InputSignal* s = _inputSignalDar[i];
		if (s)
			s->mouseDoublePressed(code, this);
	}
}

void Panel::internalMouseReleased(MouseCode code)
{
	for (int i = 0; i < _inputSignalDar.getCount(); i++)
	{
		InputSignal* s = _inputSignalDar[i];
		if (s)
			s->mouseReleased(code, this);
	}
}

void Panel::internalMouseWheeled(int delta)
{
	for (int i = 0; i < _inputSignalDar.getCount(); i++)
	{
		InputSignal* s = _inputSignalDar[i];
		if (s)
			s->mouseWheeled(delta, this);
	}
}

void Panel::internalKeyPressed(KeyCode code)
{
	for (int i = 0; i < _inputSignalDar.getCount(); i++)
	{
		InputSignal* s = _inputSignalDar[i];
		if (s)
			s->keyPressed(code, this);
	}
}

void Panel::internalKeyTyped(KeyCode code)
{
	for (int i = 0; i < _inputSignalDar.getCount(); i++)
	{
		InputSignal* s = _inputSignalDar[i];
		if (s)
			s->keyTyped(code, this);
	}
}

void Panel::internalKeyReleased(KeyCode code)
{
	for (int i = 0; i < _inputSignalDar.getCount(); i++)
	{
		InputSignal* s = _inputSignalDar[i];
		if (s)
			s->keyReleased(code, this);
	}
}

void Panel::internalKeyFocusTicked()
{
	for (int i = 0; i < _inputSignalDar.getCount(); i++)
	{
		InputSignal* s = _inputSignalDar[i];
		if (s)
			s->keyFocusTicked(this);
	}
}

void Panel::internalFocusChanged(bool lost)
{
	for (int i = 0; i < _focusChangeSignalDar.getCount(); i++)
	{
		FocusChangeSignal* s = _focusChangeSignalDar[i];
		if (s)
			s->focusChanged(lost, this);
	}
}

void Panel::internalSetCursor()
{
	if (_surfaceBase)
	{
		if (_cursor)
			_surfaceBase->setCursor(_cursor);
		else
		{
			Scheme* scheme = App::getInstance() ? App::getInstance()->getScheme() : null;
			if (scheme)
			{
				Cursor* c = scheme->getCursor(_schemeCursor);
				if (c)
					_surfaceBase->setCursor(c);
			}
		}
	}
}

}
