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

// CS 1.6 PC tab metrics. Scaled at runtime via VS().
static const int TAB_HEIGHT_BASE   = 24;
static const int TAB_TEXT_HEIGHT_B = 12;
static const int TAB_TEXT_PAD_B    = 12;
static const int TAB_GAP           = 0;

// Compute width of tab i based on text length (using mainui FontManager).
static int ComputeTabWidth( const char *text )
{
	HFont font = uiStatic.hDefaultFont;
	int textW = 0;
	if ( g_FontMgr && font && text && text[0] )
		textW = g_FontMgr->GetTextWideScaled( font, text, VS(TAB_TEXT_HEIGHT_B) );
	else if ( text )
		textW = (int)strlen( text ) * (VS(TAB_TEXT_HEIGHT_B) * 6 / 10);
	return textW + VS(TAB_TEXT_PAD_B) * 2;
}

TabPanel::TabPanel(int x, int y, int wide, int tall) : Panel(x, y, wide, tall)
{
	_selectedTab = 0;
	// Visual colors driven by g_Scheme at draw time
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

	// Page panels live below the tab strip, filling remaining space
	for (int i = 0; i < _tabDar.getCount(); i++)
	{
		Tab* tab = _tabDar[i];
		if (tab && tab->panel)
			tab->panel->setBounds(0, VS(TAB_HEIGHT_BASE), wide, tall - VS(TAB_HEIGHT_BASE));
	}
}

void TabPanel::paintBackground()
{
	// Background is drawn by the parent dialog (frameBgColor); TabPanel itself
	// is transparent. Drawing nothing avoids a visible seam under the tabs.
}

void TabPanel::paint()
{
	int wide, tall;
	getSize(wide, tall);

	int tabCount = _tabDar.getCount();
	if (tabCount <= 0)
		return;

	unsigned int frameBg     = g_Scheme.frameBgColor       ? g_Scheme.frameBgColor       : 0xE65F684E;
	unsigned int inactiveBg  = g_Scheme.tabInactiveBgColor ? g_Scheme.tabInactiveBgColor : 0xE64E5643;
	unsigned int textColor   = g_Scheme.tabTextColor       ? g_Scheme.tabTextColor       : 0xFFDCDCDC;
	unsigned int selTextCol  = g_Scheme.tabSelectedTextColor ? g_Scheme.tabSelectedTextColor : 0xFFBFB85E;
	unsigned int bright      = g_Scheme.borderBright       ? g_Scheme.borderBright       : 0xC85F6558;
	unsigned int dark        = g_Scheme.borderDark         ? g_Scheme.borderDark         : 0xC8282C24;

	// Track the active tab's x range so we can draw the strip-bottom line around it
	int activeX = 0, activeW = 0;

	int x = 0;
	for (int i = 0; i < tabCount; i++)
	{
		Tab* tab = _tabDar[i];
		if (!tab) continue;

		int width = ComputeTabWidth(tab->text);
		bool selected = (i == _selectedTab);

		if (selected)
		{
			activeX = x;
			activeW = width;

			// Active tab: same olive as panel below, taller by 1px so it
			// "merges" into the panel with no horizontal seam underneath.
			schemeBgColor(this, frameBg);
			drawFilledRect(x + 1, 0, x + width - 1, VS(TAB_HEIGHT_BASE) + 1);

			// Top + left bright edge
			schemeBgColor(this, bright);
			drawFilledRect(x, 0, x + width, 1);
			drawFilledRect(x, 0, x + 1, VS(TAB_HEIGHT_BASE) + 1);

			// Right dark edge
			schemeBgColor(this, dark);
			drawFilledRect(x + width - 1, 0, x + width, VS(TAB_HEIGHT_BASE) + 1);
		}
		else
		{
			// Inactive tab: 2px shorter at the top, all 4 bevels + bottom edge
			int yTop = 2;

			schemeBgColor(this, inactiveBg);
			drawFilledRect(x + 1, yTop, x + width - 1, VS(TAB_HEIGHT_BASE) - 1);

			schemeBgColor(this, bright);
			drawFilledRect(x, yTop, x + width, yTop + 1);
			drawFilledRect(x, yTop, x + 1, VS(TAB_HEIGHT_BASE) - 1);

			schemeBgColor(this, dark);
			drawFilledRect(x + width - 1, yTop, x + width, VS(TAB_HEIGHT_BASE) - 1);
			drawFilledRect(x, VS(TAB_HEIGHT_BASE) - 1, x + width, VS(TAB_HEIGHT_BASE));
		}

		// Tab label (vertically centered in the strip)
		int textLen = (int)strlen(tab->text);
		if (textLen > 0)
		{
			schemeFgColor(this, selected ? selTextCol : textColor);
			drawSetTextFont(Scheme::sf_primary1);
			int textY = (VS(TAB_HEIGHT_BASE) - VS(TAB_TEXT_HEIGHT_B)) / 2;
			drawPrintText(x + VS(TAB_TEXT_PAD_B), textY, tab->text, textLen);
		}

		x += width + TAB_GAP;
	}

	// Strip-bottom dark line everywhere except under the active tab
	schemeBgColor(this, dark);
	if (activeX > 0)
		drawFilledRect(0, VS(TAB_HEIGHT_BASE), activeX, VS(TAB_HEIGHT_BASE) + 1);
	if (activeX + activeW < wide)
		drawFilledRect(activeX + activeW, VS(TAB_HEIGHT_BASE), wide, VS(TAB_HEIGHT_BASE) + 1);
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
				int x = 0;
				int tabCount = _tabDar.getCount();
				for (int i = 0; i < tabCount; i++)
				{
					Tab* tab = _tabDar[i];
					if (!tab) continue;
					int w = ComputeTabWidth(tab->text);
					if (mx >= x && mx < x + w)
					{
						setSelectedTab(i);
						break;
					}
					x += w + TAB_GAP;
				}
			}
		}
	}
	Panel::internalMousePressed(code);
}

}
