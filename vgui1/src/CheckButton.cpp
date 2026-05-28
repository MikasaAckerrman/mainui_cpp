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

	// Draw panel background
	drawSetColor(192, 192, 192, 0);
	drawFilledRect(0, 0, wide, tall);

	// Draw checkbox square (13x13 sunken box)
	int boxSize = 13;
	int boxY = (tall - boxSize) / 2;
	int boxX = 2;

	// Sunken border for checkbox
	drawSetColor(128, 128, 128, 0);
	drawFilledRect(boxX, boxY, boxX + boxSize, boxY + 1);
	drawFilledRect(boxX, boxY, boxX + 1, boxY + boxSize);
	drawSetColor(255, 255, 255, 0);
	drawFilledRect(boxX, boxY + boxSize - 1, boxX + boxSize, boxY + boxSize);
	drawFilledRect(boxX + boxSize - 1, boxY, boxX + boxSize, boxY + boxSize);

	// White fill
	drawSetColor(255, 255, 255, 0);
	drawFilledRect(boxX + 1, boxY + 1, boxX + boxSize - 1, boxY + boxSize - 1);

	// Draw checkmark if selected
	if (isSelected())
	{
		drawSetColor(0, 0, 0, 0);
		// Simple checkmark pattern
		int cx = boxX + 3;
		int cy = boxY + 5;
		for (int i = 0; i < 3; i++)
		{
			drawFilledRect(cx + i, cy + i, cx + i + 2, cy + i + 2);
		}
		for (int i = 0; i < 4; i++)
		{
			drawFilledRect(cx + 3 + i, cy + 2 - i, cx + 3 + i + 2, cy + 2 - i + 2);
		}
	}
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
