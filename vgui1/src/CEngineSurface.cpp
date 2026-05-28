// CEngineSurface - bridge between VGUI1 library and Xash3D engine drawing API
// This file is compiled directly into libmenu.so, so it uses EngFuncs directly
// instead of going through g_api callbacks.

#include <VGUI_SurfaceBase.h>
#include <VGUI_Panel.h>
#include <VGUI_App.h>
#include <VGUI_Cursor.h>
#include <VGUI_Font.h>
#include <string.h>

#include "enginecallback_menu.h"

// From Utils.h - avoid including the full header to prevent key definition conflicts
extern void UI_GetCursorPos( int *pos_x, int *pos_y );

static inline unsigned int PackRGBA_local( unsigned int r, unsigned int g, unsigned int b, unsigned int a )
{
	return ((a)<<24|(r)<<16|(g)<<8|(b));
}

namespace vgui
{

class CEngineSurface : public SurfaceBase
{
public:
	CEngineSurface(Panel* embeddedPanel);
	virtual ~CEngineSurface();

public:
	// SurfaceBase pure virtuals
	virtual void setTitle(const char* title);
	virtual bool setFullscreenMode(int wide, int tall, int bpp);
	virtual void setWindowedMode();
	virtual void setAsTopMost(bool state);
	virtual void createPopup(Panel* embeddedPanel);
	virtual bool hasFocus();
	virtual bool isWithin(int x, int y);
	virtual int createNewTextureID();
	virtual void GetMousePos(int &x, int &y);
	virtual void drawSetColor(int r, int g, int b, int a);
	virtual void drawFilledRect(int x0, int y0, int x1, int y1);
	virtual void drawOutlinedRect(int x0, int y0, int x1, int y1);
	virtual void drawSetTextFont(Font* font);
	virtual void drawSetTextColor(int r, int g, int b, int a);
	virtual void drawSetTextPos(int x, int y);
	virtual void drawPrintText(const char* text, int textLen);
	virtual void drawSetTextureRGBA(int id, const char* rgba, int wide, int tall);
	virtual void drawSetTexture(int id);
	virtual void drawTexturedRect(int x0, int y0, int x1, int y1);
	virtual void invalidate(Panel* panel);
	virtual void enableMouseCapture(bool state);
	virtual void setCursor(Cursor* cursor);
	virtual void swapBuffers();
	virtual void pushMakeCurrent(Panel* panel, bool useInsets);
	virtual void popMakeCurrent(Panel* panel);
	virtual void applyChanges();

private:
	int _drawColor[4];
	int _textColor[4];
	int _textPos[2];
	int _currentTexture;
	int _translateX;
	int _translateY;
	int _clipRect[4];
	Font* _currentFont;

