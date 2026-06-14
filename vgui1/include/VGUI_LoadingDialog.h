#ifndef VGUI_LOADING_DIALOG_H
#define VGUI_LOADING_DIALOG_H

#include <VGUI_Frame.h>

namespace vgui
{

class ProgressBarPanel;
class Label;
class Button;

class VguiLoadingDialog : public Frame
{
public:
	VguiLoadingDialog(int screenW, int screenH);
	void setProgress(float p);
	void setStatusText(const char* text);
private:
	ProgressBarPanel* _progressBar;
	Label*            _statusLabel;
	Button*           _cancelBtn;
};

void VGUI_Loading_Show(bool show, const char* statusText, float progress);
bool VGUI_Loading_IsVisible();

}

#ifdef __cplusplus
extern "C" {
#endif

void VGUI_ShowLoading(bool show, const char* statusText, float progress);
bool VGUI_IsLoadingVisible(void);

#ifdef __cplusplus
}
#endif

#endif