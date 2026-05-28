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

	setBgColor(255, 255, 255, 0);
	setFgColor(0, 0, 0, 0);
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
	repaint();
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

	// White field background
	int r, g, b, a;
	getBgColor(r, g, b, a);
	drawSetColor(r, g, b, a);
	drawFilledRect(0, 0, wide, tall);

	// Sunken border (GoldSrc text field)
	drawSetColor(128, 128, 128, 0);
	drawFilledRect(0, 0, wide, 1);
	drawFilledRect(0, 0, 1, tall);
	drawSetColor(255, 255, 255, 0);
	drawFilledRect(0, tall - 1, wide, tall);
	drawFilledRect(wide - 1, 0, wide, tall);
}

void TextEntry::paint()
{
	int pwide, ptall;
	getPaintSize(pwide, ptall);

	if (_textLen == 0 && !hasFocus())
		return;

	int r, g, b, a;
	getFgColor(r, g, b, a);
	drawSetTextColor(r, g, b, a);

	if (_font)
		drawSetTextFont(_font);
	else
		drawSetTextFont(_schemeFont);

	// Draw text with basic offset
	int textX = 3 - _scrollOffset;
	int textY = 2;
	drawPrintText(textX, textY, _text, _textLen);

	// Draw cursor if focused
	if (hasFocus() && _editable)
	{
		int cursorX = textX + _cursorPos * 8; // Approximate char width
		drawSetColor(0, 0, 0, 0);
		drawFilledRect(cursorX, 2, cursorX + 1, ptall - 2);
	}
}

void TextEntry::internalMousePressed(MouseCode code)
{
	if (code == MOUSE_LEFT)
	{
		requestFocus();
		// Place cursor (approximate)
		App* app = App::getInstance();
		if (app)
		{
			int mx, my;
			app->getCursorPos(mx, my);
			screenToLocal(mx, my);
			int charPos = (mx - 3 + _scrollOffset) / 8;
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
