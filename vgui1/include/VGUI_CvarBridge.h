#ifndef VGUI_CVARBRIDGE_H
#define VGUI_CVARBRIDGE_H

namespace vgui
{

class Panel;

float VGUI_GetCvarFloat(const char* name);
void VGUI_SetCvarFloat(const char* name, float value);
const char* VGUI_GetCvarString(const char* name);
void VGUI_SetCvarString(const char* name, const char* value);
void VGUI_ClientCmd(const char* cmd);
void VGUI_GetScreenSize(int* w, int* h);
void VGUI_SetKeyGrabCallback(void (*cb)(int engineKeyCode));
Panel* VGUI_GetRootPanel();

}

#endif
