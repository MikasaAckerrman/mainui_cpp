// VguiOptionsDialog.cpp - Pixel-perfect CS 1.6 GoldSrc VGUI Options dialog
//
// All coordinates use VS() scaling from a 640x480 reference grid.
// Layout matches original GoldSrc VGUI: fixed coords, 4px-aligned spacing,
// double-bevel borders, compact titlebar, overlapping tab bevels.

extern void UI_FillRect( int x, int y, int width, int height, const unsigned int color );
extern void UI_EnableTextInput( bool enable );
#include "BaseMenu.h"
#include "FontManager.h"
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
#include <VGUI_ListPanel.h>
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
// Inline widgets for Multiplayer tab (GoldSrc layout)
// ====================================================================

// Recessed preview box (avatar/logo slot) - GoldSrc dark olive inset
class PreviewBox : public Panel
{
public:
	PreviewBox(int x, int y, int w, int h) : Panel(x, y, w, h) {}
protected:
	virtual void paintBackground()
	{
		int wide, tall;
		getSize(wide, tall);
		// Recessed slot fill: same as TextEntry/list background (canon WindowBG).
		unsigned int slotBg = g_Scheme.fieldBgColor ? g_Scheme.fieldBgColor : 0xE63E4637;
		schemeBgColor(this, slotBg);
		drawFilledRect(0, 0, wide, tall);
		// Canonical 1px inset (sunken): TL=dark, BR=bright.
		unsigned int dark = g_Scheme.borderDark ? g_Scheme.borderDark : 0xFF282E22;
		unsigned int bright = g_Scheme.borderBright ? g_Scheme.borderBright : 0xFF889180;
		schemeBgColor(this, dark);
		drawFilledRect(0, 0, wide, 1);
		drawFilledRect(0, 0, 1, tall);
		schemeBgColor(this, bright);
		drawFilledRect(0, tall - 1, wide, tall);
		drawFilledRect(wide - 1, 0, wide, tall);
	}
};

// Page panel that proportionally scales every child when its own size
// changes. Pages are built with absolute pixel coordinates that match a
// reference dialog size (clientW x pageH at construction time); when the
// user resizes the dialog, TabPanel calls setSize on each page with new
// dimensions and we rescale every child by the ratio newSize/refSize.
//
// Reference size is captured once on the FIRST resize that actually
// changes the bounds (Panel ctor already sets size, so we cannot use
// "first setSize call" - that one would be a no-op). Subsequent resizes
// always scale relative to the captured reference, so rounding errors
// do not accumulate.
class LayoutPage : public Panel
{
public:
	LayoutPage(int x, int y, int w, int h) : Panel(x, y, w, h), _refW(0), _refH(0) {}

	virtual void setSize(int wide, int tall)
	{
		int oldW = getWide(), oldH = getTall();
		Panel::setSize(wide, tall);

		// Capture the first non-trivial size as the reference. Pages are
		// constructed with their target initial size, so the ctor sets
		// _refW/_refH==0 and the very first matching setSize call leaves
		// them zero - that is fine, the scaling block is guarded by
		// _refW>0. Reference is taken at the construction-time bounds.
		if (_refW <= 0 || _refH <= 0)
		{
			_refW = oldW > 0 ? oldW : wide;
			_refH = oldH > 0 ? oldH : tall;
			return;
		}
		if (wide == oldW && tall == oldH)
			return;

		// Scale every child proportionally to the resize ratio. setBounds
		// triggers child setSize which (for nested LayoutPage / Slider /
		// CheckButton) recomputes their internals.
		int cnt = getChildCount();
		for (int i = 0; i < cnt; i++)
		{
			Panel* c = getChild(i);
			if (!c) continue;
			int cx, cy, cw, ch;
			c->getBounds(cx, cy, cw, ch);
			int nx = (cx * wide + _refW / 2) / _refW;
			int ny = (cy * tall + _refH / 2) / _refH;
			int nw = (cw * wide + _refW / 2) / _refW;
			int nh = (ch * tall + _refH / 2) / _refH;
			c->setBounds(nx, ny, nw, nh);
		}
		_refW = wide;
		_refH = tall;
	}

private:
	int _refW;
	int _refH;
};


// PasswordTextEntry: masks with '*', eye icon toggle
class PasswordTextEntry : public CvarTextEntry
{
public:
	PasswordTextEntry(const char* cvarName, int x, int y, int w, int h)
		: CvarTextEntry(cvarName, x, y, w, h), _showText(false) {}

	virtual void reset()
	{
		CvarTextEntry::reset();
		_showText = false;
	}
protected:
	virtual void paint()
	{
		int pwide, ptall;
		getPaintSize(pwide, ptall);

		int len = getTextLength();
		if (len > 0 || hasFocus())
		{
			char buf[256];
			int n;
			if (_showText)
			{
				getText(0, buf, sizeof(buf));
				n = (int)strlen(buf);
				if (n > 255) n = 255;
			}
			else
			{
				n = (len < 255) ? len : 255;
				for (int i = 0; i < n; i++) buf[i] = '*';
				buf[n] = 0;
			}

			unsigned int textCol = g_Scheme.fieldTextColor ? g_Scheme.fieldTextColor : 0xFFFFFFFF;
			schemeFgColor(this, textCol);
			drawSetTextFont(Scheme::sf_primary1);

			int textX = VS(6);
			int textY = (ptall - VS(11)) / 2;
			if (textY < 2) textY = 2;
			drawPrintText(textX, textY, buf, n);

			if (hasFocus())
			{
				// Use the FontManager's real width for the masked text just
				// like the parent TextEntry does for the plain text path.
				// Without this the caret used a guessed 8px/char and drifted
				// off the actual asterisks at scaled resolutions.
				HFont hFont = g_FontMgr ? g_FontMgr->GetVGUIFont(VS(12)) : 0;
				int caretX;
				if (g_FontMgr && hFont && n > 0)
					caretX = textX + g_FontMgr->GetTextWideScaled(hFont, buf, VS(12));
				else
					caretX = textX + n * 8;
				schemeBgColor(this, textCol);
				drawFilledRect(caretX, 3, caretX + 1, ptall - 3);
			}
		}

		// Eye icon
		HIMAGE icon = getEyeIcon();
		if (icon)
		{
			int iconBox = ptall - VS(4);
			int iconX = pwide - iconBox - VS(2);
			int iconY = (ptall - iconBox) / 2;
			int sx = iconX, sy = iconY;
			localToScreen(sx, sy);
			EngFuncs::PIC_Set(icon, 255, 255, 255, 255);
			EngFuncs::PIC_DrawTrans(sx, sy, iconBox, iconBox);
		}
	}

	virtual void internalMousePressed(MouseCode code)
	{
		if (code == MOUSE_LEFT && getEyeIcon())
		{
			App* app = App::getInstance();
			if (app)
			{
				int mx, my;
				app->getCursorPos(mx, my);
				screenToLocal(mx, my);
				int pwide, ptall;
				getPaintSize(pwide, ptall);
				int iconBox = ptall - VS(4);
				int iconX = pwide - iconBox - VS(2);
				if (mx >= iconX && mx < pwide && my >= 0 && my < ptall)
				{
					_showText = !_showText;
					repaint();
					return;
				}
			}
		}
		CvarTextEntry::internalMousePressed(code);
	}
private:
	HIMAGE getEyeIcon()
	{
		static HIMAGE s_lock   = (HIMAGE)-1;
		static HIMAGE s_unlock = (HIMAGE)-1;
		if (s_lock   == (HIMAGE)-1) s_lock   = EngFuncs::PIC_Load("gfx/vgui2/eye_lock.tga");
		if (s_unlock == (HIMAGE)-1) s_unlock = EngFuncs::PIC_Load("gfx/vgui2/eye_unlock.tga");
		return _showText ? s_unlock : s_lock;
	}
	bool _showText;
};


