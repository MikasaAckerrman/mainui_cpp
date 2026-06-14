// VguiCreateGame.cpp - Pixel-accurate CS 1.6 GoldSrc VGUI "Создать сервер"
// (Create Server) dialog.
//
// Layout (matches the PC CS 1.6 "Создать сервер" window):
//   - Title bar: Steam icon + "Создать сервер" + close (X) button  (drawn by Frame)
//   - 480x380 reference units, FIXED size (drag-move only, not resizable)
//   - Three tabs: "Сервер" | "Игра" | "Настройки ботов"
//   - Bottom button row (right-aligned, 10px from right/bottom):
//       "Запуск" (80x24) + "Отмена" (80x24)
//
// Because VGUI1 has no native ComboBox / RadioButton / scroll container, this
// file implements lightweight ones (SimpleCombo, SimpleRadioGroup, SimpleCheck,
// VScrollPanel) on top of Panel, following the drawing conventions used by
// VguiOptionsDialog.cpp and VguiConsole.cpp. All coordinates use VS() scaling
// from the 640x480 reference grid.

// Heavy mainui headers FIRST to avoid the `null` macro clash documented in
// VGUI_SchemeColors.h (TrackerScheme.h pulls in EventSystem.h which uses a
// `null` parameter name that collides with VGUI's `null` macro).
extern void UI_FillRect( int x, int y, int width, int height, const unsigned int color );
extern void UI_EnableTextInput( bool enable );
#include "BaseMenu.h"
#include "FontManager.h"
#include "TrackerScheme.h"

#include <VGUI_Log.h>
#include <VGUI_SchemeColors.h>
#include <VGUI_UIScale.h>
#include <VGUI_Scheme.h>
#include <VGUI_Frame.h>
#include <VGUI_Panel.h>
#include <VGUI_Label.h>
#include <VGUI_Button.h>
#include <VGUI_CheckButton.h>
#include <VGUI_TextEntry.h>
#include <VGUI_TabPanel.h>
#include <VGUI_App.h>
#include <VGUI_ActionSignal.h>
#include <VGUI_CvarBridge.h>
#include <VGUI_Dar.h>
#include <VGUI_CreateGame.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// Lazy-init entry point implemented in vgui_main.cpp (ensures App/Scheme/root
// panel exist before we add the window).
extern "C" void VGUI_EnsureInitialized(int screenW, int screenH);

namespace vgui
{

// ====================================================================
// Layout constants (640x480 reference, scaled via VS). 480x380 dialog.
// ====================================================================
static const int CG_DLG_W = 480;
static const int CG_DLG_H = 380;

// Canonical CS 1.6 selected/active accent (gold-orange).
#define CG_ORANGE 0xFFB8A010u

// ====================================================================
// SimpleCheck - a 14x14 checkbox + label. Checked = orange vector tick.
// ====================================================================
class SimpleCheck : public Panel
{
public:
	SimpleCheck(const char* text, bool checked, int x, int y, int w, int h)
		: Panel(x, y, w, h), _checked(checked)
	{
		_text[0] = 0;
		if (text) vgui_strcpy(_text, sizeof(_text), text);
		setPaintBackgroundEnabled(true);
	}

	bool isChecked() { return _checked; }
	void setChecked(bool state) { _checked = state; repaint(); }

protected:
	virtual void paintBackground()
	{
		int wide, tall;
		getSize(wide, tall);

		int box = VS(14);
		int boxY = (tall - box) / 2;
		if (boxY < 0) boxY = 0;

		unsigned int fieldBg = g_Scheme.fieldBgColor ? g_Scheme.fieldBgColor : 0xFF3E4637;
		unsigned int bright  = g_Scheme.borderBright ? g_Scheme.borderBright : 0xFF889180;
		unsigned int dark    = g_Scheme.borderDark   ? g_Scheme.borderDark   : 0xFF282E22;

		// Box background + sunken 1px inset border.
		schemeBgColor(this, fieldBg);
		drawFilledRect(0, boxY, box, boxY + box);
		schemeBgColor(this, dark);
		drawFilledRect(0, boxY, box, boxY + 1);
		drawFilledRect(0, boxY, 1, boxY + box);
		schemeBgColor(this, bright);
		drawFilledRect(0, boxY + box - 1, box, boxY + box);
		drawFilledRect(box - 1, boxY, box, boxY + box);

		// Orange vector check mark (two strokes forming a tick).
		if (_checked)
		{
			int t = VS(2);
			if (t < 1) t = 1;
			int cx = box / 2;
			int cy = boxY + box / 2;
			schemeBgColor(this, CG_ORANGE);
			// short down-right stroke
			drawFilledRect(cx - VS(3), cy,         cx - VS(3) + t, cy + VS(3));
			drawFilledRect(cx - VS(3), cy + VS(2), cx,             cy + VS(3));
			// long up-right stroke
			drawFilledRect(cx - VS(1), cy + VS(1), cx + t,         cy + VS(4));
			drawFilledRect(cx,         cy - VS(3), cx + t,         cy + VS(3));
		}

		// Label text.
		unsigned int textCol = g_Scheme.labelTextColor ? g_Scheme.labelTextColor : 0xFFD8DED3;
		schemeFgColor(this, textCol);
		drawSetTextFont(Scheme::sf_primary1);
		int textY = (tall - VS(11)) / 2;
		if (textY < 0) textY = 0;
		drawPrintText(box + VS(6), textY, _text, (int)strlen(_text));
	}

	virtual void internalMousePressed(MouseCode code)
	{
		if (code == MOUSE_LEFT)
		{
			_checked = !_checked;
			repaint();
		}
		Panel::internalMousePressed(code);
	}

private:
	char _text[128];
	bool _checked;
};

// ====================================================================
// SimpleRadioGroup / SimpleRadioButton - mutually exclusive radio set.
// Each button: 14x14 circle + label. Selected = filled orange disc and
// orange label text.
// ====================================================================
class SimpleRadioGroup;

class SimpleRadioButton : public Panel
{
public:
	SimpleRadioButton(SimpleRadioGroup* group, int index, const char* text,
	                  int x, int y, int w, int h)
		: Panel(x, y, w, h), _group(group), _index(index)
	{
		_text[0] = 0;
		if (text) vgui_strcpy(_text, sizeof(_text), text);
		setPaintBackgroundEnabled(true);
	}

	int getIndex() { return _index; }

protected:
	virtual void paintBackground();          // defined after SimpleRadioGroup
	virtual void internalMousePressed(MouseCode code);

private:
	void fillDisc(int cx, int cy, int r, unsigned int col)
	{
		schemeBgColor(this, col);
		for (int dy = -r; dy <= r; dy++)
		{
			int span = (int)(sqrt((double)(r * r - dy * dy)) + 0.5);
			drawFilledRect(cx - span, cy + dy, cx + span + 1, cy + dy + 1);
		}
	}

