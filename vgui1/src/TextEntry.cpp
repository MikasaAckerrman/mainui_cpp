// Include heavy mainui headers BEFORE VGUI_*.h to avoid the `null` macro clash.
#include "BaseMenu.h"
#include "FontManager.h"
extern void UI_FillRect( int x, int y, int width, int height, const unsigned int color );
extern void UI_EnableTextInput( bool enable );  // mainui bridge -> EngFuncs::EnableTextInput
#include "TrackerScheme.h"

#include <VGUI_SchemeColors.h>
#include <VGUI_UIScale.h>
#include <VGUI_TextEntry.h>
#include <VGUI_App.h>
#include <VGUI_ActionSignal.h>
#include <VGUI_Font.h>
#include <VGUI_Scheme.h>
#include <string.h>

namespace vgui
{

// Width (px) of the first n bytes of s as actually rendered by CEngineSurface
// (mainui FontManager, sf_primary1 height). Used for caret placement and
// click-to-position so they track the proportional glyphs instead of a fixed
// 8px/char guess. Falls back to 8px/char only if the font manager is absent.
static int TE_MeasureWidth(const char* s, int n)
{
	if (n <= 0)
		return 0;
	if (n > 255)
		n = 255;
	HFont hFont = g_FontMgr ? g_FontMgr->GetVGUIFont(VS(12)) : 0;
	if (!g_FontMgr || !hFont)
		return n * 8;
	char tmp[256];
	memcpy(tmp, s, n);
	tmp[n] = 0;
	return g_FontMgr->GetTextWideScaled(hFont, tmp, VS(12));
}

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
	_scrollOffset = 0;
	_selectStart = -1;
	_selectEnd = -1;
	repaint();
}

void TextEntry::internalFocusChanged(bool lost)
{
	UI_EnableTextInput(!lost);
	Panel::internalFocusChanged(lost);
}

void TextEntry::getText(int offset, char* buf, int bufLen)
{
	if (!buf || bufLen <= 0) return;
	if (offset < 0) offset = 0;
	if (offset >= _textLen) { buf[0] = 0; return; }
	int copyLen = _textLen - offset;
	if (copyLen >= bufLen) copyLen = bufLen - 1;
	memcpy(buf, _text + offset, copyLen);
	buf[copyLen] = 0;
}

int TextEntry::getTextLength() { return _textLen; }
void TextEntry::setEditable(bool state) { _editable = state; }
void TextEntry::setFont(Font* font) { _font = font; }
void TextEntry::setFont(Scheme::SchemeFont schemeFont) { _schemeFont = schemeFont; _font = null; }

void TextEntry::addActionSignal(ActionSignal* s) { _actionSignalDar.addElement(s); }

void TextEntry::fireActionSignal()
{
	for (int i = 0; i < _actionSignalDar.getCount(); i++)
	{
		ActionSignal* s = _actionSignalDar[i];
		if (s) s->actionPerformed(this);
	}
}

void TextEntry::selectNone() { _selectStart = -1; _selectEnd = -1; repaint(); }
void TextEntry::selectAll() { _selectStart = 0; _selectEnd = _textLen; repaint(); }

