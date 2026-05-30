// Include heavy mainui headers BEFORE VGUI_*.h to avoid the `null` macro clash.
extern void UI_FillRect( int x, int y, int width, int height, const unsigned int color );
#include "TrackerScheme.h"

#include <VGUI_SchemeColors.h>
#include <VGUI_UIScale.h>
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

	// 13x13 sunken box at REFERENCE 640x480; scale via VS() so the checkbox
	// stays proportional to the text on HD screens. Hardcoded 13 used to
	// shrink the box to a near-invisible dot at vguiScale 2-3.
	int boxSize = VS(13);
	int boxY = (tall - boxSize) / 2;
	int boxX = VS(2);

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

	// Check mark when selected. Sized in VS() so the glyph thickens with the
	// box on HD screens; was a tight 3x3+4-row pattern that looked dotted.
	if (isSelected())
	{
		schemeBgColor(this, markCol);
		int cx = boxX + VS(3);
		int cy = boxY + VS(5);
		int t  = VS(2); if (t < 2) t = 2;            // brush thickness
		int s  = VS(1); if (s < 1) s = 1;            // step
		// Diagonal down-right then up-right (simple checkmark)
		for (int i = 0; i < 3 * s; i++)
			drawFilledRect(cx + i, cy + i, cx + i + t, cy + i + t);
		for (int i = 0; i < 4 * s; i++)
			drawFilledRect(cx + 3 * s + i, cy + 2 * s - i, cx + 3 * s + i + t, cy + 2 * s - i + t);
	}
}

void CheckButton::paint()
{
	// Draw label text to the right of the scaled checkbox (canon spacing).
	int wide, tall;
	getSize(wide, tall);

	char buf[256];
	getText(buf, sizeof(buf));
	int textLen = (int)strlen(buf);
	if (textLen == 0)
		return;

	schemeFgColor(this, g_Scheme.labelTextColor ? g_Scheme.labelTextColor : 0xFFD8DED3);
	drawSetTextFont(Scheme::sf_primary1);
	int textX = VS(2) + VS(13) + VS(6);  // box(left+width) + 6px spacing
	int textY = (tall - VS(12)) / 2;
	drawPrintText(textX, textY, buf, textLen);
}

void CheckButton::internalMousePressed(MouseCode code)
{
	if (code == MOUSE_LEFT && isEnabled())
	{
		setSelected(!isSelected());
		fireActionSignal();
	}
	// Don't call Button::internalMousePressed - we handle toggle differently
	Panel::internalMousePressed(code);
}

}