	char _text[128];
	SimpleRadioGroup* _group;
	int _index;
	bool isSelectedInGroup();
};

class SimpleRadioGroup
{
public:
	SimpleRadioGroup() : _selected(0) {}

	void addButton(SimpleRadioButton* b) { _btns.addElement(b); }
	int  getSelectedIndex() { return _selected; }

	void setSelectedIndex(int i)
	{
		_selected = i;
		for (int k = 0; k < _btns.getCount(); k++)
			if (_btns[k]) _btns[k]->repaint();
	}

private:
	Dar<SimpleRadioButton*> _btns;
	int _selected;
};

bool SimpleRadioButton::isSelectedInGroup()
{
	return _group && _group->getSelectedIndex() == _index;
}

void SimpleRadioButton::paintBackground()
{
	int wide, tall;
	getSize(wide, tall);

	bool sel = isSelectedInGroup();

	int r  = VS(7);
	int cx = r;
	int cy = tall / 2;

	unsigned int ring   = g_Scheme.borderBright ? g_Scheme.borderBright : 0xFF889180;
	unsigned int hollow = g_Scheme.fieldBgColor ? g_Scheme.fieldBgColor : 0xFF3E4637;

	// Outer ring, hollow interior, then filled orange dot when selected.
	fillDisc(cx, cy, r, ring);
	fillDisc(cx, cy, r - VS(1), hollow);
	if (sel)
		fillDisc(cx, cy, r - VS(3), CG_ORANGE);

	// Label text (orange when active, else normal label color).
	unsigned int textCol = sel ? CG_ORANGE
		: (g_Scheme.labelTextColor ? g_Scheme.labelTextColor : 0xFFD8DED3);
	schemeFgColor(this, textCol);
	drawSetTextFont(Scheme::sf_primary1);
	int textY = (tall - VS(11)) / 2;
	if (textY < 0) textY = 0;
	drawPrintText(2 * r + VS(6), textY, _text, (int)strlen(_text));
}

void SimpleRadioButton::internalMousePressed(MouseCode code)
{
	if (code == MOUSE_LEFT && _group)
		_group->setSelectedIndex(_index);
	Panel::internalMousePressed(code);
}

// ====================================================================
// SimpleCombo - a dropdown emulation. Button-like panel with text + "▼".
// Clicking opens a popup list below; clicking an item selects it; clicking
// outside (popup holds mouse capture) closes it.
// ====================================================================
class SimpleCombo;

class ComboPopup : public Panel
{
public:
	ComboPopup(SimpleCombo* owner, const char* const* items, int count,
	           int x, int y, int w, int h)
		: Panel(x, y, w, h), _owner(owner), _items(items), _count(count)
	{
		_itemH = VS(20);
		setVisible(false);
	}

protected:
	virtual void paintBackground();              // defined after SimpleCombo
	virtual void internalMousePressed(MouseCode code);

private:
	SimpleCombo* _owner;
	const char* const* _items;
	int _count;
	int _itemH;
};

class SimpleCombo : public Panel
{
public:
	SimpleCombo(const char* const* items, int count, int x, int y, int w, int h)
		: Panel(x, y, w, h), _items(items), _count(count), _sel(0),
		  _popup(0), _orangeSelected(false)
	{
		setPaintBackgroundEnabled(true);
	}

	int  getSelectedIndex() { return _sel; }
	void setSelectedIndex(int i)
	{
		if (i >= 0 && i < _count) { _sel = i; repaint(); }
	}
	const char* getSelectedText()
	{
		return (_sel >= 0 && _sel < _count && _items[_sel]) ? _items[_sel] : "";
	}
	// Tab 1 "Расположение" shows its selection in orange.
	void setOrangeWhenSelected(bool b) { _orangeSelected = b; }

	int  itemCount() { return _count; }
	const char* itemText(int i) { return (i >= 0 && i < _count && _items[i]) ? _items[i] : ""; }

	void onItemPicked(int i) { setSelectedIndex(i); closePopup(); }
	void closePopup()
	{
		if (_popup) { _popup->setAsMouseCapture(false); _popup->setVisible(false); }
		repaint();
	}

protected:
	virtual void paintBackground()
	{
		int wide, tall;
		getSize(wide, tall);
		int arrowW = VS(16);

		unsigned int fieldBg = g_Scheme.fieldBgColor ? g_Scheme.fieldBgColor : 0xFF3E4637;
		unsigned int bright  = g_Scheme.borderBright ? g_Scheme.borderBright : 0xFF889180;
		unsigned int dark    = g_Scheme.borderDark   ? g_Scheme.borderDark   : 0xFF282E22;

		// Field background + sunken inset border.
		schemeBgColor(this, fieldBg);
		drawFilledRect(0, 0, wide, tall);
		schemeBgColor(this, dark);
		drawFilledRect(0, 0, wide, 1);
		drawFilledRect(0, 0, 1, tall);
		schemeBgColor(this, bright);
		drawFilledRect(0, tall - 1, wide, tall);
		drawFilledRect(wide - 1, 0, wide, tall);

		// Arrow zone separator on the right edge.
		schemeBgColor(this, dark);
		drawFilledRect(wide - arrowW - 1, 2, wide - arrowW, tall - 2);

		// Selected text.
		unsigned int textCol = (_orangeSelected)
			? CG_ORANGE
			: (g_Scheme.fieldTextColor ? g_Scheme.fieldTextColor : 0xFFD8DED3);
		schemeFgColor(this, textCol);
		drawSetTextFont(Scheme::sf_primary1);
		const char* v = getSelectedText();
		int textY = (tall - VS(11)) / 2;
		if (textY < 2) textY = 2;
		drawPrintText(VS(6), textY, v, (int)strlen(v));

		// Down triangle "▼" in the arrow zone.
		unsigned int glyph = g_Scheme.buttonTextColor ? g_Scheme.buttonTextColor : 0xFFD8DED3;
		schemeBgColor(this, glyph);
		int cx = wide - arrowW / 2;
		int cy = tall / 2 - VS(1);
		int t = VS(1) > 0 ? VS(1) : 1;
		drawFilledRect(cx - VS(3), cy,         cx + VS(3), cy + t);
		drawFilledRect(cx - VS(2), cy + t,     cx + VS(2), cy + 2 * t);
		drawFilledRect(cx - VS(1), cy + 2 * t, cx + VS(1), cy + 3 * t);
	}

	virtual void internalMousePressed(MouseCode code)
	{
		if (code == MOUSE_LEFT)
		{
			if (_popup && _popup->isVisible())
				closePopup();
			else
				openPopup();
		}
		Panel::internalMousePressed(code);
	}

private:
	void openPopup()
	{
		Panel* par = getParent();
		if (!par) return;
		if (!_popup)
		{
			int x, y;  getPos(x, y);
			int w, h;  getSize(w, h);
			_popup = new ComboPopup(this, _items, _count, x, y + h, w, _count * VS(20) + 2);
			par->addChild(_popup);
		}
		_popup->setVisible(true);
		_popup->setAsMouseCapture(true);
		repaint();
	}

