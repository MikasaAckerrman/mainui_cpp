/*
WndOptions.cpp -- Source Engine-style windowed Options dialog
Copyright (C) 2024 DragonSlayer Team

CMenuFrameTabbed with Controls/Video/Audio/Game tabs.
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "controls/FrameTabbed.h"
#include "Action.h"
#include "Slider.h"
#include "CheckBox.h"
#include "keydefs.h"
#include "Utils.h"
#include "TrackerScheme.h"

class CMenuWndOptions : public CMenuFrameTabbed
{
public:
	CMenuWndOptions();
	bool IsRoot() const override { return false; }

private:
	void _Init() override;
	void _VidInit() override;

	// Controls tab
	CMenuAction controlsLabel;

	// Video tab
	CMenuSlider brightness;
	CMenuSlider gamma;

	// Audio tab
	CMenuSlider volume;
	CMenuSlider musicVolume;

	// Game tab
	CMenuAction gameLabel;
};

static CMenuWndOptions *s_pWndOptions = NULL;

CMenuWndOptions::CMenuWndOptions() : CMenuFrameTabbed( "Options" )
{
}

void CMenuWndOptions::_Init()
{
	int w = (int)(uiStatic.width * 0.6f);
	int h = (int)(768 * 0.55f);
	int x = (uiStatic.width - w) / 2;
	int y = (768 - h) / 2;
	SetRect( x, y, w, h );

	// === Controls tab ===
	AddTab( "Controls" );

	controlsLabel.szName = "Key bindings (use Options > Controls for full list)";
	controlsLabel.iFlags |= QMF_INACTIVE;
	controlsLabel.SetCoord( 16, 16 );
	AddItem( controlsLabel );

	// === Video tab ===
	AddTab( "Video" );

	brightness.szName = "Brightness";
	brightness.Setup( 0.0f, 3.0f, 0.1f );
	brightness.SetRect( 16, 16, w - 32, 32 );
	brightness.LinkCvar( "brightness" );
	AddItem( brightness );

	gamma.szName = "Gamma";
	gamma.Setup( 0.5f, 3.0f, 0.1f );
	gamma.SetRect( 16, 64, w - 32, 32 );
	gamma.LinkCvar( "gamma" );
	AddItem( gamma );

	// === Audio tab ===
	AddTab( "Audio" );

	volume.szName = "Sound Volume";
	volume.Setup( 0.0f, 1.0f, 0.05f );
	volume.SetRect( 16, 16, w - 32, 32 );
	volume.LinkCvar( "volume" );
	AddItem( volume );

	musicVolume.szName = "Music Volume";
	musicVolume.Setup( 0.0f, 1.0f, 0.05f );
	musicVolume.SetRect( 16, 64, w - 32, 32 );
	musicVolume.LinkCvar( "MP3Volume" );
	AddItem( musicVolume );

	// === Game tab ===
	AddTab( "Game" );

	gameLabel.szName = "Game settings available via full Options menu";
	gameLabel.iFlags |= QMF_INACTIVE;
	gameLabel.SetCoord( 16, 16 );
	AddItem( gameLabel );

	SetActiveTab( 0 );
}

void CMenuWndOptions::_VidInit()
{
	int w = (int)(uiStatic.width * 0.6f);
	int h = (int)(768 * 0.55f);
	int x = (uiStatic.width - w) / 2;
	int y = (768 - h) / 2;
	SetRect( x, y, w, h );
}

// ============= Public API =============

void WndOptions_Precache()
{
	s_pWndOptions = new CMenuWndOptions();
}

void WndOptions_Show()
{
	if( s_pWndOptions )
		s_pWndOptions->Show();
}

void WndOptions_Shutdown()
{
	delete s_pWndOptions;
	s_pWndOptions = NULL;
}

ADD_MENU4( menu_wndoptions, WndOptions_Precache, WndOptions_Show, WndOptions_Shutdown );
