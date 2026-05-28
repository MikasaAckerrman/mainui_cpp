#ifndef VGUI_H
#define VGUI_H

#include <stddef.h>

namespace vgui
{
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

#define null 0

enum KeyCode
{
	KEY_0 = 0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
	KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J,
	KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,
	KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
	KEY_PAD_0, KEY_PAD_1, KEY_PAD_2, KEY_PAD_3, KEY_PAD_4,
	KEY_PAD_5, KEY_PAD_6, KEY_PAD_7, KEY_PAD_8, KEY_PAD_9,
	KEY_PAD_DIVIDE, KEY_PAD_MULTIPLY, KEY_PAD_MINUS, KEY_PAD_PLUS,
	KEY_PAD_ENTER, KEY_PAD_DECIMAL,
	KEY_LBRACKET, KEY_RBRACKET, KEY_SEMICOLON, KEY_APOSTROPHE,
	KEY_BACKQUOTE, KEY_COMMA, KEY_PERIOD, KEY_SLASH, KEY_BACKSLASH,
	KEY_MINUS, KEY_EQUAL, KEY_ENTER, KEY_SPACE, KEY_BACKSPACE, KEY_TAB,
	KEY_CAPSLOCK, KEY_NUMLOCK, KEY_ESCAPE, KEY_SCROLLLOCK,
	KEY_INSERT, KEY_DELETE, KEY_HOME, KEY_END, KEY_PAGEUP, KEY_PAGEDOWN,
	KEY_BREAK, KEY_LSHIFT, KEY_RSHIFT, KEY_LALT, KEY_RALT,
	KEY_LCONTROL, KEY_RCONTROL, KEY_LWIN, KEY_RWIN, KEY_APP,
	KEY_UP, KEY_LEFT, KEY_DOWN, KEY_RIGHT,
	KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
	KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
	KEY_LAST
};

enum MouseCode
{
	MOUSE_LEFT = 0,
	MOUSE_RIGHT,
	MOUSE_MIDDLE,
	MOUSE_LAST
};

// Utility functions
void vgui_setMalloc(void* (*mallocFunc)(size_t size));
void vgui_setFree(void (*freeFunc)(void* memblock));
void vgui_strcpy(char* dst, int dstLen, const char* src);
char* vgui_strdup(const char* src);
int vgui_printf(const char* format, ...);
int vgui_dprintf(const char* format, ...);
int vgui_dprintf2(const char* format, ...);
void vgui_internal_free(void* ptr);

}

#endif
