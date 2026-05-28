// vgui_main.cpp - Entry points for vgui_support module
// Implements the callbacks the engine calls: Startup, Shutdown, Paint, Mouse, Key, MouseMove

#include <VGUI_App.h>
#include <VGUI_Panel.h>
#include <VGUI_SurfaceBase.h>
#include <VGUI_Scheme.h>
#include <VGUI_Font.h>
#include <string.h>

// Forward declarations from CEngineSurface.cpp
namespace vgui
{
class CEngineSurface;
CEngineSurface* EngineSurface_Create(Panel* embeddedPanel);
void EngineSurface_Destroy();
}

// Forward declaration from VguiOptionsDialog.cpp
void VGUI_OptionsShutdown(void);

// Engine API struct (matches vgui_api.h from engine)
#ifndef VGUI_API_DEFINED
#define VGUI_API_DEFINED

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef int qboolean;

typedef struct
{
	vec2_t point;
	vec2_t coord;
} vpoint_t;

typedef int VGUI_DefaultCursor;

enum VGUI_KeyAction { KA_TYPED = 0, KA_PRESSED, KA_RELEASED };
enum VGUI_MouseAction { MA_PRESSED = 0, MA_RELEASED, MA_DOUBLE, MA_WHEEL };

typedef unsigned int key_modifier_t;

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
	key_modifier_t (*GetKeyModifiers)(void);
	// Engine-called callbacks (filled by us)
	void (*Startup)(int width, int height);
	void (*Shutdown)(void);
	void *(*GetPanel)(void);
	void (*Paint)(void);
	void (*Mouse)(enum VGUI_MouseAction action, int code);
	void (*Key)(enum VGUI_KeyAction action, int code);
	void (*MouseMove)(int x, int y);
	void (*TextInput)(const char *text);
} vguiapi_t;

#endif // VGUI_API_DEFINED

// Global engine API pointer
vguiapi_t *g_api = 0;

// Cvar bridge function pointers
static float (*g_pfnGetCvarFloat)(const char* name) = 0;
static void (*g_pfnSetCvarFloat)(const char* name, float value) = 0;
static const char* (*g_pfnGetCvarString)(const char* name) = 0;
static void (*g_pfnSetCvarString)(const char* name, const char* value) = 0;
static void (*g_pfnClientCmd)(const char* cmd) = 0;

// VGUI state
static vgui::App *s_app = 0;
static vgui::Panel *s_rootPanel = 0;
static vgui::Scheme *s_scheme = 0;
static int s_screenWidth = 0;
static int s_screenHeight = 0;

// ====================================================================
// Callbacks
// ====================================================================

static void VGUI_Startup(int width, int height)
{
	s_screenWidth = width;
	s_screenHeight = height;

	if (!s_app)
	{
		s_app = new vgui::App(true);
		s_scheme = new vgui::Scheme();

		// Create default fonts matching GoldSrc CS 1.6 style
		// Primary font: Tahoma 12px (used for labels, buttons)
		vgui::Font* fontPrimary1 = new vgui::Font("Tahoma", 12, 0, 0, 400, false, false, false, false);
		// Secondary font: Tahoma 14px (titles, headers)
		vgui::Font* fontPrimary2 = new vgui::Font("Tahoma", 14, 0, 0, 700, false, false, false, false);
		// Small font: Tahoma 10px
		vgui::Font* fontPrimary3 = new vgui::Font("Tahoma", 10, 0, 0, 400, false, false, false, false);

		s_scheme->setFont(vgui::Scheme::sf_primary1, fontPrimary1);
		s_scheme->setFont(vgui::Scheme::sf_primary2, fontPrimary2);
		s_scheme->setFont(vgui::Scheme::sf_primary3, fontPrimary3);

		// Set GoldSrc scheme colors
		s_scheme->setColor(vgui::Scheme::sc_primary1, 192, 192, 192, 0);   // gray bg
		s_scheme->setColor(vgui::Scheme::sc_primary2, 128, 128, 128, 0);   // dark gray
		s_scheme->setColor(vgui::Scheme::sc_primary3, 64, 64, 64, 0);      // darker
		s_scheme->setColor(vgui::Scheme::sc_secondary1, 0, 0, 128, 0);     // dark blue (title)
		s_scheme->setColor(vgui::Scheme::sc_secondary2, 255, 255, 255, 0); // white (text)
		s_scheme->setColor(vgui::Scheme::sc_secondary3, 0, 0, 0, 0);       // black
		s_scheme->setColor(vgui::Scheme::sc_white, 255, 255, 255, 0);
		s_scheme->setColor(vgui::Scheme::sc_black, 0, 0, 0, 0);

		s_app->setScheme(s_scheme);
	}

	if (!s_rootPanel)
	{
		s_rootPanel = new vgui::Panel(0, 0, width, height);
		vgui::EngineSurface_Create(s_rootPanel);
	}
	else
	{
		s_rootPanel->setSize(width, height);
	}

	if (g_api && g_api->DrawInit)
		g_api->DrawInit();
}