	const char* const* _items;
	int _count;
	int _sel;
	ComboPopup* _popup;
	bool _orangeSelected;
};

void ComboPopup::paintBackground()
{
	int wide, tall;
	getSize(wide, tall);

	unsigned int fieldBg = g_Scheme.fieldBgColor ? g_Scheme.fieldBgColor : 0xFF3E4637;
	unsigned int bright  = g_Scheme.borderBright ? g_Scheme.borderBright : 0xFF889180;
	unsigned int dark    = g_Scheme.borderDark   ? g_Scheme.borderDark   : 0xFF282E22;

	// Popup background + outline.
	schemeBgColor(this, fieldBg);
	drawFilledRect(0, 0, wide, tall);
	schemeBgColor(this, dark);
	drawFilledRect(0, 0, wide, 1);
	drawFilledRect(0, 0, 1, tall);
	drawFilledRect(0, tall - 1, wide, tall);
	drawFilledRect(wide - 1, 0, wide, tall);

	drawSetTextFont(Scheme::sf_primary1);
	int active = _owner ? _owner->getSelectedIndex() : -1;
	for (int i = 0; i < _count; i++)
	{
		int iy = 1 + i * _itemH;
		bool sel = (i == active);
		if (sel)
		{
			unsigned int selBg = g_Scheme.listSelectedBgColor ? g_Scheme.listSelectedBgColor : 0xFF4C5844;
			schemeBgColor(this, selBg);
			drawFilledRect(1, iy, wide - 1, iy + _itemH);
		}
		unsigned int col = sel ? CG_ORANGE
			: (g_Scheme.listTextColor ? g_Scheme.listTextColor : 0xFFD8DED3);
		schemeFgColor(this, col);
		const char* s = (_items[i] ? _items[i] : "");
		int textY = iy + (_itemH - VS(11)) / 2;
		drawPrintText(VS(6), textY, s, (int)strlen(s));
	}
}

void ComboPopup::internalMousePressed(MouseCode code)
{
	if (code == MOUSE_LEFT)
	{
		App* app = App::getInstance();
		if (app)
		{
			int mx, my;
			app->getCursorPos(mx, my);
			int ax = 0, ay = 0;
			localToScreen(ax, ay);
			int lx = mx - ax;
			int ly = my - ay;
			int wide, tall;
			getSize(wide, tall);
			if (lx >= 0 && lx < wide && ly >= 1 && ly < tall - 1)
			{
				int idx = (ly - 1) / _itemH;
				if (idx >= 0 && idx < _count)
				{
					if (_owner) _owner->onItemPicked(idx);
					return;
				}
			}
			// Click outside the list area: close.
			if (_owner) _owner->closePopup();
		}
	}
	Panel::internalMousePressed(code);
}

// ====================================================================
// VScrollPanel - a vertically scrollable container. Children are added to
// the inner content panel at absolute positions; the content panel is shifted
// by the scroll offset and is clipped to the visible area by the surface. A
// 16px scrollbar is drawn down the right edge.
// ====================================================================
class VScrollPanel : public Panel
{
public:
	VScrollPanel(int x, int y, int w, int h)
		: Panel(x, y, w, h), _scrollY(0), _contentH(0),
		  _dragging(false), _dragStartY(0), _dragStartScroll(0)
	{
		_barW = VS(16);
		_content = new Panel(0, 0, w - _barW, h);
		_content->setPaintBackgroundEnabled(false);
		addChild(_content);
		setPaintBackgroundEnabled(true);
	}

	// Add a child at absolute content coordinates.
	void addItem(Panel* p) { if (_content) _content->addChild(p); }
	int  innerWidth() { int w, t; getSize(w, t); return w - _barW; }

	void setContentHeight(int h)
	{
		_contentH = h;
		int w, t;
		getSize(w, t);
		if (_content) _content->setSize(w - _barW, h < t ? t : h);
		clampScroll();
		applyScroll();
	}

protected:
	virtual void paintBackground()
	{
		int wide, tall;
		getSize(wide, tall);

		unsigned int fieldBg = g_Scheme.fieldBgColor ? g_Scheme.fieldBgColor : 0xFF3E4637;
		unsigned int track   = g_Scheme.fieldBgColor ? g_Scheme.fieldBgColor : 0xFF3E4637;
		unsigned int btnBg   = g_Scheme.buttonBgColor ? g_Scheme.buttonBgColor : 0xFF4C5844;
		unsigned int bright  = g_Scheme.borderBright ? g_Scheme.borderBright : 0xFF889180;
		unsigned int dark    = g_Scheme.borderDark   ? g_Scheme.borderDark   : 0xFF282E22;

		// Field background behind the whole panel.
		schemeBgColor(this, fieldBg);
		drawFilledRect(0, 0, wide, tall);
		schemeBgColor(this, dark);
		drawFilledRect(0, 0, wide, 1);
		drawFilledRect(0, 0, 1, tall);
		schemeBgColor(this, bright);
		drawFilledRect(0, tall - 1, wide, tall);
		drawFilledRect(wide - 1, 0, wide, tall);

		// Scrollbar track on the right.
		int barX = wide - _barW;
		schemeBgColor(this, track);
		drawFilledRect(barX, 0, wide, tall);

		// Up/down buttons.
		bevelRaised(barX, 0, wide, _barW, btnBg, bright, dark);
		drawArrow(barX + _barW / 2, _barW / 2, true);
		bevelRaised(barX, tall - _barW, wide, tall, btnBg, bright, dark);
		drawArrow(barX + _barW / 2, tall - _barW + _barW / 2, false);

		// Thumb.
		int thumbY, thumbH;
		computeThumb(thumbY, thumbH);
		bevelRaised(barX, thumbY, wide, thumbY + thumbH, btnBg, bright, dark);
	}

	virtual void internalMouseWheeled(int delta)
	{
		_scrollY -= delta * VS(20);
		clampScroll();
		applyScroll();
		repaint();
		Panel::internalMouseWheeled(delta);
	}

	virtual void internalMousePressed(MouseCode code)
	{
		if (code == MOUSE_LEFT)
		{
			App* app = App::getInstance();
			if (app)
			{
				int mx, my;
				app->getCursorPos(mx, my);
				int ax = 0, ay = 0;
				localToScreen(ax, ay);
				int lx = mx - ax;
				int ly = my - ay;
				int wide, tall;
				getSize(wide, tall);
				int barX = wide - _barW;
				if (lx >= barX)
				{
					if (ly < _barW)            { _scrollY -= VS(20); }
					else if (ly >= tall - _barW) { _scrollY += VS(20); }
					else
					{
						_dragging = true;
						_dragStartY = ly;
						_dragStartScroll = _scrollY;
						setAsMouseCapture(true);
					}
					clampScroll();
					applyScroll();
					repaint();
				}
			}
		}
		Panel::internalMousePressed(code);
	}

