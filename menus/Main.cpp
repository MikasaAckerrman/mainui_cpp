/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


#include "Framework.h"
#include "Action.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "YesNoMessageBox.h"
#include "keydefs.h"
#include "MenuStrings.h"
#include "PlayerIntroduceDialog.h"
#include "gameinfo.h"
#include "AnimatedBanner.h"
#include "MovieBanner.h"

// Windowed dialogs (Source Engine-style)
extern void WndConsole_Show( void );
extern void WndServerBrowser_Show( void );
extern void WndCreateGame_Show( void );

// VGUI1 Options dialog - linked directly into libmenu.so
extern "C" void VGUI_SetScreenSize(int w, int h);
extern "C" void VGUI_SetCvarFuncs(
	float(*)(const char*), void(*)(const char*,float),
	const char*(*)(const char*), void(*)(const char*,const char*), void(*)(const char*));
extern "C" void VGUI_ShowOptions(void);

// WHICH MENU DOES A MAIN-MENU BUTTON OPEN?
//
// The windowed dialogs (CMenuFrame / CMenuFrameTabbed, plus the VGUI1 Options
// panel) were wired straight onto the main menu buttons in May. That turned out
// to be a mistake with a cost that is easy to understate: the ONLY way into a
// server, a local game or the settings is through those buttons, so as long as
// the windowed dialogs are incomplete the game is unreachable -- and every other
// change in the engine becomes untestable on the device along with it.
//
// The native mainui screens were never deleted. CMenuServerBrowser (1391 lines,
// menus/ServerBrowser.cpp), CMenuCreateGame, CMenuOptions and the whole
// Configuration tree are all still registered and working; they were simply
// disconnected from the buttons. So this is a routing decision, not a rewrite.
//
// `ui_windowed_dialogs` picks the routing:
//
//    0 (default) -- native mainui screens. The game is playable.
//    1           -- the windowed dialogs, for working on them.
//
// It is deliberately NOT FCVAR_ARCHIVE. A half-finished UI that a config file
// can make permanent is how the game became unreachable in the first place; this
// way a bad experiment lasts until the next restart and no further.
static cvar_t *ui_windowed_dialogs;

static bool UI_UseWindowedDialogs( void )
{
	return ui_windowed_dialogs && ui_windowed_dialogs->value != 0.0f;
}

static void UI_VguiClientCmd( const char *cmd )
{
	EngFuncs::ClientCmd( false, cmd );
}

static bool s_vguiInitDone = false;

static void UI_ShowVguiOptions( void )
{
	if( !s_vguiInitDone )
	{
		Con_Printf( "[VGUI] init: ScreenWidth=%g ScreenHeight=%g\n", ScreenWidth, ScreenHeight );
		VGUI_SetCvarFuncs(
			EngFuncs::engfuncs.pfnGetCvarFloat,
			EngFuncs::engfuncs.pfnCvarSetValue,
			EngFuncs::engfuncs.pfnGetCvarString,
			EngFuncs::engfuncs.pfnCvarSetString,
			UI_VguiClientCmd
		);
		s_vguiInitDone = true;
		Con_Printf( "[VGUI] init done, calling VGUI_ShowOptions\n" );
	}
	// Refresh the screen size on EVERY open: the player may have changed
	// resolution since the dialog was first initialized. This keeps the VGUI
	// canvas, hit-testing and the drag/resize clamps aligned with the real
	// screen (cvar funcs only need to be wired once, above).
	VGUI_SetScreenSize( ScreenWidth, ScreenHeight );
	VGUI_ShowOptions();
	Con_Printf( "[VGUI] returned from VGUI_ShowOptions\n" );
}

// --- button routing -------------------------------------------------------
//
// One function per button, each choosing between the native screen and the
// windowed dialog. Written as functions rather than as an `if` around the
// assignment in _VidInit because _VidInit runs once per video init, while the
// cvar can be flipped at any point from the console -- reading it at click time
// means the switch takes effect immediately instead of after a vid_restart.

static void UI_Main_MultiplayerCb( void )
{
	if( UI_UseWindowedDialogs( ))
		WndServerBrowser_Show();
	else
		UI_MultiPlayer_Menu();
}

static void UI_Main_ConfigurationCb( void )
{
	if( UI_UseWindowedDialogs( ))
		UI_ShowVguiOptions();
	else
		UI_Options_Menu();
}

