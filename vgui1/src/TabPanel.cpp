// Include heavy mainui headers BEFORE VGUI_*.h to avoid the `null` macro clash.
#include "BaseMenu.h"
#include "FontManager.h"
extern void UI_FillRect( int x, int y, int width, int height, const unsigned int color );
#include "TrackerScheme.h"

#include <VGUI_SchemeColors.h>
#include <VGUI_UIScale.h>
#include <VGUI_TabPanel.h>
#include <VGUI_App.h>
#include <string.h>

namespace vgui
{

// GoldSrc CS 1.6 tab metrics (pixel-perfect @ 640x480, scaled via VS).
static const int TAB_HEIGHT_BASE      = 24;
static const int TAB_INACTIVE_SHRINK  = 2;
static const int TAB_TEXT_HEIGHT_B    = 12;  // text glyph height
static const int TAB_TEXT_PAD_LEFT_B  = 6;
static const int TAB_TEXT_PAD_RIGHT_B = 6;
static const int TAB_OVERLAP          = 1;
static const int TAB_MIN_WIDTH        = 40;  // never thinner than this

// Each tab is sized to its text + symmetric padding (GoldSrc behavior).
// Tabs may overflow the strip on very narrow dialogs; user can resize.
static int NaturalTabWidth( const char *text )
{
	HFont font = uiStatic.hDefaultFont;
	int textW = 0;
	if ( g_FontMgr && font && text && text[0] )
		textW = g_FontMgr->GetTextWideScaled( font, text, VS(TAB_TEXT_HEIGHT_B) );
	else if ( text )
		textW = (int)strlen( text ) * (VS(TAB_TEXT_HEIGHT_B) * 6 / 10);
	int w = textW + VS(TAB_TEXT_PAD_LEFT_B) + VS(TAB_TEXT_PAD_RIGHT_B);
	int minW = VS(TAB_MIN_WIDTH);
	if (w < minW) w = minW;
	return w;
}

TabPanel::TabPanel(int x, int y, int wide, int tall) : Panel(x, y, wide, tall)
{
	_selectedTab = 0;
}

TabPanel::~TabPanel()
{
	for (int i = 0; i < _tabDar.getCount(); i++)
	{
		Tab* t = _tabDar[i];
		if (t) delete t;
	}
	_tabDar.removeAll();
}

void TabPanel::addTab(const char* text, Panel* panel)
{
	Tab* tab = new Tab;
	vgui_strcpy(tab->text, sizeof(tab->text), text ? text : "");
	tab->panel = panel;
	_tabDar.addElement(tab);

	if (panel)
	{
		addChild(panel);
		panel->setVisible(_tabDar.getCount() - 1 == _selectedTab);
	}

	invalidateLayout(true);
}

void TabPanel::setSelectedTab(int index)
{
	if (index < 0 || index >= _tabDar.getCount())
		return;

	if (_selectedTab >= 0 && _selectedTab < _tabDar.getCount())
	{
		Tab* oldTab = _tabDar[_selectedTab];
		if (oldTab && oldTab->panel)
			oldTab->panel->setVisible(false);
	}

	_selectedTab = index;

	Tab* newTab = _tabDar[_selectedTab];
	if (newTab && newTab->panel)
		newTab->panel->setVisible(true);

	repaint();
}

int TabPanel::getSelectedTab()  { return _selectedTab; }
int TabPanel::getTabCount()     { return _tabDar.getCount(); }

void TabPanel::setSize(int wide, int tall)
{
	Panel::setSize(wide, tall);
	performLayout();
}

Panel* TabPanel::getSelectedPanel()
{
	if (_selectedTab >= 0 && _selectedTab < _tabDar.getCount())
	{
		Tab* tab = _tabDar[_selectedTab];
		if (tab) return tab->panel;
	}
	return null;
}

void TabPanel::performLayout()
{
	int wide, tall;
	getSize(wide, tall);

	// Pages start below tab strip + 1px merge zone
	int pageY = VS(TAB_HEIGHT_BASE) + 2;
	for (int i = 0; i < _tabDar.getCount(); i++)
	{
		Tab* tab = _tabDar[i];
		if (tab && tab->panel)
			tab->panel->setBounds(0, pageY, wide, tall - pageY);
	}
}

void TabPanel::paintBackground()
{
	// Transparent - parent dialog draws the background
}

void TabPanel::paint()
{
	int wide, tall;
	getSize(wide, tall);

	int tabCount = _tabDar.getCount();
	if (tabCount <= 0)
		return;

	unsigned int frameBg     = g_Scheme.frameBgColor       ? g_Scheme.frameBgColor       : 0xE6646E50;
	unsigned int inactiveBg  = g_Scheme.tabInactiveBgColor ? g_Scheme.tabInactiveBgColor : 0xE6404830;
	unsigned int textColor   = g_Scheme.tabTextColor       ? g_Scheme.tabTextColor       : 0xFF909090;
	unsigned int selTextCol  = g_Scheme.tabSelectedTextColor ? g_Scheme.tabSelectedTextColor : 0xFFE8E0A0;
	unsigned int bright      = g_Scheme.borderBright       ? g_Scheme.borderBright       : 0xC87A8070;
	unsigned int dark        = g_Scheme.borderDark         ? g_Scheme.borderDark         : 0xC8282C24;

	int tabH   = VS(TAB_HEIGHT_BASE);
	int shrink = VS(TAB_INACTIVE_SHRINK);

	// Per-tab natural width (text + padding). GoldSrc canonical behavior.
	int activeX = 0, activeW = 0;

	int x = 0;
	for (int i = 0; i < tabCount; i++)
	{
		Tab* tab = _tabDar[i];
		if (!tab) continue;

		int w = NaturalTabWidth(tab->text);
		bool selected = (i == _selectedTab);

		if (selected)
		{
			activeX = x;
			activeW = w;

			// Active tab: full height, merges into page area below (+2px overlap)
			schemeBgColor(this, frameBg);
			drawFilledRect(x + 2, 0, x + w - 2, tabH + 2);

			// Outer bright top + left
			schemeBgColor(this, bright);
			drawFilledRect(x, 0, x + w - 1, 1);
			drawFilledRect(x, 0, x + 1, tabH + 2);

			// Inner highlight (1px inset from outer)
			schemeBgColor(this, 0xC8909880);
			drawFilledRect(x + 1, 1, x + w - 2, 2);
			drawFilledRect(x + 1, 1, x + 2, tabH + 1);

			// Outer dark right
			schemeBgColor(this, dark);
			drawFilledRect(x + w - 1, 0, x + w, tabH + 2);

			// Inner shadow right
			schemeBgColor(this, 0xC83A3E30);
			drawFilledRect(x + w - 2, 1, x + w - 1, tabH + 1);
		}
		else
		{
			int yTop = shrink;

			schemeBgColor(this, inactiveBg);
			drawFilledRect(x + 2, yTop + 1, x + w - 2, tabH - 1);

			// Outer bevel
			schemeBgColor(this, bright);
			drawFilledRect(x, yTop, x + w - 1, yTop + 1);
			drawFilledRect(x, yTop, x + 1, tabH - 1);

			schemeBgColor(this, dark);
			drawFilledRect(x + w - 1, yTop, x + w, tabH);
			drawFilledRect(x, tabH - 1, x + w, tabH);

			// Inner highlight (top)
			schemeBgColor(this, 0xC8686E58);
			drawFilledRect(x + 1, yTop + 1, x + w - 1, yTop + 2);
		}

		// Tab label - CENTERED horizontally within uniform tab width
		int textLen = (int)strlen(tab->text);
		if (textLen > 0)
		{
			schemeFgColor(this, selected ? selTextCol : textColor);
			drawSetTextFont(Scheme::sf_primary1);
			HFont font = uiStatic.hDefaultFont;
			int textH = VS(TAB_TEXT_HEIGHT_B);
			int textW = 0;
			if (g_FontMgr && font)
				textW = g_FontMgr->GetTextWideScaled(font, tab->text, textH);
			else
				textW = textLen * (textH * 6 / 10);
			int textX = x + (w - textW) / 2;
			int textY = (tabH - textH) / 2 - 1;
			if (!selected) textY += shrink / 2;
			drawPrintText(textX, textY, tab->text, textLen);
		}

		x += w - VS(TAB_OVERLAP);
	}

	// Bottom separator: dark + bright double line, gap under active tab
	schemeBgColor(this, dark);
	if (activeX > 0)
		drawFilledRect(0, tabH, activeX + 1, tabH + 1);
	if (activeX + activeW < wide)
		drawFilledRect(activeX + activeW - 1, tabH, wide, tabH + 1);

	schemeBgColor(this, bright);
	if (activeX > 0)
		drawFilledRect(0, tabH + 1, activeX + 1, tabH + 2);
	if (activeX + activeW < wide)
		drawFilledRect(activeX + activeW - 1, tabH + 1, wide, tabH + 2);
}

void TabPanel::internalMousePressed(MouseCode code)
{
	if (code == MOUSE_LEFT)
	{
		App* app = App::getInstance();
		if (app)
		{
			int mx, my;
			app->getCursorPos(mx, my);
			screenToLocal(mx, my);

			if (my >= 0 && my < VS(TAB_HEIGHT_BASE))
			{
				int tabCount = _tabDar.getCount();
				int x = 0;
				for (int i = 0; i < tabCount; i++)
				{
					Tab* tab = _tabDar[i];
					if (!tab) continue;
					int w = NaturalTabWidth(tab->text);
					if (mx >= x && mx < x + w)
					{
						setSelectedTab(i);
						break;
					}
					x += w - VS(TAB_OVERLAP);
				}
			}
		}
	}
	Panel::internalMousePressed(code);
}

}
