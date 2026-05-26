/*
WndCreateGame.cpp -- CS 1.6 PC-style windowed Create Game dialog
Copyright (C) 2024 DragonSlayer Team

CMenuFrameTabbed with Server/Game/Bots tabs.
Real controls: hostname, map list, max players, game mode options.
Start Server button always visible.
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "controls/FrameTabbed.h"
#include "controls/FrameButton.h"
#include "Field.h"
#include "Action.h"
#include "CheckBox.h"
#include "Slider.h"
#include "SpinControl.h"
#include "DropDown.h"
#include "keydefs.h"
#include "Utils.h"
#include "TrackerScheme.h"

// ─── Create Game window ──────────────────────────────────────────────

class CMenuWndCreateGame : public CMenuFrameTabbed
{
public:
	CMenuWndCreateGame();
	bool IsRoot() const override { return false; }

private:
	void _Init() override;
	void _VidInit() override;
	void StartServer();

	// Bottom buttons (always visible)
	CMenuFrameButton btnStart;
	CMenuFrameButton btnCancel;

	// ─── Server tab ───
	CMenuField   hostName;
	CMenuField   mapName;
	CMenuField   maxPlayers;
	CMenuField   password;
	CMenuCheckBox dedicated;

	// ─── Game tab ───
	CMenuSlider  roundTime;       // mp_roundtime
	CMenuSlider  freezeTime;      // mp_freezetime
	CMenuSlider  buyTime;         // mp_buytime
	CMenuSlider  startMoney;      // mp_startmoney
	CMenuCheckBox friendlyFire;   // mp_friendlyfire
	CMenuCheckBox footsteps;      // mp_footsteps
	CMenuCheckBox autoteambalance; // mp_autoteambalance

	// ─── Bots tab ───
	CMenuSlider  botQuota;        // bot_quota
	CMenuDropDownStr botDifficulty; // bot_difficulty
	CMenuCheckBox botsJoinTeam;   // bot_join_team
};

static CMenuWndCreateGame *s_pWndCreateGame = NULL;

CMenuWndCreateGame::CMenuWndCreateGame() : CMenuFrameTabbed( "Create Game" )
{
}

void CMenuWndCreateGame::StartServer()
{
	// Gather settings
	hostName.WriteCvar();
	maxPlayers.WriteCvar();
	password.WriteCvar();
	roundTime.WriteCvar();
	freezeTime.WriteCvar();
	buyTime.WriteCvar();
	startMoney.WriteCvar();
	friendlyFire.WriteCvar();
	footsteps.WriteCvar();
	autoteambalance.WriteCvar();
	botQuota.WriteCvar();

	// Start server with specified map
	const char *map = mapName.GetBuffer();
	if( !map || !map[0] )
		map = "de_dust2";

	EngFuncs::ClientCmd( false, "disconnect\n" );
	EngFuncs::ClientCmdF( false, "maxplayers %s\n", maxPlayers.GetBuffer() );
	EngFuncs::ClientCmdF( false, "map %s\n", map );
	Hide();
}

void CMenuWndCreateGame::_Init()
{
	int w = (int)(uiStatic.width * 0.6f);
	int h = (int)(768 * 0.65f);
	int x = (uiStatic.width - w) / 2;
	int y = (768 - h) / 2;
	SetRect( x, y, w, h );

	int cw = w - 32;
	int btnW = 100;
	int btnH = 26;
	int contentH = h - FRAME_TITLE_HEIGHT - FRAME_TAB_HEIGHT;
	int btnY = contentH - btnH - 12;

	// ─── Bottom buttons (always visible) ───
	btnStart.szName = "Start Server";
	btnStart.SetRect( 16, btnY, btnW + 10, btnH );
	SET_EVENT_MULTI( btnStart.onReleased,
	{
		CMenuWndCreateGame *self = (CMenuWndCreateGame*)pSelf->GetParent( CMenuWndCreateGame );
		self->StartServer();
	});
	AddItem( btnStart );

	btnCancel.szName = "Cancel";
	btnCancel.SetRect( 16 + btnW + 18, btnY, btnW - 20, btnH );
	SET_EVENT_MULTI( btnCancel.onReleased,
	{
		CMenuWndCreateGame *self = (CMenuWndCreateGame*)pSelf->GetParent( CMenuWndCreateGame );
		self->Hide();
	});
	AddItem( btnCancel );

	// ─────────────────────── TAB: Server ────────────────────────────
	AddTab( "Server" );

	hostName.szName = "Server Name";
	hostName.iMaxLength = 32;
	hostName.SetRect( 16, 8, cw, 28 );
	hostName.LinkCvar( "hostname" );
	AddItem( hostName );

	mapName.szName = "Map";
	mapName.iMaxLength = 32;
	mapName.SetRect( 16, 50, cw / 2, 28 );
	// No cvar link — user types map name directly
	AddItem( mapName );

	maxPlayers.szName = "Max Players";
	maxPlayers.iMaxLength = 3;
	maxPlayers.bNumbersOnly = true;
	maxPlayers.SetRect( 16, 92, 120, 28 );
	maxPlayers.LinkCvar( "maxplayers" );
	AddItem( maxPlayers );

	password.szName = "Server Password";
	password.iMaxLength = 16;
	password.bHideInput = true;
	password.SetRect( 160, 92, cw - 160, 28 );
	password.LinkCvar( "sv_password" );
	AddItem( password );

	dedicated.szName = "Dedicated Server";
	dedicated.SetCoord( 16, 134 );
	dedicated.LinkCvar( "sv_lan" );
	AddItem( dedicated );

	// ─────────────────────── TAB: Game ──────────────────────────────
	AddTab( "Game" );

	roundTime.szName = "Round Time (min)";
	roundTime.Setup( 1.0f, 9.0f, 0.5f );
	roundTime.SetRect( 16, 8, cw, 28 );
	roundTime.LinkCvar( "mp_roundtime" );
	AddItem( roundTime );

	freezeTime.szName = "Freeze Time (sec)";
	freezeTime.Setup( 0.0f, 20.0f, 1.0f );
	freezeTime.SetRect( 16, 52, cw, 28 );
	freezeTime.LinkCvar( "mp_freezetime" );
	AddItem( freezeTime );

	buyTime.szName = "Buy Time (sec)";
	buyTime.Setup( 0.0f, 90.0f, 5.0f );
	buyTime.SetRect( 16, 96, cw, 28 );
	buyTime.LinkCvar( "mp_buytime" );
	AddItem( buyTime );

	startMoney.szName = "Start Money";
	startMoney.Setup( 800.0f, 16000.0f, 100.0f );
	startMoney.SetRect( 16, 140, cw, 28 );
	startMoney.LinkCvar( "mp_startmoney" );
	AddItem( startMoney );

	friendlyFire.szName = "Friendly Fire";
	friendlyFire.SetCoord( 16, 184 );
	friendlyFire.LinkCvar( "mp_friendlyfire" );
	AddItem( friendlyFire );

	footsteps.szName = "Footsteps";
	footsteps.SetCoord( 16, 218 );
	footsteps.LinkCvar( "mp_footsteps" );
	AddItem( footsteps );

	autoteambalance.szName = "Auto Team Balance";
	autoteambalance.SetCoord( 16, 252 );
	autoteambalance.LinkCvar( "mp_autoteambalance" );
	AddItem( autoteambalance );

	// ─────────────────────── TAB: Bots ──────────────────────────────
	AddTab( "Bots" );

	botQuota.szName = "Number of Bots (0 = disabled)";
	botQuota.Setup( 0.0f, 32.0f, 1.0f );
	botQuota.SetRect( 16, 8, cw, 28 );
	botQuota.LinkCvar( "bot_quota" );
	AddItem( botQuota );

	botDifficulty.szName = "Bot Difficulty";
	botDifficulty.AddItem( "Easy", "0" );
	botDifficulty.AddItem( "Normal", "1" );
	botDifficulty.AddItem( "Hard", "2" );
	botDifficulty.AddItem( "Expert", "3" );
	botDifficulty.SetRect( 16, 48, 200, 28 );
	botDifficulty.LinkCvar( "bot_difficulty", CMenuEditable::CVAR_STRING );
	AddItem( botDifficulty );

	botsJoinTeam.szName = "Bots join both teams";
	botsJoinTeam.SetCoord( 16, 92 );
	botsJoinTeam.LinkCvar( "bot_join_after_player" );
	AddItem( botsJoinTeam );

	SetActiveTab( 0 );
}

void CMenuWndCreateGame::_VidInit()
{
	int w = (int)(uiStatic.width * 0.6f);
	int h = (int)(768 * 0.65f);
	int x = (uiStatic.width - w) / 2;
	int y = (768 - h) / 2;
	SetRect( x, y, w, h );
}

// ============= Public API =============

void WndCreateGame_Precache()
{
	s_pWndCreateGame = new CMenuWndCreateGame();
}

void WndCreateGame_Show()
{
	if( s_pWndCreateGame )
		s_pWndCreateGame->Show();
}

void WndCreateGame_Shutdown()
{
	delete s_pWndCreateGame;
	s_pWndCreateGame = NULL;
}

ADD_MENU4( menu_wndcreategame, WndCreateGame_Precache, WndCreateGame_Show, WndCreateGame_Shutdown );