static void UI_Main_ConsoleCb( void )
{
	if( UI_UseWindowedDialogs( ))
	{
		WndConsole_Show();
		return;
	}

	// The engine's own console, which is what the button did before the windowed
	// one existed: close the menu and hand the keyboard to the console.
	UI_SetActiveMenu( false );
	EngFuncs::KEY_SetDest( KEY_CONSOLE );
}

#define ART_MINIMIZE_N	"gfx/shell/min_n"
#define ART_MINIMIZE_F	"gfx/shell/min_f"
#define ART_MINIMIZE_D	"gfx/shell/min_d"
#define ART_CLOSEBTN_N	"gfx/shell/cls_n"
#define ART_CLOSEBTN_F	"gfx/shell/cls_f"
#define ART_CLOSEBTN_D	"gfx/shell/cls_d"

class CMenuMain: public CMenuFramework
{
public:
	CMenuMain() : CMenuFramework( "CMenuMain" ) { }

	bool KeyDown( int key ) override;

private:
	void _Init() override;
	void _VidInit( ) override;
	void Think() override;

	void VidInit(bool connected);

	void QuitDialogCb();
	void DisconnectCb();
	void DisconnectDialogCb();
	void HazardCourseDialogCb();
	void HazardCourseCb();

	CMenuAnimatedBanner animatedBanner;
	CMenuMovieBanner movieBanner;

	CMenuPicButton	console;
	CMenuPicButton	resumeGame;
	CMenuPicButton	disconnect;
	CMenuPicButton	newGame;
	CMenuPicButton	hazardCourse;
	CMenuPicButton	configuration;
	CMenuPicButton	saveRestore;
	CMenuPicButton	multiPlayer;
	CMenuPicButton	customGame;
	CMenuPicButton	previews;
	CMenuPicButton	quit;

	// buttons on top right. Maybe should be drawn if fullscreen == 1?
	CMenuBitmap	minimizeBtn;
	CMenuBitmap	quitButton;

	// quit dialog
	CMenuYesNoMessageBox dialog;

	bool bTrainMap;
	bool bCustomGame;
};

void CMenuMain::QuitDialogCb()
{
	if( CL_IsActive() && EngFuncs::GetCvarFloat( "host_serverstate" ) && EngFuncs::GetCvarFloat( "maxplayers" ) == 1.0f )
		dialog.SetMessage( L( "StringsList_235" ) );
	else
		dialog.SetMessage( L( "GameUI_QuitConfirmationText" ) );

	dialog.onPositive.SetCommand( false, "quit\n" );
	dialog.Show();
}

void CMenuMain::DisconnectCb()
{
	EngFuncs::ClientCmd( false, "disconnect\n" );
	VidInit( false );
	CalcPosition();
	CalcSizes();
	VidInitItems();
}

void CMenuMain::DisconnectDialogCb()
{
	dialog.onPositive = VoidCb( &CMenuMain::DisconnectCb );
	dialog.SetMessage( L( "Really disconnect?" ) );
	dialog.Show();
}

void CMenuMain::HazardCourseDialogCb()
{
	dialog.onPositive = VoidCb( &CMenuMain::HazardCourseCb );;
	dialog.SetMessage( L( "StringsList_234" ) );
	dialog.Show();
}

/*
=================
CMenuMain::Key
=================
*/
bool CMenuMain::KeyDown( int key )
{
	if( UI::Key::IsEscape( key ) )
	{
		if ( CL_IsActive( ))
		{
			if( !dialog.IsVisible() )
				UI_CloseMenu();
		}
		else
		{
			QuitDialogCb( );
		}
		return true;
	}
	return CMenuFramework::KeyDown( key );
}

/*
=================
UI_Main_HazardCourse
=================
*/
void CMenuMain::HazardCourseCb()
{
	if( EngFuncs::GetCvarFloat( "host_serverstate" ) && EngFuncs::GetCvarFloat( "maxplayers" ) > 1 )
		EngFuncs::HostEndGame( "end of the game" );

	EngFuncs::CvarSetValue( "skill", 1.0f );
	EngFuncs::CvarSetValue( "deathmatch", 0.0f );
	EngFuncs::CvarSetValue( "teamplay", 0.0f );
	EngFuncs::CvarSetValue( "pausable", 1.0f ); // singleplayer is always allowing pause
	EngFuncs::CvarSetValue( "coop", 0.0f );
	EngFuncs::CvarSetValue( "maxplayers", 1.0f ); // singleplayer

	EngFuncs::PlayBackgroundTrack( NULL, NULL );

	EngFuncs::ClientCmd( false, "hazardcourse\n" );
}