// StubComboButton: cycles options on click, draws arrow zone on right edge
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
protected:
	virtual void paintBackground()
	{
		// Use default Button bevel for the main body
		Button::paintBackground();

		// Draw separator line + arrow zone on right edge (GoldSrc dropdown look)
		int wide, tall;
		getSize(wide, tall);
		int arrowW = VS(16);
		unsigned int dark = g_Scheme.borderDark ? g_Scheme.borderDark : 0xFF282E22;
		unsigned int bright = g_Scheme.borderBright ? g_Scheme.borderBright : 0xFF889180;
		// Vertical separator
		schemeBgColor(this, dark);
		drawFilledRect(wide - arrowW - 1, 2, wide - arrowW, tall - 2);
		schemeBgColor(this, bright);
		drawFilledRect(wide - arrowW, 2, wide - arrowW + 1, tall - 2);
	}

	virtual void paint()
	{
		// Draw text in the left portion (not centered across full width)
		int wide, tall;
		getSize(wide, tall);
		int arrowW = VS(16);

		unsigned int argb;
		if (!isEnabled())
			argb = g_Scheme.labelDimColor ? g_Scheme.labelDimColor : 0xFF808080;
		else
			argb = g_Scheme.buttonTextColor ? g_Scheme.buttonTextColor : 0xFFD8DED3;

		schemeFgColor(this, argb);
		drawSetTextFont(Scheme::sf_primary1);

		char lbl[128];
		const char* v = (_idx >= 0 && _idx < _optCount && _opts[_idx]) ? _opts[_idx] : "";
		vgui_strcpy(lbl, sizeof(lbl), v);
		int textLen = (int)strlen(lbl);
		int textY = (tall - VS(11)) / 2;
		if (textY < 2) textY = 2;
		drawPrintText(VS(6), textY, lbl, textLen);

		// Arrow TGA or vector fallback in right zone
		static HIMAGE s_arrowDown = (HIMAGE)-1;
		if (s_arrowDown == (HIMAGE)-1)
			s_arrowDown = EngFuncs::PIC_Load("gfx/vgui/640_arrowdown.tga");

		unsigned int col = argb;
		if (s_arrowDown)
		{
			int iconH = tall - VS(6);
			if (iconH < VS(6)) iconH = VS(6);
			int iconW = iconH;
			int iconX = wide - arrowW / 2 - iconW / 2;
			int iconY = (tall - iconH) / 2;
			int sx = iconX, sy = iconY;
			localToScreen(sx, sy);
			int r = (col >> 16) & 0xFF;
			int g = (col >> 8) & 0xFF;
			int b = col & 0xFF;
			int a = (col >> 24) & 0xFF;
			EngFuncs::PIC_Set(s_arrowDown, r, g, b, a);
			EngFuncs::PIC_DrawTrans(sx, sy, iconW, iconH);
		}
		else
		{
			// Vector triangle fallback
			schemeBgColor(this, col);
			int cx = wide - arrowW / 2;
			int cy = tall / 2 - VS(1);
			int t = VS(1) > 0 ? VS(1) : 1;
			drawFilledRect(cx - VS(3), cy,       cx + VS(3), cy + t);
			drawFilledRect(cx - VS(2), cy + t,   cx + VS(2), cy + 2*t);
			drawFilledRect(cx - VS(1), cy + 2*t, cx + VS(1), cy + 3*t);
		}
	}
private:
	void refreshLabel()
	{
		// Label stored for Button base but we override paint() so this is cosmetic
		const char* v = (_idx >= 0 && _idx < _optCount && _opts[_idx]) ? _opts[_idx] : "";
		setText(v ? v : "");
	}
	char _cvar[64];
	const char* const* _opts;
	int _optCount;
	int _idx;
};

inline void StubCombo_CycleSignal::actionPerformed(Panel* /*p*/)
{
	if (_c) _c->cycle();
}


// InvertMouseCheckButton: toggles m_pitch sign (positive=normal, negative=inverted)
class InvertMouseCheckButton : public CheckButton
{
public:
	InvertMouseCheckButton(const char* text, int x, int y, int w, int h)
		: CheckButton(text, x, y, w, h) { reset(); }
	void reset()
	{
		float val = VGUI_GetCvarFloat("m_pitch");
		setSelected(val < 0.0f);
	}
	void apply()
	{
		float cur = VGUI_GetCvarFloat("m_pitch");
		if (cur == 0.0f) cur = 0.022f; // default if unset
		bool wantInvert = isSelected();
		if (wantInvert && cur > 0.0f)
			VGUI_SetCvarFloat("m_pitch", -cur);
		else if (!wantInvert && cur < 0.0f)
			VGUI_SetCvarFloat("m_pitch", -cur);
	}
protected:
	virtual void internalMousePressed(MouseCode code)
	{
		if (code == MOUSE_LEFT && isEnabled())
		{
			setSelected(!isSelected());
			apply();
			fireActionSignal();
		}
		Panel::internalMousePressed(code);
	}
};


// IntStubComboButton: cycles integer cvar values on click, draws arrow zone
class IntStubComboButton;
class IntStubCombo_CycleSignal : public ActionSignal
{
public:
	IntStubCombo_CycleSignal(IntStubComboButton* c) : _c(c) {}
	virtual void actionPerformed(Panel* p);
private:
	IntStubComboButton* _c;
};

class IntStubComboButton : public Button
{
public:
	IntStubComboButton(const char* cvarName, const char* const* labels, const int* values, int count,
	                   int x, int y, int w, int h)
		: Button("", x, y, w, h),
		  _labels(labels), _values(values), _count(count)
	{
		_cvar[0] = 0;
		if (cvarName) vgui_strcpy(_cvar, sizeof(_cvar), cvarName);
		_idx = 0;
		int cur = (int)VGUI_GetCvarFloat(_cvar);
		for (int i = 0; i < _count; i++)
		{
			if (_values[i] == cur) { _idx = i; break; }
		}
		refreshLabel();
		addActionSignal(new IntStubCombo_CycleSignal(this));
	}

	void cycle()
	{
		if (_count <= 0) return;
		_idx = (_idx + 1) % _count;
		VGUI_SetCvarFloat(_cvar, (float)_values[_idx]);
		refreshLabel();
	}

	void reset()
	{
		int cur = (int)VGUI_GetCvarFloat(_cvar);
		for (int i = 0; i < _count; i++)
		{
			if (_values[i] == cur) { _idx = i; break; }
		}
		refreshLabel();
	}

	void apply()
	{
		if (_idx >= 0 && _idx < _count)
			VGUI_SetCvarFloat(_cvar, (float)_values[_idx]);
	}

protected:
	virtual void paintBackground()
	{
		Button::paintBackground();
		int wide, tall;
		getSize(wide, tall);
		int arrowW = VS(16);
		unsigned int dark = g_Scheme.borderDark ? g_Scheme.borderDark : 0xFF282E22;
		unsigned int bright = g_Scheme.borderBright ? g_Scheme.borderBright : 0xFF889180;
		schemeBgColor(this, dark);
		drawFilledRect(wide - arrowW - 1, 2, wide - arrowW, tall - 2);
		schemeBgColor(this, bright);
		drawFilledRect(wide - arrowW, 2, wide - arrowW + 1, tall - 2);
	}

