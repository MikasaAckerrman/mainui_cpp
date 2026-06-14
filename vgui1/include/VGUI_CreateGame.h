#ifndef VGUI_CREATEGAME_H
#define VGUI_CREATEGAME_H

#include <VGUI.h>
#include <VGUI_Frame.h>

namespace vgui
{

class VguiCreateGame;
class Panel;
class Label;
class Button;
class TabPanel;
class SimpleCombo;
class SimpleCheck;
class SimpleRadioGroup;
class VScrollPanel;

// CS 1.6 PC-accurate "Создать сервер" (Create Server) dialog. A fixed-size
// (non-resizable) Frame with a Steam icon + title + close (X) drawn by Frame,
// three tabs ("Сервер" | "Игра" | "Настройки ботов"), and a bottom button row
// ("Запуск" + "Отмена"). All coordinates use VS() scaling from the 640x480
// reference grid, matching VguiOptionsDialog / VguiConsole conventions.
class VguiCreateGame : public Frame
{
public:
	VguiCreateGame(int screenW, int screenH);
public:
	virtual void setVisible(bool state);   // override - cleanup text input on hide
	void launchGame();                      // "Запуск": map <selected> + hide
private:
	void buildServerTab(Panel* page);       // Tab 1 - "Сервер"
	void buildGameTab(Panel* page);         // Tab 2 - "Игра"
	void buildBotsTab(Panel* page);         // Tab 3 - "Настройки ботов"
private:
	TabPanel*         _tabPanel;
	Button*           _startBtn;
	Button*           _cancelBtn;
	SimpleCombo*      _mapCombo;
	SimpleCombo*      _filterCombo;
	SimpleCombo*      _locationCombo;
	SimpleRadioGroup* _difficulty;
};

// Lifecycle helpers (implemented in VguiCreateGame.cpp). Called by the
// exported C functions in vgui_main.cpp.
void VGUI_CreateGame_Show(bool show);   // create (lazy) + show/hide the dialog
bool VGUI_CreateGame_IsVisible();       // query visibility

}

#ifdef __cplusplus
extern "C" {
#endif

// Exposed to engine / mainui:
void VGUI_ShowCreateGame(bool show);    // show/hide the Create Server dialog
bool VGUI_IsCreateGameVisible(void);    // query visibility

#ifdef __cplusplus
}
#endif

#endif // VGUI_CREATEGAME_H
