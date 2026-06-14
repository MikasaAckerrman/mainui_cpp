#include "BaseMenu.h"
#include "FontManager.h"
extern void UI_FillRect( int x, int y, int width, int height, const unsigned int color );
extern void UI_EnableTextInput( bool enable );
#include "TrackerScheme.h"
#include <VGUI_Log.h>
#include <VGUI_SchemeColors.h>
#include <VGUI_UIScale.h>
#include <VGUI_Frame.h>
#include <VGUI_Panel.h>
#include <VGUI_Label.h>
#include <VGUI_Button.h>
#include <VGUI_App.h>
#include <VGUI_ActionSignal.h>
#include <VGUI_Scheme.h>
#include <VGUI_LoadingDialog.h>
#include <string.h>

extern "C" void VGUI_EnsureInitialized(int screenW, int screenH);

namespace vgui
{

static VguiLoadingDialog* s_loading = 0;

class ProgressBarPanel : public Panel
{
public:
	ProgressBarPanel(int x, int y, int w, int h) : Panel(x, y, w, h), _progress(0.0f) {}

	void setProgress(float p)
	{
		if (p < 0.0f) p = 0.0f;
		if (p > 1.0f) p = 1.0f;
		_progress = p;
		repaint();
	}
	float getProgress() const { return _progress; }
protected:
	virtual void paintBackground()
	{
		int wide, tall;
		getSize(wide, tall);

		unsigned int fieldBg = g_Scheme.fieldBgColor ? g_Scheme.fieldBgColor : 0xFF3E4637;
		unsigned int bright  = g_Scheme.borderBright ? g_Scheme.borderBright : 0xFF889180;
		unsigned int dark    = g_Scheme.borderDark   ? g_Scheme.borderDark   : 0xFF282E22;
		const unsigned int segColor = 0xFFB8A010;

		schemeBgColor(this, fieldBg);
		drawFilledRect(0, 0, wide, tall);

		schemeBgColor(this, dark);
		drawFilledRect(0, 0, wide, 1);
		drawFilledRect(0, 0, 1, tall);
		schemeBgColor(this, bright);
		drawFilledRect(0, tall - 1, wide, tall);
		drawFilledRect(wide - 1, 0, wide, tall);

		int segW = VS(10);
		int gap  = VS(2);
		int padY = VS(4);
		if (segW < 1) segW = 1;
		if (gap  < 1) gap  = 1;

		int innerX0 = VS(2) + 1;
		int innerX1 = wide - (VS(2) + 1);
		int innerW  = innerX1 - innerX0;
		if (innerW < segW) innerW = segW;

		int step = segW + gap;
		int totalSegments = (innerW + gap) / step;
		if (totalSegments < 1) totalSegments = 1;

		int filled = (int)(_progress * (float)totalSegments);
		if (filled > totalSegments) filled = totalSegments;

		int segTop = padY;
		int segBot = tall - padY;
		if (segBot <= segTop) segBot = segTop + 1;

		schemeBgColor(this, segColor);
		for (int i = 0; i < filled; i++)
		{
			int sx = innerX0 + i * step;
			drawFilledRect(sx, segTop, sx + segW, segBot);
		}
	}
private:
	float _progress;
};

class LoadingCancelSignal : public ActionSignal
{
public:
	LoadingCancelSignal(VguiLoadingDialog* dlg) : _dlg(dlg) {}
	virtual void actionPerformed(Panel* /*p*/) { if (_dlg) _dlg->setVisible(false); }
private:
	VguiLoadingDialog* _dlg;
};

VguiLoadingDialog::VguiLoadingDialog(int screenW, int screenH)
	: Frame(0, 0, VS(340), VS(90))
{
	_progressBar = 0;
	_statusLabel = 0;
	_cancelBtn   = 0;
	int dlgW = VS(340);
	int dlgH = VS(90);
	setPos((screenW - dlgW) / 2, (screenH - dlgH) / 2);
	setSize(dlgW, dlgH);
	setSizeable(false);
	setTitle("\xD0\x97\xD0\xB0\xD0\xB3\xD1\x80\xD1\x83\xD0\xB7\xD0\xBA\xD0\xB0...");
	setVisible(false);
	Panel* client = getClient();
	if (!client) return;
	_statusLabel = new Label("", VS(10), VS(8), VS(320), VS(16));
	_statusLabel->setContentAlignment(Label::a_west);
	_statusLabel->setFont(Scheme::sf_primary1);
	client->addChild(_statusLabel);
	_progressBar = new ProgressBarPanel(VS(10), VS(30), VS(260), VS(20));
	client->addChild(_progressBar);
	_cancelBtn = new Button("\xD0\x9E\xD1\x82\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xB0", VS(278), VS(28), VS(80), VS(24));
	_cancelBtn->addActionSignal(new LoadingCancelSignal(this));
	client->addChild(_cancelBtn);
}

void VguiLoadingDialog::setProgress(float p) { if (_progressBar) _progressBar->setProgress(p); }
void VguiLoadingDialog::setStatusText(const char* text) { if (_statusLabel) _statusLabel->setText(text ? text : ""); }

void VGUI_Loading_Show(bool show, const char* statusText, float progress)
{
	if (show)
	{
		int sw = 0, sh = 0;
		VGUI_GetScreenSize(&sw, &sh);
		if (sw <= 0) sw = 640;
		if (sh <= 0) sh = 480;
		VGUI_EnsureInitialized(sw, sh);
		Panel* root = VGUI_GetRootPanel();
		if (!root) return;
		root->setSize(sw, sh);
		if (!s_loading) { s_loading = new VguiLoadingDialog(sw, sh); root->addChild(s_loading); }
		if (statusText) s_loading->setStatusText(statusText);
		s_loading->setProgress(progress);
		s_loading->setVisible(true);
	}
	else { if (s_loading) s_loading->setVisible(false); }
}

bool VGUI_Loading_IsVisible() { return s_loading && s_loading->isVisible(); }

} // namespace vgui

void VGUI_LoadingShutdown(void) { vgui::s_loading = 0; }