	virtual void paint()
	{
		int wide, tall;
		getSize(wide, tall);
		int arrowW = VS(16);

		unsigned int argb;
		if (!isEnabled())
			argb = g_Scheme.labelDimColor ? g_Scheme.labelDimColor : 0xFF808080;
		else
			argb = g_Scheme.buttonTextColor ? g_Scheme.buttonTextColor : 0xFFD8DED3;

		schemeFgColor(this, argb);
		drawSetTextFont(Scheme::sf_primary1);

		char lbl[128];
		const char* v = (_idx >= 0 && _idx < _count && _labels[_idx]) ? _labels[_idx] : "";
		vgui_strcpy(lbl, sizeof(lbl), v);
		int textLen = (int)strlen(lbl);
		int textY = (tall - VS(11)) / 2;
		if (textY < 2) textY = 2;
		drawPrintText(VS(6), textY, lbl, textLen);

		static HIMAGE s_arrowDown = (HIMAGE)-1;
		if (s_arrowDown == (HIMAGE)-1)
			s_arrowDown = EngFuncs::PIC_Load("gfx/vgui/640_arrowdown.tga");

		unsigned int col = argb;
		if (s_arrowDown)
		{
			int iconH = tall - VS(6);
			if (iconH < VS(6)) iconH = VS(6);
			int iconW = iconH;
			int iconX = wide - arrowW / 2 - iconW / 2;
			int iconY = (tall - iconH) / 2;
			int sx = iconX, sy = iconY;
			localToScreen(sx, sy);
			int r = (col >> 16) & 0xFF;
			int g = (col >> 8) & 0xFF;
			int b = col & 0xFF;
			int a = (col >> 24) & 0xFF;
			EngFuncs::PIC_Set(s_arrowDown, r, g, b, a);
			EngFuncs::PIC_DrawTrans(sx, sy, iconW, iconH);
		}
		else
		{
			schemeBgColor(this, col);
			int cx = wide - arrowW / 2;
			int cy = tall / 2 - VS(1);
			int t = VS(1) > 0 ? VS(1) : 1;
			drawFilledRect(cx - VS(3), cy,       cx + VS(3), cy + t);
			drawFilledRect(cx - VS(2), cy + t,   cx + VS(2), cy + 2*t);
			drawFilledRect(cx - VS(1), cy + 2*t, cx + VS(1), cy + 3*t);
		}
	}

private:
	void refreshLabel()
	{
		const char* v = (_idx >= 0 && _idx < _count && _labels[_idx]) ? _labels[_idx] : "";
		setText(v);
	}
	char _cvar[64];
	const char* const* _labels;
	const int* _values;
	int _count;
	int _idx;
};

inline void IntStubCombo_CycleSignal::actionPerformed(Panel* /*p*/)
{
	if (_c) _c->cycle();
}


// ====================================================================
// GoldSrc grid layout constants (@ 640x480 reference, scaled via VS)
// All multiples of 4 for pixel-perfect alignment.
// ====================================================================
static const int DLG_W = 660;   // dialog width  — matches PC CS 1.6 Options 512×406 ratio 1.261 exactly
static const int DLG_H = 524;   // dialog height — 660/524 = 1.260 ≈ PC original 512/406 = 1.261

// Form metrics for tab page content
static inline int LblX()   { return VS(10); }
static inline int LblW()   { return VS(120); }
static inline int InpX()   { return VS(140); }
static inline int InpW()   { return VS(200); }
static inline int RowH()   { return VS(26); }
static inline int FldH()   { return VS(22); }   // field/button height
static inline int FirstY() { return VS(10); }   // first row Y in page

// ====================================================================
// VguiOptionsDialog
// ====================================================================

VguiOptionsDialog::VguiOptionsDialog(int screenW, int screenH)
	: Frame(0, 0, VS(DLG_W), VS(DLG_H))
{
	VLOG("VguiOptionsDialog ctor: screenW=%d screenH=%d", screenW, screenH);
	_tabPanel = null;
	_applyBtn = null;
	_okBtn = null;
	_cancelBtn = null;
	_dirty = false;

	int dialogW = VS(DLG_W);
	int dialogH = VS(DLG_H);
	if (dialogW > screenW - VS(8)) dialogW = screenW - VS(8);
	if (dialogH > screenH - VS(8)) dialogH = screenH - VS(8);

	setPos((screenW - dialogW) / 2, (screenH - dialogH) / 2);
	setSize(dialogW, dialogH);
	// Main Options dialog matches the PC: FIXED size (only drag-move), so the
	// proportional layout always looks right. No maximize/fullscreen button -
	// the PC Options window cannot be maximized, only moved.
	setSizeable(false);
	setTitle("\xD0\x9D\xD0\xB0\xD1\x81\xD1\x82\xD1\x80\xD0\xBE\xD0\xB9\xD0\xBA\xD0\xB8"); // Настройки
	setVisible(false);

	Panel* client = getClient();
	if (!client) { VLOG("ctor: getClient() null -- abort"); return; }

	int clientW, clientH;
	client->getSize(clientW, clientH);

	// Bottom button row: 24px buttons + 8px margin (canon ~52px from bottom)
	int btnH    = VS(24);
	int btnRowH = btnH + VS(8);
	int tabH    = clientH - btnRowH;
	if (tabH < VS(100)) tabH = VS(100);

	_tabPanel = new TabPanel(0, 0, clientW, tabH);
	client->addChild(_tabPanel);

	// Tab page height (below tab strip)
	int pageH = tabH - VS(28);
	Panel* mpPage      = new LayoutPage(0, 0, clientW, pageH);
	Panel* kbPage      = new LayoutPage(0, 0, clientW, pageH);
	Panel* mousePage   = new LayoutPage(0, 0, clientW, pageH);
	Panel* audioPage   = new LayoutPage(0, 0, clientW, pageH);
	Panel* videoPage   = new LayoutPage(0, 0, clientW, pageH);
	Panel* hudPage     = new LayoutPage(0, 0, clientW, pageH);
	Panel* accountPage = new LayoutPage(0, 0, clientW, pageH);
	Panel* systemPage  = new LayoutPage(0, 0, clientW, pageH);

	// Etched border around each page: REMOVED. The active tab merges into
	// the page area (+2px overlap), and an etched border drawn on top of
	// that overlap creates a visible seam under the active tab. Without
	// the border, the tab + page form one continuous olive surface, which
	// is the GoldSrc CS 1.6 reference look.


	_tabPanel->addTab("\xD0\x9C\xD1\x83\xD0\xBB\xD1\x8C\xD1\x82\xD0\xB8\xD0\xBF\xD0\xBB\xD0\xB5\xD0\xB5\xD1\x80",   mpPage);
	_tabPanel->addTab("\xD0\x9A\xD0\xBB\xD0\xB0\xD0\xB2\xD0\xB8\xD0\xB0\xD1\x82\xD1\x83\xD1\x80\xD0\xB0",          kbPage);
	_tabPanel->addTab("\xD0\x9C\xD1\x8B\xD1\x88\xD1\x8C",          mousePage);
	_tabPanel->addTab("\xD0\x97\xD0\xB2\xD1\x83\xD0\xBA",          audioPage);
	_tabPanel->addTab("\xD0\x92\xD0\xB8\xD0\xB4\xD0\xB5\xD0\xBE", videoPage);
	_tabPanel->addTab("HUD",                                       hudPage);
	_tabPanel->addTab("\xD0\x90\xD0\xBA\xD0\xBA\xD0\xB0\xD1\x83\xD0\xBD\xD1\x82", accountPage);
	_tabPanel->addTab("\xD0\xA1\xD0\xB8\xD1\x81\xD1\x82\xD0\xB5\xD0\xBC\xD0\xB0", systemPage);

	buildMultiplayerTab(mpPage);
	buildKeyboardTab(kbPage);
	buildMouseTab(mousePage);
	buildAudioTab(audioPage);
	buildVideoTab(videoPage);
	buildHudTab(hudPage);
	buildAccountTab(accountPage);
	buildSystemTab(systemPage);

	// Bottom buttons: OK | Cancel | Apply (right-aligned, 88px wide)
	int btnW    = VS(88);
	int btnGap  = VS(6);
	int btnY    = clientH - btnH - VS(8);
	int applyX  = clientW - VS(8) - btnW;
	int cancelX = applyX  - btnGap - btnW;
	int okX     = cancelX - btnGap - btnW;

	Button* okBtn = new Button("OK", okX, btnY, btnW, btnH);
	client->addChild(okBtn);
	okBtn->addActionSignal(new OptionsOKSignal(this));
	_okBtn = okBtn;

	Button* cancelBtn = new Button("\xD0\x9E\xD1\x82\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xB0", cancelX, btnY, btnW, btnH);
	client->addChild(cancelBtn);
	cancelBtn->addActionSignal(new OptionsCancelSignal(this));
	_cancelBtn = cancelBtn;

	_applyBtn = new Button("\xD0\x9F\xD1\x80\xD0\xB8\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xB8\xD1\x82\xD1\x8C", applyX, btnY, btnW, btnH);
	client->addChild(_applyBtn);
	_applyBtn->addActionSignal(new OptionsApplySignal(this));
	_applyBtn->setEnabled(false);

	// Wire dirty tracking
	for (int i = 0; i < _checkButtons.getCount(); i++)
		if (_checkButtons[i]) _checkButtons[i]->addActionSignal(new MarkDirtyActionSignal(this));
	for (int i = 0; i < _sliders.getCount(); i++)
		if (_sliders[i]) _sliders[i]->addIntChangeSignal(new MarkDirtyIntSignal(this));
	for (int i = 0; i < _textEntries.getCount(); i++)
		if (_textEntries[i]) _textEntries[i]->addActionSignal(new MarkDirtyActionSignal(this));
	VLOG("ctor: done");
}