void CMenuMain::_Init( void )
{
	// Registered here rather than in BaseMenu's cvar block: this is the main
	// menu's own routing switch, and keeping the declaration next to the code
	// that reads it is what keeps the two from drifting apart. CvarRegister
	// returns the existing cvar if it was already created, so calling it again
	// on a second _Init is harmless.
	ui_windowed_dialogs = EngFuncs::CvarRegister( "ui_windowed_dialogs", "0", 0 );

	if( gMenu.m_gameinfo.trainmap[0] && stricmp( gMenu.m_gameinfo.trainmap, gMenu.m_gameinfo.startmap ) != 0 )
		bTrainMap = true;
	else bTrainMap = false;

	if( EngFuncs::GetCvarFloat( "host_allow_changegame" ))
		bCustomGame = true;
	else bCustomGame = false;

	// console
	console.SetNameAndStatus( L( "GameUI_Console" ), L( "Show console" ) );
	console.iFlags |= QMF_NOTIFY;
	console.SetPicture( PC_CONSOLE );
	console.SetVisibility( gpGlobals->developer );
	console.onReleased = UI_Main_ConsoleCb;

	resumeGame.SetNameAndStatus( L( "GameUI_GameMenu_ResumeGame" ), L( "StringsList_188" ) );
	resumeGame.SetPicture( PC_RESUME_GAME );
	resumeGame.iFlags |= QMF_NOTIFY;
	resumeGame.onReleased = UI_CloseMenu;

	disconnect.SetNameAndStatus( L( "GameUI_GameMenu_Disconnect" ), L( "Disconnect from server" ) );
	disconnect.SetPicture( PC_DISCONNECT );
	disconnect.iFlags |= QMF_NOTIFY;
	disconnect.onReleased = VoidCb( &CMenuMain::DisconnectDialogCb );

	newGame.SetNameAndStatus( L( "GameUI_NewGame" ), L( "StringsList_189" ) );
	newGame.SetPicture( PC_NEW_GAME );
	newGame.iFlags |= QMF_NOTIFY;
	newGame.onReleased = UI_NewGame_Menu;

	hazardCourse.SetNameAndStatus( L( "GameUI_TrainingRoom" ), L( "StringsList_190" ) );
	hazardCourse.SetPicture( PC_HAZARD_COURSE );
	hazardCourse.iFlags |= QMF_NOTIFY;
	hazardCourse.onReleasedClActive = VoidCb( &CMenuMain::HazardCourseDialogCb );
	hazardCourse.onReleased = VoidCb( &CMenuMain::HazardCourseCb );

	multiPlayer.SetNameAndStatus( L( "GameUI_Multiplayer" ), L( "StringsList_198" ) );
	multiPlayer.SetPicture( PC_MULTIPLAYER );
	multiPlayer.iFlags |= QMF_NOTIFY;
	multiPlayer.onReleased = UI_Main_MultiplayerCb;

	configuration.SetNameAndStatus( L( "GameUI_Options" ), L( "StringsList_193" ) );
	configuration.SetPicture( PC_CONFIG );
	configuration.iFlags |= QMF_NOTIFY;
	configuration.onReleased = UI_Main_ConfigurationCb;

	saveRestore.iFlags |= QMF_NOTIFY;

	customGame.SetNameAndStatus( L( "GameUI_ChangeGame" ), L( "StringsList_530" ) );
	customGame.SetPicture( PC_CUSTOM_GAME );
	customGame.iFlags |= QMF_NOTIFY;
	customGame.onReleased = UI_CustomGame_Menu;

	previews.SetNameAndStatus( L( "Previews" ), L( "StringsList_400" ) );
	previews.SetPicture( PC_PREVIEWS );
	previews.iFlags |= QMF_NOTIFY;
	SET_EVENT( previews.onReleased, EngFuncs::ShellExecute( MenuStrings[ IDS_MEDIA_PREVIEWURL ], NULL, false ) );

	quit.SetNameAndStatus( L( "GameUI_GameMenu_Quit" ), L( "GameUI_QuitConfirmationText" ) );
	quit.SetPicture( PC_QUIT );
	quit.iFlags |= QMF_NOTIFY;
	quit.onReleased = VoidCb( &CMenuMain::QuitDialogCb );

	quitButton.SetPicture( ART_CLOSEBTN_N, ART_CLOSEBTN_F, ART_CLOSEBTN_D );
	quitButton.iFlags = QMF_MOUSEONLY;
	quitButton.eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
	quitButton.onReleased = VoidCb( &CMenuMain::QuitDialogCb );

	minimizeBtn.SetPicture( ART_MINIMIZE_N, ART_MINIMIZE_F, ART_MINIMIZE_D );
	minimizeBtn.iFlags = QMF_MOUSEONLY;
	minimizeBtn.eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
	minimizeBtn.onReleased.SetCommand( false, "minimize\n" );

	if ( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY || gMenu.m_gameinfo.startmap[0] == 0 )
		newGame.SetGrayed( true );

	if ( gMenu.m_gameinfo.gamemode == GAME_SINGLEPLAYER_ONLY )
		multiPlayer.SetGrayed( true );

	if ( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY )
	{
		saveRestore.SetGrayed( true );
		hazardCourse.SetGrayed( true );
	}

	// too short execute string - not a real command
	if( strlen( MenuStrings[IDS_MEDIA_PREVIEWURL] ) <= 3 )
	{
		previews.SetGrayed( true );
	}

	// server.dll needs for reading savefiles or startup newgame
	if( !EngFuncs::CheckGameDll( ))
	{
		saveRestore.SetGrayed( true );
		hazardCourse.SetGrayed( true );
		newGame.SetGrayed( true );
	}

	if( FBitSet( gMenu.m_gameinfo.flags, GFL_ANIMATED_TITLE ))
	{
		if( animatedBanner.TryLoad())
			AddItem( animatedBanner );
	}
	else
	{
		AddItem( movieBanner );
	}

	dialog.Link( this );

	AddItem( banner );
	AddItem( console );
	AddItem( disconnect );
	AddItem( resumeGame );
	AddItem( newGame );

	if ( bTrainMap )
		AddItem( hazardCourse );

	AddItem( configuration );
	AddItem( saveRestore );
	AddItem( multiPlayer );

	if ( bCustomGame )
		AddItem( customGame );

	AddItem( previews );
	AddItem( quit );
	AddItem( minimizeBtn );
	AddItem( quitButton );
}

