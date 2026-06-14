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
static const int TAB_MIN_WIDTH        = 40;  // never thinner than this

// Each tab is sized to its text + symmetric padding (GoldSrc behavior).
// Tabs may overflow the strip on very narrow dialogs; user can resize.
static int NaturalTabWidth( const char *text )
{
	// Measure with the SAME native-size face CEngineSurface renders the tab
	// label with (GetVGUIFont), so the natural width and the painted glyphs
	// agree - otherwise stretched cells were sized off a different atlas.
	HFont font = g_FontMgr ? g_FontMgr->GetVGUIFont( VS(TAB_TEXT_HEIGHT_B) ) : 0;
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
	tab->naturalW = -1; // computed lazily
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

void TabPanel::layoutTabs(int wide, int* xs, int* ws, int maxOut)
{
	int count = _tabDar.getCount();
	if (count > maxOut) count = maxOut;
	if (count <= 0) return;

	// PC CS 1.6 PropertySheet: all tabs have exactly the same width,
	// spanning the full dialog width. They scale proportionally when
	// the window is resized.
	int x = 0;
	int tabW = wide / count;
	for (int i = 0; i < count; i++)
	{
		int w = (i == count - 1) ? (wide - x) : tabW;
		xs[i] = x;
		ws[i] = w;
		x += w;
	}
}

void TabPanel::paint()
{
	int wide, tall;
	getSize(wide, tall);

	int tabCount = _tabDar.getCount();
	if (tabCount <= 0)
		return;

	unsigned int frameBg     = g_Scheme.frameBgColor       ? g_Scheme.frameBgColor       : 0xFF4C5844;
	unsigned int inactiveBg  = g_Scheme.tabInactiveBgColor ? g_Scheme.tabInactiveBgColor : 0xFF4C5844;
	unsigned int textColor   = g_Scheme.tabTextColor       ? g_Scheme.tabTextColor       : 0xFFA0AA95;
	unsigned int selTextCol  = g_Scheme.tabSelectedTextColor ? g_Scheme.tabSelectedTextColor : 0xFFC4B550;
	unsigned int bright      = g_Scheme.borderBright       ? g_Scheme.borderBright       : 0xFF889180;
	unsigned int dark        = g_Scheme.borderDark         ? g_Scheme.borderDark         : 0xFF282E22;

	int tabH   = VS(TAB_HEIGHT_BASE);
	int shrink = VS(TAB_INACTIVE_SHRINK);

	// Tab row stretched to span the full strip width (proportional to each
	// tab's natural text width). Height stays fixed (TAB_HEIGHT_BASE).
	enum { MAXT = 64 };
	int xs[MAXT], ws[MAXT];
	if (tabCount > MAXT) tabCount = MAXT;
	layoutTabs(wide, xs, ws, MAXT);

	int activeX = 0, activeW = 0;

	for (int i = 0; i < tabCount; i++)
	{
		Tab* tab = _tabDar[i];
		if (!tab) continue;

		int x = xs[i];
		int w = ws[i];
		bool selected = (i == _selectedTab);

		if (selected)
		{
			activeX = x;
			activeW = w;

			// Active tab fills with body color and overlaps the page area
			// by 2px so the bottom seam under the tab disappears (canon
			// TabActiveBorder.Bottom = ControlBG offset 6 2).
			schemeBgColor(this, frameBg);
			drawFilledRect(x + 1, 0, x + w - 1, tabH + 2);

			// 1px raised border: Top + Left = bright, Right = dark.
			// No inner second band - that was non-canonical.
			schemeBgColor(this, bright);
			drawFilledRect(x, 0, x + w - 1, 1);
			drawFilledRect(x, 0, x + 1, tabH + 2);
			schemeBgColor(this, dark);
			drawFilledRect(x + w - 1, 0, x + w, tabH + 2);
		}
		else
		{
			int yTop = shrink;

			schemeBgColor(this, inactiveBg);
			drawFilledRect(x + 1, yTop + 1, x + w - 1, tabH);

			// 1px raised: Top + Left = bright, Right + Bottom = dark.
			schemeBgColor(this, bright);
			drawFilledRect(x, yTop, x + w - 1, yTop + 1);
			drawFilledRect(x, yTop, x + 1, tabH);
			schemeBgColor(this, dark);
			drawFilledRect(x + w - 1, yTop, x + w, tabH);
			drawFilledRect(x, tabH - 1, x + w, tabH);
		}

		// Tab label - CENTERED horizontally in each tab (CS 1.6 PC behavior).
		// Vertically centred in the tab (active tab is 2px taller because
		// inactive tabs are pushed down by `shrink`).
		int textLen = (int)strlen(tab->text);
		if (textLen > 0)
		{
			schemeFgColor(this, selected ? selTextCol : textColor);
			drawSetTextFont(Scheme::sf_primary1);
			int textH = VS(TAB_TEXT_HEIGHT_B);
			// Center text horizontally in the tab (CS 1.6 PC behavior).
			// Measure text width, then: center = x + (tabW - textW) / 2.
			// Clamp to minimum left pad so text never clips the left border.
			HFont cFont = g_FontMgr ? g_FontMgr->GetVGUIFont( textH ) : 0;
			int textMW = 0;
			if ( g_FontMgr && cFont && textLen > 0 )
				textMW = g_FontMgr->GetTextWideScaled( cFont, tab->text, textH );
			else
				textMW = textLen * ( textH * 6 / 10 );
			int textX = x + ( w - textMW ) / 2;
			if ( textX < x + VS(TAB_TEXT_PAD_LEFT_B) ) textX = x + VS(TAB_TEXT_PAD_LEFT_B);
			int textY = (tabH - textH) / 2 - 1;
			if (!selected) textY += shrink / 2;
			drawPrintText(textX, textY, tab->text, textLen);
		}
	}

	// Content-area RAISED bevel (canon CS 1.6 PropertySheet content panel,
	// 1px). Active tab covers its top edge with body color, creating the
	// "merge" with the body the canon look depends on.
	//   TOP    = bright (with gap under the active tab)
	//   LEFT   = bright
	//   RIGHT  = dark
	//   BOTTOM = dark
	int bodyTop = tabH;
	schemeBgColor(this, bright);
	int gapL = activeX;
	int gapR = activeX + activeW;
	if (gapL > 0)
		drawFilledRect(0, bodyTop, gapL, bodyTop + 1);
	if (gapR < wide)
		drawFilledRect(gapR, bodyTop, wide, bodyTop + 1);
	drawFilledRect(0, bodyTop, 1, tall);
	schemeBgColor(this, dark);
	drawFilledRect(wide - 1, bodyTop, wide, tall);
	drawFilledRect(0, tall - 1, wide, tall);
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
				int wide, tall;
				getSize(wide, tall);
				int tabCount = _tabDar.getCount();
				enum { MAXT = 64 };
				int xs[MAXT], ws[MAXT];
				if (tabCount > MAXT) tabCount = MAXT;
				layoutTabs(wide, xs, ws, MAXT);
				for (int i = 0; i < tabCount; i++)
				{
					Tab* tab = _tabDar[i];
					if (!tab) continue;
					if (mx >= xs[i] && mx < xs[i] + ws[i])
					{
						setSelectedTab(i);
						break;
					}
				}
			}
		}
	}
	Panel::internalMousePressed(code);
}

}