void VguiOptionsDialog::setDirty(bool dirty)
{
	_dirty = dirty;
	if (_applyBtn) _applyBtn->setEnabled(dirty);
}

void VguiOptionsDialog::setSize(int wide, int tall)
{
	Frame::setSize(wide, tall);

	Panel* client = getClient();
	if (!client) return;

	int clientW, clientH;
	client->getSize(clientW, clientH);

	int btnH    = VS(24);
	int btnRowH = btnH + VS(8);   // canon: button row sits ~52px from bottom (was 16+24=40)
	int tabH    = clientH - btnRowH;
	if (tabH < VS(100)) tabH = VS(100);

	if (_tabPanel) _tabPanel->setBounds(0, 0, clientW, tabH);

	int btnW   = VS(88);
	int btnGap = VS(6);
	int btnY   = clientH - btnH - VS(8);
	int applyX  = clientW - VS(8) - btnW;
	int cancelX = applyX  - btnGap - btnW;
	int okX     = cancelX - btnGap - btnW;

	if (_okBtn)     _okBtn->setBounds(okX, btnY, btnW, btnH);
	if (_cancelBtn) _cancelBtn->setBounds(cancelX, btnY, btnW, btnH);
	if (_applyBtn)  _applyBtn->setBounds(applyX, btnY, btnW, btnH);
}

// Forward declaration for the static instance pointer used by the grab callback.
// Declared here (before setVisible) so setVisible can clear the grab on hide.
class KeyBindList;
static KeyBindList* s_activeKeyBindList = 0;
static KeyBindList* s_keyBindListInstance = 0; // persistent ref for re-show refresh

// Custom widget pointers for resetAll()/applyAll() lifecycle participation.
// These widgets are not CvarCheckButton so they cannot go in _checkButtons.
static InvertMouseCheckButton* s_invertMouseBtn = 0;
static IntStubComboButton* s_resCombo = 0;
static IntStubComboButton* s_fsCombo = 0;

void VguiOptionsDialog::setVisible(bool state)
{
	if (!state)
	{
		// Cancel any active key-grab mode so the callback does not fire
		// into a hidden (or later destroyed) KeyBindList. This handles the
		// case where ESC hides the dialog before the grab callback fires.
		VGUI_SetKeyGrabCallback(0);
		s_activeKeyBindList = 0;

		App* app = App::getInstance();
		if (app) app->requestFocus(null);
		UI_EnableTextInput(false);
	}
	Frame::setVisible(state);
}

void VguiOptionsDialog::applyAll()
{
	for (int i = 0; i < _checkButtons.getCount(); i++)
		if (_checkButtons[i]) _checkButtons[i]->apply();
	for (int i = 0; i < _sliders.getCount(); i++)
		if (_sliders[i]) _sliders[i]->apply();
	for (int i = 0; i < _textEntries.getCount(); i++)
		if (_textEntries[i]) _textEntries[i]->apply();
	// Custom widgets not in typed Dars
	if (s_invertMouseBtn) s_invertMouseBtn->apply();
	if (s_resCombo) s_resCombo->apply();
	if (s_fsCombo) s_fsCombo->apply();
	setDirty(false);
}

void VguiOptionsDialog::resetAll()
{
	for (int i = 0; i < _checkButtons.getCount(); i++)
		if (_checkButtons[i]) _checkButtons[i]->reset();
	for (int i = 0; i < _sliders.getCount(); i++)
		if (_sliders[i]) _sliders[i]->reset();
	for (int i = 0; i < _textEntries.getCount(); i++)
		if (_textEntries[i]) _textEntries[i]->reset();
	// Custom widgets not in typed Dars
	if (s_invertMouseBtn) s_invertMouseBtn->reset();
	if (s_resCombo) s_resCombo->reset();
	if (s_fsCombo) s_fsCombo->reset();
	setDirty(false);
}


// ====================================================================
// Tab builders - GoldSrc pixel-perfect grid layout
// ====================================================================

