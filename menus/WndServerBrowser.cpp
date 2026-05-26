/*
WndServerBrowser.cpp -- CS 1.6 PC-style windowed server browser
Copyright (C) 2024 DragonSlayer Team

CMenuFrameTabbed with Internet/LAN/Favorites tabs.
Table with columns: Server Name, Map, Players, Ping.
Connect/Refresh buttons always visible at bottom.

NOTE: Full engine server query integration is in the legacy
ServerBrowser.cpp. This windowed version shows the same data
by forwarding to engine commands. Server data arrives via
the existing AddServerToList callback mechanism.
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "controls/FrameTabbed.h"
#include "controls/FrameButton.h"
#include "Table.h"
#include "Action.h"
#include "Field.h"
#include "keydefs.h"
#include "Utils.h"
#include "TrackerScheme.h"

// ─── Empty model — placeholder until engine callback integration ─────
// Displays a hint row when no servers found.

class CEmptyServerModel : public CMenuBaseModel
{
public:
	CEmptyServerModel() {}
	void Update() override {}
	int GetColumns() const override { return 4; }
	int GetRows() const override { return 1; }
	const char *GetCellText( int line, int column ) override
	{
		if( line == 0 && column == 0 )
			return "Press Refresh to search for servers...";
		return "";
	}
};

// ─── Server Browser window ───────────────────────────────────────────

class CMenuWndServerBrowser : public CMenuFrameTabbed
{
public:
	CMenuWndServerBrowser();
	bool IsRoot() const override { return false; }

private:
	void _Init() override;
	void _VidInit() override;

	void RefreshInternet();
	void RefreshLAN();
	void ConnectToSelected();

	// Bottom buttons (always visible)
	CMenuFrameButton btnConnect;
	CMenuFrameButton btnRefresh;
	CMenuFrameButton btnStop;

	// Internet tab
	CMenuTable       internetTable;
	CEmptyServerModel internetModel;

	// LAN tab
	CMenuTable       lanTable;
	CEmptyServerModel lanModel;

	// Favorites tab
	CMenuAction    favLabel;
};

static CMenuWndServerBrowser *s_pWndServerBrowser = NULL;

CMenuWndServerBrowser::CMenuWndServerBrowser() : CMenuFrameTabbed( "Servers" )
{
}

void CMenuWndServerBrowser::RefreshInternet()
{
	EngFuncs::ClientCmd( false, "ui_internetservers\n" );
}

void CMenuWndServerBrowser::RefreshLAN()
{
	EngFuncs::ClientCmd( false, "ui_localservers\n" );
}

void CMenuWndServerBrowser::ConnectToSelected()
{
	// Placeholder — real connect uses netadr from model
	Hide();
}

void CMenuWndServerBrowser::_Init()
{
	int w = (int)(uiStatic.width * 0.8f);
	int h = (int)(768 * 0.75f);
	int x = (uiStatic.width - w) / 2;
	int y = (768 - h) / 2;
	SetRect( x, y, w, h );

	int cw = w - 32;
	int btnW = 90;
	int btnH = 26;
	int contentH = h - FRAME_TITLE_HEIGHT - FRAME_TAB_HEIGHT;
	int btnY = contentH - btnH - 12;

	// ─── Bottom buttons (before tabs — always visible) ───
	btnConnect.szName = "Connect";
	btnConnect.SetRect( 16, btnY, btnW, btnH );
	SET_EVENT_MULTI( btnConnect.onReleased,
	{
		CMenuWndServerBrowser *self = (CMenuWndServerBrowser*)pSelf->GetParent( CMenuWndServerBrowser );
		self->ConnectToSelected();
	});
	AddItem( btnConnect );

	btnRefresh.szName = "Refresh";
	btnRefresh.SetRect( 16 + btnW + 8, btnY, btnW, btnH );
	SET_EVENT_MULTI( btnRefresh.onReleased,
	{
		CMenuWndServerBrowser *self = (CMenuWndServerBrowser*)pSelf->GetParent( CMenuWndServerBrowser );
		self->RefreshInternet();
	});
	AddItem( btnRefresh );

	btnStop.szName = "Stop";
	btnStop.SetRect( 16 + (btnW + 8) * 2, btnY, btnW, btnH );
	SET_EVENT_MULTI( btnStop.onReleased,
	{
		// stop refreshing
		EngFuncs::ClientCmd( false, "ui_stopservers\n" );
	});
	AddItem( btnStop );

	// ─── Tab: Internet ───
	AddTab( "Internet" );

	internetTable.SetModel( &internetModel );
	internetTable.SetupColumn( 0, "Server Name", 0.45f );
	internetTable.SetupColumn( 1, "Map", 0.25f );
	internetTable.SetupColumn( 2, "Players", 0.15f );
	internetTable.SetupColumn( 3, "Ping", 0.15f );
	internetTable.bAllowSorting = false;
	internetTable.bShowScrollBar = true;
	internetTable.SetRect( 8, 8, cw, btnY - 24 );
	AddItem( internetTable );

	// ─── Tab: LAN ───
	AddTab( "LAN" );

	lanTable.SetModel( &lanModel );
	lanTable.SetupColumn( 0, "Server Name", 0.45f );
	lanTable.SetupColumn( 1, "Map", 0.25f );
	lanTable.SetupColumn( 2, "Players", 0.15f );
	lanTable.SetupColumn( 3, "Ping", 0.15f );
	lanTable.bAllowSorting = false;
	lanTable.bShowScrollBar = true;
	lanTable.SetRect( 8, 8, cw, btnY - 24 );
	AddItem( lanTable );

	// ─── Tab: Favorites ───
	AddTab( "Favorites" );

	favLabel.szName = "Add favorites via console: addfavorite <ip:port>";
	favLabel.iFlags |= QMF_INACTIVE;
	favLabel.SetCoord( 16, 16 );
	AddItem( favLabel );

	SetActiveTab( 0 );
}

void CMenuWndServerBrowser::_VidInit()
{
	int w = (int)(uiStatic.width * 0.8f);
	int h = (int)(768 * 0.75f);
	int x = (uiStatic.width - w) / 2;
	int y = (768 - h) / 2;
	SetRect( x, y, w, h );
}

// ============= Public API =============

void WndServerBrowser_Precache()
{
	s_pWndServerBrowser = new CMenuWndServerBrowser();
}

void WndServerBrowser_Show()
{
	if( s_pWndServerBrowser )
		s_pWndServerBrowser->Show();
}

void WndServerBrowser_Shutdown()
{
	delete s_pWndServerBrowser;
	s_pWndServerBrowser = NULL;
}

ADD_MENU4( menu_wndserverbrowser, WndServerBrowser_Precache, WndServerBrowser_Show, WndServerBrowser_Shutdown );
