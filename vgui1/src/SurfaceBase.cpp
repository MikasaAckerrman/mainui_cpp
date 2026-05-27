#include <VGUI_SurfaceBase.h>
#include <VGUI_Panel.h>
#include <VGUI_App.h>
#include <VGUI_Cursor.h>
#include <VGUI_ImagePanel.h>
#include <string.h>

namespace vgui
{

SurfaceBase::SurfaceBase(Panel* embeddedPanel)
{
	_embeddedPanel = embeddedPanel;
	_needsSwap = false;
	_app = App::getInstance();
	_currentCursor = null;
	_emulatedCursor = null;
	if (_app)
		_app->surfaceBaseCreated(this);
	if (_embeddedPanel)
		_embeddedPanel->setSurfaceBaseTraverse(this);
}

SurfaceBase::~SurfaceBase()
{
	if (_app)
		_app->surfaceBaseDeleted(this);
}

Panel* SurfaceBase::getPanel()
{
	return _embeddedPanel;
}

void SurfaceBase::requestSwap()
{
	_needsSwap = true;
}

void SurfaceBase::resetModeInfo()
{
	_modeInfoDar.removeAll();
}

int SurfaceBase::getModeInfoCount()
{
	return _modeInfoDar.getCount();
}

bool SurfaceBase::getModeInfo(int mode, int& wide, int& tall, int& bpp)
{
	if (mode < 0 || mode >= _modeInfoDar.getCount())
		return false;
	char* info = _modeInfoDar[mode];
	if (!info)
		return false;
	memcpy(&wide, info, sizeof(int));
	memcpy(&tall, info + sizeof(int), sizeof(int));
	memcpy(&bpp, info + sizeof(int) * 2, sizeof(int));
	return true;
}

App* SurfaceBase::getApp()
{
	return _app;
}

void SurfaceBase::setEmulatedCursorVisible(bool state)
{
	if (_emulatedCursor)
		_emulatedCursor->setVisible(state);
}

void SurfaceBase::setEmulatedCursorPos(int x, int y)
{
	if (_emulatedCursor)
		_emulatedCursor->setPos(x, y);
}

void SurfaceBase::addModeInfo(int wide, int tall, int bpp)
{
	char* info = new char[sizeof(int) * 3];
	memcpy(info, &wide, sizeof(int));
	memcpy(info + sizeof(int), &tall, sizeof(int));
	memcpy(info + sizeof(int) * 2, &bpp, sizeof(int));
	_modeInfoDar.addElement(info);
}

}
