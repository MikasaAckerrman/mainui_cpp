// Include heavy mainui headers BEFORE VGUI_*.h to avoid the `null` macro clash.
#include "BaseMenu.h"
#include "FontManager.h"
extern void UI_FillRect( int x, int y, int width, int height, const unsigned int color );
#include "TrackerScheme.h"

#include <VGUI_SchemeColors.h>
#include <VGUI_UIScale.h>
#include <VGUI_Label.h>
#include <VGUI_Panel.h>
#include <VGUI_Image.h>
#include <VGUI_Font.h>
#include <VGUI_Scheme.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

namespace vgui
{

Label::Label(const char* text) : Panel(0, 0, 100, 20)
{
	_text[0] = 0;
	if (text)
		vgui_strcpy(_text, sizeof(_text), text);
	_textAlignment = a_west;
	_contentAlignment = a_west;
	_font = null;
	_schemeFont = Scheme::sf_primary1;
	_image = null;
	_contentFitted = false;
}

Label::Label(const char* text, int x, int y, int wide, int tall) : Panel(x, y, wide, tall)
{
	_text[0] = 0;
	if (text)
		vgui_strcpy(_text, sizeof(_text), text);
	_textAlignment = a_west;
	_contentAlignment = a_west;
	_font = null;
	_schemeFont = Scheme::sf_primary1;
	_image = null;
	_contentFitted = false;
}

void Label::setText(const char* format, ...)
{
	if (!format)
	{
		_text[0] = 0;
		return;
	}
	va_list args;
	va_start(args, format);
	vsnprintf(_text, sizeof(_text), format, args);
	va_end(args);
	_text[sizeof(_text) - 1] = 0;
	repaint();
}

void Label::getText(char* buf, int bufLen)
{
	if (buf && bufLen > 0)
		vgui_strcpy(buf, bufLen, _text);
}

void Label::setFont(Scheme::SchemeFont schemeFont)
{
	_schemeFont = schemeFont;
	_font = null;
}

void Label::setFont(Font* font)
{
	_font = font;
}

void Label::getTextSize(int& wide, int& tall)
{
	// Measure with mainui's FontManager - the SAME font/metrics CEngineSurface
	// uses to actually render text. The VGUI Font object's metrics are not used
	// by the engine surface, so the old "len*8" estimate left button text
	// off-centre and label text mis-aligned. VS(12) matches the sf_primary1
	// glyph height used in drawPrintText.
	int h = VS(12);
	HFont hFont = uiStatic.hDefaultFont;
	if (_text[0] && g_FontMgr && hFont)
		wide = g_FontMgr->GetTextWideScaled(hFont, _text, h);
	else
		wide = (int)strlen(_text) * 8;
	tall = VS(14);
}

void Label::getContentSize(int& wide, int& tall)
{
	getTextSize(wide, tall);
	if (_image)
	{
		int iw, ih;
		_image->getSize(iw, ih);
		wide += iw;
		if (ih > tall)
			tall = ih;
	}
}

void Label::setTextAlignment(Alignment alignment)
{
	_textAlignment = alignment;
}

Label::Alignment Label::getTextAlignment()
{
	return _textAlignment;
}

void Label::setContentAlignment(Alignment alignment)
{
	_contentAlignment = alignment;
}

void Label::setImage(Image* image)
{
	_image = image;
}

void Label::setContentFitted(bool state)
{
	_contentFitted = state;
	if (_contentFitted)
	{
		int w, t;
		getContentSize(w, t);
		setSize(w + 4, t + 4);
	}
}

void Label::computeAlignment(int& tx, int& ty, int twide, int ttall, int pwide, int ptall)
{
	switch (_contentAlignment)
	{
	case a_northwest:
		tx = 0; ty = 0;
		break;
	case a_north:
		tx = (pwide - twide) / 2; ty = 0;
		break;
	case a_northeast:
		tx = pwide - twide; ty = 0;
		break;
	case a_west:
		tx = 0; ty = (ptall - ttall) / 2;
		break;
	case a_center:
		tx = (pwide - twide) / 2; ty = (ptall - ttall) / 2;
		break;
	case a_east:
		tx = pwide - twide; ty = (ptall - ttall) / 2;
		break;
	case a_southwest:
		tx = 0; ty = ptall - ttall;
		break;
	case a_south:
		tx = (pwide - twide) / 2; ty = ptall - ttall;
		break;
	case a_southeast:
		tx = pwide - twide; ty = ptall - ttall;
		break;
	default:
		tx = 0; ty = 0;
		break;
	}
}

void Label::paintBackground()
{
	// Labels typically have transparent background
	// Only paint if bg color was explicitly set
}

void Label::paint()
{
	int pwide, ptall;
	getPaintSize(pwide, ptall);

	int textLen = (int)strlen(_text);
	if (textLen == 0 && !_image)
		return;

	int twide, ttall;
	getContentSize(twide, ttall);

	int tx, ty;
	computeAlignment(tx, ty, twide, ttall, pwide, ptall);

	// Draw image if present
	if (_image)
	{
		_image->doPaint(this);
	}

	// Draw text
	if (textLen > 0)
	{
		// If user explicitly set fgColor (alpha != 0 in VGUI inverted = not opaque),
		// honor it. Otherwise pick label color from g_Scheme.
		int r, g, b, a;
		getFgColor(r, g, b, a);
		if (r == 0 && g == 0 && b == 0 && a == 0)
		{
			// Default: use scheme labelTextColor
			schemeFgColor(this, g_Scheme.labelTextColor ? g_Scheme.labelTextColor : 0xFFC8C8C8);
		}
		else
		{
			drawSetTextColor(r, g, b, a);
		}

		if (_font)
			drawSetTextFont(_font);
		else
			drawSetTextFont(_schemeFont);

		drawPrintText(tx, ty, _text, textLen);
	}
}

}
