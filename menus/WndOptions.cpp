/*
WndOptions.cpp -- CS 1.6 PC-style windowed Options dialog
Copyright (C) 2024 DragonSlayer Team

CMenuFrameTabbed with Multiplayer/Keyboard/Audio/Video/Voice tabs.
OK/Cancel/Apply buttons always visible at bottom.
Each tab has real controls linked to cvars.
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "controls/FrameTabbed.h"
#include "Action.h"
#include "Slider.h"
#include "CheckBox.h"
#include "SpinControl.h"
#include "DropDown.h"
#include "Field.h"
#include "keydefs.h"
#include "Utils.h"
#include "TrackerScheme.h"

// ─── Button helper ────────────────────────────────────────────────────
// Simple clickable text button drawn with a flat bg + hover highlight.
// Used for OK/Cancel/Apply at bottom of Options.
class CMenuFlatButton : public CMenuAction
{
public:
	CMenuFlatButton()
	{
		m_bLimitBySize = true;
	}
	void Init( const char *name, int x, int y, int w, int h )
	{
		szName = name;
		SetRect( x, y, w, h );
		unsigned int bg = Scheme_GetColor( g_Scheme.buttonBgColor, 0x40505040 );
		unsigned int hover = Scheme_GetColor( g_Scheme.buttonArmedBgColor, 0x60707050 );
		SetBackground( bg, hover );
	}
};

// ─── Options window ───────────────────────────────────────────────────

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

	// ─── Bottom buttons (always visible — added before tabs) ───
	CMenuFlatButton btnOK;
	CMenuFlatButton btnCancel;
	CMenuFlatButton btnApply;

	// ─── Multiplayer tab ───
	CMenuField     playerName;
	CMenuCheckBox  sprayCrosshair;   // cl_crosshair_size approach
	CMenuCheckBox  autoHelp;         // cl_autohelp

	// ─── Keyboard tab ───
	CMenuAction    kbHint;
	CMenuCheckBox  developerConsole; // con_enable style
	CMenuSlider    mouseSens;        // sensitivity
	CMenuCheckBox  rawInput;         // m_rawinput
	CMenuCheckBox  mouseFilter;      // m_filter
	CMenuCheckBox  invertMouse;      // m_pitch < 0

	// ─── Audio tab ───
	CMenuSlider    sndVolume;        // volume
	CMenuSlider    musicVolume;      // MP3Volume
	CMenuSlider    suitVolume;       // suitvolume
	CMenuCheckBox  eax;              // s_eax (placeholder)
	CMenuCheckBox  a3d;              // s_a3d (placeholder)

	// ─── Video tab ───
	CMenuSlider    brightness;       // brightness
	CMenuSlider    gamma;            // gamma
	CMenuDropDownInt  dispMode;      // windowed/fullscreen
	CMenuCheckBox  vsync;            // gl_vsync
	CMenuCheckBox  hdr;              // mat_hdr_level approach
	CMenuSlider    fov;              // default_fov

	// ─── Voice tab ───
	CMenuCheckBox  voiceEnable;      // voice_modenable
	CMenuSlider    voiceTx;          // voice_scale
	CMenuSlider    voiceRx;          // voice_inputfromfile approach — placeholder
	CMenuCheckBox  openMic;          // voice_vox
};

static CMenuWndOptions *s_pWndOptions = NULL;

CMenuWndOptions::CMenuWndOptions() : CMenuFrameTabbed( "Options" )
{
}

void CMenuWndOptions::ApplySettings()
{
	// Write all cvars from editable controls
	playerName.WriteCvar();
	sprayCrosshair.WriteCvar();
	autoHelp.WriteCvar();
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
	voiceEnable.WriteCvar();
	voiceTx.WriteCvar();
	openMic.WriteCvar();
}

void CMenuWndOptions::CancelSettings()
{
	// Discard changes
	playerName.DiscardChanges();
	sprayCrosshair.DiscardChanges();
	autoHelp.DiscardChanges();
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
	voiceEnable.DiscardChanges();
	voiceTx.DiscardChanges();
	openMic.DiscardChanges();
	Hide();
}

void CMenuWndOptions::_Init()
{
	int w = (int)(uiStatic.width * 0.62f);
	int h = (int)(768 * 0.7f);
	int x = (uiStatic.width - w) / 2;
	int y = (768 - h) / 2;
	SetRect( x, y, w, h );

	// Content area width for controls (inside frame border+padding)
	int cw = w - 32; // 16px padding each side
	int btnW = 80;
	int btnH = 26;
	int btnY = h - 56; // from window top (logical), will be offset by GetPositionOffset

	// ─── Bottom buttons — added BEFORE any tab so they're always visible ───
	btnOK.Init( "OK", 16, btnY, btnW, btnH );
	SET_EVENT_MULTI( btnOK.onReleased,
	{
		CMenuWndOptions *self = (CMenuWndOptions*)pSelf->GetParent( CMenuWndOptions );
		self->ApplySettings();
		self->Hide();
	});
	AddItem( btnOK );

	btnCancel.Init( "Cancel", 16 + btnW + 8, btnY, btnW, btnH );
	SET_EVENT_MULTI( btnCancel.onReleased,
	{
		CMenuWndOptions *self = (CMenuWndOptions*)pSelf->GetParent( CMenuWndOptions );
		self->CancelSettings();
	});
	AddItem( btnCancel );

	btnApply.Init( "Apply", 16 + (btnW + 8) * 2, btnY, btnW, btnH );
	SET_EVENT_MULTI( btnApply.onReleased,
	{
		CMenuWndOptions *self = (CMenuWndOptions*)pSelf->GetParent( CMenuWndOptions );
		self->ApplySettings();
	});
	AddItem( btnApply );

	// ─────────────────────── TAB: Multiplayer ───────────────────────
	AddTab( "Multiplayer" );

	playerName.szName = "Player Name";
	playerName.iMaxLength = 32;
	playerName.SetRect( 16, 8, cw, 28 );
	playerName.LinkCvar( "name" );
	AddItem( playerName );

	sprayCrosshair.szName = "Dynamic Crosshair";
	sprayCrosshair.SetCoord( 16, 48 );
	sprayCrosshair.LinkCvar( "cl_dynamiccrosshair" );
	AddItem( sprayCrosshair );

	autoHelp.szName = "Auto-Help";
	autoHelp.SetCoord( 16, 82 );
	autoHelp.LinkCvar( "cl_autohelp" );
	AddItem( autoHelp );

	// ─────────────────────── TAB: Keyboard ──────────────────────────
	AddTab( "Keyboard" );

	kbHint.szName = "Use console for key binds: bind <key> <command>";
	kbHint.iFlags |= QMF_INACTIVE;
	kbHint.SetCoord( 16, 8 );
	AddItem( kbHint );

	developerConsole.szName = "Enable Developer Console (~)";
	developerConsole.SetCoord( 16, 42 );
	developerConsole.LinkCvar( "con_enable" );
	AddItem( developerConsole );

	mouseSens.szName = "Mouse Sensitivity";
	mouseSens.Setup( 0.5f, 20.0f, 0.5f );
	mouseSens.SetRect( 16, 82, cw, 28 );
	mouseSens.LinkCvar( "sensitivity" );
	AddItem( mouseSens );

	rawInput.szName = "Raw Input";
	rawInput.SetCoord( 16, 122 );
	rawInput.LinkCvar( "m_rawinput" );
	AddItem( rawInput );

	mouseFilter.szName = "Mouse Filter";
	mouseFilter.SetCoord( 16, 156 );
	mouseFilter.LinkCvar( "m_filter" );
	AddItem( mouseFilter );

	invertMouse.szName = "Reverse Mouse";
	invertMouse.SetCoord( 16, 190 );
	invertMouse.LinkCvar( "m_pitch" );
	invertMouse.bInvertMask = true;
	AddItem( invertMouse );

	// ─────────────────────── TAB: Audio ─────────────────────────────
	AddTab( "Audio" );

	sndVolume.szName = "Sound Effects Volume";
	sndVolume.Setup( 0.0f, 1.0f, 0.05f );
	sndVolume.SetRect( 16, 8, cw, 28 );
	sndVolume.LinkCvar( "volume" );
	AddItem( sndVolume );

	musicVolume.szName = "MP3 Volume";
	musicVolume.Setup( 0.0f, 1.0f, 0.05f );
	musicVolume.SetRect( 16, 52, cw, 28 );
	musicVolume.LinkCvar( "MP3Volume" );
	AddItem( musicVolume );

	suitVolume.szName = "HEV Suit Volume";
	suitVolume.Setup( 0.0f, 1.0f, 0.05f );
	suitVolume.SetRect( 16, 96, cw, 28 );
	suitVolume.LinkCvar( "suitvolume" );
	AddItem( suitVolume );

	eax.szName = "Enable EAX (requires EAX-capable card)";
	eax.SetCoord( 16, 144 );
	eax.LinkCvar( "s_eax" );
	AddItem( eax );

	a3d.szName = "Enable A3D";
	a3d.SetCoord( 16, 178 );
	a3d.LinkCvar( "s_a3d" );
	AddItem( a3d );

	// ─────────────────────── TAB: Video ─────────────────────────────
	AddTab( "Video" );

	brightness.szName = "Brightness";
	brightness.Setup( 0.0f, 3.0f, 0.1f );
	brightness.SetRect( 16, 8, cw, 28 );
	brightness.LinkCvar( "brightness" );
	AddItem( brightness );

	gamma.szName = "Gamma";
	gamma.Setup( 0.5f, 3.0f, 0.1f );
	gamma.SetRect( 16, 52, cw, 28 );
	gamma.LinkCvar( "gamma" );
	AddItem( gamma );

	dispMode.szName = "Display Mode";
	dispMode.AddItem( "Fullscreen", 1 );
	dispMode.AddItem( "Windowed", 0 );
	dispMode.SetRect( 16, 96, 200, 28 );
	dispMode.LinkCvar( "fullscreen", CMenuEditable::CVAR_VALUE );
	AddItem( dispMode );

	vsync.szName = "V-Sync";
	vsync.SetCoord( 16, 140 );
	vsync.LinkCvar( "gl_vsync" );
	AddItem( vsync );

	hdr.szName = "HDR";
	hdr.SetCoord( 16, 174 );
	hdr.LinkCvar( "mat_hdr_level" );
	AddItem( hdr );

	fov.szName = "Field of View";
	fov.Setup( 70.0f, 120.0f, 5.0f );
	fov.SetRect( 16, 214, cw, 28 );
	fov.LinkCvar( "default_fov" );
	AddItem( fov );

	// ─────────────────────── TAB: Voice ─────────────────────────────
	AddTab( "Voice" );

	voiceEnable.szName = "Enable Voice in this game";
	voiceEnable.SetCoord( 16, 8 );
	voiceEnable.LinkCvar( "voice_modenable" );
	AddItem( voiceEnable );

	voiceTx.szName = "Transmit Volume";
	voiceTx.Setup( 0.0f, 1.0f, 0.05f );
	voiceTx.SetRect( 16, 48, cw, 28 );
	voiceTx.LinkCvar( "voice_scale" );
	AddItem( voiceTx );

	voiceRx.szName = "Receive Volume";
	voiceRx.Setup( 0.0f, 1.0f, 0.05f );
	voiceRx.SetRect( 16, 92, cw, 28 );
	voiceRx.LinkCvar( "voice_scale" ); // same cvar, placeholder for now
	AddItem( voiceRx );

	openMic.szName = "Open Microphone (Voice Activation)";
	openMic.SetCoord( 16, 136 );
	openMic.LinkCvar( "voice_vox" );
	AddItem( openMic );

	SetActiveTab( 0 );
}

void CMenuWndOptions::_VidInit()
{
	int w = (int)(uiStatic.width * 0.62f);
	int h = (int)(768 * 0.7f);
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