void VguiOptionsDialog::buildMultiplayerTab(Panel* page)
{
	// GoldSrc CS 1.6 Multiplayer tab reference layout (@ 520x380 dialog,
	// page area ~500x300):
	//   Left column (x=10..230): Avatar group, Logo group, hint, Advanced
	//   Right column (x=240..490): Player name, VIP password
	//
	// Each group: label (12px tall, light grey), preview box (56x56), then
	// action buttons aligned to vertical center of the preview box.
	int leftX   = VS(10);
	int rightX  = VS(240);
	int colW    = VS(240);
	int slotSz  = VS(56);    // smaller preview boxes (GoldSrc proportion)
	int btnW    = VS(110);
	int btnH    = VS(22);
	int gap     = VS(6);

	// ---- Left column: AVATAR group ----
	int y = VS(8);
	page->addChild(new Label("\xD0\x90\xD0\xB2\xD0\xB0\xD1\x82\xD0\xB0\xD1\x80",
		leftX, y, VS(60), VS(14)));
	y += VS(14);
	page->addChild(new PreviewBox(leftX, y, slotSz, slotSz));
	// Avatar action buttons: flush with TOP and BOTTOM of the preview box,
	// not vertically centered as a group. This is the canon CS 1.6 layout -
	// the buttons line up exactly with the slot's top/bottom edges so they
	// frame it instead of looking "trapped" in dead space beside it.
	int actX = leftX + slotSz + gap;
	int btnTopY = y;
	int btnBotY = y + slotSz - btnH;
	page->addChild(new Button("\xD0\x97\xD0\xB0\xD0\xB3\xD1\x80\xD1\x83\xD0\xB7\xD0\xB8\xD1\x82\xD1\x8C...",
		actX, btnTopY, btnW, btnH));
	static const char* k_teams[] = { "cts_team", "ts_team", "vip_team", "admin_team" };
	StubComboButton* teamCombo = new StubComboButton("logo_team", k_teams, 4,
		actX, btnBotY, btnW, btnH);
	teamCombo->addActionSignal(new MarkDirtyActionSignal(this));
	page->addChild(teamCombo);
	y += slotSz + VS(10);

	// ---- Left column: LOGO group ----
	page->addChild(new Label("\xD0\x9B\xD0\xBE\xD0\xB3\xD0\xBE\xD1\x82\xD0\xB8\xD0\xBF",
		leftX, y, VS(60), VS(14)));
	y += VS(14);
	page->addChild(new PreviewBox(leftX, y, slotSz, slotSz));
	int btn2TopY = y;
	int btn2BotY = y + slotSz - btnH;
	static const char* k_logos[] = { "lambda", "skull", "ts_team", "cts_team", "n0!se" };
	StubComboButton* logoCombo = new StubComboButton("cl_logofile", k_logos, 5,
		actX, btn2TopY, btnW, btnH);
	logoCombo->addActionSignal(new MarkDirtyActionSignal(this));
	page->addChild(logoCombo);
	page->addChild(new Button(
		"\xD0\x98\xD0\xB7\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xB8\xD1\x82\xD1\x8C \xD1\x86\xD0\xB2\xD0\xB5\xD1\x82",
		actX, btn2BotY, btnW, btnH));
	y += slotSz + VS(8);

	// Dim hint label, single line if it fits
	page->addChild(new Label(
		"\xD0\x9B\xD0\xBE\xD0\xB3\xD0\xBE\xD1\x82\xD0\xB8\xD0\xBF \xD0\xB8\xD0\xB7\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xB8\xD1\x82\xD1\x81\xD1\x8F \xD0\xBF\xD0\xBE\xD1\x81\xD0\xBB\xD0\xB5 \xD1\x81\xD0\xBE\xD0\xB5\xD0\xB4\xD0\xB8\xD0\xBD\xD0\xB5\xD0\xBD\xD0\xB8\xD1\x8F.",
		leftX, y, colW, VS(14)));
	y += VS(20);
	page->addChild(new Button(
		"\xD0\x94\xD0\xBE\xD0\xBF\xD0\xBE\xD0\xBB\xD0\xBD\xD0\xB8\xD1\x82\xD0\xB5\xD0\xBB\xD1\x8C\xD0\xBD\xD0\xBE...",
		leftX, y, btnW, btnH));

	// ---- Right column: Имя игрока + VIP/Admin пароль ----
	int ry = VS(8);
	int fieldW = VS(220);
	page->addChild(new Label("\xD0\x98\xD0\xBC\xD1\x8F \xD0\xB8\xD0\xB3\xD1\x80\xD0\xBE\xD0\xBA\xD0\xB0",
		rightX, ry, VS(140), VS(14)));
	ry += VS(16);
	CvarTextEntry* nameEntry = new CvarTextEntry("name", rightX, ry, fieldW, FldH());
	page->addChild(nameEntry);
	_textEntries.addElement(nameEntry);
	ry += VS(34);

	page->addChild(new Label(
		"\xD0\x9F\xD0\xB0\xD1\x80\xD0\xBE\xD0\xBB\xD1\x8C VIP/Admin",
		rightX, ry, VS(180), VS(14)));
	ry += VS(16);
	PasswordTextEntry* pwdEntry = new PasswordTextEntry("vip_password", rightX, ry, fieldW, FldH());
	page->addChild(pwdEntry);
	_textEntries.addElement(pwdEntry);
}


// ====================================================================
// Keyboard tab - key binding classes and data
// ====================================================================

struct KeyBindDef {
	const char* actionName; // UTF-8 Russian
	const char* command;    // engine command
	const char* defaultKey; // default key name (for EngFuncs::ClientCmd bind)
};

static const KeyBindDef s_keyBindings[] = {
	{"\xD0\x92\xD0\xBF\xD0\xB5\xD1\x80\xD0\xB5\xD0\xB4", "+forward", "W"},
	{"\xD0\x9D\xD0\xB0\xD0\xB7\xD0\xB0\xD0\xB4", "+back", "S"},
	{"\xD0\x92\xD0\xBB\xD0\xB5\xD0\xB2\xD0\xBE", "+moveleft", "A"},
	{"\xD0\x92\xD0\xBF\xD1\x80\xD0\xB0\xD0\xB2\xD0\xBE", "+moveright", "D"},
	{"\xD0\x9F\xD1\x80\xD1\x8B\xD0\xB6\xD0\xBE\xD0\xBA", "+jump", "SPACE"},
	{"\xD0\x9F\xD1\x80\xD0\xB8\xD1\x81\xD0\xB5\xD1\x81\xD1\x82\xD1\x8C", "+duck", "CTRL"},
	{"\xD0\x9F\xD0\xB5\xD1\x80\xD0\xB5\xD0\xB7\xD0\xB0\xD1\x80\xD1\x8F\xD0\xB4\xD0\xBA\xD0\xB0", "+reload", "R"},
	{"\xD0\x98\xD1\x81\xD0\xBF\xD0\xBE\xD0\xBB\xD1\x8C\xD0\xB7\xD0\xBE\xD0\xB2\xD0\xB0\xD1\x82\xD1\x8C", "+use", "E"},
	{"\xD0\x9E\xD1\x80\xD1\x83\xD0\xB6\xD0\xB8\xD0\xB5 1", "slot1", "1"},
	{"\xD0\x9E\xD1\x80\xD1\x83\xD0\xB6\xD0\xB8\xD0\xB5 2", "slot2", "2"},
	{"\xD0\x9E\xD1\x80\xD1\x83\xD0\xB6\xD0\xB8\xD0\xB5 3", "slot3", "3"},
	{"\xD0\x9E\xD1\x80\xD1\x83\xD0\xB6\xD0\xB8\xD0\xB5 4", "slot4", "4"},
	{"\xD0\x9E\xD1\x80\xD1\x83\xD0\xB6\xD0\xB8\xD0\xB5 5", "slot5", "5"},
	{"\xD0\x90\xD1\x82\xD0\xB0\xD0\xBA\xD0\xB0", "+attack", "MOUSE1"},
	{"\xD0\x90\xD0\xBB\xD1\x8C\xD1\x82. \xD0\xB0\xD1\x82\xD0\xB0\xD0\xBA\xD0\xB0", "+attack2", "MOUSE2"},
	{"\xD0\x9E\xD0\xB1\xD1\x89\xD0\xB8\xD0\xB9 \xD1\x87\xD0\xB0\xD1\x82", "messagemode", "Y"},
	{"\xD0\x9A\xD0\xBE\xD0\xBC\xD0\xB0\xD0\xBD\xD0\xB4\xD0\xBD\xD1\x8B\xD0\xB9 \xD1\x87\xD0\xB0\xD1\x82", "messagemode2", "U"},
	{"\xD0\x9A\xD0\xBE\xD0\xBD\xD1\x81\xD0\xBE\xD0\xBB\xD1\x8C", "toggleconsole", "`"},
	{"\xD0\xA1\xD0\xBA\xD1\x80\xD0\xB8\xD0\xBD\xD1\x88\xD0\xBE\xD1\x82", "screenshot", "F5"},
	{"\xD0\xA2\xD0\xB0\xD0\xB1\xD0\xBB\xD0\xBE", "+showscores", "TAB"},
	{"\xD0\xA4\xD0\xBE\xD0\xBD\xD0\xB0\xD1\x80\xD0\xB8\xD0\xBA", "impulse 100", "F"},
	{"\xD0\xA1\xD0\xBF\xD1\x80\xD0\xB5\xD0\xB9", "impulse 201", "T"},
	{"\xD0\x92\xD1\x8B\xD0\xB1\xD1\x80\xD0\xBE\xD1\x81\xD0\xB8\xD1\x82\xD1\x8C", "drop", "G"},
	{"\xD0\x9A\xD1\x83\xD0\xBF\xD0\xB8\xD1\x82\xD1\x8C", "buy", "B"},
	{"\xD0\xA0\xD0\xB0\xD0\xB4\xD0\xB8\xD0\xBE", "radio1", "Z"},
};
static const int NUM_KEY_BINDINGS = sizeof(s_keyBindings) / sizeof(s_keyBindings[0]);

