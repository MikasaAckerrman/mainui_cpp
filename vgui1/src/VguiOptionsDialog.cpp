// VguiOptionsDialog.cpp - Options dialog with tabbed cvar controls
//
// All pixel coordinates are passed through VS() so the dialog scales with
// screen size (mainui's logical 768-unit reference => physical pixels).

// Heavy mainui headers BEFORE VGUI_*.h to avoid the `null` macro clash.
extern void UI_FillRect( int x, int y, int width, int height, const unsigned int color );
// mainui bridge - declared at global scope to avoid namespace mangling and
// to skip pulling Utils.h (which would cause the `null` macro clash).
extern void UI_EnableTextInput( bool enable );
#include "TrackerScheme.h"

#include <VGUI_Log.h>
#include <VGUI_SchemeColors.h>
#include <VGUI_Scheme.h>
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
#include <VGUI_EtchedBorder.h>
#include <VGUI_UIScale.h>
#include <string.h>

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

	// "Sticker" effect: 2px etched groove around each page area, drawn after
	// children so it sits on top of the content perimeter. One static border
	// instance reused by all 8 pages -- it is stateless.
	static EtchedBorder s_pageBorder;
	mpPage     ->setBorder(&s_pageBorder);
	kbPage     ->setBorder(&s_pageBorder);
	mousePage  ->setBorder(&s_pageBorder);
	audioPage  ->setBorder(&s_pageBorder);
	videoPage  ->setBorder(&s_pageBorder);
	hudPage    ->setBorder(&s_pageBorder);
	accountPage->setBorder(&s_pageBorder);
	systemPage ->setBorder(&s_pageBorder);

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
// TextEntry and tell the engine to hide the soft keyboard. Frame::setVisible
// handles drag/resize state and mouse capture cleanup.
void VguiOptionsDialog::setVisible(bool state)
{
	if (!state)
	{
		App* app = App::getInstance();
		if (app)
			app->requestFocus(null); // dispatches internalFocusChanged(true)
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

// ====================================================================
// Lightweight inline widgets used by the Multiplayer tab.
// Real implementations (image upload, popup combo, password masking
// with cursor metrics) come in later PRs; this is the visual layout
// pass so the tab matches PC CS 1.6 reference at a glance.
// ====================================================================

// Recessed olive square -- avatar / logo preview slot. No image yet.
class PreviewBox : public Panel
{
public:
	PreviewBox(int x, int y, int w, int h) : Panel(x, y, w, h) {}
protected:
	virtual void paintBackground()
	{
		int wide, tall;
		getSize(wide, tall);
		unsigned int bg     = g_Scheme.fieldBgColor    ? g_Scheme.fieldBgColor    : 0xE6555F4B;
		unsigned int dark   = g_Scheme.borderDark      ? g_Scheme.borderDark      : 0xC8282C24;
		unsigned int bright = g_Scheme.borderBright    ? g_Scheme.borderBright    : 0xC85F6558;
		schemeBgColor(this, bg);
		drawFilledRect(0, 0, wide, tall);
		// Lowered look: dark on top+left, bright on bottom+right
		schemeBgColor(this, dark);
		drawFilledRect(0, 0, wide, 1);
		drawFilledRect(0, 0, 1, tall);
		schemeBgColor(this, bright);
		drawFilledRect(0, tall - 1, wide, tall);
		drawFilledRect(wide - 1, 0, wide, tall);
	}
};

// TextEntry that renders '*' for every character. Inherits all editing,
// cursor, focus, IME and dirty-signal logic from CvarTextEntry; only the
// visual paint is replaced. Cursor metric is approximate (8px monospace)
// because the real cursor X is computed from private members of TextEntry.
class PasswordTextEntry : public CvarTextEntry
{
public:
	PasswordTextEntry(const char* cvarName, int x, int y, int w, int h)
		: CvarTextEntry(cvarName, x, y, w, h) {}
protected:
	virtual void paint()
	{
		int pwide, ptall;
		getPaintSize(pwide, ptall);

		int len = getTextLength();
		if (len == 0 && !hasFocus())
			return;

		char stars[256];
		int n = (len < 255) ? len : 255;
		for (int i = 0; i < n; i++) stars[i] = '*';
		stars[n] = 0;

		unsigned int textCol = g_Scheme.fieldTextColor ? g_Scheme.fieldTextColor : 0xFFFFFFFF;
		schemeFgColor(this, textCol);
		drawSetTextFont(Scheme::sf_primary1);

		int textX = 4;
		int textY = 3;
		drawPrintText(textX, textY, stars, n);

		if (hasFocus())
		{
			int cursorX = textX + n * 8;
			schemeBgColor(this, textCol);
			drawFilledRect(cursorX, 2, cursorX + 1, ptall - 2);
		}
	}
};

// Stub combo-box: button that cycles through a fixed list of options on
// click and writes the current option to a cvar. No popup yet -- popup
// menu widget is a separate PR. Visually shows "value [v]" so user sees
// the text and the dropdown affordance.
class StubComboButton;
class StubCombo_CycleSignal : public ActionSignal
{
public:
	StubCombo_CycleSignal(StubComboButton* c) : _c(c) {}
	virtual void actionPerformed(Panel* p);
private:
	StubComboButton* _c;
};

class StubComboButton : public Button
{
public:
	StubComboButton(const char* cvarName, const char* const* opts, int optCount,
	                int x, int y, int w, int h)
		: Button("", x, y, w, h),
		  _opts(opts), _optCount(optCount)
	{
		_cvar[0] = 0;
		if (cvarName) vgui_strcpy(_cvar, sizeof(_cvar), cvarName);
		_idx = 0;
		const char* cur = VGUI_GetCvarString(_cvar);
		for (int i = 0; i < _optCount; i++)
		{
			if (cur && opts[i] && strcmp(cur, opts[i]) == 0) { _idx = i; break; }
		}
		_buf[0] = 0;
		refreshLabel();
		addActionSignal(new StubCombo_CycleSignal(this));
	}

	void cycle()
	{
		if (_optCount <= 0) return;
		_idx = (_idx + 1) % _optCount;
		if (_opts[_idx])
			VGUI_SetCvarString(_cvar, _opts[_idx]);
		refreshLabel();
	}
private:
	void refreshLabel()
	{
		const char* v = (_idx >= 0 && _idx < _optCount && _opts[_idx]) ? _opts[_idx] : "";
		int n = 0;
		while (v[n] && n < 60) { _buf[n] = v[n]; n++; }
		const char* tail = "  [v]";
		for (int i = 0; tail[i] && n < 63; i++) _buf[n++] = tail[i];
		_buf[n] = 0;
		setText("%s", _buf);
	}
	char _cvar[64];
	const char* const* _opts;
	int _optCount;
	int _idx;
	char _buf[64];
};

inline void StubCombo_CycleSignal::actionPerformed(Panel* /*p*/)
{
	if (_c) _c->cycle();
}

void VguiOptionsDialog::buildMultiplayerTab(Panel* page)
{
	// Two-column layout matching PC CS 1.6:
	//   Left  column: Avatar slot + "Загрузить..." + cts_team combo,
	//                 Logo slot   + lambda combo  + "Изменить цвет",
	//                 hint label, "Дополнительно..."
	//   Right column: Имя игрока field, Пароль для VIP/Admin field
	int leftX  = VS(14);
	int rightX = VS(280);
	int colW   = VS(240);
	int y = VS(10);

	// ---- Left column: Avatar group -------------------------------------
	// "Аватар"
	page->addChild(new Label("\xD0\x90\xD0\xB2\xD0\xB0\xD1\x82\xD0\xB0\xD1\x80",
		leftX, y, VS(80), FldH()));
	y += VS(16);
	int slotSize = VS(64);
	page->addChild(new PreviewBox(leftX, y, slotSize, slotSize));
	// "Загрузить..."
	page->addChild(new Button("\xD0\x97\xD0\xB0\xD0\xB3\xD1\x80\xD1\x83\xD0\xB7\xD0\xB8\xD1\x82\xD1\x8C...",
		leftX + slotSize + VS(8), y, VS(120), VS(22)));
	// cts_team combo placed below the load button
	static const char* k_teams[] = { "cts_team", "ts_team", "vip_team", "admin_team" };
	StubComboButton* teamCombo = new StubComboButton("logo_team", k_teams, 4,
		leftX + slotSize + VS(8), y + VS(28), VS(120), VS(22));
	teamCombo->addActionSignal(new MarkDirtyActionSignal(this));
	page->addChild(teamCombo);
	y += slotSize + VS(12);

	// ---- Left column: Logo group ---------------------------------------
	// "Логотип"
	page->addChild(new Label("\xD0\x9B\xD0\xBE\xD0\xB3\xD0\xBE\xD1\x82\xD0\xB8\xD0\xBF",
		leftX, y, VS(80), FldH()));
	y += VS(16);
	page->addChild(new PreviewBox(leftX, y, slotSize, slotSize));
	// lambda combo (cl_logofile cvar in CS 1.6)
	static const char* k_logos[] = { "lambda", "skull", "ts_team", "cts_team", "n0!se" };
	StubComboButton* logoCombo = new StubComboButton("cl_logofile", k_logos, 5,
		leftX + slotSize + VS(8), y, VS(120), VS(22));
	logoCombo->addActionSignal(new MarkDirtyActionSignal(this));
	page->addChild(logoCombo);
	// "Изменить цвет"
	page->addChild(new Button("\xD0\x98\xD0\xB7\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xB8\xD1\x82\xD1\x8C \xD1\x86\xD0\xB2\xD0\xB5\xD1\x82",
		leftX + slotSize + VS(8), y + VS(28), VS(120), VS(22)));
	y += slotSize + VS(12);

	// "Логотип изменится после соединения с сервером." -- dim hint
	Label* hint = new Label(
		"\xD0\x9B\xD0\xBE\xD0\xB3\xD0\xBE\xD1\x82\xD0\xB8\xD0\xBF \xD0\xB8\xD0\xB7\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xB8\xD1\x82\xD1\x81\xD1\x8F \xD0\xBF\xD0\xBE\xD1\x81\xD0\xBB\xD0\xB5 \xD1\x81\xD0\xBE\xD0\xB5\xD0\xB4\xD0\xB8\xD0\xBD\xD0\xB5\xD0\xBD\xD0\xB8\xD1\x8F \xD1\x81 \xD1\x81\xD0\xB5\xD1\x80\xD0\xB2\xD0\xB5\xD1\x80\xD0\xBE\xD0\xBC.",
		leftX, y, colW, FldH() * 2);
	page->addChild(hint);
	y += VS(28);

	// "Дополнительно..."
	page->addChild(new Button("\xD0\x94\xD0\xBE\xD0\xBF\xD0\xBE\xD0\xBB\xD0\xBD\xD0\xB8\xD1\x82\xD0\xB5\xD0\xBB\xD1\x8C\xD0\xBD\xD0\xBE...",
		leftX, y, VS(120), VS(22)));

	// ---- Right column: Имя игрока + Пароль -----------------------------
	int ry = VS(10);
	// "Имя игрока"
	page->addChild(new Label("\xD0\x98\xD0\xBC\xD1\x8F \xD0\xB8\xD0\xB3\xD1\x80\xD0\xBE\xD0\xBA\xD0\xB0",
		rightX, ry, VS(120), FldH()));
	ry += VS(16);
	CvarTextEntry* nameEntry = new CvarTextEntry("name", rightX, ry, colW, FldH());
	page->addChild(nameEntry);
	_textEntries.addElement(nameEntry);
	ry += VS(36);

	// "Пароль для VIP/Admin доступа"
	page->addChild(new Label(
		"\xD0\x9F\xD0\xB0\xD1\x80\xD0\xBE\xD0\xBB\xD1\x8C \xD0\xB4\xD0\xBB\xD1\x8F VIP/Admin \xD0\xB4\xD0\xBE\xD1\x81\xD1\x82\xD1\x83\xD0\xBF\xD0\xB0",
		rightX, ry, VS(220), FldH()));
	ry += VS(16);
	PasswordTextEntry* pwdEntry = new PasswordTextEntry("vip_password", rightX, ry, colW, FldH());
	page->addChild(pwdEntry);
	_textEntries.addElement(pwdEntry);
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
