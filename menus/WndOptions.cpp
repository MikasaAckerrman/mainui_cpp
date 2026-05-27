/*
WndOptions.cpp -- CS 1.6 PC-style windowed Options dialog
Copyright (C) 2024 DragonSlayer Team

CMenuFrameTabbed with 8 Russian-named tabs.
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

	// Inner class for GoldSrc-style preview box with inset border
	class CMenuPreviewBox : public CMenuAction
	{
	public:
		void Draw() override
		{
			// Dark fill inside
			UI_FillRect( m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h, 0xFF1A1A1A );

			// GoldSrc inset border: dark top+left, bright bottom+right
			DrawGoldSrcInset( m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h );
		}
	};

	// Inner class for 1px separator line
	class CMenuSeparator : public CMenuAction
	{
	public:
		void Draw() override
		{
			UI_FillRect( m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h,
				Scheme_GetColor( g_Scheme.borderDark, 0xFF30342B ) );
		}
	};

	// Inner class for eye toggle button
	class CMenuEyeButton : public CMenuFrameButton
	{
	public:
		CMenuField *m_pField;

		void Draw() override
		{
			bool focused = ( m_pParent && this == m_pParent->ItemAtCursor() );
			unsigned int bg = focused ? 0xFF6B7360 : 0xFF5B6350;

			UI_FillRect( m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h, bg );

			// Raised border
			DrawGoldSrcRaised( m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h );

			// Draw eye glyph - larger and thicker for mobile visibility
			unsigned int glyphColor = 0xFFFFFFFF;
			int pad = 4;
			int eyeW = m_scSize.w - pad * 2;
			int eyeH = m_scSize.h - pad * 2;
			int cx = m_scPos.x + m_scSize.w / 2;
			int cy = m_scPos.y + m_scSize.h / 2;

			// Horizontal bar (eye shape) - 3px thick
			UI_FillRect( cx - eyeW/3, cy - 1, eyeW*2/3, 3, glyphColor );
			// Circle pupil - 5x5
			UI_FillRect( cx - 2, cy - 2, 5, 5, glyphColor );
			// Top arc hint
			UI_FillRect( cx - eyeW/4, cy - 3, eyeW/2, 2, glyphColor );
			// Bottom arc hint
			UI_FillRect( cx - eyeW/4, cy + 2, eyeW/2, 2, glyphColor );
		}
	};

	// Inner class for Steam icon area
	class CMenuSteamIcon : public CMenuAction
	{
	public:
		void Draw() override
		{
			unsigned int bg = 0xFF4A4A4A;

			UI_FillRect( m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h, bg );

			// Raised border
			DrawGoldSrcRaised( m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h );

			// Draw "S" shape for Steam
			unsigned int sColor = 0xFFFFFFFF;
			int x0 = m_scPos.x + (int)(6 * uiStatic.scaleX);
			int y0 = m_scPos.y + (int)(5 * uiStatic.scaleY);
			int sw = (int)(12 * uiStatic.scaleX);
			int sh = (int)(14 * uiStatic.scaleY);
			if( sw < 6 ) sw = 6;
			if( sh < 7 ) sh = 7;

			// Top bar
			UI_FillRect( x0, y0, sw, 2, sColor );
			// Left side top half
			UI_FillRect( x0, y0, 2, sh / 2, sColor );
			// Middle bar
			UI_FillRect( x0, y0 + sh / 2 - 1, sw, 2, sColor );
			// Right side bottom half
			UI_FillRect( x0 + sw - 2, y0 + sh / 2, 2, sh / 2, sColor );
			// Bottom bar
			UI_FillRect( x0, y0 + sh - 2, sw, 2, sColor );
		}
	};

	// Bottom buttons (always visible - added before tabs)
	CMenuFrameButton btnOK;
	CMenuFrameButton btnCancel;
	CMenuFrameButton btnApply;
	CMenuSeparator   btnSeparator;

	// Multiplayer tab - labels
	CMenuAction      avatarLabel;
	CMenuAction      logoLabel;
	CMenuAction      playerNameLabel;
	CMenuAction      passwordLabel;
	CMenuAction      logoHint;

	// Multiplayer tab - controls
	CMenuPreviewBox  avatarPreview;
	CMenuFrameButton btnUpload;
	CMenuDropDownStr modelSelect;
	CMenuPreviewBox  logoPreview;
	CMenuDropDownStr logoSelect;
	CMenuFrameButton btnColor;
	CMenuFrameButton btnAdvanced;
	CMenuField       playerName;
	CMenuField       adminPassword;
	CMenuEyeButton   btnEye;
	CMenuSteamIcon   steamIcon;

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
	CMenuCheckBox  developerMode;
	CMenuAction    systemHint;
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
	int w = (int)(uiStatic.width * 0.60f);
	int h = (int)(768 * 0.90f);
	int x = (uiStatic.width - w) / 2;
	int y = (768 - h) / 2;
	SetRect( x, y, w, h );

	// Padding and spacing - GoldSrc pixel-perfect metrics
	int pad = 18;           // outer padding from window edge
	int cw = w - pad * 2;  // content width
	int btnW = 130;         // button width
	int btnH = 34;          // button height
	int btnGap = 12;        // gap between buttons
	int fieldH = 28;        // input/dropdown height
	int labelGap = 8;       // gap between label and input below it
	int groupGap = 16;      // gap between groups
	int colGap = 48;        // gap between columns

	// Content area
	int contentH = h - FRAME_TITLE_HEIGHT - FRAME_TAB_HEIGHT;
	int btnY = contentH - btnH - pad + 12; // raised 12px higher
	int rightEdge = w - pad;

	// ---- Bottom buttons (added BEFORE any tab, so always visible) ----
	// Order from right: Apply, Cancel, OK
	btnApply.szName = "Apply";
	btnApply.SetRect( rightEdge - btnW, btnY, btnW, btnH );
	SET_EVENT_MULTI( btnApply.onReleased,
	{
		CMenuWndOptions *self = (CMenuWndOptions*)pSelf->GetParent( CMenuWndOptions );
		self->ApplySettings();
	});
	AddItem( btnApply );
	btnApply.iFlags |= QMF_GRAYED;

	// 1px separator line above the buttons
	btnSeparator.szName = "";
	btnSeparator.iFlags |= QMF_INACTIVE;
	btnSeparator.SetRect( pad, btnY - 8, cw, 1 );
	AddItem( btnSeparator );

	btnCancel.szName = "Cancel";
	btnCancel.SetRect( rightEdge - btnW * 2 - btnGap, btnY, btnW, btnH );
	SET_EVENT_MULTI( btnCancel.onReleased,
	{
		CMenuWndOptions *self = (CMenuWndOptions*)pSelf->GetParent( CMenuWndOptions );
		self->CancelSettings();
	});
	AddItem( btnCancel );

	btnOK.szName = "OK";
	btnOK.SetRect( rightEdge - btnW * 3 - btnGap * 2, btnY, btnW, btnH );
	SET_EVENT_MULTI( btnOK.onReleased,
	{
		CMenuWndOptions *self = (CMenuWndOptions*)pSelf->GetParent( CMenuWndOptions );
		self->ApplySettings();
		self->Hide();
	});
	AddItem( btnOK );

	// ---- TAB 0: Multiplayer ----
	AddTab( (const char*)u8"\u041C\u0443\u043B\u044C\u0442\u0438\u043F\u043B\u0435\u0435\u0440" );

	// Two-column layout - GoldSrc precise spacing
	int leftColW = 128 + labelGap + 200; // preview(128) + gap + controls
	int rightCol = pad + leftColW + colGap;
	int rightColW = w - rightCol - pad;
	if( rightColW < 200 ) rightColW = 200;

	// Steam icon - top left corner of multiplayer tab
	steamIcon.szName = "";
	steamIcon.iFlags |= QMF_INACTIVE;
	steamIcon.SetRect( pad, pad, 24, 24 );
	AddItem( steamIcon );

	// Left column - Avatar section
	int ly = pad + 12;

	avatarLabel.szName = (const char*)u8"\u0410\u0432\u0430\u0442\u0430\u0440";
	avatarLabel.iFlags |= QMF_INACTIVE;
	avatarLabel.SetCoord( pad + 24 + labelGap, ly );
	AddItem( avatarLabel );

	ly += FRAME_TEXT_HEIGHT + labelGap;

	avatarPreview.szName = "";
	avatarPreview.iFlags |= QMF_INACTIVE;
	avatarPreview.SetRect( pad, ly, 128, 128 );
	AddItem( avatarPreview );

	btnUpload.szName = (const char*)u8"\u0417\u0430\u0433\u0440\u0443\u0437\u0438\u0442\u044C...";
	btnUpload.SetRect( pad, ly + 128 + labelGap, 140, 32 );
	AddItem( btnUpload );

	modelSelect.szName = "";
	modelSelect.AddItem( "arctic", "arctic" );
	modelSelect.AddItem( "guerilla", "guerilla" );
	modelSelect.AddItem( "leet", "leet" );
	modelSelect.SetRect( pad, ly + 128 + labelGap + 32 + labelGap, 236, fieldH );
	AddItem( modelSelect );

	ly += 128 + labelGap + 32 + labelGap + fieldH + groupGap + 18;

	// Left column - Logo section
	logoLabel.szName = (const char*)u8"\u041B\u043E\u0433\u043E\u0442\u0438\u043F";
	logoLabel.iFlags |= QMF_INACTIVE;
	logoLabel.SetCoord( pad, ly );
	AddItem( logoLabel );

	ly += FRAME_TEXT_HEIGHT + labelGap;

	logoPreview.szName = "";
	logoPreview.iFlags |= QMF_INACTIVE;
	logoPreview.SetRect( pad, ly, 128, 128 );
	AddItem( logoPreview );

	logoSelect.szName = "";
	logoSelect.AddItem( "lambda", "lambda" );
	logoSelect.AddItem( "skull", "skull" );
	logoSelect.AddItem( "cross", "cross" );
	logoSelect.SetRect( pad, ly + 128 + labelGap, 236, fieldH );
	AddItem( logoSelect );

	btnColor.szName = (const char*)u8"\u0418\u0437\u043C\u0435\u043D\u0438\u0442\u044C \u0446\u0432\u0435\u0442";
	btnColor.SetRect( pad, ly + 128 + labelGap + fieldH + labelGap, 140, 32 );
	AddItem( btnColor );

	// Helper text below logo section
	logoHint.szName = (const char*)u8"\u041B\u043E\u0433\u043E\u0442\u0438\u043F \u0438\u0437\u043C\u0435\u043D\u0438\u0442\u0441\u044F \u043F\u043E\u0441\u043B\u0435 \u0441\u043E\u0435\u0434\u0438\u043D\u0435\u043D\u0438\u044F \u0441 \u0441\u0435\u0440\u0432\u0435\u0440\u043E\u043C.";
	logoHint.iFlags |= QMF_INACTIVE;
	logoHint.SetCoord( pad, ly + 128 + labelGap + fieldH + labelGap + 32 + labelGap );
	AddItem( logoHint );

	// Advanced button at bottom of left column
	btnAdvanced.szName = (const char*)u8"\u0414\u043E\u043F\u043E\u043B\u043D\u0438\u0442\u0435\u043B\u044C\u043D\u043E...";
	btnAdvanced.SetRect( pad, ly + 128 + labelGap + fieldH + labelGap + 32 + labelGap + FRAME_TEXT_HEIGHT + groupGap, 180, 32 );
	AddItem( btnAdvanced );

	// Right column
	int ry = pad + 24;

	playerNameLabel.szName = (const char*)u8"\u0418\u043C\u044F \u0438\u0433\u0440\u043E\u043A\u0430";
	playerNameLabel.iFlags |= QMF_INACTIVE;
	playerNameLabel.SetCoord( rightCol, ry );
	AddItem( playerNameLabel );

	ry += FRAME_TEXT_HEIGHT + labelGap;

	playerName.szName = "";
	playerName.iMaxLength = 32;
	playerName.SetRect( rightCol, ry, rightColW, fieldH );
	playerName.LinkCvar( "name" );
	AddItem( playerName );

	ry += fieldH + groupGap;

	passwordLabel.szName = (const char*)u8"\u041F\u0430\u0440\u043E\u043B\u044C \u0434\u043B\u044F VIP/Admin \u0434\u043E\u0441\u0442\u0443\u043F\u0430";
	passwordLabel.iFlags |= QMF_INACTIVE;
	passwordLabel.SetCoord( rightCol, ry );
	AddItem( passwordLabel );

	ry += FRAME_TEXT_HEIGHT + labelGap;

	adminPassword.szName = "";
	adminPassword.iMaxLength = 64;
	adminPassword.bHideInput = true;
	adminPassword.SetRect( rightCol, ry, rightColW - 28 - labelGap, fieldH );
	adminPassword.LinkCvar( "password" );
	AddItem( adminPassword );

	// Eye icon button to toggle password visibility
	btnEye.szName = "";
	btnEye.m_pField = &adminPassword;
	btnEye.SetRect( rightCol + rightColW - 28, ry + (fieldH - 28) / 2, 28, 28 );
	SET_EVENT_MULTI( btnEye.onReleased,
	{
		CMenuWndOptions *self = (CMenuWndOptions*)pSelf->GetParent( CMenuWndOptions );
		self->adminPassword.bHideInput = !self->adminPassword.bHideInput;
	});
	AddItem( btnEye );

	// ---- TAB 1: Keyboard ----
	AddTab( (const char*)u8"\u041A\u043B\u0430\u0432\u0438\u0430\u0442\u0443\u0440\u0430" );

	kbHint.szName = (const char*)u8"\u041D\u0430\u0441\u0442\u0440\u043E\u0439\u043A\u0438 \u043A\u043B\u0430\u0432\u0438\u0448 \u0434\u043E\u0441\u0442\u0443\u043F\u043D\u044B \u0447\u0435\u0440\u0435\u0437 \u043A\u043E\u043D\u0441\u043E\u043B\u044C: bind <key> <command>";
	kbHint.iFlags |= QMF_INACTIVE;
	kbHint.SetCoord( pad, pad );
	AddItem( kbHint );

	developerConsole.szName = (const char*)u8"\u0412\u043A\u043B\u044E\u0447\u0438\u0442\u044C \u043A\u043E\u043D\u0441\u043E\u043B\u044C \u0440\u0430\u0437\u0440\u0430\u0431\u043E\u0442\u0447\u0438\u043A\u0430 (~)";
	developerConsole.SetCoord( pad, pad + groupGap + FRAME_TEXT_HEIGHT );
	developerConsole.LinkCvar( "con_enable" );
	AddItem( developerConsole );

	// ---- TAB 2: Mouse ----
	AddTab( (const char*)u8"\u041C\u044B\u0448\u044C" );

	mouseSens.szName = (const char*)u8"\u0427\u0443\u0432\u0441\u0442\u0432\u0438\u0442\u0435\u043B\u044C\u043D\u043E\u0441\u0442\u044C \u043C\u044B\u0448\u0438";
	mouseSens.Setup( 0.5f, 20.0f, 0.5f );
	mouseSens.SetRect( pad, pad, cw, 28 );
	mouseSens.LinkCvar( "sensitivity" );
	AddItem( mouseSens );

	rawInput.szName = (const char*)u8"\u041F\u0440\u044F\u043C\u043E\u0439 \u0432\u0432\u043E\u0434";
	rawInput.SetCoord( pad, pad + 40 );
	rawInput.LinkCvar( "m_rawinput" );
	AddItem( rawInput );

	mouseFilter.szName = (const char*)u8"\u0424\u0438\u043B\u044C\u0442\u0440 \u043C\u044B\u0448\u0438";
	mouseFilter.SetCoord( pad, pad + 68 );
	mouseFilter.LinkCvar( "m_filter" );
	AddItem( mouseFilter );

	invertMouse.szName = (const char*)u8"\u0418\u043D\u0432\u0435\u0440\u0442\u0438\u0440\u043E\u0432\u0430\u0442\u044C \u043C\u044B\u0448\u044C";
	invertMouse.SetCoord( pad, pad + 96 );
	invertMouse.LinkCvar( "lookspring" );
	AddItem( invertMouse );

	// ---- TAB 3: Audio ----
	AddTab( (const char*)u8"\u0417\u0432\u0443\u043A" );

	sndVolume.szName = (const char*)u8"\u0413\u0440\u043E\u043C\u043A\u043E\u0441\u0442\u044C \u0437\u0432\u0443\u043A\u043E\u0432";
	sndVolume.Setup( 0.0f, 1.0f, 0.05f );
	sndVolume.SetRect( pad, pad, cw, 28 );
	sndVolume.LinkCvar( "volume" );
	AddItem( sndVolume );

	musicVolume.szName = (const char*)u8"\u0413\u0440\u043E\u043C\u043A\u043E\u0441\u0442\u044C MP3";
	musicVolume.Setup( 0.0f, 1.0f, 0.05f );
	musicVolume.SetRect( pad, pad + 40, cw, 28 );
	musicVolume.LinkCvar( "MP3Volume" );
	AddItem( musicVolume );

	suitVolume.szName = (const char*)u8"\u0413\u0440\u043E\u043C\u043A\u043E\u0441\u0442\u044C HEV";
	suitVolume.Setup( 0.0f, 1.0f, 0.05f );
	suitVolume.SetRect( pad, pad + 80, cw, 28 );
	suitVolume.LinkCvar( "suitvolume" );
	AddItem( suitVolume );

	eax.szName = (const char*)u8"\u0412\u043A\u043B\u044E\u0447\u0438\u0442\u044C EAX";
	eax.SetCoord( pad, pad + 120 );
	eax.LinkCvar( "s_eax" );
	AddItem( eax );

	a3d.szName = (const char*)u8"\u0412\u043A\u043B\u044E\u0447\u0438\u0442\u044C A3D";
	a3d.SetCoord( pad, pad + 148 );
	a3d.LinkCvar( "s_a3d" );
	AddItem( a3d );

	// ---- TAB 4: Video ----
	AddTab( (const char*)u8"\u0412\u0438\u0434\u0435\u043E" );

	brightness.szName = (const char*)u8"\u042F\u0440\u043A\u043E\u0441\u0442\u044C";
	brightness.Setup( 0.0f, 3.0f, 0.1f );
	brightness.SetRect( pad, pad, cw, 28 );
	brightness.LinkCvar( "brightness" );
	AddItem( brightness );

	gamma.szName = (const char*)u8"\u0413\u0430\u043C\u043C\u0430";
	gamma.Setup( 0.5f, 3.0f, 0.1f );
	gamma.SetRect( pad, pad + 40, cw, 28 );
	gamma.LinkCvar( "gamma" );
	AddItem( gamma );

	dispMode.szName = (const char*)u8"\u0420\u0435\u0436\u0438\u043C \u043E\u0442\u043E\u0431\u0440\u0430\u0436\u0435\u043D\u0438\u044F";
	dispMode.AddItem( (const char*)u8"\u041F\u043E\u043B\u043D\u044B\u0439 \u044D\u043A\u0440\u0430\u043D", 1 );
	dispMode.AddItem( (const char*)u8"\u041E\u043A\u043E\u043D\u043D\u044B\u0439", 0 );
	dispMode.SetRect( pad, pad + 80, 236, fieldH );
	dispMode.LinkCvar( "fullscreen", CMenuEditable::CVAR_VALUE );
	AddItem( dispMode );

	vsync.szName = "V-Sync";
	vsync.SetCoord( pad, pad + 80 + fieldH + labelGap );
	vsync.LinkCvar( "gl_vsync" );
	AddItem( vsync );

	hdr.szName = "Overbright";
	hdr.SetCoord( pad, pad + 80 + fieldH + labelGap + 28 );
	hdr.LinkCvar( "gl_overbright" );
	AddItem( hdr );

	fov.szName = (const char*)u8"\u041F\u043E\u043B\u0435 \u0437\u0440\u0435\u043D\u0438\u044F";
	fov.Setup( 70.0f, 120.0f, 5.0f );
	fov.SetRect( pad, pad + 80 + fieldH + labelGap + 56 + labelGap, cw, 28 );
	fov.LinkCvar( "default_fov" );
	AddItem( fov );

	// ---- TAB 5: HUD ----
	AddTab( "HUD" );

	showHud.szName = (const char*)u8"\u041F\u043E\u043A\u0430\u0437\u044B\u0432\u0430\u0442\u044C HUD";
	showHud.SetCoord( pad, pad );
	showHud.LinkCvar( "hud_draw" );
	AddItem( showHud );

	showWeapon.szName = (const char*)u8"\u041F\u043E\u043A\u0430\u0437\u044B\u0432\u0430\u0442\u044C \u043E\u0440\u0443\u0436\u0438\u0435";
	showWeapon.SetCoord( pad, pad + 28 );
	showWeapon.LinkCvar( "r_drawviewmodel" );
	AddItem( showWeapon );

	showRadar.szName = (const char*)u8"\u041F\u043E\u043A\u0430\u0437\u044B\u0432\u0430\u0442\u044C \u0440\u0430\u0434\u0430\u0440";
	showRadar.SetCoord( pad, pad + 56 );
	showRadar.LinkCvar( "overview_mode" );
	AddItem( showRadar );

	// ---- TAB 6: Account ----
	AddTab( (const char*)u8"\u0410\u043A\u043A\u0430\u0443\u043D\u0442" );

	accountHint.szName = (const char*)u8"\u041D\u0430\u0441\u0442\u0440\u043E\u0439\u043A\u0438 \u0430\u043A\u043A\u0430\u0443\u043D\u0442\u0430";
	accountHint.iFlags |= QMF_INACTIVE;
	accountHint.SetCoord( pad, pad );
	AddItem( accountHint );

	// ---- TAB 7: System ----
	AddTab( (const char*)u8"\u0421\u0438\u0441\u0442\u0435\u043C\u0430" );

	developerMode.szName = (const char*)u8"\u0420\u0435\u0436\u0438\u043C \u0440\u0430\u0437\u0440\u0430\u0431\u043E\u0442\u0447\u0438\u043A\u0430";
	developerMode.SetCoord( pad, pad );
	developerMode.LinkCvar( "developer" );
	AddItem( developerMode );

	systemHint.szName = (const char*)u8"\u0418\u043D\u0444\u043E\u0440\u043C\u0430\u0446\u0438\u044F \u043E \u0441\u0438\u0441\u0442\u0435\u043C\u0435 \u0438 \u0434\u0438\u0430\u0433\u043D\u043E\u0441\u0442\u0438\u043A\u0430.";
	systemHint.iFlags |= QMF_INACTIVE;
	systemHint.SetCoord( pad, pad + 36 );
	AddItem( systemHint );

	SetActiveTab( 0 );
}

void CMenuWndOptions::_VidInit()
{
	if( m_bUserMoved )
		return;
	int w = (int)(uiStatic.width * 0.60f);
	int h = (int)(768 * 0.90f);
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