class KeyBindRow : public Panel
{
public:
	KeyBindRow(const char* action, int x, int y, int wide, int tall, int splitX)
		: Panel(x, y, wide, tall), _splitX(splitX)
	{
		vgui_strcpy(_action, sizeof(_action), action);
		_key[0] = 0;
	}
	void setKeyText(const char* key)
	{
		vgui_strcpy(_key, sizeof(_key), key ? key : "");
		repaint();
	}
	const char* getKeyText() const { return _key; }
protected:
	virtual void paint()
	{
		int wide, tall;
		getSize(wide, tall);
		unsigned int textCol = g_Scheme.fieldTextColor ? g_Scheme.fieldTextColor : 0xFFFFFFFF;
		schemeFgColor(this, textCol);
		drawSetTextFont(Scheme::sf_primary1);
		int textY = (tall - VS(11)) / 2;
		if (textY < 1) textY = 1;
		drawPrintText(VS(4), textY, _action, (int)strlen(_action));
		drawPrintText(_splitX + VS(4), textY, _key, (int)strlen(_key));
	}
	virtual void paintBackground() {} // ListPanel handles background
private:
	char _action[128];
	char _key[64];
	int _splitX;
};

class KeyBindList : public ListPanel
{
public:
	KeyBindList(int x, int y, int wide, int tall, int splitX)
		: ListPanel(x, y, wide, tall), _splitX(splitX), _grabIndex(-1)
	{
		// Populate rows
		for (int i = 0; i < NUM_KEY_BINDINGS; i++)
		{
			KeyBindRow* row = new KeyBindRow(s_keyBindings[i].actionName,
				0, 0, wide - 16, VS(18), _splitX);
			addItem(row);
		}
	}

	virtual ~KeyBindList()
	{
		if (s_activeKeyBindList == this)
		{
			s_activeKeyBindList = 0;
			VGUI_SetKeyGrabCallback(0);
		}
		if (s_keyBindListInstance == this)
			s_keyBindListInstance = 0;
	}

	void populateFromEngine()
	{
		for (int i = 0; i < NUM_KEY_BINDINGS; i++)
		{
			KeyBindRow* row = static_cast<KeyBindRow*>(getItem(i));
			if (!row) continue;
			const char* found = findBoundKey(s_keyBindings[i].command);
			row->setKeyText(found);
		}
	}

	void restoreDefaults()
	{
		// Unbind all keys for our commands first
		for (int i = 0; i < NUM_KEY_BINDINGS; i++)
			unbindCommand(s_keyBindings[i].command);
		// Bind defaults
		char cmd[256];
		for (int i = 0; i < NUM_KEY_BINDINGS; i++)
		{
			snprintf(cmd, sizeof(cmd), "bind \"%s\" \"%s\"\n",
				s_keyBindings[i].defaultKey, s_keyBindings[i].command);
			EngFuncs::ClientCmd(false, cmd);
		}
		populateFromEngine();
	}

	void onKeyGrabbed(int engineKeyCode)
	{
		if (_grabIndex < 0 || _grabIndex >= NUM_KEY_BINDINGS)
		{
			_grabIndex = -1;
			return;
		}
		const char* keyName = EngFuncs::KeynumToString(engineKeyCode);
		if (!keyName || !keyName[0] || strcmp(keyName, "<OUT OF RANGE>") == 0)
		{
			_grabIndex = -1;
			populateFromEngine();
			return;
		}
		// Unbind old key for this command
		unbindCommand(s_keyBindings[_grabIndex].command);
		// Bind new key
		char cmd[256];
		snprintf(cmd, sizeof(cmd), "bind \"%s\" \"%s\"\n",
			keyName, s_keyBindings[_grabIndex].command);
		EngFuncs::ClientCmd(false, cmd);
		_grabIndex = -1;
		populateFromEngine();
	}

	bool isGrabbing() const { return _grabIndex >= 0; }

protected:
	virtual void internalMousePressed(MouseCode code)
	{
		// If already grabbing, the mouse hook in vgui_main will handle it
		if (_grabIndex >= 0) return;

		// Let ListPanel handle selection
		ListPanel::internalMousePressed(code);

		int sel = getSelectedIndex();
		if (sel >= 0 && sel < NUM_KEY_BINDINGS)
		{
			enterGrabMode(sel);
		}
	}

private:
	void enterGrabMode(int index)
	{
		_grabIndex = index;
		// Show "???" in the key column while waiting
		KeyBindRow* row = static_cast<KeyBindRow*>(getItem(index));
		if (row) row->setKeyText("???");
		s_activeKeyBindList = this;
		VGUI_SetKeyGrabCallback(staticKeyGrabCallback);
	}

	static void staticKeyGrabCallback(int engineKeyCode)
	{
		if (s_activeKeyBindList)
			s_activeKeyBindList->onKeyGrabbed(engineKeyCode);
		s_activeKeyBindList = 0;
	}

	const char* findBoundKey(const char* command)
	{
		for (int k = 0; ; k++)
		{
			const char* str = EngFuncs::KeynumToString(k);
			if (!str || strcmp(str, "<OUT OF RANGE>") == 0)
				break;
			const char* binding = EngFuncs::KEY_GetBinding(k);
			if (binding && binding[0])
			{
				if (stricmp(binding, command) == 0)
					return str;
			}
		}
		return "";
	}

	void unbindCommand(const char* command)
	{
		for (int k = 0; ; k++)
		{
			const char* str = EngFuncs::KeynumToString(k);
			if (!str || strcmp(str, "<OUT OF RANGE>") == 0)
				break;
			const char* binding = EngFuncs::KEY_GetBinding(k);
			if (binding && binding[0] && stricmp(binding, command) == 0)
				EngFuncs::KEY_SetBinding(k, "");
		}
	}

	int _splitX;
	int _grabIndex;
};

// ActionSignal for the Defaults button
class KbDefaultsSignal : public ActionSignal
{
public:
	KbDefaultsSignal(KeyBindList* list) : _list(list) {}
	virtual void actionPerformed(Panel* panel) { if (_list) _list->restoreDefaults(); }
private:
	KeyBindList* _list;
};

void VguiOptionsDialog::buildKeyboardTab(Panel* page)
{
	int pageW, pageH;
	page->getSize(pageW, pageH);

	// Column headers
	int headerY = VS(4);
	int headerH = VS(16);
	int listX = VS(8);
	int listW = pageW - VS(16);
	int splitX = listW * 60 / 100; // 60% for action, 40% for key

	Label* hdrAction = new Label(
		"\xD0\x94\xD0\xB5\xD0\xB9\xD1\x81\xD1\x82\xD0\xB2\xD0\xB8\xD0\xB5",
		listX + VS(4), headerY, splitX, headerH);
	page->addChild(hdrAction);

	Label* hdrKey = new Label(
		"\xD0\x9A\xD0\xBB\xD0\xB0\xD0\xB2\xD0\xB8\xD1\x88\xD0\xB0",
		listX + splitX + VS(4), headerY, listW - splitX, headerH);
	page->addChild(hdrKey);

	// Key bind list
	int listY = headerY + headerH + VS(2);
	int btnH = VS(24);
	int listH = pageH - listY - btnH - VS(12);

	KeyBindList* kbList = new KeyBindList(listX, listY, listW, listH, splitX);
	page->addChild(kbList);
	kbList->populateFromEngine();
	s_keyBindListInstance = kbList;

	// Defaults button
	int btnW = VS(120);
	int btnX = listX;
	int btnY = listY + listH + VS(4);
	Button* defBtn = new Button(
		"\xD0\x9F\xD0\xBE \xD1\x83\xD0\xBC\xD0\xBE\xD0\xBB\xD1\x87\xD0\xB0\xD0\xBD\xD0\xB8\xD1\x8E",
		btnX, btnY, btnW, btnH);
	defBtn->addActionSignal(new KbDefaultsSignal(kbList));
	page->addChild(defBtn);
}

