// vgui_main.cpp - Entry points for vgui_support module
// Implements the callbacks the engine calls: Startup, Shutdown, Paint, Mouse, Key, MouseMove

#include <VGUI_App.h>
#include <VGUI_Panel.h>
#include <VGUI_SurfaceBase.h>
#include <VGUI_Scheme.h>
#include <VGUI_Font.h>
#include <string.h>
#include "keydefs.h"

// Forward declarations from CEngineSurface.cpp
namespace vgui
{
class CEngineSurface;
CEngineSurface* EngineSurface_Create(Panel* embeddedPanel);
void EngineSurface_Destroy();
}

// Diagnostic logging
#include <VGUI_Log.h>

// Forward declaration from VguiOptionsDialog.cpp
void VGUI_OptionsShutdown(void);

// Forward declarations from VguiConsole.cpp
namespace vgui
{
void VGUI_Console_Output(const char* text);
void VGUI_Console_Show(bool show);
bool VGUI_Console_IsVisible();
}
void VGUI_ConsoleShutdown(void);
void VGUI_LoadingShutdown(void);

// Forward declarations from VguiLoadingDialog.cpp
namespace vgui
{
void VGUI_Loading_Show(bool show, const char* statusText, float progress);
bool VGUI_Loading_IsVisible();
}

// Forward declarations from VguiCreateGame.cpp
namespace vgui
{
void VGUI_CreateGame_Show(bool show);
bool VGUI_CreateGame_IsVisible();
}
void VGUI_CreateGameShutdown(void);

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

// Key-grab callback: when set, intercepts the next key/mouse press at the
// engine-code level, before VGUI KeyCode translation. Used by the Keyboard
// tab to capture which key the user wants to bind.
static void (*g_pfnKeyGrabCallback)(int engineKeyCode) = 0;

// VGUI state
static vgui::App *s_app = 0;
static vgui::Panel *s_rootPanel = 0;
static vgui::Scheme *s_scheme = 0;
static int s_screenWidth = 0;
static int s_screenHeight = 0;

// Global UI scale derived from physical screen height (mainui logical 768 ref).
namespace vgui { float g_vguiScale = 1.0f; }

static void VGUI_ComputeScale(int screenH)
{
	float s = (float)screenH / 768.0f;
	if (s < 1.0f) s = 1.0f;
	if (s > 3.0f) s = 3.0f;
	vgui::g_vguiScale = s;
}

// ====================================================================
// Callbacks
// ====================================================================

