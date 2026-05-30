// Include heavy mainui headers BEFORE VGUI_*.h to avoid the `null` macro clash.
extern void UI_FillRect( int x, int y, int width, int height, const unsigned int color );
#include "TrackerScheme.h"

#include <VGUI_SchemeColors.h>
#include <VGUI_CheckButton.h>
#include <VGUI_ActionSignal.h>

namespace vgui
{

CheckButton::CheckButton(const char* text, int x, int y) : Button(text, x, y)
{
	setContentAlignment(a_west);
}

CheckButton::CheckButton(const char* text, int x, int y, int wide, int tall) : Button(text, x, y, wide, tall)
{
	setContentAlignment(a_west);
}

void CheckButton::paintBackground()
{
	int wide, tall;
	getSize(wide, tall);

	unsigned int fieldBg = g_Scheme.fieldBgColor    ? g_Scheme.fieldBgColor    : 0xFF3E4637;
	unsigned int bright  = g_Scheme.borderBright    ? g_Scheme.borderBright    : 0xFF889180;
	unsigned int dark    = g_Scheme.borderDark      ? g_Scheme.borderDark      : 0xFF282E22;
	// Canon CS 1.6 check mark = BrightControlText (gold 196,181,80), NOT field text.
	unsigned int markCol = g_Scheme.checkMarkColor  ? g_Scheme.checkMarkColor  :
	                       (g_Scheme.tabSelectedTextColor ? g_Scheme.tabSelectedTextColor : 0xFFC4B550);

	// Checkbox sits transparently on the parent panel background -- do NOT
	// fill the whole rect, otherwise we paint over the form's olive panel.

	// 13x13 sunken box, vertically centered, 2px from left edge
	int boxSize = 13;
	int boxY = (tall - boxSize) / 2;
	int boxX = 2;

	// Field background inside the box
	schemeBgColor(this, fieldBg);
	drawFilledRect(boxX + 1, boxY + 1, boxX + boxSize - 1, boxY + boxSize - 1);

	// Sunken bevel: dark top + left, bright bottom + right
	schemeBgColor(this, dark);
	drawFilledRect(boxX, boxY, boxX + boxSize, boxY + 1);
	drawFilledRect(boxX, boxY, boxX + 1, boxY + boxSize);

	schemeBgColor(this, bright);
	drawFilledRect(boxX, boxY + boxSize - 1, boxX + boxSize, boxY + boxSize);
	drawFilledRect(boxX + boxSize - 1, boxY, boxX + boxSize, boxY + boxSize);

	// Check mark when selected
	if (isSelected())
	{
		schemeBgColor(this, markCol);
		int cx = boxX + 3;
		int cy = boxY + 5;
		// Diagonal down-right then up-right (simple checkmark)
		for (int i = 0; i < 3; i++)
			drawFilledRect(cx + i, cy + i, cx + i + 2, cy + i + 2);
		for (int i = 0; i < 4; i++)
			drawFilledRect(cx + 3 + i, cy + 2 - i, cx + 3 + i + 2, cy + 2 - i + 2);
	}
}

void CheckButton::paint()
{
	// Draw label text to the right of the 13x13 checkbox so it doesn't overlap
	int wide, tall;
	getSize(wide, tall);

	char buf[256];
	getText(buf, sizeof(buf));
	int textLen = (int)strlen(buf);
	if (textLen == 0)
		return;

	schemeFgColor(this, g_Scheme.labelTextColor ? g_Scheme.labelTextColor : 0xFFD8DED3);
	drawSetTextFont(Scheme::sf_primary1);
	int textX = 2 + 13 + 6; // box (2px from left, 13 wide) + 6px spacing
	int textY = (tall - 12) / 2;
	drawPrintText(textX, textY, buf, textLen);
}

void CheckButton::internalMousePressed(MouseCode code)
{
	if (code == MOUSE_LEFT)
	{
		setSelected(!isSelected());
		fireActionSignal();
	}
	// Don't call Button::internalMousePressed - we handle toggle differently
	Panel::internalMousePressed(code);
}

}