/*
=================
UI_Main_Init
=================
*/
void CMenuMain::VidInit( bool connected )
{
	int hoffset = ( 70 / 640.0 ) * 1024.0;

	// in original menu Previews is located at specific point
	int previews_voffset = ( 404 / 480.0 ) * 768.0;

	// no visible console button gap
	int ygap = (( 404 - 373 ) / 480.0 ) * 768.0;

	// statically positioned items
	minimizeBtn.SetRect( uiStatic.width - 72, 13, 32, 32 );
	quitButton.SetRect( uiStatic.width - 36, 13, 32, 32 );

	previews.SetCoord( hoffset, previews_voffset );
	quit.SetCoord( hoffset, previews_voffset + ygap );

	// let's start calculating positions
	int yoffset = previews_voffset - ygap;

	if( bCustomGame )
	{
		customGame.SetCoord( hoffset, yoffset );
		yoffset -= ygap;
	}

	multiPlayer.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	bool single = gpGlobals->maxClients < 2;

	saveRestore.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	configuration.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	if( bTrainMap )
	{
		hazardCourse.SetCoord( hoffset, yoffset );
		yoffset -= ygap;
	}

	newGame.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	if( connected )
	{
		resumeGame.SetCoord( hoffset, yoffset );
		yoffset -= ygap;

		if( !single )
		{
			disconnect.SetCoord( hoffset, yoffset );
			yoffset -= ygap;
		}
	}

	console.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	// now figure out what's visible
	resumeGame.SetVisibility( connected );
	disconnect.SetVisibility( connected && !single );

	if( connected && single )
	{
		saveRestore.SetNameAndStatus( L( "Save\\Load Game" ), L( "StringsList_192" ) );
		saveRestore.SetPicture( PC_SAVE_LOAD_GAME );
		saveRestore.onReleased = UI_SaveLoad_Menu;
	}
	else
	{
		saveRestore.SetNameAndStatus( L( "GameUI_LoadGame" ), L( "StringsList_191" ) );
		saveRestore.SetPicture( PC_LOAD_GAME );
		saveRestore.onReleased = UI_LoadGame_Menu;
	}
}

void CMenuMain::_VidInit()
{
	VidInit( CL_IsActive() );
}

void CMenuMain::Think()
{
	if( gpGlobals->developer )
	{
		if( !console.IsVisible( ))
			console.Show();
	}
	else
	{
		if( console.IsVisible( ))
			console.Hide();
	}

	CMenuFramework::Think();
}

ADD_MENU( menu_main, CMenuMain, UI_Main_Menu );