	virtual void internalCursorMoved(int x, int y)
	{
		if (_dragging)
		{
			App* app = App::getInstance();
			if (app)
			{
				int mx, my;
				app->getCursorPos(mx, my);
				int ax = 0, ay = 0;
				localToScreen(ax, ay);
				int lx = mx - ax;
				int ly = my - ay;
				int wide, tall;
				getSize(wide, tall);
				int trackH = tall - 2 * _barW;
				if (trackH < VS(8)) trackH = VS(8);
				int maxScroll = _contentH - tall;
				if (maxScroll < 0) maxScroll = 0;
				int thumbY, thumbH;
				computeThumb(thumbY, thumbH);
				int avail = trackH - thumbH;
				if (avail < 1) avail = 1;
				int dy = ly - _dragStartY;
				int ns = _dragStartScroll + (dy * maxScroll) / avail;
				_scrollY = ns;
				clampScroll();
				applyScroll();
				repaint();
			}
		}
		Panel::internalCursorMoved(x, y);
	}

	virtual void internalMouseReleased(MouseCode code)
	{
		if (code == MOUSE_LEFT && _dragging)
		{
			_dragging = false;
			setAsMouseCapture(false);
		}
		Panel::internalMouseReleased(code);
	}

private:
	void applyScroll() { if (_content) _content->setPos(0, -_scrollY); }

	void clampScroll()
	{
		int w, t;
		getSize(w, t);
		int maxS = _contentH - t;
		if (maxS < 0) maxS = 0;
		if (_scrollY > maxS) _scrollY = maxS;
		if (_scrollY < 0) _scrollY = 0;
	}

	void computeThumb(int& thumbY, int& thumbH)
	{
		int wide, tall;
		getSize(wide, tall);
		int trackY0 = _barW;
		int trackH = tall - 2 * _barW;
		if (trackH < VS(8)) trackH = VS(8);

		if (_contentH <= tall)
		{
			thumbH = trackH;
			thumbY = trackY0;
			return;
		}
		thumbH = trackH * tall / _contentH;
		int minTh = VS(16);
		if (thumbH < minTh) thumbH = minTh;
		if (thumbH > trackH) thumbH = trackH;

		int maxScroll = _contentH - tall;
		int avail = trackH - thumbH;
		int fromTop = (maxScroll > 0) ? (avail * _scrollY / maxScroll) : 0;
		thumbY = trackY0 + fromTop;
	}

	void bevelRaised(int x0, int y0, int x1, int y1,
	                 unsigned int bg, unsigned int tl, unsigned int br)
	{
		schemeBgColor(this, bg);
		drawFilledRect(x0 + 1, y0 + 1, x1 - 1, y1 - 1);
		schemeBgColor(this, tl);
		drawFilledRect(x0, y0, x1, y0 + 1);
		drawFilledRect(x0, y0, x0 + 1, y1);
		schemeBgColor(this, br);
		drawFilledRect(x0, y1 - 1, x1, y1);
		drawFilledRect(x1 - 1, y0, x1, y1);
	}

	void drawArrow(int cx, int cy, bool up)
	{
		unsigned int glyph = g_Scheme.buttonTextColor ? g_Scheme.buttonTextColor : 0xFFD8DED3;
		int t = VS(1);
		if (t < 1) t = 1;
		int top = cy - (3 * t) / 2;
		schemeBgColor(this, glyph);
		if (up)
		{
			drawFilledRect(cx - VS(1), top,         cx + VS(1), top + t);
			drawFilledRect(cx - VS(2), top + t,     cx + VS(2), top + 2 * t);
			drawFilledRect(cx - VS(3), top + 2 * t, cx + VS(3), top + 3 * t);
		}
		else
		{
			drawFilledRect(cx - VS(3), top,         cx + VS(3), top + t);
			drawFilledRect(cx - VS(2), top + t,     cx + VS(2), top + 2 * t);
			drawFilledRect(cx - VS(1), top + 2 * t, cx + VS(1), top + 3 * t);
		}
	}