void VguiOptionsDialog::buildMouseTab(Panel* page)
{
	int y = FirstY();

	// Row 1: Invert mouse (m_pitch sign flip)
	InvertMouseCheckButton* invertBtn = new InvertMouseCheckButton(
		"\xD0\x98\xD0\xBD\xD0\xB2\xD0\xB5\xD1\x80\xD1\x82\xD0\xB8\xD1\x80\xD0\xBE\xD0\xB2\xD0\xB0\xD1\x82\xD1\x8C \xD0\xBC\xD1\x8B\xD1\x88\xD1\x8C",
		LblX(), y, VS(200), FldH());
	page->addChild(invertBtn);
	invertBtn->addActionSignal(new MarkDirtyActionSignal(this));
	s_invertMouseBtn = invertBtn;
	y += RowH();

	// Row 2: Sensitivity slider
	page->addChild(new Label("\xD0\xA7\xD1\x83\xD0\xB2\xD1\x81\xD1\x82\xD0\xB2\xD0\xB8\xD1\x82\xD0\xB5\xD0\xBB\xD1\x8C\xD0\xBD\xD0\xBE\xD1\x81\xD1\x82\xD1\x8C:",
		LblX(), y, LblW(), FldH()));
	CvarSlider* sensSlider = new CvarSlider("sensitivity", InpX(), y, InpW(), FldH(), 1, 20);
	page->addChild(sensSlider); _sliders.addElement(sensSlider);
	y += RowH();

	// Row 3: Zoom sensitivity slider
	page->addChild(new Label("\xD0\xA7\xD1\x83\xD0\xB2\xD1\x81\xD1\x82\xD0\xB2. \xD0\xB7\xD1\x83\xD0\xBC\xD0\xB0:",
		LblX(), y, LblW(), FldH()));
	CvarSlider* zoomSlider = new CvarSlider("zoom_sensitivity_ratio", InpX(), y, InpW(), FldH(), 0, 100, 0.0f, 2.0f);
	page->addChild(zoomSlider); _sliders.addElement(zoomSlider);
	y += RowH();

	// Row 4: Mouse filter
	CvarCheckButton* filter = new CvarCheckButton("m_filter",
		"\xD0\xA4\xD0\xB8\xD0\xBB\xD1\x8C\xD1\x82\xD1\x80 \xD0\xBC\xD1\x8B\xD1\x88\xD0\xB8",
		LblX(), y, VS(200), FldH());
	page->addChild(filter); _checkButtons.addElement(filter);
	y += RowH();

	// Row 5: Raw input
	CvarCheckButton* rawinput = new CvarCheckButton("m_rawinput",
		"\xD0\x9F\xD1\x80\xD1\x8F\xD0\xBC\xD0\xBE\xD0\xB9 \xD0\xB2\xD0\xB2\xD0\xBE\xD0\xB4",
		LblX(), y, VS(200), FldH());
	page->addChild(rawinput); _checkButtons.addElement(rawinput);
	y += RowH();

	// Row 6: Custom acceleration
	CvarCheckButton* customaccel = new CvarCheckButton("m_customaccel",
		"\xD0\xA1\xD0\xB2\xD0\xBE\xD1\x91 \xD1\x83\xD1\x81\xD0\xBA\xD0\xBE\xD1\x80\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB5",
		LblX(), y, VS(200), FldH());
	page->addChild(customaccel); _checkButtons.addElement(customaccel);
}

void VguiOptionsDialog::buildAudioTab(Panel* page)
{
	int y = FirstY();

	page->addChild(new Label("\xD0\x93\xD1\x80\xD0\xBE\xD0\xBC\xD0\xBA\xD0\xBE\xD1\x81\xD1\x82\xD1\x8C:",
		LblX(), y, LblW(), FldH()));
	CvarSlider* volSlider = new CvarSlider("volume", InpX(), y, InpW(), FldH(), 0, 100, 0.0f, 1.0f);
	page->addChild(volSlider); _sliders.addElement(volSlider);
	y += RowH();

	page->addChild(new Label("\xD0\x93\xD1\x80\xD0\xBE\xD0\xBC\xD0\xBA\xD0\xBE\xD1\x81\xD1\x82\xD1\x8C HEV:",
		LblX(), y, LblW(), FldH()));
	CvarSlider* suitSlider = new CvarSlider("suitvolume", InpX(), y, InpW(), FldH(), 0, 100, 0.0f, 1.0f);
	page->addChild(suitSlider); _sliders.addElement(suitSlider);
	y += RowH();

	CvarCheckButton* a3d = new CvarCheckButton("s_a3d", "A3D Audio",
		LblX(), y, VS(200), FldH());
	page->addChild(a3d); _checkButtons.addElement(a3d);
	y += RowH();

	CvarCheckButton* eax = new CvarCheckButton("s_eax",
		"\xD0\xAD\xD1\x84\xD1\x84\xD0\xB5\xD0\xBA\xD1\x82\xD1\x8B EAX",
		LblX(), y, VS(200), FldH());
	page->addChild(eax); _checkButtons.addElement(eax);
}


void VguiOptionsDialog::buildVideoTab(Panel* page)
{
	int y = FirstY();

	// Static data for resolution combo
	static const char* k_resLabels[] = {"640x480", "800x600", "1024x768", "1280x720", "1280x1024", "1366x768", "1600x900", "1920x1080"};
	static const int k_resValues[] = {0, 1, 2, 3, 4, 5, 6, 7};

	// Static data for fullscreen combo
	static const char* k_fsLabels[] = {"\xD0\x9E\xD0\xBA\xD0\xBE\xD0\xBD\xD0\xBD\xD1\x8B\xD0\xB9", "\xD0\x9F\xD0\xBE\xD0\xBB\xD0\xBD\xD1\x8B\xD0\xB9 \xD1\x8D\xD0\xBA\xD1\x80\xD0\xB0\xD0\xBD"};
	static const int k_fsValues[] = {0, 1};

	// Row 1: Resolution combo
	page->addChild(new Label("\xD0\xA0\xD0\xB0\xD0\xB7\xD1\x80\xD0\xB5\xD1\x88\xD0\xB5\xD0\xBD\xD0\xB8\xD0\xB5:",
		LblX(), y, LblW(), FldH()));
	IntStubComboButton* resCombo = new IntStubComboButton("vid_mode", k_resLabels, k_resValues, 8,
		InpX(), y, InpW(), FldH());
	page->addChild(resCombo);
	resCombo->addActionSignal(new MarkDirtyActionSignal(this));
	s_resCombo = resCombo;
	y += RowH();

	// Row 2: Display mode combo
	page->addChild(new Label("\xD0\xA0\xD0\xB5\xD0\xB6\xD0\xB8\xD0\xBC \xD1\x8D\xD0\xBA\xD1\x80\xD0\xB0\xD0\xBD\xD0\xB0:",
		LblX(), y, LblW(), FldH()));
	IntStubComboButton* fsCombo = new IntStubComboButton("fullscreen", k_fsLabels, k_fsValues, 2,
		InpX(), y, InpW(), FldH());
	page->addChild(fsCombo);
	fsCombo->addActionSignal(new MarkDirtyActionSignal(this));
	s_fsCombo = fsCombo;
	y += RowH();

	// Row 3: Gamma slider
	page->addChild(new Label("\xD0\x93\xD0\xB0\xD0\xBC\xD0\xBC\xD0\xB0:",
		LblX(), y, LblW(), FldH()));
	CvarSlider* gammaSlider = new CvarSlider("gamma", InpX(), y, InpW(), FldH(), 0, 100, 1.8f, 3.0f);
	page->addChild(gammaSlider); _sliders.addElement(gammaSlider);
	y += RowH();

	// Row 4: Brightness slider
	page->addChild(new Label("\xD0\xAF\xD1\x80\xD0\xBA\xD0\xBE\xD1\x81\xD1\x82\xD1\x8C:",
		LblX(), y, LblW(), FldH()));
	CvarSlider* brightSlider = new CvarSlider("brightness", InpX(), y, InpW(), FldH(), 0, 100, 0.0f, 2.0f);
	page->addChild(brightSlider); _sliders.addElement(brightSlider);
	y += RowH();

	// Row 5: VSync checkbox
	CvarCheckButton* vsync = new CvarCheckButton("gl_vsync",
		"\xD0\x92\xD0\xB5\xD1\x80\xD1\x82. \xD1\x81\xD0\xB8\xD0\xBD\xD1\x85\xD1\x80\xD0\xBE\xD0\xBD\xD0\xB8\xD0\xB7\xD0\xB0\xD1\x86\xD0\xB8\xD1\x8F",
		LblX(), y, VS(200), FldH());
	page->addChild(vsync); _checkButtons.addElement(vsync);
}

