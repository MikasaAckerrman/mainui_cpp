// VguiOptionsDialog.cpp - Options dialog with tabbed cvar controls
//
// All pixel coordinates are passed through VS() so the dialog scales with
// screen size (mainui's logical 768-unit reference => physical pixels).

#include <VGUI_Log.h>
#include <VGUI_OptionsDialog.h>
#include <VGUI_TabPanel.h>
#include <VGUI_Panel.h>
#include <VGUI_Label.h>
#include <VGUI_Button.h>
#include <VGUI_ActionSignal.h>
#include <VGUI_IntChangeSignal.h>
#include <VGUI_CvarBridge.h>
#include <VGUI_CvarCheckButton.h>
#include <VGUI_CvarSlider.h>
#include <VGUI_CvarTextEntry.h>
#include <VGUI_App.h>
#include <VGUI_UIScale.h>
#include <string.h>

// mainui bridge - declared at global scope to avoid namespace mangling and
// to skip pulling Utils.h (which would cause the `null` macro clash).
extern void UI_EnableTextInput( bool enable );

namespace vgui
{

// ====================================================================
// Action signals
// ====================================================================
class OptionsOKSignal : public ActionSignal
{
public:
	OptionsOKSignal(VguiOptionsDialog* dlg) : _dlg(dlg) {}
	virtual void actionPerformed(Panel* panel) { _dlg->applyAll(); _dlg->setVisible(false); }
private:
	VguiOptionsDialog* _dlg;
};

class OptionsCancelSignal : public ActionSignal
{
public:
	OptionsCancelSignal(VguiOptionsDialog* dlg) : _dlg(dlg) {}
	virtual void actionPerformed(Panel* panel) { _dlg->setVisible(false); }
private:
	VguiOptionsDialog* _dlg;
};

class OptionsApplySignal : public ActionSignal
{
public:
	OptionsApplySignal(VguiOptionsDialog* dlg) : _dlg(dlg) {}
	virtual void actionPerformed(Panel* panel) { _dlg->applyAll(); }
private:
	VguiOptionsDialog* _dlg;
};

// Listener that flips the dialog's dirty flag whenever a cvar widget changes.
class MarkDirtyActionSignal : public ActionSignal
{
public:
	MarkDirtyActionSignal(VguiOptionsDialog* dlg) : _dlg(dlg) {}
	virtual void actionPerformed(Panel* panel) { _dlg->setDirty(true); }
private:
	VguiOptionsDialog* _dlg;
};

class MarkDirtyIntSignal : public IntChangeSignal
{
public:
	MarkDirtyIntSignal(VguiOptionsDialog* dlg) : _dlg(dlg) {}
	virtual void intChanged(int value, Panel* panel) { _dlg->setDirty(true); }
private:
	VguiOptionsDialog* _dlg;
};

// ====================================================================
// VguiOptionsDialog
// ====================================================================

VguiOptionsDialog::VguiOptionsDialog(int screenW, int screenH)
	: Frame(0, 0, VS(480), VS(380))
{
	VLOG("VguiOptionsDialog ctor: screenW=%d screenH=%d scale=%.2f", screenW, screenH, vgui::g_vguiScale);
	// CRITICAL: zero ALL pointers BEFORE the virtual setSize() below, since
	// VguiOptionsDialog::setSize reads _tabPanel/_okBtn/_cancelBtn/_applyBtn
	// to relayout, and the most-derived dispatch happens during ctor body.
	_tabPanel = null;
	_applyBtn = null;
	_okBtn = null;
	_cancelBtn = null;
	_dirty = false;

	// Dialog uses CS 1.6 PC fixed proportions, scaled by VS() so it stays
	// readable on HD/4K screens. NOT proportional to screen size.
	int dialogW = VS(640);
	int dialogH = VS(440);
	if (dialogW > screenW - VS(8)) dialogW = screenW - VS(8);
	if (dialogH > screenH - VS(8)) dialogH = screenH - VS(8);

	setPos((screenW - dialogW) / 2, (screenH - dialogH) / 2);
	setSize(dialogW, dialogH);
	setTitle("\xD0\x9D\xD0\xB0\xD1\x81\xD1\x82\xD1\x80\xD0\xBE\xD0\xB9\xD0\xBA\xD0\xB8"); // "Настройки"
	setVisible(false);

	Panel* client = getClient();
	if (!client)
	{
		VLOG("ctor: getClient() returned null -- abort");
		return;
	}
	VLOG("ctor: dialog %dx%d, client ready", dialogW, dialogH);

	int clientW, clientH;
	client->getSize(clientW, clientH);

	// Reserve space for the OK/Cancel/Apply row at the bottom
	int btnH      = VS(22);
	int btnRowH   = btnH + VS(16);
	int tabH      = clientH - btnRowH;
	int minTabH   = VS(120);
	if (tabH < minTabH) tabH = minTabH;

	_tabPanel = new TabPanel(0, 0, clientW, tabH);
	client->addChild(_tabPanel);

	// Pages live below the 24-unit (scaled) tab strip
	int pageH = tabH - VS(28);
	Panel* mpPage      = new Panel(0, 0, clientW, pageH);
	Panel* kbPage      = new Panel(0, 0, clientW, pageH);
	Panel* mousePage   = new Panel(0, 0, clientW, pageH);
	Panel* audioPage   = new Panel(0, 0, clientW, pageH);
	Panel* videoPage   = new Panel(0, 0, clientW, pageH);
	Panel* hudPage     = new Panel(0, 0, clientW, pageH);
	Panel* accountPage = new Panel(0, 0, clientW, pageH);
	Panel* systemPage  = new Panel(0, 0, clientW, pageH);

	_tabPanel->addTab("\xD0\x9C\xD1\x83\xD0\xBB\xD1\x8C\xD1\x82\xD0\xB8\xD0\xBF\xD0\xBB\xD0\xB5\xD0\xB5\xD1\x80",   mpPage);      // Мультиплеер
	_tabPanel->addTab("\xD0\x9A\xD0\xBB\xD0\xB0\xD0\xB2\xD0\xB8\xD0\xB0\xD1\x82\xD1\x83\xD1\x80\xD0\xB0",          kbPage);      // Клавиатура
	_tabPanel->addTab("\xD0\x9C\xD1\x8B\xD1\x88\xD1\x8C",                                                          mousePage);   // Мышь
	_tabPanel->addTab("\xD0\x97\xD0\xB2\xD1\x83\xD0\xBA",                                                          audioPage);   // Звук
	_tabPanel->addTab("\xD0\x92\xD0\xB8\xD0\xB4\xD0\xB5\xD0\xBE",                                                  videoPage);   // Видео
	_tabPanel->addTab("HUD",                                                                                       hudPage);
	_tabPanel->addTab("\xD0\x90\xD0\xBA\xD0\xBA\xD0\xB0\xD1\x83\xD0\xBD\xD1\x82",                                  accountPage); // Аккаунт
	_tabPanel->addTab("\xD0\xA1\xD0\xB8\xD1\x81\xD1\x82\xD0\xB5\xD0\xBC\xD0\xB0",                                  systemPage);  // Система

	VLOG("ctor: building tabs");
	buildMultiplayerTab(mpPage);  VLOG("ctor: mp tab built");
	buildKeyboardTab(kbPage);
	buildMouseTab(mousePage);
	buildAudioTab(audioPage);
	buildVideoTab(videoPage);
	buildHudTab(hudPage);
	buildAccountTab(accountPage);
	buildSystemTab(systemPage);
	VLOG("ctor: all tabs built. checks=%d sliders=%d entries=%d",
		_checkButtons.getCount(), _sliders.getCount(), _textEntries.getCount());

	// Bottom button row: OK | Cancel | Apply, anchored to bottom-right
	int btnW    = VS(80);
	int btnGap  = VS(6);
	int btnY    = clientH - btnH - VS(8);
	int applyX  = clientW - VS(8) - btnW;
	int cancelX = applyX  - btnGap - btnW;
	int okX     = cancelX - btnGap - btnW;

	Button* okBtn = new Button("OK", okX, btnY, btnW, btnH);
	client->addChild(okBtn);
	okBtn->addActionSignal(new OptionsOKSignal(this));
	_okBtn = okBtn;

	Button* cancelBtn = new Button("\xD0\x9E\xD1\x82\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xB0", cancelX, btnY, btnW, btnH); // Отмена
	client->addChild(cancelBtn);
	cancelBtn->addActionSignal(new OptionsCancelSignal(this));
	_cancelBtn = cancelBtn;

	_applyBtn = new Button("\xD0\x9F\xD1\x80\xD0\xB8\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xB8\xD1\x82\xD1\x8C", applyX, btnY, btnW, btnH); // Применить
	client->addChild(_applyBtn);
	_applyBtn->addActionSignal(new OptionsApplySignal(this));
	_applyBtn->setEnabled(false); // becomes enabled when something changes
	VLOG("ctor: buttons created");

	// Wire dirty-tracking signals on every cvar widget so Apply lights up.
	// Guard each entry: a corrupted Dar slot must not deref-crash here.
	for (int i = 0; i < _checkButtons.getCount(); i++)
		if (_checkButtons[i]) _checkButtons[i]->addActionSignal(new MarkDirtyActionSignal(this));
	for (int i = 0; i < _sliders.getCount(); i++)
		if (_sliders[i]) _sliders[i]->addIntChangeSignal(new MarkDirtyIntSignal(this));
	for (int i = 0; i < _textEntries.getCount(); i++)
		if (_textEntries[i]) _textEntries[i]->addActionSignal(new MarkDirtyActionSignal(this));
	VLOG("ctor: dirty signals wired -- ctor done");
}

void VguiOptionsDialog::setDirty(bool dirty)
{
	_dirty = dirty;
	if (_applyBtn)
		_applyBtn->setEnabled(dirty);
}

// Re-layout TabPanel and bottom buttons to match the new size.
void VguiOptionsDialog::setSize(int wide, int tall)
{
	Frame::setSize(wide, tall);

	Panel* client = getClient();
	if (!client)
		return;

	int clientW, clientH;
	client->getSize(clientW, clientH);

	int btnH    = VS(22);
	int btnRowH = btnH + VS(16);
	int tabH    = clientH - btnRowH;
	if (tabH < VS(120)) tabH = VS(120);

	if (_tabPanel)
		_tabPanel->setBounds(0, 0, clientW, tabH);

	int btnW   = VS(80);
	int btnGap = VS(6);
	int btnY   = clientH - btnH - VS(8);
	int applyX  = clientW - VS(8) - btnW;
	int cancelX = applyX  - btnGap - btnW;
	int okX     = cancelX - btnGap - btnW;

	if (_okBtn)     _okBtn->setBounds(okX,     btnY, btnW, btnH);
	if (_cancelBtn) _cancelBtn->setBounds(cancelX, btnY, btnW, btnH);
	if (_applyBtn)  _applyBtn->setBounds(applyX,  btnY, btnW, btnH);
}

// Override: when the dialog is hidden, drop keyboard focus from any inner
// TextEntry and tell the engine to hide the soft keyboard. Otherwise the
// blinking cursor and Android IME stay active after closing the menu.
void VguiOptionsDialog::setVisible(bool state)
{
	if (!state)
	{
		App* app = App::getInstance();
		if (app)
			app->requestFocus(null); // dispatches internalFocusChanged(true) on prev focus
		UI_EnableTextInput(false);
	}
	Frame::setVisible(state);
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
	setDirty(false);
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
	setDirty(false);
}

// ====================================================================
// Tab builders -- coordinates scaled via VS()
// ====================================================================

// Common form metrics
static inline int LblX()    { return VS(12); }
static inline int LblW()    { return VS(110); }
static inline int InpX()    { return VS(130); }
static inline int InpW()    { return VS(220); }
static inline int RowH()    { return VS(28); }
static inline int RowGap()  { return VS(6); }
static inline int FldH()    { return VS(20); }
static inline int FirstY()  { return VS(14); }

void VguiOptionsDialog::buildMultiplayerTab(Panel* page)
{
	int y = FirstY();

	// "Имя игрока:"
	page->addChild(new Label("\xD0\x98\xD0\xBC\xD1\x8F \xD0\xB8\xD0\xB3\xD1\x80\xD0\xBE\xD0\xBA\xD0\xB0:", LblX(), y, LblW(), FldH()));
	CvarTextEntry* nameEntry = new CvarTextEntry("name", InpX(), y, InpW(), FldH());
	page->addChild(nameEntry);
	_textEntries.addElement(nameEntry);
	y += RowH() + RowGap();

	// "Верхний цвет:"
	page->addChild(new Label("\xD0\x92\xD0\xB5\xD1\x80\xD1\x85\xD0\xBD\xD0\xB8\xD0\xB9 \xD1\x86\xD0\xB2\xD0\xB5\xD1\x82:", LblX(), y, LblW(), FldH()));
	CvarSlider* topSlider = new CvarSlider("topcolor", InpX(), y, InpW(), FldH(), 0, 255);
	page->addChild(topSlider);
	_sliders.addElement(topSlider);
	y += RowH() + RowGap();

	// "Нижний цвет:"
	page->addChild(new Label("\xD0\x9D\xD0\xB8\xD0\xB6\xD0\xBD\xD0\xB8\xD0\xB9 \xD1\x86\xD0\xB2\xD0\xB5\xD1\x82:", LblX(), y, LblW(), FldH()));
	CvarSlider* botSlider = new CvarSlider("bottomcolor", InpX(), y, InpW(), FldH(), 0, 255);
	page->addChild(botSlider);
	_sliders.addElement(botSlider);
}

void VguiOptionsDialog::buildKeyboardTab(Panel* page)
{
	// "Привязки клавиш"
	page->addChild(new Label("\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD1\x8F\xD0\xB7\xD0\xBA\xD0\xB8 \xD0\xBA\xD0\xBB\xD0\xB0\xD0\xB2\xD0\xB8\xD1\x88", LblX(), FirstY(), VS(200), FldH()));
}

void VguiOptionsDialog::buildMouseTab(Panel* page)
{
	int y = FirstY();

	// "Фильтр мыши"
	CvarCheckButton* filter = new CvarCheckButton("m_filter", "\xD0\xA4\xD0\xB8\xD0\xBB\xD1\x8C\xD1\x82\xD1\x80 \xD0\xBC\xD1\x8B\xD1\x88\xD0\xB8", LblX(), y, VS(220), FldH());
	page->addChild(filter); _checkButtons.addElement(filter);
	y += RowH();

	// "Чувствительность:"
	page->addChild(new Label("\xD0\xA7\xD1\x83\xD0\xB2\xD1\x81\xD1\x82\xD0\xB2\xD0\xB8\xD1\x82\xD0\xB5\xD0\xBB\xD1\x8C\xD0\xBD\xD0\xBE\xD1\x81\xD1\x82\xD1\x8C:", LblX(), y, LblW(), FldH()));
	CvarSlider* sensSlider = new CvarSlider("sensitivity", InpX(), y, InpW(), FldH(), 1, 20);
	page->addChild(sensSlider); _sliders.addElement(sensSlider);
	y += RowH() + RowGap();

	// "Прямой ввод"
	CvarCheckButton* rawinput = new CvarCheckButton("m_rawinput", "\xD0\x9F\xD1\x80\xD1\x8F\xD0\xBC\xD0\xBE\xD0\xB9 \xD0\xB2\xD0\xB2\xD0\xBE\xD0\xB4", LblX(), y, VS(220), FldH());
	page->addChild(rawinput); _checkButtons.addElement(rawinput);
	y += RowH();

	// "Своё ускорение"
	CvarCheckButton* customaccel = new CvarCheckButton("m_customaccel", "\xD0\xA1\xD0\xB2\xD0\xBE\xD1\x91 \xD1\x83\xD1\x81\xD0\xBA\xD0\xBE\xD1\x80\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB5", LblX(), y, VS(220), FldH());
	page->addChild(customaccel); _checkButtons.addElement(customaccel);
}

void VguiOptionsDialog::buildAudioTab(Panel* page)
{
	int y = FirstY();

	// "Громкость:"
	page->addChild(new Label("\xD0\x93\xD1\x80\xD0\xBE\xD0\xBC\xD0\xBA\xD0\xBE\xD1\x81\xD1\x82\xD1\x8C:", LblX(), y, LblW(), FldH()));
	CvarSlider* volSlider = new CvarSlider("volume", InpX(), y, InpW(), FldH(), 0, 100, 0.0f, 1.0f);
	page->addChild(volSlider); _sliders.addElement(volSlider);
	y += RowH() + RowGap();

	// "Громкость HEV:"
	page->addChild(new Label("\xD0\x93\xD1\x80\xD0\xBE\xD0\xBC\xD0\xBA\xD0\xBE\xD1\x81\xD1\x82\xD1\x8C HEV:", LblX(), y, LblW(), FldH()));
	CvarSlider* suitSlider = new CvarSlider("suitvolume", InpX(), y, InpW(), FldH(), 0, 100, 0.0f, 1.0f);
	page->addChild(suitSlider); _sliders.addElement(suitSlider);
	y += RowH() + RowGap();

	CvarCheckButton* a3d = new CvarCheckButton("s_a3d", "A3D Audio", LblX(), y, VS(220), FldH());
	page->addChild(a3d); _checkButtons.addElement(a3d);
	y += RowH();

	// "Эффекты EAX"
	CvarCheckButton* eax = new CvarCheckButton("s_eax", "\xD0\xAD\xD1\x84\xD1\x84\xD0\xB5\xD0\xBA\xD1\x82\xD1\x8B EAX", LblX(), y, VS(220), FldH());
	page->addChild(eax); _checkButtons.addElement(eax);
}

void VguiOptionsDialog::buildVideoTab(Panel* page)
{
	int y = FirstY();

	// "Гамма:"
	page->addChild(new Label("\xD0\x93\xD0\xB0\xD0\xBC\xD0\xBC\xD0\xB0:", LblX(), y, LblW(), FldH()));
	CvarSlider* gammaSlider = new CvarSlider("gamma", InpX(), y, InpW(), FldH(), 0, 100, 1.8f, 3.0f);
	page->addChild(gammaSlider); _sliders.addElement(gammaSlider);
	y += RowH() + RowGap();

	// "Яркость:"
	page->addChild(new Label("\xD0\xAF\xD1\x80\xD0\xBA\xD0\xBE\xD1\x81\xD1\x82\xD1\x8C:", LblX(), y, LblW(), FldH()));
	CvarSlider* brightSlider = new CvarSlider("brightness", InpX(), y, InpW(), FldH(), 0, 100, 0.0f, 2.0f);
	page->addChild(brightSlider); _sliders.addElement(brightSlider);
	y += RowH() + RowGap();

	// "Верт. синхронизация"
	CvarCheckButton* vsync = new CvarCheckButton("gl_vsync", "\xD0\x92\xD0\xB5\xD1\x80\xD1\x82. \xD1\x81\xD0\xB8\xD0\xBD\xD1\x85\xD1\x80\xD0\xBE\xD0\xBD\xD0\xB8\xD0\xB7\xD0\xB0\xD1\x86\xD0\xB8\xD1\x8F", LblX(), y, VS(220), FldH());
	page->addChild(vsync); _checkButtons.addElement(vsync);
}

void VguiOptionsDialog::buildHudTab(Panel* page)
{
	int y = FirstY();

	// "Рисовать HUD"
	CvarCheckButton* hudDraw = new CvarCheckButton("hud_draw", "\xD0\xA0\xD0\xB8\xD1\x81\xD0\xBE\xD0\xB2\xD0\xB0\xD1\x82\xD1\x8C HUD", LblX(), y, VS(220), FldH());
	page->addChild(hudDraw); _checkButtons.addElement(hudDraw);
	y += RowH();

	// "Показывать FPS"
	CvarCheckButton* showFps = new CvarCheckButton("cl_showfps", "\xD0\x9F\xD0\xBE\xD0\xBA\xD0\xB0\xD0\xB7\xD1\x8B\xD0\xB2\xD0\xB0\xD1\x82\xD1\x8C FPS", LblX(), y, VS(220), FldH());
	page->addChild(showFps); _checkButtons.addElement(showFps);
	y += RowH();

	// "Масштаб HUD:"
	page->addChild(new Label("\xD0\x9C\xD0\xB0\xD1\x81\xD1\x88\xD1\x82\xD0\xB0\xD0\xB1 HUD:", LblX(), y, LblW(), FldH()));
	CvarSlider* scaleSlider = new CvarSlider("hud_scale", InpX(), y, InpW(), FldH(), 0, 10, 0.0f, 2.0f);
	page->addChild(scaleSlider); _sliders.addElement(scaleSlider);
	y += RowH() + RowGap();

	// "Показывать прицел"
	CvarCheckButton* crosshair = new CvarCheckButton("crosshair", "\xD0\x9F\xD0\xBE\xD0\xBA\xD0\xB0\xD0\xB7\xD1\x8B\xD0\xB2\xD0\xB0\xD1\x82\xD1\x8C \xD0\xBF\xD1\x80\xD0\xB8\xD1\x86\xD0\xB5\xD0\xBB", LblX(), y, VS(220), FldH());
	page->addChild(crosshair); _checkButtons.addElement(crosshair);
}

void VguiOptionsDialog::buildAccountTab(Panel* page)
{
	// "Настройки аккаунта"
	page->addChild(new Label("\xD0\x9D\xD0\xB0\xD1\x81\xD1\x82\xD1\x80\xD0\xBE\xD0\xB9\xD0\xBA\xD0\xB8 \xD0\xB0\xD0\xBA\xD0\xBA\xD0\xB0\xD1\x83\xD0\xBD\xD1\x82\xD0\xB0", LblX(), FirstY(), VS(200), FldH()));
}

void VguiOptionsDialog::buildSystemTab(Panel* page)
{
	// "Режим разработчика"
	CvarCheckButton* dev = new CvarCheckButton("developer", "\xD0\xA0\xD0\xB5\xD0\xB6\xD0\xB8\xD0\xBC \xD1\x80\xD0\xB0\xD0\xB7\xD1\x80\xD0\xB0\xD0\xB1\xD0\xBE\xD1\x82\xD1\x87\xD0\xB8\xD0\xBA\xD0\xB0", LblX(), FirstY(), VS(220), FldH());
	page->addChild(dev); _checkButtons.addElement(dev);
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

extern "C" void VGUI_EnsureInitialized(int screenW, int screenH);

OPTDLG_EXPORT void VGUI_ShowOptions(void)
{
	VLOG("VGUI_ShowOptions ENTRY");

	int sw = 0, sh = 0;
	vgui::VGUI_GetScreenSize(&sw, &sh);
	if (sw <= 0) sw = 640;
	if (sh <= 0) sh = 480;
	VLOG("ShowOptions: screen %dx%d", sw, sh);

	VGUI_EnsureInitialized(sw, sh);
	VLOG("ShowOptions: ensure-init returned");

	vgui::Panel* root = vgui::VGUI_GetRootPanel();
	if (!root)
	{
		VLOG("ShowOptions: VGUI_GetRootPanel() == null -- abort");
		return;
	}
	VLOG("ShowOptions: root=%p", (void*)root);

	if (!vgui::g_pOptionsDialog)
	{
		VLOG("ShowOptions: creating dialog (first time)");
		vgui::g_pOptionsDialog = new vgui::VguiOptionsDialog(sw, sh);
		VLOG("ShowOptions: ctor returned dlg=%p", (void*)vgui::g_pOptionsDialog);
		root->addChild(vgui::g_pOptionsDialog);
		VLOG("ShowOptions: addChild done");
	}
	else
	{
		VLOG("ShowOptions: reusing existing dialog");
		int dlgW, dlgH;
		vgui::g_pOptionsDialog->getSize(dlgW, dlgH);
		vgui::g_pOptionsDialog->setPos((sw - dlgW) / 2, (sh - dlgH) / 2);
	}

	VLOG("ShowOptions: about to resetAll");
	vgui::g_pOptionsDialog->resetAll();
	VLOG("ShowOptions: resetAll done, setting visible");
	vgui::g_pOptionsDialog->setVisible(true);
	VLOG("ShowOptions: EXIT (visible=true)");
}

OPTDLG_EXPORT void VGUI_HideOptions(void)
{
	if (vgui::g_pOptionsDialog)
		vgui::g_pOptionsDialog->setVisible(false);
}

} // extern "C"
