#ifndef VGUI_OPTIONSDIALOG_H
#define VGUI_OPTIONSDIALOG_H

#include <VGUI.h>
#include <VGUI_Frame.h>
#include <VGUI_Dar.h>

namespace vgui
{

class VguiOptionsDialog;
class TabPanel;
class Button;
class CvarCheckButton;
class CvarSlider;
class CvarTextEntry;

class VguiOptionsDialog : public Frame
{
public:
	VguiOptionsDialog(int screenW, int screenH);
public:
	void applyAll();
	void resetAll();
	void setDirty(bool dirty);
	virtual void setSize(int wide, int tall);   // override - reposition buttons & tabs
	virtual void setVisible(bool state);        // override - cleanup text input on hide
private:
	void buildMultiplayerTab(Panel* page);
	void buildKeyboardTab(Panel* page);
	void buildMouseTab(Panel* page);
	void buildAudioTab(Panel* page);
	void buildVideoTab(Panel* page);
	void buildHudTab(Panel* page);
	void buildAccountTab(Panel* page);
	void buildSystemTab(Panel* page);
private:
	TabPanel* _tabPanel;
	Button*   _applyBtn;
	Button*   _okBtn;
	Button*   _cancelBtn;
	bool      _dirty;
	Dar<CvarCheckButton*> _checkButtons;
	Dar<CvarSlider*> _sliders;
	Dar<CvarTextEntry*> _textEntries;
};

}

#ifdef __cplusplus
extern "C" {
#endif

void VGUI_ShowOptions(void);
void VGUI_HideOptions(void);

#ifdef __cplusplus
}
#endif

#endif