	struct PanelState
	{
		int translateX, translateY;
		int clipRect[4];
	};
	PanelState _stateStack[64];
	int _stateStackCount;
};

// ====================================================================
// Implementation
// ====================================================================

CEngineSurface::CEngineSurface(Panel* embeddedPanel) : SurfaceBase(embeddedPanel)
{
	memset(_drawColor, 0, sizeof(_drawColor));
	memset(_textColor, 0, sizeof(_textColor));
	memset(_textPos, 0, sizeof(_textPos));
	_currentTexture = 0;
	_translateX = 0;
	_translateY = 0;
	memset(_clipRect, 0, sizeof(_clipRect));
	_stateStackCount = 0;
	_currentFont = 0;
}

CEngineSurface::~CEngineSurface()
{
}

void CEngineSurface::setTitle(const char* title)
{
	// Not applicable for embedded engine surface
}

bool CEngineSurface::setFullscreenMode(int wide, int tall, int bpp)
{
	return false;
}

void CEngineSurface::setWindowedMode()
{
}

void CEngineSurface::setAsTopMost(bool state)
{
}

void CEngineSurface::createPopup(Panel* embeddedPanel)
{
}

bool CEngineSurface::hasFocus()
{
	return true;
}

bool CEngineSurface::isWithin(int x, int y)
{
	return true;
}

int CEngineSurface::createNewTextureID()
{
	static int s_nextTextureId = 1;
	return s_nextTextureId++;
}

void CEngineSurface::GetMousePos(int &x, int &y)
{
	UI_GetCursorPos(&x, &y);
}

void CEngineSurface::drawSetColor(int r, int g, int b, int a)
{
	_drawColor[0] = r;
	_drawColor[1] = g;
	_drawColor[2] = b;
	_drawColor[3] = a;
}

void CEngineSurface::drawFilledRect(int x0, int y0, int x1, int y1)
{
	// Apply translation
	x0 += _translateX;
	y0 += _translateY;
	x1 += _translateX;
	y1 += _translateY;

	// Clip
	if (x0 < _clipRect[0]) x0 = _clipRect[0];
	if (y0 < _clipRect[1]) y0 = _clipRect[1];
	if (x1 > _clipRect[2]) x1 = _clipRect[2];
	if (y1 > _clipRect[3]) y1 = _clipRect[3];
	if (x0 >= x1 || y0 >= y1)
		return;

	EngFuncs::FillRGBA(x0, y0, x1 - x0, y1 - y0,
		_drawColor[0], _drawColor[1], _drawColor[2], _drawColor[3]);
}

void CEngineSurface::drawOutlinedRect(int x0, int y0, int x1, int y1)
{
	drawFilledRect(x0, y0, x1, y0 + 1);       // top
	drawFilledRect(x0, y1 - 1, x1, y1);       // bottom
	drawFilledRect(x0, y0 + 1, x0 + 1, y1 - 1); // left
	drawFilledRect(x1 - 1, y0 + 1, x1, y1 - 1); // right
}

void CEngineSurface::drawSetTextFont(Font* font)
{
	_currentFont = font;
}

void CEngineSurface::drawSetTextColor(int r, int g, int b, int a)
{
	_textColor[0] = r;
	_textColor[1] = g;
	_textColor[2] = b;
	_textColor[3] = a;
}

void CEngineSurface::drawSetTextPos(int x, int y)
{
	_textPos[0] = x + _translateX;
	_textPos[1] = y + _translateY;
}

void CEngineSurface::drawPrintText(const char* text, int textLen)
{
	if (!text || textLen <= 0)
		return;

	int x = _textPos[0];
	int y = _textPos[1];
	int charH = (_currentFont && _currentFont->getTall() > 0) ? _currentFont->getTall() : 14;
	unsigned int color = PackRGBA_local(_textColor[0], _textColor[1], _textColor[2], _textColor[3]);

	for (int i = 0; i < textLen && text[i]; i++)
	{
		int a, b, c;
		if (_currentFont)
			_currentFont->getCharABCwide((unsigned char)text[i], a, b, c);
		else
		{
			a = 0; b = 8; c = 0;
		}

		int charW = a + b + c;
		if (charW <= 0) charW = 8;

		x += a; // leading space

		if (x >= _clipRect[0] && x + b <= _clipRect[2] &&
			y >= _clipRect[1] && y + charH <= _clipRect[3])
		{
			EngFuncs::DrawCharacter(x, y, b, charH, (unsigned char)text[i], color, 0);
		}

		x += b + c; // char body + trailing space
	}

	_textPos[0] = x;
}

void CEngineSurface::drawSetTextureRGBA(int id, const char* rgba, int wide, int tall)
{
	// No-op: Options dialog does not use textures
}

void CEngineSurface::drawSetTexture(int id)
{
	_currentTexture = id;
}

void CEngineSurface::drawTexturedRect(int x0, int y0, int x1, int y1)
{
	// No-op: Options dialog does not use textured rects
}

void CEngineSurface::invalidate(Panel* panel)
{
	if (panel)
		panel->repaint();
}

void CEngineSurface::enableMouseCapture(bool state)
{
	// Handled by engine
}

void CEngineSurface::setCursor(Cursor* cursor)
{
	_currentCursor = cursor;
}

void CEngineSurface::swapBuffers()
{
	// Engine handles buffer swapping
}

void CEngineSurface::pushMakeCurrent(Panel* panel, bool useInsets)
{
	if (!panel)
		return;

	// Save current state
	if (_stateStackCount < 64)
	{
		PanelState& s = _stateStack[_stateStackCount++];
		s.translateX = _translateX;
		s.translateY = _translateY;
		s.clipRect[0] = _clipRect[0];
		s.clipRect[1] = _clipRect[1];
		s.clipRect[2] = _clipRect[2];
		s.clipRect[3] = _clipRect[3];
	}

	int x, y, wide, tall;
	panel->getPos(x, y);
	panel->getSize(wide, tall);

	if (useInsets)
	{
		int inLeft, inTop, inRight, inBottom;
		panel->getInset(inLeft, inTop, inRight, inBottom);
		x += inLeft;
		y += inTop;
		wide -= (inLeft + inRight);
		tall -= (inTop + inBottom);
	}

	_translateX += x;
	_translateY += y;

	// Update clip rect
	int newClip[4];
	newClip[0] = _translateX;
	newClip[1] = _translateY;
	newClip[2] = _translateX + wide;
	newClip[3] = _translateY + tall;

	// Intersect with parent clip
	if (_stateStackCount > 1)
	{
		PanelState& parent = _stateStack[_stateStackCount - 1];
		if (newClip[0] < parent.clipRect[0]) newClip[0] = parent.clipRect[0];
		if (newClip[1] < parent.clipRect[1]) newClip[1] = parent.clipRect[1];
		if (newClip[2] > parent.clipRect[2]) newClip[2] = parent.clipRect[2];
		if (newClip[3] > parent.clipRect[3]) newClip[3] = parent.clipRect[3];
	}

	_clipRect[0] = newClip[0];
	_clipRect[1] = newClip[1];
	_clipRect[2] = newClip[2];
	_clipRect[3] = newClip[3];
}

void CEngineSurface::popMakeCurrent(Panel* panel)
{
	if (_stateStackCount > 0)
	{
		_stateStackCount--;
		PanelState& s = _stateStack[_stateStackCount];
		_translateX = s.translateX;
		_translateY = s.translateY;
		_clipRect[0] = s.clipRect[0];
		_clipRect[1] = s.clipRect[1];
		_clipRect[2] = s.clipRect[2];
		_clipRect[3] = s.clipRect[3];
	}
}

void CEngineSurface::applyChanges()
{
	// Engine handles this
}

// ====================================================================
// Factory function - called from vgui_support startup
// ====================================================================
static CEngineSurface* s_engineSurface = null;

CEngineSurface* EngineSurface_Create(Panel* embeddedPanel)
{
	if (!s_engineSurface)
		s_engineSurface = new CEngineSurface(embeddedPanel);
	return s_engineSurface;
}

void EngineSurface_Destroy()
{
	if (s_engineSurface)
	{
		delete s_engineSurface;
		s_engineSurface = null;
	}
}

}