	Panel* _content;
	int _barW;
	int _scrollY;
	int _contentH;
	bool _dragging;
	int  _dragStartY;
	int  _dragStartScroll;
};

// ====================================================================
// Separator helper: a flat 1px horizontal line panel.
// ====================================================================
class SeparatorLine : public Panel
{
public:
	SeparatorLine(int x, int y, int w, int h) : Panel(x, y, w, h) {}
protected:
	virtual void paintBackground()
	{
		int wide, tall;
		getSize(wide, tall);
		unsigned int dark = g_Scheme.borderDark ? g_Scheme.borderDark : 0xFF282E22;
		schemeBgColor(this, dark);
		drawFilledRect(0, 0, wide, tall);
	}
};

// ====================================================================
// Map list source.
//
// The task references EngFuncs::GetMapsList(); the engine API actually exposes
// EngFuncs::GetFilesList("maps/*.bsp", &n, gamedironly). We use that when it
// returns results, stripping the "maps/" prefix and ".bsp" suffix, and fall
// back to the canonical CS 1.6 starter maps otherwise.
// ====================================================================
static char  s_mapNames[64][64];
static const char* s_mapPtrs[64];
static int   s_mapCount = 0;

static const char* s_fallbackMaps[] =
	{ "de_dust2", "de_inferno", "de_nuke", "cs_assault", "cs_italy" };
static const int s_fallbackMapCount = 5;

static void StripMapName(const char* in, char* out, int outLen)
{
	const char* base = in;
	for (const char* p = in; *p; p++)
		if (*p == '/' || *p == '\\') base = p + 1;
	int i = 0;
	for (; base[i] && i < outLen - 1; i++) out[i] = base[i];
	out[i] = 0;
	// Drop ".bsp"
	int n = (int)strlen(out);
	if (n > 4 && strcmp(out + n - 4, ".bsp") == 0) out[n - 4] = 0;
}

static void BuildMapList()
{
	s_mapCount = 0;
	int numFiles = 0;
	char** files = EngFuncs::GetFilesList("maps/*.bsp", &numFiles, 1 /* gamedironly */);
	if (files && numFiles > 0)
	{
		for (int i = 0; i < numFiles && s_mapCount < 64; i++)
		{
			if (!files[i]) continue;
			StripMapName(files[i], s_mapNames[s_mapCount], sizeof(s_mapNames[0]));
			if (s_mapNames[s_mapCount][0])
			{
				s_mapPtrs[s_mapCount] = s_mapNames[s_mapCount];
				s_mapCount++;
			}
		}
	}
	if (s_mapCount == 0)
	{
		for (int i = 0; i < s_fallbackMapCount && i < 64; i++)
		{
			vgui_strcpy(s_mapNames[i], sizeof(s_mapNames[0]), s_fallbackMaps[i]);
			s_mapPtrs[i] = s_mapNames[i];
			s_mapCount++;
		}
	}
}

// ====================================================================
// Combo option tables (UTF-8 hex escapes, MSVC-safe).
// ====================================================================
static const char* k_filterOpts[] = {
	"\xD0\x92\xD1\x81\xD0\xB5 \xD0\xBA\xD0\xB0\xD1\x80\xD1\x82\xD1\x8B", // Все карты
	"\xD0\xA2\xD0\xBE\xD0\xBB\xD1\x8C\xD0\xBA\xD0\xBE de_",             // Только de_
	"\xD0\xA2\xD0\xBE\xD0\xBB\xD1\x8C\xD0\xBA\xD0\xBE cs_"              // Только cs_
};
static const char* k_locationOpts[] = {
	"\xD0\x9B\xD1\x8E\xD0\xB1\xD0\xBE\xD0\xB5",                         // Любое
	"\xD0\x93\xD0\xBE\xD1\x80\xD0\xBE\xD0\xB4",                         // Город
	"\xD0\xA1\xD0\xBA\xD0\xBB\xD0\xB0\xD0\xB4",                         // Склад
	"\xD0\x9F\xD1\x83\xD1\x81\xD1\x82\xD1\x8B\xD0\xBD\xD1\x8F"          // Пустыня
};
static const char* k_teamOpts[] = {
	"\xD0\xA1\xD0\xBB\xD1\x83\xD1\x87\xD0\xB0\xD0\xB9\xD0\xBD\xD0\xBE", // Случайно
	"\xD0\xA1\xD0\xBF\xD0\xB5\xD1\x86\xD0\xBD\xD0\xB0\xD0\xB7",         // Спецназ
	"\xD0\xA2\xD0\xB5\xD1\x80\xD1\x80\xD0\xBE\xD1\x80\xD0\xB8\xD1\x81\xD1\x82\xD1\x8B" // Террористы
};
static const char* k_chatterOpts[] = {
	"\xD0\x9D\xD0\xBE\xD1\x80\xD0\xBC\xD0\xB0\xD0\xBB\xD1\x8C\xD0\xBD\xD0\xBE",         // Нормально
	"\xD0\x9C\xD0\xB8\xD0\xBD\xD0\xB8\xD0\xBC\xD0\xB0\xD0\xBB\xD1\x8C\xD0\xBD\xD0\xBE", // Минимально
	"\xD0\xA1\xD1\x82\xD0\xB0\xD0\xBD\xD0\xB4\xD0\xB0\xD1\x80\xD1\x82\xD0\xBD\xD0\xBE\xD0\xB5 \xD1\x80\xD0\xB0\xD0\xB4\xD0\xB8\xD0\xBE", // Стандартное радио
	"\xD0\x92\xD1\x8B\xD0\xBA\xD0\xBB",                                                 // Выкл
	"\xD0\xA1\xD0\xBB\xD1\x83\xD1\x87\xD0\xB0\xD0\xB9\xD0\xBD\xD0\xBE"                  // Случайно
};

// ====================================================================
// Action signals
// ====================================================================
class CreateGameStartSignal : public ActionSignal
{
public:
	CreateGameStartSignal(VguiCreateGame* dlg) : _dlg(dlg) {}
	virtual void actionPerformed(Panel* /*p*/) { if (_dlg) _dlg->launchGame(); }
private:
	VguiCreateGame* _dlg;
};

class CreateGameCancelSignal : public ActionSignal
{
public:
	CreateGameCancelSignal(VguiCreateGame* dlg) : _dlg(dlg) {}
	virtual void actionPerformed(Panel* /*p*/) { if (_dlg) _dlg->setVisible(false); }
private:
	VguiCreateGame* _dlg;
};

// ====================================================================
// VguiCreateGame
// ====================================================================
static VguiCreateGame* s_createGame = 0;

VguiCreateGame::VguiCreateGame(int screenW, int screenH)
	: Frame(0, 0, VS(CG_DLG_W), VS(CG_DLG_H))
{
	VLOG("VguiCreateGame ctor: screen=%dx%d", screenW, screenH);
	_tabPanel = 0;
	_startBtn = 0;
	_cancelBtn = 0;
	_mapCombo = 0;
	_filterCombo = 0;
	_locationCombo = 0;
	_difficulty = 0;

	int dlgW = VS(CG_DLG_W);
	int dlgH = VS(CG_DLG_H);
	if (dlgW > screenW - VS(8)) dlgW = screenW - VS(8);
	if (dlgH > screenH - VS(8)) dlgH = screenH - VS(8);

	setPos((screenW - dlgW) / 2, (screenH - dlgH) / 2);
	setSize(dlgW, dlgH);
	setSizeable(false);                 // fixed size: drag-move only
	setTitle("\xD0\xA1\xD0\xBE\xD0\xB7\xD0\xB4\xD0\xB0\xD1\x82\xD1\x8C \xD1\x81\xD0\xB5\xD1\x80\xD0\xB2\xD0\xB5\xD1\x80"); // Создать сервер
	setVisible(false);

	Panel* client = getClient();
	if (!client) { VLOG("VguiCreateGame: getClient() null -- abort"); return; }

	int clientW, clientH;
	client->getSize(clientW, clientH);

	// Bottom button row: 24px buttons, 10px from right/bottom.
	int btnH    = VS(24);
	int btnRowH = btnH + VS(10) + VS(6);
	int tabH    = clientH - btnRowH;
	if (tabH < VS(100)) tabH = VS(100);

	_tabPanel = new TabPanel(0, 0, clientW, tabH);
	client->addChild(_tabPanel);

	int pageH = tabH - VS(28);
	Panel* serverPage = new Panel(0, 0, clientW, pageH);
	Panel* gamePage   = new Panel(0, 0, clientW, pageH);
	Panel* botsPage   = new Panel(0, 0, clientW, pageH);

	_tabPanel->addTab("\xD0\xA1\xD0\xB5\xD1\x80\xD0\xB2\xD0\xB5\xD1\x80", serverPage);            // Сервер
	_tabPanel->addTab("\xD0\x98\xD0\xB3\xD1\x80\xD0\xB0", gamePage);                              // Игра
	_tabPanel->addTab("\xD0\x9D\xD0\xB0\xD1\x81\xD1\x82\xD1\x80\xD0\xBE\xD0\xB9\xD0\xBA\xD0\xB8 \xD0\xB1\xD0\xBE\xD1\x82\xD0\xBE\xD0\xB2", botsPage); // Настройки ботов

	buildServerTab(serverPage);
	buildGameTab(gamePage);
	buildBotsTab(botsPage);

	// Bottom buttons: "Запуск" + "Отмена" (Отмена rightmost), 80x24, 10px gap.
	int btnW   = VS(80);
	int gap    = VS(6);
	int btnY   = clientH - btnH - VS(10);
	int cancelX = clientW - VS(10) - btnW;
	int startX  = cancelX - gap - btnW;

	_startBtn = new Button("\xD0\x97\xD0\xB0\xD0\xBF\xD1\x83\xD1\x81\xD0\xBA", startX, btnY, btnW, btnH); // Запуск
	_startBtn->addActionSignal(new CreateGameStartSignal(this));
	client->addChild(_startBtn);

	_cancelBtn = new Button("\xD0\x9E\xD1\x82\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xB0", cancelX, btnY, btnW, btnH); // Отмена
	_cancelBtn->addActionSignal(new CreateGameCancelSignal(this));
	client->addChild(_cancelBtn);

	VLOG("VguiCreateGame ctor: done");
}

VguiCreateGame::~VguiCreateGame()
{
	// SimpleRadioGroup is a plain class (not a Panel child), so it is not
	// freed by the panel tree - delete it explicitly to avoid a leak.
	delete _difficulty;
	_difficulty = 0;
}

// --------------------------------------------------------------------
// TAB 1 - "Сервер"
// --------------------------------------------------------------------
void VguiCreateGame::buildServerTab(Panel* page)
{
	int wide, tall;
	page->getSize(wide, tall);

	int labelX = VS(10), labelW = VS(120);
	int comboX = VS(140), comboW = VS(290), comboH = VS(22);

	BuildMapList();

	// Row y=10: "Карта"
	page->addChild(new Label("\xD0\x9A\xD0\xB0\xD1\x80\xD1\x82\xD0\xB0", labelX, VS(10), labelW, VS(22)));
	_mapCombo = new SimpleCombo(s_mapPtrs, s_mapCount, comboX, VS(10), comboW, comboH);
	page->addChild(_mapCombo);

	// Row y=40: "Фильтр"
	page->addChild(new Label("\xD0\xA4\xD0\xB8\xD0\xBB\xD1\x8C\xD1\x82\xD1\x80", labelX, VS(40), labelW, VS(22)));
	_filterCombo = new SimpleCombo(k_filterOpts, 3, comboX, VS(40), comboW, comboH);
	page->addChild(_filterCombo);

	// Row y=70: "Расположение" (orange text when selected)
	page->addChild(new Label("\xD0\xA0\xD0\xB0\xD1\x81\xD0\xBF\xD0\xBE\xD0\xBB\xD0\xBE\xD0\xB6\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB5", labelX, VS(70), labelW, VS(22)));
	_locationCombo = new SimpleCombo(k_locationOpts, 4, comboX, VS(70), comboW, comboH);
	_locationCombo->setOrangeWhenSelected(true);
	page->addChild(_locationCombo);

	// Separator at y=100 (full width minus 20px margins).
	page->addChild(new SeparatorLine(VS(10), VS(100), wide - VS(20), VS(1)));

	// y=110: "Включить ботов в эту игру"
	page->addChild(new SimpleCheck("\xD0\x92\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB8\xD1\x82\xD1\x8C \xD0\xB1\xD0\xBE\xD1\x82\xD0\xBE\xD0\xB2 \xD0\xB2 \xD1\x8D\xD1\x82\xD1\x83 \xD0\xB8\xD0\xB3\xD1\x80\xD1\x83",
		false, VS(10), VS(110), wide - VS(20), VS(22)));

	// y=138: "Число ботов" + TextEntry
	page->addChild(new Label("\xD0\xA7\xD0\xB8\xD1\x81\xD0\xBB\xD0\xBE \xD0\xB1\xD0\xBE\xD1\x82\xD0\xBE\xD0\xB2", VS(10), VS(138), VS(120), VS(22)));
	page->addChild(new TextEntry("8", VS(220), VS(138), VS(60), VS(22)));

	// y=162: "Сложность"
	page->addChild(new Label("\xD0\xA1\xD0\xBB\xD0\xBE\xD0\xB6\xD0\xBD\xD0\xBE\xD1\x81\xD1\x82\xD1\x8C", VS(10), VS(162), VS(120), VS(22)));

	// y=178,200,222,244: 4 radio options ("Легкая" selected by default).
	_difficulty = new SimpleRadioGroup();
	SimpleRadioButton* r0 = new SimpleRadioButton(_difficulty, 0, "\xD0\x9B\xD0\xB5\xD0\xB3\xD0\xBA\xD0\xB0\xD1\x8F", VS(10), VS(178), wide - VS(20), VS(18)); // Легкая
	SimpleRadioButton* r1 = new SimpleRadioButton(_difficulty, 1, "\xD0\xA1\xD1\x80\xD0\xB5\xD0\xB4\xD0\xBD\xD1\x8F\xD1\x8F", VS(10), VS(200), wide - VS(20), VS(18)); // Средняя
	SimpleRadioButton* r2 = new SimpleRadioButton(_difficulty, 2, "\xD0\xA2\xD1\x8F\xD0\xB6\xD0\xB5\xD0\xBB\xD0\xB0\xD1\x8F", VS(10), VS(222), wide - VS(20), VS(18)); // Тяжелая
	SimpleRadioButton* r3 = new SimpleRadioButton(_difficulty, 3, "\xD0\xAD\xD0\xBA\xD1\x81\xD0\xBF\xD0\xB5\xD1\x80\xD1\x82", VS(10), VS(244), wide - VS(20), VS(18)); // Эксперт
	_difficulty->addButton(r0);
	_difficulty->addButton(r1);
	_difficulty->addButton(r2);
	_difficulty->addButton(r3);
	_difficulty->setSelectedIndex(0);
	page->addChild(r0);
	page->addChild(r1);
	page->addChild(r2);
	page->addChild(r3);
}

// --------------------------------------------------------------------
// TAB 2 - "Игра" (scrollable label + TextEntry rows, then checkboxes)
// --------------------------------------------------------------------
struct GameField { const char* label; const char* value; };

void VguiCreateGame::buildGameTab(Panel* page)
{
	int wide, tall;
	page->getSize(wide, tall);

	VScrollPanel* scroll = new VScrollPanel(0, 0, wide, tall);
	page->addChild(scroll);

	static const GameField fields[] = {
		{ "\xD0\x9D\xD0\xB0\xD0\xB7\xD0\xB2\xD0\xB0\xD0\xBD\xD0\xB8\xD0\xB5 \xD1\x81\xD0\xB5\xD1\x80\xD0\xB2\xD0\xB5\xD1\x80\xD0\xB0:", "Counter-Strike" }, // Название сервера:
		{ "\xD0\x9C\xD0\xB0\xD0\xBA\xD1\x81\xD0\xB8\xD0\xBC\xD1\x83\xD0\xBC \xD0\xB8\xD0\xB3\xD1\x80\xD0\xBE\xD0\xBA\xD0\xBE\xD0\xB2:", "32" }, // Максимум игроков:
		{ "\xD0\x9F\xD0\xB0\xD1\x80\xD0\xBE\xD0\xBB\xD1\x8C \xD1\x81\xD0\xB5\xD1\x80\xD0\xB2\xD0\xB5\xD1\x80\xD0\xB0:", "" }, // Пароль сервера:
		{ "\xD0\x92\xD1\x80\xD0\xB5\xD0\xBC\xD1\x8F \xD0\xBA\xD0\xB0\xD1\x80\xD1\x82\xD1\x8B (\xD0\xBC\xD0\xB8\xD0\xBD):", "20" }, // Время карты (мин):
		{ "\xD0\x9E\xD0\xB3\xD1\x80\xD0\xB0\xD0\xBD\xD0\xB8\xD1\x87\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB5 \xD0\xBF\xD0\xBE \xD0\xBF\xD0\xBE\xD0\xB1\xD0\xB5\xD0\xB4\xD0\xB0\xD0\xBC:", "0" }, // Ограничение по победам:
		{ "\xD0\x9E\xD0\xB3\xD1\x80\xD0\xB0\xD0\xBD\xD0\xB8\xD1\x87\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB5 \xD0\xBF\xD0\xBE \xD1\x80\xD0\xB0\xD1\x83\xD0\xBD\xD0\xB4\xD0\xB0\xD0\xBC:", "0" }, // Ограничение по раундам:
		{ "\xD0\x92\xD1\x80\xD0\xB5\xD0\xBC\xD1\x8F \xD1\x80\xD0\xB0\xD1\x83\xD0\xBD\xD0\xB4\xD0\xB0 (\xD0\xBC\xD0\xB8\xD0\xBD):", "5" }, // Время раунда (мин):
		{ "\xD0\x92\xD1\x80\xD0\xB5\xD0\xBC\xD1\x8F \xD0\xB7\xD0\xB0\xD0\xBC\xD0\xBE\xD1\x80\xD0\xBE\xD0\xB7\xD0\xBA\xD0\xB8 (\xD1\x81\xD0\xB5\xD0\xBA):", "0" }, // Время заморозки (сек):
		{ "\xD0\x92\xD1\x80\xD0\xB5\xD0\xBC\xD1\x8F \xD0\xB4\xD0\xBB\xD1\x8F \xD0\xBF\xD0\xBE\xD0\xBA\xD1\x83\xD0\xBF\xD0\xBA\xD0\xB8 (\xD0\xBC\xD0\xB8\xD0\xBD):", "1.5" }, // Время для покупки (мин):
		{ "\xD0\x9D\xD0\xB0\xD1\x87\xD0\xB0\xD0\xBB\xD1\x8C\xD0\xBD\xD0\xB0\xD1\x8F \xD1\x81\xD1\x83\xD0\xBC\xD0\xBC\xD0\xB0 \xD0\xB4\xD0\xB5\xD0\xBD\xD0\xB5\xD0\xB3:", "16000" } // Начальная сумма денег:
	};
	int fieldCount = (int)(sizeof(fields) / sizeof(fields[0]));

	int rowH = VS(28);
	int labelX = VS(10), labelW = VS(180);
	int entryX = VS(196), entryW = VS(220), entryH = VS(22);

	int y = VS(4);
	for (int i = 0; i < fieldCount; i++)
	{
		scroll->addItem(new Label(fields[i].label, labelX, y + (rowH - VS(22)) / 2, labelW, VS(22)));
		scroll->addItem(new TextEntry(fields[i].value, entryX, y + (rowH - entryH) / 2, entryW, entryH));
		y += rowH;
	}

	// Checkboxes: "Звук шагов" (checked), "Наблюдение после смерти".
	y += VS(6);
	scroll->addItem(new SimpleCheck("\xD0\x97\xD0\xB2\xD1\x83\xD0\xBA \xD1\x88\xD0\xB0\xD0\xB3\xD0\xBE\xD0\xB2", true, labelX, y, VS(300), VS(22))); // Звук шагов
	y += rowH;
	scroll->addItem(new SimpleCheck("\xD0\x9D\xD0\xB0\xD0\xB1\xD0\xBB\xD1\x8E\xD0\xB4\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB5 \xD0\xBF\xD0\xBE\xD1\x81\xD0\xBB\xD0\xB5 \xD1\x81\xD0\xBC\xD0\xB5\xD1\x80\xD1\x82\xD0\xB8", false, labelX, y, VS(300), VS(22))); // Наблюдение после смерти
	y += rowH;

	scroll->setContentHeight(y + VS(6));
}

// --------------------------------------------------------------------
// TAB 3 - "Настройки ботов"
// --------------------------------------------------------------------
void VguiCreateGame::buildBotsTab(Panel* page)
{
	int wide, tall;
	page->getSize(wide, tall);

	// y=10: "Префикс к имени" + TextEntry
	page->addChild(new Label("\xD0\x9F\xD1\x80\xD0\xB5\xD1\x84\xD0\xB8\xD0\xBA\xD1\x81 \xD0\xBA \xD0\xB8\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xB8", VS(10), VS(10), VS(140), VS(22))); // Префикс к имени
	page->addChild(new TextEntry("", VS(158), VS(10), VS(180), VS(22)));

	// y=40: "Команда ботов" dropdown
	page->addChild(new Label("\xD0\x9A\xD0\xBE\xD0\xBC\xD0\xB0\xD0\xBD\xD0\xB4\xD0\xB0 \xD0\xB1\xD0\xBE\xD1\x82\xD0\xBE\xD0\xB2", VS(10), VS(40), VS(140), VS(22))); // Команда ботов
	page->addChild(new SimpleCombo(k_teamOpts, 3, VS(158), VS(40), VS(180), VS(22)));

	// y=70: "Разговор ботов" dropdown
	page->addChild(new Label("\xD0\xA0\xD0\xB0\xD0\xB7\xD0\xB3\xD0\xBE\xD0\xB2\xD0\xBE\xD1\x80 \xD0\xB1\xD0\xBE\xD1\x82\xD0\xBE\xD0\xB2", VS(10), VS(70), VS(140), VS(22))); // Разговор ботов
	page->addChild(new SimpleCombo(k_chatterOpts, 5, VS(158), VS(70), VS(180), VS(22)));

	// Separator at y=100
	page->addChild(new SeparatorLine(VS(10), VS(100), wide - VS(20), VS(1)));

	// y=108,132,156: behavior checkboxes (all checked)
	page->addChild(new SimpleCheck("\xD0\x91\xD0\xBE\xD1\x82\xD1\x8B \xD0\xB7\xD0\xB0\xD1\x85\xD0\xBE\xD0\xB4\xD1\x8F\xD1\x82 \xD0\xBF\xD0\xBE\xD1\x81\xD0\xBB\xD0\xB5 \xD0\xB8\xD0\xB3\xD1\x80\xD0\xBE\xD0\xBA\xD0\xB0", true, VS(10), VS(108), wide - VS(20), VS(22))); // Боты заходят после игрока
	page->addChild(new SimpleCheck("\xD0\x91\xD0\xBE\xD1\x82\xD1\x8B \xD0\xBF\xD0\xBE\xD0\xB4\xD1\x87\xD0\xB8\xD0\xBD\xD1\x8F\xD1\x8E\xD1\x82\xD1\x81\xD1\x8F \xD1\x87\xD0\xB5\xD0\xBB\xD0\xBE\xD0\xB2\xD0\xB5\xD0\xBA\xD1\x83", true, VS(10), VS(132), wide - VS(20), VS(22))); // Боты подчиняются человеку
	page->addChild(new SimpleCheck("\xD0\x91\xD0\xBE\xD1\x82\xD1\x8B \xD0\xBC\xD0\xBE\xD0\xB3\xD1\x83\xD1\x82 \xD0\xB1\xD1\x8B\xD1\x82\xD1\x8C \xD0\xBF\xD0\xBB\xD0\xBE\xD1\x85\xD0\xB8\xD0\xBC\xD0\xB8", true, VS(10), VS(156), wide - VS(20), VS(22))); // Боты могут быть плохими

	// Separator at y=182
	page->addChild(new SeparatorLine(VS(10), VS(182), wide - VS(20), VS(1)));

	// y=190: "Оружие ботов"
	page->addChild(new Label("\xD0\x9E\xD1\x80\xD1\x83\xD0\xB6\xD0\xB8\xD0\xB5 \xD0\xB1\xD0\xBE\xD1\x82\xD0\xBE\xD0\xB2", VS(10), VS(190), VS(200), VS(18))); // Оружие ботов

	// y=210..280: weapon checkboxes, single column (step ~20px).
	page->addChild(new SimpleCheck("\xD0\x9F\xD0\xB8\xD1\x81\xD1\x82\xD0\xBE\xD0\xBB\xD0\xB5\xD1\x82\xD1\x8B", true, VS(10), VS(210), wide - VS(20), VS(18))); // Пистолеты (checked)
	page->addChild(new SimpleCheck("\xD0\x94\xD1\x80\xD0\xBE\xD0\xB1\xD0\xBE\xD0\xB2\xD0\xB8\xD0\xBA\xD0\xB8", true, VS(10), VS(230), wide - VS(20), VS(18))); // Дробовики (checked)
	page->addChild(new SimpleCheck("\xD0\x90\xD0\xB2\xD1\x82\xD0\xBE\xD0\xBC\xD0\xB0\xD1\x82\xD1\x8B", true, VS(10), VS(250), wide - VS(20), VS(18))); // Автоматы (checked)
	page->addChild(new SimpleCheck("\xD0\x92\xD0\xB8\xD0\xBD\xD1\x82\xD0\xBE\xD0\xB2\xD0\xBA\xD0\xB8", true, VS(10), VS(270), wide - VS(20), VS(18))); // Винтовки (checked)
	page->addChild(new SimpleCheck("\xD0\x9F\xD1\x83\xD0\xBB\xD0\xB5\xD0\xBC\xD0\xB5\xD1\x82\xD1\x8B", true, VS(228), VS(210), wide - VS(238), VS(18))); // Пулеметы (checked)
	page->addChild(new SimpleCheck("\xD0\x93\xD1\x80\xD0\xB0\xD0\xBD\xD0\xB0\xD1\x82\xD1\x8B", true, VS(228), VS(230), wide - VS(238), VS(18))); // Гранаты (checked)
	page->addChild(new SimpleCheck("\xD0\xA1\xD0\xBD\xD0\xB0\xD0\xB9\xD0\xBF\xD0\xB5\xD1\x80\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB5 \xD0\xB2\xD0\xB8\xD0\xBD\xD1\x82\xD0\xBE\xD0\xB2\xD0\xBA\xD0\xB8", false, VS(228), VS(250), wide - VS(238), VS(18))); // Снайперские винтовки (unchecked)
	page->addChild(new SimpleCheck("\xD0\xA9\xD0\xB8\xD1\x82\xD1\x8B", false, VS(228), VS(270), wide - VS(238), VS(18))); // Щиты (unchecked)
}

// --------------------------------------------------------------------
// "Запуск": start the game on the selected map, then hide.
// --------------------------------------------------------------------
void VguiCreateGame::launchGame()
{
	const char* mapName = (_mapCombo && s_mapCount > 0) ? _mapCombo->getSelectedText() : "";
	if (!mapName || !mapName[0])
		mapName = s_fallbackMaps[0];

	char cmd[256];
	snprintf(cmd, sizeof(cmd), "map %s\n", mapName);
	VGUI_ClientCmd(cmd);

	setVisible(false);
}

void VguiCreateGame::setVisible(bool state)
{
	if (!state)
	{
		App* app = App::getInstance();
		if (app) app->requestFocus(null);
		UI_EnableTextInput(false);
	}
	Frame::setVisible(state);
}

// ====================================================================
// Lifecycle helpers
// ====================================================================
void VGUI_CreateGame_Show(bool show)
{
	if (show)
	{
		int sw = 0, sh = 0;
		VGUI_GetScreenSize(&sw, &sh);
		if (sw <= 0) sw = 640;
		if (sh <= 0) sh = 480;

		VGUI_EnsureInitialized(sw, sh);
		Panel* root = VGUI_GetRootPanel();
		if (!root) return;
		root->setSize(sw, sh);

		if (!s_createGame)
		{
			VLOG("VGUI_CreateGame_Show: creating Create Server dialog");
			s_createGame = new VguiCreateGame(sw, sh);
			root->addChild(s_createGame);
		}
		s_createGame->setVisible(true);
		VLOG("VGUI_CreateGame_Show: visible");
	}
	else
	{
		if (s_createGame)
		{
			s_createGame->setVisible(false);
			VLOG("VGUI_CreateGame_Show: hidden");
		}
	}
}

bool VGUI_CreateGame_IsVisible()
{
	return s_createGame && s_createGame->isVisible();
}

} // namespace vgui

// Called from VGUI_Shutdown (vgui_main.cpp) before the root panel tree is
// destroyed, so our cached pointer never dangles.
void VGUI_CreateGameShutdown(void)
{
	vgui::s_createGame = 0;
}

extern "C"
{

#ifdef _WIN32
#define CREATEGAME_EXPORT __declspec(dllexport)
#else
#define CREATEGAME_EXPORT __attribute__((visibility("default")))
#endif

CREATEGAME_EXPORT void VGUI_ShowCreateGame(bool show)
{
	vgui::VGUI_CreateGame_Show(show);
}

CREATEGAME_EXPORT bool VGUI_IsCreateGameVisible(void)
{
	return vgui::VGUI_CreateGame_IsVisible();
}

} // extern "C"