static void VGUI_Shutdown(void)
{
	if (g_api && g_api->DrawShutdown)
		g_api->DrawShutdown();

	// Null out the options dialog pointer before destroying the panel tree
	VGUI_OptionsShutdown();

	// Clear surface reference from panels BEFORE destroying surface
	if (s_rootPanel)
		s_rootPanel->setSurfaceBaseTraverse(0);

	vgui::EngineSurface_Destroy();

	if (s_rootPanel)
	{
		delete s_rootPanel;
		s_rootPanel = 0;
	}

	if (s_scheme)
	{
		delete s_scheme;
		s_scheme = 0;
	}

	if (s_app)
	{
		delete s_app;
		s_app = 0;
	}
}

static void *VGUI_GetPanel(void)
{
	return s_rootPanel;
}

static void VGUI_Paint(void)
{
	if (!s_rootPanel || !s_app)
		return;

	s_app->externalTick();
	s_rootPanel->solveTraverse();
	s_rootPanel->paintTraverse();
}

static void VGUI_Mouse(enum VGUI_MouseAction action, int code)
{
	if (!s_app || !s_rootPanel)
		return;

	vgui::SurfaceBase* sb = s_rootPanel->getSurfaceBase();
	vgui::MouseCode mc = (vgui::MouseCode)code;

	switch (action)
	{
	case MA_PRESSED:
		s_app->internalMousePressed(mc, sb);
		break;
	case MA_RELEASED:
		s_app->internalMouseReleased(mc, sb);
		break;
	case MA_DOUBLE:
		s_app->internalMouseDoublePressed(mc, sb);
		break;
	case MA_WHEEL:
		s_app->internalMouseWheeled(code, sb);
		break;
	}
}

static void VGUI_Key(enum VGUI_KeyAction action, int code)
{
	if (!s_app || !s_rootPanel)
		return;

	vgui::SurfaceBase* sb = s_rootPanel->getSurfaceBase();
	vgui::KeyCode kc = (vgui::KeyCode)code;

	switch (action)
	{
	case KA_PRESSED:
		s_app->internalKeyPressed(kc, sb);
		break;
	case KA_RELEASED:
		s_app->internalKeyReleased(kc, sb);
		break;
	case KA_TYPED:
		s_app->internalKeyTyped(kc, sb);
		break;
	}
}

static void VGUI_MouseMove(int x, int y)
{
	if (!s_app || !s_rootPanel)
		return;

	vgui::SurfaceBase* sb = s_rootPanel->getSurfaceBase();
	s_app->internalCursorMoved(x, y, sb);
}

static void VGUI_TextInput(const char *text)
{
	// Process UTF-8 text input as key typed events
	if (!s_app || !text || !s_rootPanel)
		return;

	vgui::SurfaceBase* sb = s_rootPanel->getSurfaceBase();
	for (int i = 0; text[i]; i++)
	{
		// Map ASCII to key codes (simplified)
		char ch = text[i];
		if (ch >= 'a' && ch <= 'z')
			s_app->internalKeyTyped((vgui::KeyCode)(vgui::KEY_A + (ch - 'a')), sb);
		else if (ch >= 'A' && ch <= 'Z')
			s_app->internalKeyTyped((vgui::KeyCode)(vgui::KEY_A + (ch - 'A')), sb);
		else if (ch >= '0' && ch <= '9')
			s_app->internalKeyTyped((vgui::KeyCode)(vgui::KEY_0 + (ch - '0')), sb);
		else if (ch == ' ')
			s_app->internalKeyTyped(vgui::KEY_SPACE, sb);
	}
}

// ====================================================================
// Cvar bridge accessors
// ====================================================================

