/*
WndCreateGame.cpp -- Source Engine-style windowed Create Game dialog
Copyright (C) 2024 DragonSlayer Team

CMenuFrameTabbed with Server/Game/Bot tabs.
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "controls/FrameTabbed.h"
#include "Field.h"
#include "Action.h"
#include "CheckBox.h"
#include "PicButton.h"
#include "keydefs.h"
#include "Utils.h"
#include "TrackerScheme.h"

class CMenuWndCreateGame : public CMenuFrameTabbed
{
public:
	CMenuWndCreateGame();
	bool IsRoot() const override { return false; }

private:
	void _Init() override;
	void _VidInit() override;
	void StartServer();

	// Server tab
	CMenuField hostName;
	CMenuField maxClients;
	CMenuField password;
	CMenuAction mapLabel;

	// Game tab
	CMenuAction gameLabel;

	// Bot tab
	CMenuAction botLabel;

	// Buttons (always visible)
	CMenuPicButton startBtn;
};

static CMenuWndCreateGame *s_pWndCreateGame = NULL;

CMenuWndCreateGame::CMenuWndCreateGame() : CMenuFrameTabbed( "Create Game" )
{
}

void CMenuWndCreateGame::_Init()
{
	int w = (int)(uiStatic.width * 0.65f);
	int h = (int)(768 * 0.6f);
	int x = (uiStatic.width - w) / 2;
	int y = (768 - h) / 2;
	SetRect( x, y, w, h );

	// === Server tab ===
	AddTab( "Server" );

	hostName.szName = "Server Name";
	hostName.iMaxLength = 28;
	hostName.SetRect( 16, 16, w - 32, 32 );
	hostName.LinkCvar( "hostname" );
	AddItem( hostName );

	maxClients.szName = "Max Players";
	maxClients.iMaxLength = 3;
	maxClients.bNumbersOnly = true;
	maxClients.SetRect( 16, 64, 120, 32 );
	maxClients.LinkCvar( "maxplayers" );
	AddItem( maxClients );

	password.szName = "Password";
	password.iMaxLength = 16;
	password.bHideInput = true;
	password.SetRect( 160, 64, w - 176, 32 );
	password.LinkCvar( "sv_password" );
	AddItem( password );

	mapLabel.szName = "Select map from console: map <mapname>";
	mapLabel.iFlags |= QMF_INACTIVE;
	mapLabel.SetCoord( 16, 112 );
	AddItem( mapLabel );

	// === Game tab ===
	AddTab( "Game" );

	gameLabel.szName = "Game settings (use server.cfg)";
	gameLabel.iFlags |= QMF_INACTIVE;
	gameLabel.SetCoord( 16, 16 );
	AddItem( gameLabel );

	// === Bot tab ===
	AddTab( "Bots" );

	botLabel.szName = "Bot configuration (use bot.cfg)";
	botLabel.iFlags |= QMF_INACTIVE;
	botLabel.SetCoord( 16, 16 );
	AddItem( botLabel );

	SetActiveTab( 0 );
}

void CMenuWndCreateGame::_VidInit()
{
	int w = (int)(uiStatic.width * 0.65f);
	int h = (int)(768 * 0.6f);
	int x = (uiStatic.width - w) / 2;
	int y = (768 - h) / 2;
	SetRect( x, y, w, h );
}

void CMenuWndCreateGame::StartServer()
{
	EngFuncs::ClientCmd( false, "disconnect\n" );
	EngFuncs::ClientCmd( false, "map de_dust2\n" );
	Hide();
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