static void VGUI_Startup(int width, int height)
{
	s_screenWidth = width;
	s_screenHeight = height;
	VGUI_ComputeScale(height);

	if (!s_app)
	{
		s_app = new vgui::App(true);
		s_scheme = new vgui::Scheme();

		// Create default fonts matching GoldSrc CS 1.6 style, scaled by screen
		int fontMed   = (int)(12.0f * vgui::g_vguiScale + 0.5f);
		int fontTitle = (int)(14.0f * vgui::g_vguiScale + 0.5f);
		int fontSmall = (int)(10.0f * vgui::g_vguiScale + 0.5f);

		vgui::Font* fontPrimary1 = new vgui::Font("Tahoma", fontMed,   0, 0, 400, false, false, false, false);
		vgui::Font* fontPrimary2 = new vgui::Font("Tahoma", fontTitle, 0, 0, 700, false, false, false, false);
		vgui::Font* fontPrimary3 = new vgui::Font("Tahoma", fontSmall, 0, 0, 400, false, false, false, false);

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

	// DrawInit no longer needed - we use EngFuncs directly
}

static void VGUI_Shutdown(void)
{
	// DrawShutdown no longer needed - we use EngFuncs directly

	// Null out the options dialog pointer before destroying the panel tree
	VGUI_OptionsShutdown();
	VGUI_ConsoleShutdown();
	VGUI_LoadingShutdown();
	VGUI_CreateGameShutdown();

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
	// No-op: painting is now handled by VGUI_PaintAll called from BaseMenu.cpp.
	// This avoids double-traversal if the engine ever calls Paint via InitAPI.
}

void VGUI_Mouse(enum VGUI_MouseAction action, int code)
{
	if (!s_app || !s_rootPanel)
		return;

	if (g_pfnKeyGrabCallback && action == MA_PRESSED)
	{
		// Mouse buttons are K_MOUSE1 + code (code is 0-based from MOUSE_LEFT)
		g_pfnKeyGrabCallback(K_MOUSE1 + code);
		g_pfnKeyGrabCallback = 0; // one-shot
		return;
	}

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

// Translate Xash3D engine key codes (ASCII for printable, K_* defines from
// keydefs.h for non-printable) to the vgui::KeyCode enum used by VGUI1
// widgets. Returns vgui::KEY_LAST for keys that have no VGUI equivalent
// (gamepad, joystick, mouse-paired engine codes etc.) so the caller can
// drop them.
static vgui::KeyCode EngineKeyToVgui(int engineCode)
{
	using namespace vgui;
	if (engineCode <= 0)
		return KEY_LAST;

	// ASCII-range printable keys (engine sends letters/digits as raw ASCII)
	if (engineCode >= '0' && engineCode <= '9')
		return (KeyCode)(KEY_0 + (engineCode - '0'));
	if (engineCode >= 'a' && engineCode <= 'z')
		return (KeyCode)(KEY_A + (engineCode - 'a'));
	if (engineCode >= 'A' && engineCode <= 'Z')
		return (KeyCode)(KEY_A + (engineCode - 'A'));

	switch (engineCode)
	{
	// Specials defined in sdk_includes/engine/keydefs.h
	case 9:   return KEY_TAB;          // K_TAB
	case 13:  return KEY_ENTER;        // K_ENTER
	case 27:  return KEY_ESCAPE;       // K_ESCAPE (intercepted upstream, kept for completeness)
	case 32:  return KEY_SPACE;        // K_SPACE
	case 70:  return KEY_SCROLLLOCK;   // K_SCROLLLOCK
	case 127: return KEY_BACKSPACE;    // K_BACKSPACE
	case 128: return KEY_UP;           // K_UPARROW
	case 129: return KEY_DOWN;         // K_DOWNARROW
	case 130: return KEY_LEFT;         // K_LEFTARROW
	case 131: return KEY_RIGHT;        // K_RIGHTARROW
	case 132: return KEY_LALT;         // K_ALT
	case 133: return KEY_LCONTROL;     // K_CTRL
	case 134: return KEY_LSHIFT;       // K_SHIFT
	case 135: case 136: case 137: case 138: case 139: case 140:
	case 141: case 142: case 143: case 144: case 145: case 146:
		return (KeyCode)(KEY_F1 + (engineCode - 135));    // K_F1..K_F12
	case 147: return KEY_INSERT;       // K_INS
	case 148: return KEY_DELETE;       // K_DEL
	case 149: return KEY_PAGEDOWN;     // K_PGDN
	case 150: return KEY_PAGEUP;       // K_PGUP
	case 151: return KEY_HOME;         // K_HOME
	case 152: return KEY_END;          // K_END
	case 175: return KEY_CAPSLOCK;     // K_CAPSLOCK
	case 178: return KEY_NUMLOCK;      // K_KP_NUMLOCK

	// ASCII punctuation (TextEntry needs these for typing)
	case '`':  return KEY_BACKQUOTE;
	case '-':  return KEY_MINUS;
	case '=':  return KEY_EQUAL;
	case '[':  return KEY_LBRACKET;
	case ']':  return KEY_RBRACKET;
	case ';':  return KEY_SEMICOLON;
	case '\'': return KEY_APOSTROPHE;
	case ',':  return KEY_COMMA;
	case '.':  return KEY_PERIOD;
	case '/':  return KEY_SLASH;
	case '\\': return KEY_BACKSLASH;
	}
	return KEY_LAST;
}

void VGUI_Key(enum VGUI_KeyAction action, int code)
{
	if (!s_app || !s_rootPanel)
		return;

	if (g_pfnKeyGrabCallback && action == KA_PRESSED)
	{
		g_pfnKeyGrabCallback(code);
		g_pfnKeyGrabCallback = 0; // one-shot
		return;
	}

	vgui::SurfaceBase* sb = s_rootPanel->getSurfaceBase();
	vgui::KeyCode kc = EngineKeyToVgui(code);
	if (kc == vgui::KEY_LAST)
		return; // unmapped engine key (mouse, joystick, gamepad) -- ignore

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

void VGUI_MouseMove(int x, int y)
{
	if (!s_app || !s_rootPanel)
		return;

	vgui::SurfaceBase* sb = s_rootPanel->getSurfaceBase();
	s_app->internalCursorMoved(x, y, sb);
}

static void VGUI_TextInput(const char *text)
{
	// UTF-8 text from the engine's IME / soft keyboard. Translate ASCII bytes
	// to vgui::KeyCode via the shared engine->VGUI table so it routes through
	// TextEntry::internalKeyTyped exactly like a hardware keypress.
	if (!s_app || !text || !s_rootPanel)
		return;

	vgui::SurfaceBase* sb = s_rootPanel->getSurfaceBase();
	for (int i = 0; text[i]; i++)
	{
		unsigned char ch = (unsigned char)text[i];
		// Skip UTF-8 continuation bytes -- VGUI1 has no Cyrillic, just drop
		// multibyte chars for now. Latin/digits/punctuation pass through.
		if (ch >= 0x80)
			continue;
		// Deliver the LITERAL character. The old KeyCode round-trip
		// (getKeyCodeChar with shifted=false) downcased capitals and dropped
		// shifted symbols like ! @ # $; this preserves them.
		s_app->internalCharTyped((char)ch, sb);
	}
}

// ====================================================================
// Cvar bridge accessors
// ====================================================================

namespace vgui
{

void VGUI_SetKeyGrabCallback(void (*cb)(int engineKeyCode))
{
	g_pfnKeyGrabCallback = cb;
}

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
	VLOG("VGUI_EnsureInitialized: %dx%d (rootPanel=%p, app=%p)", screenW, screenH, (void*)s_rootPanel, (void*)s_app);
	if (s_rootPanel)
	{
		VLOG("EnsureInit: already initialized -- skip");
		return;
	}

	if (screenW <= 0) screenW = 640;
	if (screenH <= 0) screenH = 480;

	s_screenWidth = screenW;
	s_screenHeight = screenH;
	VGUI_ComputeScale(screenH);
	VLOG("EnsureInit: computed scale=%.2f", vgui::g_vguiScale);

	if (!s_app)
	{
		s_app = new vgui::App(true);
		s_scheme = new vgui::Scheme();
		VLOG("EnsureInit: app=%p scheme=%p created", (void*)s_app, (void*)s_scheme);

		// Font sizes scale with screen height so HD devices get readable text
		int fontMed   = (int)(12.0f * vgui::g_vguiScale + 0.5f);
		int fontTitle = (int)(14.0f * vgui::g_vguiScale + 0.5f);
		int fontSmall = (int)(10.0f * vgui::g_vguiScale + 0.5f);

		vgui::Font* fontPrimary1 = new vgui::Font("Tahoma", fontMed,   0, 0, 400, false, false, false, false);
		vgui::Font* fontPrimary2 = new vgui::Font("Tahoma", fontTitle, 0, 0, 700, false, false, false, false);
		vgui::Font* fontPrimary3 = new vgui::Font("Tahoma", fontSmall, 0, 0, 400, false, false, false, false);

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
		VLOG("EnsureInit: scheme attached");
	}

	s_rootPanel = new vgui::Panel(0, 0, screenW, screenH);
	VLOG("EnsureInit: root panel created %p (%dx%d)", (void*)s_rootPanel, screenW, screenH);
	vgui::EngineSurface_Create(s_rootPanel);
	VLOG("EnsureInit: engine surface created");
}

// ====================================================================
// Export: called by mainui to paint VGUI1 panels each frame
// ====================================================================
extern "C"
{

#ifndef EXPORT
#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif
#endif

EXPORT int VGUI_IsVisible(void)
{
	if (!s_rootPanel)
		return 0;

	int count = s_rootPanel->getChildCount();
	for (int i = 0; i < count; i++)
	{
		vgui::Panel* child = s_rootPanel->getChild(i);
		if (child && child->isVisible())
			return 1;
	}
	return 0;
}

EXPORT void VGUI_PaintAll(void)
{
	if (!s_rootPanel || !s_app)
		return;

	s_app->externalTick();
	s_rootPanel->solveTraverse();
	s_rootPanel->paintTraverse();
}

EXPORT void VGUI_ForwardMouse(int action, int code)
{
	VGUI_Mouse((enum VGUI_MouseAction)action, code);
}

EXPORT void VGUI_ForwardKey(int action, int code)
{
	VGUI_Key((enum VGUI_KeyAction)action, code);
}

EXPORT void VGUI_ForwardMouseMove(int x, int y)
{
	VGUI_MouseMove(x, y);
}

// Engine -> VGUI text input bridge. Called from UI_CharEvent for every typed
// character (desktop hardware keyboard) and from the engine's IME callback
// on touch platforms. ASCII printable chars get inserted into the focused
// TextEntry; UTF-8 multibyte bytes are dropped (no Cyrillic input yet).
EXPORT void VGUI_ForwardCharInput(const char *text)
{
	VGUI_TextInput(text);
}

// ====================================================================
// Console bridge: exposed to engine / mainui
// ====================================================================
EXPORT void VGUI_ConsoleOutput(const char* text)
{
	vgui::VGUI_Console_Output(text);
}

EXPORT void VGUI_ShowConsole(bool show)
{
	vgui::VGUI_Console_Show(show);
}

EXPORT bool VGUI_IsConsoleVisible(void)
{
	return vgui::VGUI_Console_IsVisible();
}

// ====================================================================
// Create Server dialog bridge: exposed to engine / mainui
// ====================================================================
EXPORT void VGUI_ShowCreateGame(bool show)
{
	vgui::VGUI_CreateGame_Show(show);
}

EXPORT bool VGUI_IsCreateGameVisible(void)
{
	return vgui::VGUI_CreateGame_IsVisible();
}

// ====================================================================
// Loading dialog bridge: exposed to engine / mainui
// ====================================================================
EXPORT void VGUI_ShowLoading(bool show, const char* statusText, float progress)
{
	vgui::VGUI_Loading_Show(show, statusText, progress);
}

EXPORT bool VGUI_IsLoadingVisible(void)
{
	return vgui::VGUI_Loading_IsVisible();
}

} // extern "C"

// ====================================================================
// Export: called by engine to initialize vgui_support
// ====================================================================
extern "C"
{

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
