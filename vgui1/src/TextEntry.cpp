// Include heavy mainui headers BEFORE VGUI_*.h to avoid the `null` macro clash.
extern void UI_FillRect( int x, int y, int width, int height, const unsigned int color );
extern void UI_EnableTextInput( bool enable );  // mainui bridge -> EngFuncs::EnableTextInput
#include "TrackerScheme.h"

#include <VGUI_SchemeColors.h>
#include <VGUI_TextEntry.h>
#include <VGUI_App.h>
#include <VGUI_ActionSignal.h>
#include <VGUI_Font.h>
#include <VGUI_Scheme.h>
#include <string.h>

namespace vgui
{

TextEntry::TextEntry(const char* text, int x, int y, int wide, int tall) : Panel(x, y, wide, tall)
{
	_text[0] = 0;
	_textLen = 0;
	if (text)
	{
		vgui_strcpy(_text, sizeof(_text), text);
		_textLen = (int)strlen(_text);
	}
	_cursorPos = _textLen;
	_selectStart = -1;
	_selectEnd = -1;
	_scrollOffset = 0;
	_editable = true;
	_font = null;
	_schemeFont = Scheme::sf_primary1;

	// Visual colors driven by g_Scheme at draw time
}

void TextEntry::setText(const char* text, int textLen)
{
	if (!text || textLen <= 0)
	{
		_text[0] = 0;
		_textLen = 0;
	}
	else
	{
		int copyLen = textLen;
		if (copyLen >= (int)sizeof(_text))
			copyLen = (int)sizeof(_text) - 1;
		memcpy(_text, text, copyLen);
		_text[copyLen] = 0;
		_textLen = copyLen;
	}
	_cursorPos = _textLen;
	_scrollOffset = 0;     // reset horizontal scroll so old offset doesn't bleed across opens
	_selectStart = -1;
	_selectEnd = -1;
	repaint();
}

// Override: when this entry gains keyboard focus, ask the engine to bring up
// the Android soft keyboard. When focus is lost, hide it. Without this the
// IME never appears on touch-only devices.
void TextEntry::internalFocusChanged(bool lost)
{
	UI_EnableTextInput(!lost);
	Panel::internalFocusChanged(lost);
}

void TextEntry::getText(int offset, char* buf, int bufLen)
{
	if (!buf || bufLen <= 0)
		return;
	if (offset < 0) offset = 0;
	if (offset >= _textLen)
	{
		buf[0] = 0;
		return;
	}
	int copyLen = _textLen - offset;
	if (copyLen >= bufLen)
		copyLen = bufLen - 1;
	memcpy(buf, _text + offset, copyLen);
	buf[copyLen] = 0;
}

int TextEntry::getTextLength()
{
	return _textLen;
}

void TextEntry::setEditable(bool state)
{
	_editable = state;
}

void TextEntry::setFont(Font* font)
{
	_font = font;
}

void TextEntry::setFont(Scheme::SchemeFont schemeFont)
{
	_schemeFont = schemeFont;
	_font = null;
}

void TextEntry::addActionSignal(ActionSignal* s)
{
	_actionSignalDar.addElement(s);
}

void TextEntry::fireActionSignal()
{
	for (int i = 0; i < _actionSignalDar.getCount(); i++)
	{
		ActionSignal* s = _actionSignalDar[i];
		if (s)
			s->actionPerformed(this);
	}
}

void TextEntry::selectNone()
{
	_selectStart = -1;
	_selectEnd = -1;
	repaint();
}

void TextEntry::selectAll()
{
	_selectStart = 0;
	_selectEnd = _textLen;
	repaint();
}

void TextEntry::paintBackground()
{
	int wide, tall;
	getSize(wide, tall);

	unsigned int bg     = g_Scheme.fieldBgColor   ? g_Scheme.fieldBgColor   : 0xE655604B;
	unsigned int bright = g_Scheme.borderBright   ? g_Scheme.borderBright   : 0xC85F6558;
	unsigned int dark   = g_Scheme.borderDark     ? g_Scheme.borderDark     : 0xC8282C24;

	// Field background
	schemeBgColor(this, bg);
	drawFilledRect(0, 0, wide, tall);

	// Sunken bevel: dark top + left, bright bottom + right
	schemeBgColor(this, dark);
	drawFilledRect(0, 0, wide, 1);
	drawFilledRect(0, 0, 1, tall);

	schemeBgColor(this, bright);
	drawFilledRect(0, tall - 1, wide, tall);
	drawFilledRect(wide - 1, 0, wide, tall);
}

void TextEntry::paint()
{
	int pwide, ptall;
	getPaintSize(pwide, ptall);

	if (_textLen == 0 && !hasFocus())
		return;

	unsigned int textCol = g_Scheme.fieldTextColor ? g_Scheme.fieldTextColor : 0xFFFFFFFF;
	schemeFgColor(this, textCol);

	if (_font)
		drawSetTextFont(_font);
	else
		drawSetTextFont(_schemeFont);

	int textX = 4 - _scrollOffset;
	int textY = 3;
	drawPrintText(textX, textY, _text, _textLen);

	// Draw cursor if focused
	if (hasFocus() && _editable)
	{
		// Compute cursor X using font metrics
		int cursorX = textX;
		if (_font)
		{
			for (int i = 0; i < _cursorPos && i < _textLen; i++)
			{
				int a, b, c;
				_font->getCharABCwide((unsigned char)_text[i], a, b, c);
				cursorX += a + b + c;
			}
		}
		else
		{
			cursorX += _cursorPos * 8;
		}
		// Cursor uses field text color
		schemeBgColor(this, g_Scheme.fieldTextColor ? g_Scheme.fieldTextColor : 0xFFFFFFFF);
		drawFilledRect(cursorX, 2, cursorX + 1, ptall - 2);
	}
}

void TextEntry::internalMousePressed(MouseCode code)
{
	if (code == MOUSE_LEFT)
	{
		requestFocus();
		App* app = App::getInstance();
		if (app)
		{
			int mx, my;
			app->getCursorPos(mx, my);
			screenToLocal(mx, my);

			// Find char position using font metrics
			int targetX = mx - 3 + _scrollOffset;
			int charPos = 0;
			int accum = 0;

			if (_font)
			{
				for (int i = 0; i < _textLen; i++)
				{
					int a, b, c;
					_font->getCharABCwide((unsigned char)_text[i], a, b, c);
					int cw = a + b + c;
					if (accum + cw / 2 > targetX)
						break;
					accum += cw;
					charPos++;
				}
			}
			else
			{
				charPos = targetX / 8;
			}

			if (charPos < 0) charPos = 0;
			if (charPos > _textLen) charPos = _textLen;
			_cursorPos = charPos;
			repaint();
		}
	}
	Panel::internalMousePressed(code);
}

void TextEntry::internalCursorMoved(int x, int y)
{
	Panel::internalCursorMoved(x, y);
}

void TextEntry::internalMouseReleased(MouseCode code)
{
	Panel::internalMouseReleased(code);
}

void TextEntry::internalKeyPressed(KeyCode code)
{
	if (!_editable)
	{
		Panel::internalKeyPressed(code);
		return;
	}

	switch (code)
	{
	case KEY_LEFT:
		if (_cursorPos > 0) _cursorPos--;
		break;
	case KEY_RIGHT:
		if (_cursorPos < _textLen) _cursorPos++;
		break;
	case KEY_HOME:
		_cursorPos = 0;
		break;
	case KEY_END:
		_cursorPos = _textLen;
		break;
	case KEY_BACKSPACE:
		if (_cursorPos > 0 && _textLen > 0)
		{
			memmove(_text + _cursorPos - 1, _text + _cursorPos, _textLen - _cursorPos + 1);
			_cursorPos--;
			_textLen--;
		}
		break;
	case KEY_DELETE:
		if (_cursorPos < _textLen)
		{
			memmove(_text + _cursorPos, _text + _cursorPos + 1, _textLen - _cursorPos);
			_textLen--;
		}
		break;
	case KEY_ENTER:
		fireActionSignal();
		break;
	default:
		break;
	}
	repaint();
	Panel::internalKeyPressed(code);
}

void TextEntry::internalKeyTyped(KeyCode code)
{
	if (!_editable)
	{
		Panel::internalKeyTyped(code);
		return;
	}

	App* app = App::getInstance();
	if (!app)
	{
		Panel::internalKeyTyped(code);
		return;
	}

	char ch = app->getKeyCodeChar(code, false);
	if (ch && ch != '\n' && ch != '\t' && _textLen < (int)sizeof(_text) - 1)
	{
		memmove(_text + _cursorPos + 1, _text + _cursorPos, _textLen - _cursorPos + 1);
		_text[_cursorPos] = ch;
		_cursorPos++;
		_textLen++;
		repaint();
	}

	Panel::internalKeyTyped(code);
}

}