namespace vgui
{

float VGUI_GetCvarFloat(const char* name)
{
	if (g_pfnGetCvarFloat)
		return g_pfnGetCvarFloat(name);
	return 0.0f;
}

void VGUI_SetCvarFloat(const char* name, float value)
{
	if (g_pfnSetCvarFloat)
		g_pfnSetCvarFloat(name, value);
}

const char* VGUI_GetCvarString(const char* name)
{
	if (g_pfnGetCvarString)
		return g_pfnGetCvarString(name);
	return "";
}

void VGUI_SetCvarString(const char* name, const char* value)
{
	if (g_pfnSetCvarString)
		g_pfnSetCvarString(name, value);
}

void VGUI_ClientCmd(const char* cmd)
{
	if (g_pfnClientCmd)
		g_pfnClientCmd(cmd);
}

void VGUI_GetScreenSize(int* w, int* h)
{
	if (w) *w = s_screenWidth;
	if (h) *h = s_screenHeight;
}

}

// ====================================================================
// Root panel accessor for other modules
// ====================================================================

namespace vgui
{
Panel* VGUI_GetRootPanel()
{
	return s_rootPanel;
}
}

// ====================================================================
// Lazy initialization - called from VGUI_ShowOptions if engine didn't
// call InitAPI+Startup (e.g. when loaded via dlopen from mainui)
// ====================================================================
extern "C" void VGUI_EnsureInitialized(int screenW, int screenH)
{
	if (s_rootPanel)
		return; // already initialized

	if (screenW <= 0) screenW = 640;
	if (screenH <= 0) screenH = 480;

	s_screenWidth = screenW;
	s_screenHeight = screenH;

	if (!s_app)
	{
		s_app = new vgui::App(true);
		s_scheme = new vgui::Scheme();

		vgui::Font* fontPrimary1 = new vgui::Font("Tahoma", 12, 0, 0, 400, false, false, false, false);
		vgui::Font* fontPrimary2 = new vgui::Font("Tahoma", 14, 0, 0, 700, false, false, false, false);
		vgui::Font* fontPrimary3 = new vgui::Font("Tahoma", 10, 0, 0, 400, false, false, false, false);

		s_scheme->setFont(vgui::Scheme::sf_primary1, fontPrimary1);
		s_scheme->setFont(vgui::Scheme::sf_primary2, fontPrimary2);
		s_scheme->setFont(vgui::Scheme::sf_primary3, fontPrimary3);

		s_scheme->setColor(vgui::Scheme::sc_primary1, 192, 192, 192, 0);
		s_scheme->setColor(vgui::Scheme::sc_primary2, 128, 128, 128, 0);
		s_scheme->setColor(vgui::Scheme::sc_primary3, 64, 64, 64, 0);
		s_scheme->setColor(vgui::Scheme::sc_secondary1, 0, 0, 128, 0);
		s_scheme->setColor(vgui::Scheme::sc_secondary2, 255, 255, 255, 0);
		s_scheme->setColor(vgui::Scheme::sc_secondary3, 0, 0, 0, 0);
		s_scheme->setColor(vgui::Scheme::sc_white, 255, 255, 255, 0);
		s_scheme->setColor(vgui::Scheme::sc_black, 0, 0, 0, 0);

		s_app->setScheme(s_scheme);
	}

	s_rootPanel = new vgui::Panel(0, 0, screenW, screenH);
	vgui::EngineSurface_Create(s_rootPanel);
}

// ====================================================================
// Export: called by engine to initialize vgui_support
// ====================================================================
extern "C"
{

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

EXPORT void VGUI_SetCvarFuncs(
	float (*pfnGetCvarFloat)(const char*),
	void (*pfnSetCvarFloat)(const char*, float),
	const char* (*pfnGetCvarString)(const char*),
	void (*pfnSetCvarString)(const char*, const char*),
	void (*pfnClientCmd)(const char*))
{
	g_pfnGetCvarFloat = pfnGetCvarFloat;
	g_pfnSetCvarFloat = pfnSetCvarFloat;
	g_pfnGetCvarString = pfnGetCvarString;
	g_pfnSetCvarString = pfnSetCvarString;
	g_pfnClientCmd = pfnClientCmd;
}

EXPORT void VGUI_SetScreenSize(int w, int h)
{
	s_screenWidth = w;
	s_screenHeight = h;
}

EXPORT void InitAPI(vguiapi_t *api)
{
	g_api = api;

	// Register our callbacks
	api->Startup = VGUI_Startup;
	api->Shutdown = VGUI_Shutdown;
	api->GetPanel = VGUI_GetPanel;
	api->Paint = VGUI_Paint;
	api->Mouse = VGUI_Mouse;
	api->Key = VGUI_Key;
	api->MouseMove = VGUI_MouseMove;
	api->TextInput = VGUI_TextInput;
}

} // extern "C"