void TextEntry::paintBackground()
{
	int wide, tall;
	getSize(wide, tall);

	unsigned int bg     = g_Scheme.fieldBgColor ? g_Scheme.fieldBgColor : 0xE63E4637;
	unsigned int bright = g_Scheme.borderBright ? g_Scheme.borderBright : 0xFF889180;
	unsigned int dark   = g_Scheme.borderDark   ? g_Scheme.borderDark   : 0xFF282E22;

	// Field background (darker than frame for recessed look)
	schemeBgColor(this, bg);
	drawFilledRect(0, 0, wide, tall);

	// Canon CS 1.6 InsetBorder = 1px only:
	// Top + Left = BorderDark; Bottom + Right = BorderBright (sunken look).
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

	// Scroll-on-overflow: keep the caret visible inside the field. Without
	// this, typing past the right edge would push the caret off-screen and
	// the user would lose sight of where they are. Computed here so it tracks
	// every paint (covers caret moves from arrow keys, click-to-position, etc).
	int leftPad  = VS(6);
	int rightPad = VS(6);
	int visibleW = pwide - leftPad - rightPad;
	if (visibleW < 1) visibleW = 1;

	int caretAbs = TE_MeasureWidth(_text, _cursorPos);
	int totalW   = TE_MeasureWidth(_text, _textLen);

	// Slide the scroll window so the caret sits inside [scroll, scroll+visibleW].
	if (caretAbs - _scrollOffset > visibleW)
		_scrollOffset = caretAbs - visibleW;
	if (caretAbs - _scrollOffset < 0)
		_scrollOffset = caretAbs;
	// Don't scroll past content; if everything fits, anchor at 0.
	if (_scrollOffset > totalW - visibleW)
		_scrollOffset = totalW - visibleW;
	if (_scrollOffset < 0)
		_scrollOffset = 0;

	// GoldSrc left padding: 6px inside the inset border
	int textX = leftPad - _scrollOffset;
	// Vertical center: text baseline aligned to center of field
	int textY = (ptall - VS(11)) / 2;
	if (textY < 2) textY = 2;

	drawPrintText(textX, textY, _text, _textLen);

	// Cursor
	if (hasFocus() && _editable)
	{
		int prefix = (_cursorPos < _textLen) ? _cursorPos : _textLen;
		int cursorX = textX + TE_MeasureWidth(_text, prefix);
		schemeBgColor(this, textCol);
		drawFilledRect(cursorX, 3, cursorX + 1, ptall - 3);
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

			int targetX = mx - VS(6) + _scrollOffset;
			int charPos = 0;
			// Walk character boundaries using real rendered widths; stop at the
			// gap nearest the click (midpoint between glyph edges).
			while (charPos < _textLen)
			{
				int wHere = TE_MeasureWidth(_text, charPos);
				int wNext = TE_MeasureWidth(_text, charPos + 1);
				if (targetX < (wHere + wNext) / 2)
					break;
				charPos++;
			}

			if (charPos < 0) charPos = 0;
			if (charPos > _textLen) charPos = _textLen;
			_cursorPos = charPos;
			repaint();
		}
	}
	Panel::internalMousePressed(code);
}

void TextEntry::internalCursorMoved(int x, int y) { Panel::internalCursorMoved(x, y); }
void TextEntry::internalMouseReleased(MouseCode code) { Panel::internalMouseReleased(code); }

void TextEntry::internalKeyPressed(KeyCode code)
{
	if (!_editable) { Panel::internalKeyPressed(code); return; }

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
			fireActionSignal();
		}
		break;
	case KEY_DELETE:
		if (_cursorPos < _textLen)
		{
			memmove(_text + _cursorPos, _text + _cursorPos + 1, _textLen - _cursorPos);
			_textLen--;
			fireActionSignal();
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

// Insert a single printable ASCII char at the caret. Shared by the typed-key
// path and the literal char/IME path. Ignores control chars and respects the
// buffer limit; keeps the null terminator intact via the +1 in the memmove.
void TextEntry::insertChar(char ch)
{
	if (!_editable)
		return;
	if (ch < 32 || (unsigned char)ch >= 127) // printable ASCII only
		return;
	if (_textLen >= (int)sizeof(_text) - 1)
		return;
	memmove(_text + _cursorPos + 1, _text + _cursorPos, _textLen - _cursorPos + 1);
	_text[_cursorPos] = ch;
	_cursorPos++;
	_textLen++;
	repaint();
	fireActionSignal();
}

void TextEntry::internalKeyTyped(KeyCode code)
{
	if (!_editable) { Panel::internalKeyTyped(code); return; }

	App* app = App::getInstance();
	if (!app) { Panel::internalKeyTyped(code); return; }

	// Legacy KeyCode-typed path (case-less). The primary text route is
	// internalCharTyped, which preserves case/symbols.
	char ch = app->getKeyCodeChar(code, false);
	if (ch != '\n' && ch != '\t')
		insertChar(ch);

	Panel::internalKeyTyped(code);
}

// Primary text-input path: the engine's char/IME route delivers the already
// resolved character (correct case, shifted symbols). Insert it verbatim.
void TextEntry::internalCharTyped(char ch)
{
	insertChar(ch);
}

}
