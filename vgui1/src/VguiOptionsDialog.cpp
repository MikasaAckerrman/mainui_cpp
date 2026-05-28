// VguiOptionsDialog.cpp - Options dialog with tabbed cvar controls

#include <VGUI_OptionsDialog.h>
#include <VGUI_TabPanel.h>
#include <VGUI_Panel.h>
#include <VGUI_Label.h>
#include <VGUI_Button.h>
#include <VGUI_ActionSignal.h>
#include <VGUI_CvarBridge.h>
#include <VGUI_CvarCheckButton.h>
#include <VGUI_CvarSlider.h>
#include <VGUI_CvarTextEntry.h>
#include <string.h>

namespace vgui
{

// ====================================================================
// Action signal for OK button (apply all and hide)
// ====================================================================
class OptionsOKSignal : public ActionSignal
{
public:
	OptionsOKSignal(VguiOptionsDialog* dlg) : _dlg(dlg) {}
	virtual void actionPerformed(Panel* panel)
	{
		_dlg->applyAll();
		_dlg->setVisible(false);
	}
private:
	VguiOptionsDialog* _dlg;
};

// ====================================================================
// Action signal for Cancel button (hide without applying)
// ====================================================================
class OptionsCancelSignal : public ActionSignal
{
public:
	OptionsCancelSignal(VguiOptionsDialog* dlg) : _dlg(dlg) {}
	virtual void actionPerformed(Panel* panel)
	{
		_dlg->setVisible(false);
	}
private:
	VguiOptionsDialog* _dlg;
};

// ====================================================================
// Action signal for Apply button (apply all but keep dialog open)
// ====================================================================
class OptionsApplySignal : public ActionSignal
{
public:
	OptionsApplySignal(VguiOptionsDialog* dlg) : _dlg(dlg) {}
	virtual void actionPerformed(Panel* panel)
	{
		_dlg->applyAll();
	}
private:
	VguiOptionsDialog* _dlg;
};

// ====================================================================
// VguiOptionsDialog
// ====================================================================

VguiOptionsDialog::VguiOptionsDialog(int screenW, int screenH)
	: Frame(0, 0, 480, 380)
{
	// Compute dialog size proportionally to screen
	int dialogW = (screenW * 4) / 5;    // 80%
	int dialogH = (screenH * 80) / 100; // 80%

	// Cap to reasonable limits (CS 1.6 PC reference is ~1100x650)
	if (dialogW > 1100) dialogW = 1100;
	if (dialogH > 650) dialogH = 650;
	if (dialogW < 500) dialogW = 500;
	if (dialogH < 320) dialogH = 320;

	// Center on screen
	int posX = (screenW - dialogW) / 2;
	int posY = (screenH - dialogH) / 2;

	setPos(posX, posY);
	setSize(dialogW, dialogH);  // Frame::setSize relayouts client/caption/closeBtn
	setTitle("Options");
	setVisible(false);

	_tabPanel = null;

	Panel* client = getClient();
	if (!client)
		return;

	int clientW, clientH;
	client->getSize(clientW, clientH);

	// Reserve space for buttons at bottom (36 px)
	int tabH = clientH - 36;
	if (tabH < 100) tabH = 100;

	// Create tab panel filling client area (use addChild, not setParent)
	_tabPanel = new TabPanel(0, 0, clientW, tabH);
	client->addChild(_tabPanel);

	// Create pages for each tab
	Panel* mpPage = new Panel(0, 0, clientW, tabH - 28);
	Panel* kbPage = new Panel(0, 0, clientW, tabH - 28);
	Panel* mousePage = new Panel(0, 0, clientW, tabH - 28);
	Panel* audioPage = new Panel(0, 0, clientW, tabH - 28);
	Panel* videoPage = new Panel(0, 0, clientW, tabH - 28);
	Panel* hudPage = new Panel(0, 0, clientW, tabH - 28);
	Panel* accountPage = new Panel(0, 0, clientW, tabH - 28);
	Panel* systemPage = new Panel(0, 0, clientW, tabH - 28);

	_tabPanel->addTab("Multiplayer", mpPage);
	_tabPanel->addTab("Keyboard", kbPage);
	_tabPanel->addTab("Mouse", mousePage);
	_tabPanel->addTab("Audio", audioPage);
	_tabPanel->addTab("Video", videoPage);
	_tabPanel->addTab("HUD", hudPage);
	_tabPanel->addTab("Account", accountPage);
	_tabPanel->addTab("System", systemPage);

	// Populate tabs
	buildMultiplayerTab(mpPage);
	buildKeyboardTab(kbPage);
	buildMouseTab(mousePage);
	buildAudioTab(audioPage);
	buildVideoTab(videoPage);
	buildHudTab(hudPage);
	buildAccountTab(accountPage);
	buildSystemTab(systemPage);

	// Bottom button row: OK | Cancel | Apply, anchored to bottom-right of client area
	int btnW = 80;
	int btnH = 22;
	int btnGap = 6;
	int btnY = clientH - btnH - 8;
	int btnRight = clientW - 8;

	// Apply (right-most), Cancel, OK (left-most of the trio)
	int applyX  = btnRight - btnW;
	int cancelX = applyX  - btnGap - btnW;
	int okX     = cancelX - btnGap - btnW;

	Button* okBtn = new Button("OK", okX, btnY, btnW, btnH);
	client->addChild(okBtn);
	okBtn->addActionSignal(new OptionsOKSignal(this));

	Button* cancelBtn = new Button("Cancel", cancelX, btnY, btnW, btnH);
	client->addChild(cancelBtn);
	cancelBtn->addActionSignal(new OptionsCancelSignal(this));

	Button* applyBtn = new Button("Apply", applyX, btnY, btnW, btnH);
	client->addChild(applyBtn);
	applyBtn->addActionSignal(new OptionsApplySignal(this));
}

void VguiOptionsDialog::applyAll()
{
	int i;
	for (i = 0; i < _checkButtons.getCount(); i++)
		_checkButtons[i]->apply();
	for (i = 0; i < _sliders.getCount(); i++)
		_sliders[i]->apply();
	for (i = 0; i < _textEntries.getCount(); i++)
		_textEntries[i]->apply();
}

void VguiOptionsDialog::resetAll()
{
	int i;
	for (i = 0; i < _checkButtons.getCount(); i++)
		_checkButtons[i]->reset();
	for (i = 0; i < _sliders.getCount(); i++)
		_sliders[i]->reset();
	for (i = 0; i < _textEntries.getCount(); i++)
		_textEntries[i]->reset();
}

// ====================================================================
// Tab builders
// ====================================================================

void VguiOptionsDialog::buildMultiplayerTab(Panel* page)
{
	int y = 8;

	Label* nameLabel = new Label("Player name:", 8, y, 100, 20);
	page->addChild(nameLabel);

	CvarTextEntry* nameEntry = new CvarTextEntry("name", 112, y, 180, 20);
	page->addChild(nameEntry);
	_textEntries.addElement(nameEntry);
	y += 30;

	Label* topLabel = new Label("Top color:", 8, y, 100, 20);
	page->addChild(topLabel);

	CvarSlider* topSlider = new CvarSlider("topcolor", 112, y, 180, 20, 0, 255);
	page->addChild(topSlider);
	_sliders.addElement(topSlider);
	y += 30;

	Label* botLabel = new Label("Bottom color:", 8, y, 100, 20);
	page->addChild(botLabel);

	CvarSlider* botSlider = new CvarSlider("bottomcolor", 112, y, 180, 20, 0, 255);
	page->addChild(botSlider);
	_sliders.addElement(botSlider);
}

void VguiOptionsDialog::buildKeyboardTab(Panel* page)
{
	Label* lbl = new Label("Key bindings", 8, 8, 200, 20);
	page->addChild(lbl);
}

void VguiOptionsDialog::buildMouseTab(Panel* page)
{
	int y = 8;

	CvarCheckButton* filter = new CvarCheckButton("m_filter", "Mouse filter", 8, y, 200, 20);
	page->addChild(filter);
	_checkButtons.addElement(filter);
	y += 28;

	Label* sensLabel = new Label("Sensitivity:", 8, y, 100, 20);
	page->addChild(sensLabel);

	CvarSlider* sensSlider = new CvarSlider("sensitivity", 112, y, 180, 20, 1, 20);
	page->addChild(sensSlider);
	_sliders.addElement(sensSlider);
	y += 30;

	CvarCheckButton* rawinput = new CvarCheckButton("m_rawinput", "Raw input", 8, y, 200, 20);
	page->addChild(rawinput);
	_checkButtons.addElement(rawinput);
	y += 28;

	CvarCheckButton* customaccel = new CvarCheckButton("m_customaccel", "Custom acceleration", 8, y, 200, 20);
	page->addChild(customaccel);
	_checkButtons.addElement(customaccel);
}

void VguiOptionsDialog::buildAudioTab(Panel* page)
{
	int y = 8;

	Label* volLabel = new Label("Volume:", 8, y, 100, 20);
	page->addChild(volLabel);

	CvarSlider* volSlider = new CvarSlider("volume", 112, y, 180, 20, 0, 100, 0.0f, 1.0f);
	page->addChild(volSlider);
	_sliders.addElement(volSlider);
	y += 30;

	Label* suitLabel = new Label("Suit volume:", 8, y, 100, 20);
	page->addChild(suitLabel);

	CvarSlider* suitSlider = new CvarSlider("suitvolume", 112, y, 180, 20, 0, 100, 0.0f, 1.0f);
	page->addChild(suitSlider);
	_sliders.addElement(suitSlider);
	y += 30;

	CvarCheckButton* a3d = new CvarCheckButton("s_a3d", "A3D Audio", 8, y, 200, 20);
	page->addChild(a3d);
	_checkButtons.addElement(a3d);
	y += 28;

	CvarCheckButton* eax = new CvarCheckButton("s_eax", "EAX effects", 8, y, 200, 20);
	page->addChild(eax);
	_checkButtons.addElement(eax);
}

void VguiOptionsDialog::buildVideoTab(Panel* page)
{
	int y = 8;

	Label* gammaLabel = new Label("Gamma:", 8, y, 100, 20);
	page->addChild(gammaLabel);

	CvarSlider* gammaSlider = new CvarSlider("gamma", 112, y, 180, 20, 0, 100, 1.8f, 3.0f);
	page->addChild(gammaSlider);
	_sliders.addElement(gammaSlider);
	y += 30;

	Label* brightLabel = new Label("Brightness:", 8, y, 100, 20);
	page->addChild(brightLabel);

	CvarSlider* brightSlider = new CvarSlider("brightness", 112, y, 180, 20, 0, 100, 0.0f, 2.0f);
	page->addChild(brightSlider);
	_sliders.addElement(brightSlider);
	y += 30;

	CvarCheckButton* vsync = new CvarCheckButton("gl_vsync", "VSync", 8, y, 200, 20);
	page->addChild(vsync);
	_checkButtons.addElement(vsync);
}

void VguiOptionsDialog::buildHudTab(Panel* page)
{
	int y = 8;

	CvarCheckButton* hudDraw = new CvarCheckButton("hud_draw", "Draw HUD", 8, y, 200, 20);
	page->addChild(hudDraw);
	_checkButtons.addElement(hudDraw);
	y += 28;

	CvarCheckButton* showFps = new CvarCheckButton("cl_showfps", "Show FPS", 8, y, 200, 20);
	page->addChild(showFps);
	_checkButtons.addElement(showFps);
	y += 28;

	Label* scaleLabel = new Label("HUD scale:", 8, y, 100, 20);
	page->addChild(scaleLabel);

	CvarSlider* scaleSlider = new CvarSlider("hud_scale", 112, y, 180, 20, 0, 10, 0.0f, 2.0f);
	page->addChild(scaleSlider);
	_sliders.addElement(scaleSlider);
	y += 30;

	CvarCheckButton* crosshair = new CvarCheckButton("crosshair", "Show crosshair", 8, y, 200, 20);
	page->addChild(crosshair);
	_checkButtons.addElement(crosshair);
}

void VguiOptionsDialog::buildAccountTab(Panel* page)
{
	Label* lbl = new Label("Account settings", 8, 8, 200, 20);
	page->addChild(lbl);
}

void VguiOptionsDialog::buildSystemTab(Panel* page)
{
	int y = 8;

	CvarCheckButton* dev = new CvarCheckButton("developer", "Developer mode", 8, y, 200, 20);
	page->addChild(dev);
	_checkButtons.addElement(dev);
}

// ====================================================================
// Global dialog instance and exported functions
// ====================================================================

static VguiOptionsDialog* g_pOptionsDialog = null;

} // namespace vgui