void VguiOptionsDialog::buildHudTab(Panel* page)
{
	int y = FirstY();

	CvarCheckButton* hudDraw = new CvarCheckButton("hud_draw",
		"\xD0\xA0\xD0\xB8\xD1\x81\xD0\xBE\xD0\xB2\xD0\xB0\xD1\x82\xD1\x8C HUD",
		LblX(), y, VS(200), FldH());
	page->addChild(hudDraw); _checkButtons.addElement(hudDraw);
	y += RowH();

	CvarCheckButton* showFps = new CvarCheckButton("cl_showfps",
		"\xD0\x9F\xD0\xBE\xD0\xBA\xD0\xB0\xD0\xB7\xD1\x8B\xD0\xB2\xD0\xB0\xD1\x82\xD1\x8C FPS",
		LblX(), y, VS(200), FldH());
	page->addChild(showFps); _checkButtons.addElement(showFps);
	y += RowH();

	page->addChild(new Label("\xD0\x9C\xD0\xB0\xD1\x81\xD1\x88\xD1\x82\xD0\xB0\xD0\xB1 HUD:",
		LblX(), y, LblW(), FldH()));
	CvarSlider* scaleSlider = new CvarSlider("hud_scale", InpX(), y, InpW(), FldH(), 0, 10, 0.0f, 2.0f);
	page->addChild(scaleSlider); _sliders.addElement(scaleSlider);
	y += RowH();

	CvarCheckButton* crosshair = new CvarCheckButton("crosshair",
		"\xD0\x9F\xD0\xBE\xD0\xBA\xD0\xB0\xD0\xB7\xD1\x8B\xD0\xB2\xD0\xB0\xD1\x82\xD1\x8C \xD0\xBF\xD1\x80\xD0\xB8\xD1\x86\xD0\xB5\xD0\xBB",
		LblX(), y, VS(200), FldH());
	page->addChild(crosshair); _checkButtons.addElement(crosshair);
}

void VguiOptionsDialog::buildAccountTab(Panel* page)
{
	page->addChild(new Label("\xD0\x9D\xD0\xB0\xD1\x81\xD1\x82\xD1\x80\xD0\xBE\xD0\xB9\xD0\xBA\xD0\xB8 \xD0\xB0\xD0\xBA\xD0\xBA\xD0\xB0\xD1\x83\xD0\xBD\xD1\x82\xD0\xB0",
		LblX(), FirstY(), VS(180), FldH()));
}

void VguiOptionsDialog::buildSystemTab(Panel* page)
{
	CvarCheckButton* dev = new CvarCheckButton("developer",
		"\xD0\xA0\xD0\xB5\xD0\xB6\xD0\xB8\xD0\xBC \xD1\x80\xD0\xB0\xD0\xB7\xD1\x80\xD0\xB0\xD0\xB1\xD0\xBE\xD1\x82\xD1\x87\xD0\xB8\xD0\xBA\xD0\xB0",
		LblX(), FirstY(), VS(200), FldH());
	page->addChild(dev); _checkButtons.addElement(dev);
}


// ====================================================================
// Global dialog instance and exported functions
// ====================================================================

static VguiOptionsDialog* g_pOptionsDialog = null;

} // namespace vgui

void VGUI_OptionsShutdown(void)
{
	vgui::g_pOptionsDialog = null;
	vgui::s_keyBindListInstance = 0;
	vgui::s_activeKeyBindList = 0;
	vgui::s_invertMouseBtn = 0;
	vgui::s_resCombo = 0;
	vgui::s_fsCombo = 0;
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

	VGUI_EnsureInitialized(sw, sh);

	vgui::Panel* root = vgui::VGUI_GetRootPanel();
	if (!root) return;

	// Keep the root canvas matched to the CURRENT screen size. The root panel
	// is created once and would otherwise keep its initial dimensions forever;
	// after a resolution change that stale size desyncs drawing, hit-testing
	// and the drag/resize clamps from the real screen. One coordinate space.
	root->setSize(sw, sh);

	if (!vgui::g_pOptionsDialog)
	{
		vgui::g_pOptionsDialog = new vgui::VguiOptionsDialog(sw, sh);
		root->addChild(vgui::g_pOptionsDialog);
	}

	// Re-clamp to the CURRENT screen on every show: the resolution may have
	// changed since the dialog was created, and it must never start larger
	// than the display. Shrink to fit (keep an 8px margin), then center.
	{
		int margin = vgui::VS(8);
		int dlgW, dlgH;
		vgui::g_pOptionsDialog->getSize(dlgW, dlgH);
		if (dlgW > sw - margin) dlgW = sw - margin;
		if (dlgH > sh - margin) dlgH = sh - margin;
		if (dlgW < 1) dlgW = sw;
		if (dlgH < 1) dlgH = sh;
		vgui::g_pOptionsDialog->setSize(dlgW, dlgH);
		vgui::g_pOptionsDialog->setPos((sw - dlgW) / 2, (sh - dlgH) / 2);
	}

	vgui::g_pOptionsDialog->resetAll();
	// Refresh key bindings on re-open (clears stale "???" from cancelled grabs)
	if (vgui::s_keyBindListInstance)
		vgui::s_keyBindListInstance->populateFromEngine();
	vgui::g_pOptionsDialog->setVisible(true);

	// Diagnostic snapshot: everything needed to explain "window not visible".
	// If the dialog is created, visible, sized and parented to a root that the
	// paint loop traverses, but nothing shows, the fault is downstream (paint
	// not called / z-order / engine menu on top) rather than in construction.
	{
		int dx = 0, dy = 0, dw = 0, dh = 0;
		vgui::g_pOptionsDialog->getPos(dx, dy);
		vgui::g_pOptionsDialog->getSize(dw, dh);
		VLOG("VGUI_ShowOptions EXIT: dlg=%p vis=%d pos=(%d,%d) size=(%dx%d) rootChildren=%d screen=(%dx%d)",
			(void*)vgui::g_pOptionsDialog,
			vgui::g_pOptionsDialog->isVisible() ? 1 : 0,
			dx, dy, dw, dh, root->getChildCount(), sw, sh);
		VLOGFLUSH();
	}
}

OPTDLG_EXPORT void VGUI_HideOptions(void)
{
	if (vgui::g_pOptionsDialog)
		vgui::g_pOptionsDialog->setVisible(false);
}

} // extern "C"
