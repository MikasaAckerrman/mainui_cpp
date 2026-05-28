// CEngineSurface - bridge between VGUI1 library and Xash3D engine drawing API
// This implements SurfaceBase using vguiapi_t callbacks from the engine.

#include <VGUI_SurfaceBase.h>
#include <VGUI_Panel.h>
#include <VGUI_App.h>
#include <VGUI_Cursor.h>
#include <VGUI_Font.h>
#include <string.h>

// Engine interface types (minimal forward decl matching vgui_api.h)
typedef float vec_t;
typedef vec_t vec2_t[2];
typedef int qboolean;
typedef int VGUI_DefaultCursor;

typedef struct
{
	vec2_t point;
	vec2_t coord;
} vpoint_t;

typedef struct vguiapi_s
{
	qboolean initialized;
	void (*DrawInit)(void);
	void (*DrawShutdown)(void);
	void (*SetupDrawingText)(int *pColor);
	void (*SetupDrawingRect)(int *pColor);
	void (*SetupDrawingImage)(int *pColor);
	void (*BindTexture)(int id);
	void (*EnableTexture)(qboolean enable);
	void (*Reserved0)(int id, int width, int height);
	void (*UploadTexture)(int id, const char *buffer, int width, int height);
	void (*Reserved1)(int id, int drawX, int drawY, const unsigned char *rgba, int blockWidth, int blockHeight);
	void (*DrawQuad)(const vpoint_t *ul, const vpoint_t *lr);
	void (*GetTextureSizes)(int *width, int *height);
	int (*GenerateTexture)(void);
	void *(*EngineMalloc)(size_t size);
	void (*CursorSelect)(VGUI_DefaultCursor cursor);
	unsigned char (*GetColor)(int i, int j);
	qboolean (*IsInGame)(void);
	void (*EnableTextInput)(qboolean enable, qboolean force);
	void (*GetCursorPos)(int *x, int *y);
	int (*ProcessUtfChar)(int ch);
	int (*GetClipboardText)(char *buffer, size_t bufferSize);
	void (*SetClipboardText)(const char *text);
	unsigned int (*GetKeyModifiers)(void);
	void (*Startup)(int width, int height);
	void (*Shutdown)(void);
	void *(*GetPanel)(void);
	void (*Paint)(void);
	void (*Mouse)(int action, int code);
	void (*Key)(int action, int code);
	void (*MouseMove)(int x, int y);
	void (*TextInput)(const char *text);
} vguiapi_t;

extern vguiapi_t *g_api;

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
	if (g_api && g_api->GenerateTexture)
		return g_api->GenerateTexture();
	return 0;
}

void CEngineSurface::GetMousePos(int &x, int &y)
{
	if (g_api && g_api->GetCursorPos)
		g_api->GetCursorPos(&x, &y);
	else
	{
		x = 0;
		y = 0;
	}
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
	if (!g_api)
		return;

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

	if (g_api->EnableTexture)
		g_api->EnableTexture(false);

	if (g_api->SetupDrawingRect)
		g_api->SetupDrawingRect(_drawColor);

	if (g_api->DrawQuad)
	{
		vpoint_t ul, lr;
		ul.point[0] = (float)x0;
		ul.point[1] = (float)y0;
		ul.coord[0] = 0;
		ul.coord[1] = 0;
		lr.point[0] = (float)x1;
		lr.point[1] = (float)y1;
		lr.coord[0] = 1;
		lr.coord[1] = 1;
		g_api->DrawQuad(&ul, &lr);
	}

	if (g_api->EnableTexture)
		g_api->EnableTexture(true);
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
	// Font selection handled by engine - stub
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
	if (!g_api || !text || textLen <= 0)
		return;

	if (g_api->SetupDrawingText)
		g_api->SetupDrawingText(_textColor);

	// Engine handles text rendering via its own font system
	// Each character is rendered as a textured quad
	int x = _textPos[0];
	int y = _textPos[1];
	int charW = 8; // Approximate fixed-width char size

	for (int i = 0; i < textLen && text[i]; i++)
	{
		// Clip check
		if (x >= _clipRect[0] && x + charW <= _clipRect[2] &&
			y >= _clipRect[1] && y + 14 <= _clipRect[3])
		{
			if (g_api->DrawQuad)
			{
				vpoint_t ul, lr;
				ul.point[0] = (float)x;
				ul.point[1] = (float)y;
				ul.coord[0] = 0;
				ul.coord[1] = 0;
				lr.point[0] = (float)(x + charW);
				lr.point[1] = (float)(y + 14);
				lr.coord[0] = 1;
				lr.coord[1] = 1;
				g_api->DrawQuad(&ul, &lr);
			}
		}
		x += charW;
	}

	_textPos[0] = x;
}

void CEngineSurface::drawSetTextureRGBA(int id, const char* rgba, int wide, int tall)
{
	if (g_api && g_api->UploadTexture)
		g_api->UploadTexture(id, rgba, wide, tall);
}

void CEngineSurface::drawSetTexture(int id)
{
	_currentTexture = id;
	if (g_api && g_api->BindTexture)
		g_api->BindTexture(id);
}

void CEngineSurface::drawTexturedRect(int x0, int y0, int x1, int y1)
{
	if (!g_api)
		return;

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

	if (g_api->SetupDrawingImage)
		g_api->SetupDrawingImage(_drawColor);

	if (g_api->DrawQuad)
	{
		vpoint_t ul, lr;
		ul.point[0] = (float)x0;
		ul.point[1] = (float)y0;
		ul.coord[0] = 0;
		ul.coord[1] = 0;
		lr.point[0] = (float)x1;
		lr.point[1] = (float)y1;
		lr.coord[0] = 1;
		lr.coord[1] = 1;
		g_api->DrawQuad(&ul, &lr);
	}
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

	if (g_api && g_api->CursorSelect && cursor)
	{
		Cursor::DefaultCursor dc = cursor->getDefaultCursor();
		g_api->CursorSelect((VGUI_DefaultCursor)dc);
	}
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