// Called from VGUI_Shutdown to prevent dangling pointer after panel tree deletion
void VGUI_OptionsShutdown(void)
{
	vgui::g_pOptionsDialog = null;
}

extern "C"
{

#ifdef _WIN32
#define OPTDLG_EXPORT __declspec(dllexport)
#else
#define OPTDLG_EXPORT __attribute__((visibility("default")))
#endif

// Forward: ensure VGUI1 core is initialized (defined in vgui_main.cpp)
extern "C" void VGUI_EnsureInitialized(int screenW, int screenH);

OPTDLG_EXPORT void VGUI_ShowOptions(void)
{
	// Get screen size from caller or use defaults
	int sw = 0, sh = 0;
	vgui::VGUI_GetScreenSize(&sw, &sh);

	VGUI_EnsureInitialized(sw, sh);

	vgui::Panel* root = vgui::VGUI_GetRootPanel();
	if (!root)
		return;

	if (!vgui::g_pOptionsDialog)
	{
		if (sw <= 0) sw = 640;
		if (sh <= 0) sh = 480;
		vgui::g_pOptionsDialog = new vgui::VguiOptionsDialog(sw, sh);
		root->addChild(vgui::g_pOptionsDialog);
	}

	vgui::g_pOptionsDialog->resetAll();
	vgui::g_pOptionsDialog->setVisible(true);
}

OPTDLG_EXPORT void VGUI_HideOptions(void)
{
	if (vgui::g_pOptionsDialog)
		vgui::g_pOptionsDialog->setVisible(false);
}

} // extern "C"
