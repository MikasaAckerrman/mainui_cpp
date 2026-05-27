/*
WndOptions.cpp -- CS 1.6 PC-style windowed Options dialog
Copyright (C) 2024 DragonSlayer Team

CMenuFrameTabbed with Multiplayer/Keyboard/Mouse/Audio/Video/HUD/Account/System tabs.
OK/Cancel/Apply buttons always visible at bottom (right-aligned).
Each tab has real controls linked to cvars.
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "controls/FrameTabbed.h"
#include "controls/FrameButton.h"
#include "Action.h"
#include "Slider.h"
#include "CheckBox.h"
#include "SpinControl.h"
#include "DropDown.h"
#include "Field.h"
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

	void ApplySettings();
	void CancelSettings();

	// Bottom buttons (always visible - added before tabs)
	CMenuFrameButton btnOK;
	CMenuFrameButton btnCancel;
	CMenuFrameButton btnApply;

	// Multiplayer tab
	CMenuField       playerName;
	CMenuField       adminPassword;
	CMenuAction      avatarPreview;
	CMenuFrameButton btnUpload;
	CMenuDropDownStr modelSelect;
	CMenuAction      logoPreview;
	CMenuDropDownStr logoSelect;
	CMenuFrameButton btnColor;

	// Keyboard tab
	CMenuAction    kbHint;
	CMenuCheckBox  developerConsole;

	// Mouse tab
	CMenuSlider    mouseSens;
	CMenuCheckBox  rawInput;
	CMenuCheckBox  mouseFilter;
	CMenuCheckBox  invertMouse;

	// Audio tab
	CMenuSlider    sndVolume;
	CMenuSlider    musicVolume;
	CMenuSlider    suitVolume;
	CMenuCheckBox  eax;
	CMenuCheckBox  a3d;

	// Video tab
	CMenuSlider      brightness;
	CMenuSlider      gamma;
	CMenuDropDownInt dispMode;
	CMenuCheckBox    vsync;
	CMenuCheckBox    hdr;
	CMenuSlider      fov;

	// HUD tab
	CMenuCheckBox  showHud;
	CMenuCheckBox  showWeapon;
	CMenuCheckBox  showRadar;

	// Account tab
	CMenuAction    accountHint;

	// System tab
	CMenuAction    systemHint;
	CMenuCheckBox  developerMode;
};

static CMenuWndOptions *s_pWndOptions = NULL;

CMenuWndOptions::CMenuWndOptions() : CMenuFrameTabbed( "Options" )
{
}

void CMenuWndOptions::ApplySettings()
{
	playerName.WriteCvar();
	adminPassword.WriteCvar();
	developerConsole.WriteCvar();
	mouseSens.WriteCvar();
	rawInput.WriteCvar();
	mouseFilter.WriteCvar();
	invertMouse.WriteCvar();
	sndVolume.WriteCvar();
	musicVolume.WriteCvar();
	suitVolume.WriteCvar();
	eax.WriteCvar();
	a3d.WriteCvar();
	brightness.WriteCvar();
	gamma.WriteCvar();
	dispMode.WriteCvar();
	vsync.WriteCvar();
	hdr.WriteCvar();
	fov.WriteCvar();
	showHud.WriteCvar();
	showWeapon.WriteCvar();
	showRadar.WriteCvar();
	developerMode.WriteCvar();
}

void CMenuWndOptions::CancelSettings()
{
	playerName.DiscardChanges();
	adminPassword.DiscardChanges();
	developerConsole.DiscardChanges();
	mouseSens.DiscardChanges();
	rawInput.DiscardChanges();
	mouseFilter.DiscardChanges();
	invertMouse.DiscardChanges();
	sndVolume.DiscardChanges();
	musicVolume.DiscardChanges();
	suitVolume.DiscardChanges();
	eax.DiscardChanges();
	a3d.DiscardChanges();
	brightness.DiscardChanges();
	gamma.DiscardChanges();
	dispMode.DiscardChanges();
	vsync.DiscardChanges();
	hdr.DiscardChanges();
	fov.DiscardChanges();
	showHud.DiscardChanges();
	showWeapon.DiscardChanges();
	showRadar.DiscardChanges();
	developerMode.DiscardChanges();
	Hide();
}

void CMenuWndOptions::_Init()
{
	int w = (int)(uiStatic.width * 0.72f);
	int h = (int)(768 * 0.70f);
	int x = (uiStatic.width - w) / 2;
	int y = (768 - h) / 2;
	SetRect( x, y, w, h );

	// Content area width for controls (inside frame border+padding)
	int cw = w - 32; // 16px padding each side
	int btnW = 85;
	int btnH = 25;
	int contentH = h - FRAME_TITLE_HEIGHT - FRAME_TAB_HEIGHT;
	int btnY = contentH - btnH - 10;
	int rightEdge = w - 16; // 16px padding from right frame edge

	// Bottom buttons - right-aligned, added BEFORE any tab so always visible
	// Order from right: Apply, Cancel, OK
	btnApply.szName = "Apply";
	btnApply.SetRect( rightEdge - btnW, btnY, btnW, btnH );
	SET_EVENT_MULTI( btnApply.onReleased,
	{
		CMenuWndOptions *self = (CMenuWndOptions*)pSelf->GetParent( CMenuWndOptions );
		self->ApplySettings();
	});
	AddItem( btnApply );

	btnCancel.szName = "Cancel";
	btnCancel.SetRect( rightEdge - btnW*2 - 8, btnY, btnW, btnH );
	SET_EVENT_MULTI( btnCancel.onReleased,
	{
		CMenuWndOptions *self = (CMenuWndOptions*)pSelf->GetParent( CMenuWndOptions );
		self->CancelSettings();
	});
	AddItem( btnCancel );

	btnOK.szName = "OK";
	btnOK.SetRect( rightEdge - btnW*3 - 16, btnY, btnW, btnH );
	SET_EVENT_MULTI( btnOK.onReleased,
	{
		CMenuWndOptions *self = (CMenuWndOptions*)pSelf->GetParent( CMenuWndOptions );
		self->ApplySettings();
		self->Hide();
	});
	AddItem( btnOK );

	// ---- TAB: Multiplayer ----
	AddTab( "Multiplayer" );

	// 2-column layout
	int leftColW = (int)(cw * 0.4f);
	int rightCol = 16 + leftColW + 16;
	int rightColW = cw - leftColW - 16;

	// Left column
	avatarPreview.szName = "Model Preview";
	avatarPreview.iFlags |= QMF_INACTIVE;
	avatarPreview.SetRect( 16, 8, leftColW, 100 );
	AddItem( avatarPreview );

	btnUpload.szName = "Upload";
	btnUpload.SetRect( 16, 116, 70, 22 );
	AddItem( btnUpload );

	modelSelect.szName = "";
	modelSelect.AddItem( "arctic", "arctic" );
	modelSelect.AddItem( "guerilla", "guerilla" );
	modelSelect.AddItem( "leet", "leet" );
	modelSelect.SetRect( 16, 146, leftColW, 24 );
	AddItem( modelSelect );

	logoPreview.szName = "Logo Preview";
	logoPreview.iFlags |= QMF_INACTIVE;
	logoPreview.SetRect( 16, 180, leftColW, 60 );
	AddItem( logoPreview );

	logoSelect.szName = "";
	logoSelect.AddItem( "lambda", "lambda" );
	logoSelect.AddItem( "skull", "skull" );
	logoSelect.AddItem( "cross", "cross" );
	logoSelect.SetRect( 16, 248, leftColW, 24 );
	AddItem( logoSelect );

	btnColor.szName = "Change Color";
	btnColor.SetRect( 16, 280, 100, 22 );
	AddItem( btnColor );

	// Right column
	playerName.szName = "Player Name";
	playerName.iMaxLength = 32;
	playerName.SetRect( rightCol, 8, rightColW, 28 );
	playerName.LinkCvar( "name" );
	AddItem( playerName );

	adminPassword.szName = "VIP/Admin Password";
	adminPassword.iMaxLength = 64;
	adminPassword.bHideInput = true;
	adminPassword.SetRect( rightCol, 52, rightColW, 28 );
	adminPassword.LinkCvar( "password" );
	AddItem( adminPassword );

	// ---- TAB: Keyboard ----
	AddTab( "Keyboard" );

	kbHint.szName = "Use console for key binds: bind <key> <command>";
	kbHint.iFlags |= QMF_INACTIVE;
	kbHint.SetCoord( 16, 8 );
	AddItem( kbHint );

	developerConsole.szName = "Enable Developer Console (~)";
	developerConsole.SetCoord( 16, 42 );
	developerConsole.LinkCvar( "con_enable" );
	AddItem( developerConsole );

	// ---- TAB: Mouse ----
	AddTab( "Mouse" );

	mouseSens.szName = "Mouse Sensitivity";
	mouseSens.Setup( 0.5f, 20.0f, 0.5f );
	mouseSens.SetRect( 16, 8, cw, 28 );
	mouseSens.LinkCvar( "sensitivity" );
	AddItem( mouseSens );

	rawInput.szName = "Raw Input";
	rawInput.SetCoord( 16, 48 );
	rawInput.LinkCvar( "m_rawinput" );
	AddItem( rawInput );

	mouseFilter.szName = "Mouse Filter";
	mouseFilter.SetCoord( 16, 76 );
	mouseFilter.LinkCvar( "m_filter" );
	AddItem( mouseFilter );

	invertMouse.szName = "Invert Mouse";
	invertMouse.SetCoord( 16, 104 );
	invertMouse.LinkCvar( "lookspring" );
	AddItem( invertMouse );

	// ---- TAB: Audio ----
	AddTab( "Audio" );

	sndVolume.szName = "Sound Effects Volume";
	sndVolume.Setup( 0.0f, 1.0f, 0.05f );
	sndVolume.SetRect( 16, 8, cw, 28 );
	sndVolume.LinkCvar( "volume" );
	AddItem( sndVolume );

	musicVolume.szName = "MP3 Volume";
	musicVolume.Setup( 0.0f, 1.0f, 0.05f );
	musicVolume.SetRect( 16, 48, cw, 28 );
	musicVolume.LinkCvar( "MP3Volume" );
	AddItem( musicVolume );

	suitVolume.szName = "HEV Suit Volume";
	suitVolume.Setup( 0.0f, 1.0f, 0.05f );
	suitVolume.SetRect( 16, 88, cw, 28 );
	suitVolume.LinkCvar( "suitvolume" );
	AddItem( suitVolume );

	eax.szName = "Enable EAX (requires EAX-capable card)";
	eax.SetCoord( 16, 128 );
	eax.LinkCvar( "s_eax" );
	AddItem( eax );

	a3d.szName = "Enable A3D";
	a3d.SetCoord( 16, 156 );
	a3d.LinkCvar( "s_a3d" );
	AddItem( a3d );

	// ---- TAB: Video ----
	AddTab( "Video" );

	brightness.szName = "Brightness";
	brightness.Setup( 0.0f, 3.0f, 0.1f );
	brightness.SetRect( 16, 8, cw, 28 );
	brightness.LinkCvar( "brightness" );
	AddItem( brightness );

	gamma.szName = "Gamma";
	gamma.Setup( 0.5f, 3.0f, 0.1f );
	gamma.SetRect( 16, 48, cw, 28 );
	gamma.LinkCvar( "gamma" );
	AddItem( gamma );

	dispMode.szName = "Display Mode";
	dispMode.AddItem( "Fullscreen", 1 );
	dispMode.AddItem( "Windowed", 0 );
	dispMode.SetRect( 16, 88, 200, 28 );
	dispMode.LinkCvar( "fullscreen", CMenuEditable::CVAR_VALUE );
	AddItem( dispMode );

	vsync.szName = "V-Sync";
	vsync.SetCoord( 16, 128 );
	vsync.LinkCvar( "gl_vsync" );
	AddItem( vsync );

	hdr.szName = "Overbright";
	hdr.SetCoord( 16, 156 );
	hdr.LinkCvar( "gl_overbright" );
	AddItem( hdr );

	fov.szName = "Field of View";
	fov.Setup( 70.0f, 120.0f, 5.0f );
	fov.SetRect( 16, 188, cw, 28 );
	fov.LinkCvar( "default_fov" );
	AddItem( fov );

	// ---- TAB: HUD ----
	AddTab( "HUD" );

	showHud.szName = "Draw HUD";
	showHud.SetCoord( 16, 8 );
	showHud.LinkCvar( "hud_draw" );
	AddItem( showHud );

	showWeapon.szName = "Show Weapon Model";
	showWeapon.SetCoord( 16, 36 );
	showWeapon.LinkCvar( "r_drawviewmodel" );
	AddItem( showWeapon );

	showRadar.szName = "Show Radar/Overview";
	showRadar.SetCoord( 16, 64 );
	showRadar.LinkCvar( "overview_mode" );
	AddItem( showRadar );

	// ---- TAB: Account ----
	AddTab( "Account" );

	accountHint.szName = "Account settings are not available in this version.";
	accountHint.iFlags |= QMF_INACTIVE;
	accountHint.SetCoord( 16, 8 );
	AddItem( accountHint );

	// ---- TAB: System ----
	AddTab( "System" );

	systemHint.szName = "System information and diagnostics.";
	systemHint.iFlags |= QMF_INACTIVE;
	systemHint.SetCoord( 16, 8 );
	AddItem( systemHint );

	developerMode.szName = "Developer Mode";
	developerMode.SetCoord( 16, 42 );
	developerMode.LinkCvar( "developer" );
	AddItem( developerMode );

	SetActiveTab( 0 );
}

void CMenuWndOptions::_VidInit()
{
	if( m_bUserMoved )
		return;

	int w = (int)(uiStatic.width * 0.72f);
	int h = (int)(768 * 0.70f);
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
