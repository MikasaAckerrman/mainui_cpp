/*
WndServerBrowser.cpp -- CS 1.6 PC-style windowed server browser
Copyright (C) 2024 DragonSlayer Team

CMenuFrameTabbed with Internet / LAN tabs.
Table columns: Server Name, Map, Players, Ping.
Connect / Refresh buttons always visible at bottom.

Server data arrives via WndServerBrowser_AddServerToList(),
called from UI_AddServerToList in ServerBrowser.cpp whenever
the engine delivers a new server to the menu DLL.
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

// ─── Per-entry data ────────────────────────────────────────────────

struct WndSrvEntry
{
	netadr_t adr;
	char     name[64];
	char     mapname[64];
	char     clientsstr[16];   // "12/32"
	char     pingstr[16];      // "45 ms"
	char     proto[8];         // "49" or "gs"

	WndSrvEntry() { memset( this, 0, sizeof(*this) ); }

	WndSrvEntry( netadr_t a, const char *info, float ping ) : adr(a)
	{
		Q_strncpy( name,    Info_ValueForKey( info, "host" ), sizeof(name)    );
		Q_strncpy( mapname, Info_ValueForKey( info, "map"  ), sizeof(mapname) );

		int numcl = atoi( Info_ValueForKey( info, "numcl" ) );
		int maxcl = atoi( Info_ValueForKey( info, "maxcl" ) );
		snprintf( clientsstr, sizeof(clientsstr), "%d/%d", numcl, maxcl );

		if( ping > 0.0f )
			snprintf( pingstr, sizeof(pingstr), "%.0f ms", ping * 1000.0f );
		else
			Q_strncpy( pingstr, "---", sizeof(pingstr) );

		bool isGoldSrc = !strcmp( Info_ValueForKey( info, "gs" ), "1" );
		Q_strncpy( proto, isGoldSrc ? "gs" : "49", sizeof(proto) );
	}
};

// ─── Real server model ──────────────────────────────────────────────────

class CWndServerModel : public CMenuBaseModel
{
public:
	CWndServerModel() { refreshStartTime = 0.0; }

	void Update() override {}

	int GetColumns() const override { return 4; }

	int GetRows() const override
	{
		return entries.Count() ? entries.Count() : 1;
	}

	const char *GetCellText( int line, int column ) override
	{
		if( entries.Count() == 0 )
			return (column == 0) ? "Press Refresh to search for servers..." : "";

		switch( column )
		{
		case 0: return entries[line].name;
		case 1: return entries[line].mapname;
		case 2: return entries[line].clientsstr;
		case 3: return entries[line].pingstr;
		}
		return "";
	}

	void AddServer( netadr_t adr, const char *info )
	{
		for( int i = 0; i < entries.Count(); i++ )
		{
			if( !EngFuncs::NET_CompareAdr( &entries[i].adr, &adr ) )
				return;
		}

		float elapsed = ( refreshStartTime > 0.0 )
		    ? (float)( EngFuncs::DoubleTime() - refreshStartTime )
		    : 0.0f;

		entries.AddToTail( WndSrvEntry(adr, info, elapsed) );
	}

	void Clear()
	{
		entries.RemoveAll();
		refreshStartTime = EngFuncs::DoubleTime();
	}

	const char *GetProto( int idx )
	{
		return entries.IsValidIndex(idx) ? entries[idx].proto : "49";
	}

	double refreshStartTime;
	CUtlVector<WndSrvEntry> entries;
};

// ─── Window ───────────────────────────────────────────────────────────────────

class CMenuWndServerBrowser : public CMenuFrameTabbed
{
public:
	CMenuWndServerBrowser();
	bool IsRoot() const override { return false; }

	void AddServer( netadr_t adr, const char *info );

private:
	void _Init()    override;
	void _VidInit() override;

	void RefreshInternet();
	void RefreshLAN();
	void ConnectToSelected();

	// bottom buttons (added before first AddTab -- always visible)
	CMenuFrameButton btnConnect;
	CMenuFrameButton btnRefresh;
	CMenuFrameButton btnStop;

	// Internet tab
	CMenuTable      internetTable;
	CWndServerModel internetModel;

	// LAN tab
	CMenuTable      lanTable;
	CWndServerModel lanModel;
};

static CMenuWndServerBrowser *s_pWndServerBrowser = NULL;

CMenuWndServerBrowser::CMenuWndServerBrowser()
	: CMenuFrameTabbed( "Find Servers" )
{
}

void CMenuWndServerBrowser::AddServer( netadr_t adr, const char *info )
{
	// route to whichever model was refreshed most recently
	if( lanModel.refreshStartTime > internetModel.refreshStartTime )
	{
		lanModel.AddServer( adr, info );
		lanTable.SetModel( &lanModel );
	}
	else
	{
		internetModel.AddServer( adr, info );
		internetTable.SetModel( &internetModel );
	}
}

void CMenuWndServerBrowser::RefreshInternet()
{
	internetModel.Clear();
	internetTable.SetModel( &internetModel );
	EngFuncs::ClientCmd( false, "internetservers\n" );
}

void CMenuWndServerBrowser::RefreshLAN()
{
	lanModel.Clear();
	lanTable.SetModel( &lanModel );
	EngFuncs::ClientCmd( false, "localservers\n" );
}

void CMenuWndServerBrowser::ConnectToSelected()
{
	int tab = GetActiveTab();
	CMenuTable      *tbl = (tab == 1) ? &lanTable      : &internetTable;
	CWndServerModel *mdl = (tab == 1) ? &lanModel      : &internetModel;

	int idx = tbl->GetCurrentIndex();
	if( idx < 0 || !mdl->entries.IsValidIndex(idx) )
		return;

	const char *sadr = EngFuncs::NET_AdrToString( mdl->entries[idx].adr );
	const char *prot = mdl->GetProto( idx );

	EngFuncs::ClientCmdF( false, "connect \"%s\" \"%s\"\n", sadr, prot );
	Hide();
}

void CMenuWndServerBrowser::_Init()
{
	int w = (int)(uiStatic.width * 0.8f);
	int h = (int)(768 * 0.75f);
	int x = (uiStatic.width - w) / 2;
	int y = (768            - h) / 2;
	SetRect( x, y, w, h );

	int cw      = w - 32;
	int btnW    = 90;
	int btnH    = 26;
	int contentH = h - FRAME_TITLE_HEIGHT - FRAME_TAB_HEIGHT;
	int btnY    = contentH - btnH - 12;
	int tableH  = btnY - 24;

	// ─── Bottom buttons (added before first AddTab -- always visible) ───
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
		if( self->GetActiveTab() == 1 )
			self->RefreshLAN();
		else
			self->RefreshInternet();
	});
	AddItem( btnRefresh );

	btnStop.szName = "Stop";
	btnStop.SetRect( 16 + (btnW + 8) * 2, btnY, btnW, btnH );
	SET_EVENT_MULTI( btnStop.onReleased, { (void)pSelf; });
	AddItem( btnStop );

	// ─── Tab: Internet ───
	AddTab( "Internet" );

	internetTable.SetupColumn( 0, "Server Name", 0.45f );
	internetTable.SetupColumn( 1, "Map",         0.25f );
	internetTable.SetupColumn( 2, "Players",     0.15f );
	internetTable.SetupColumn( 3, "Ping",        0.15f );
	internetTable.bAllowSorting  = false;
	internetTable.bShowScrollBar = true;
	internetTable.SetRect( 8, 8, cw, tableH );
	internetTable.SetModel( &internetModel );
	AddItem( internetTable );

	// ─── Tab: LAN ───
	AddTab( "LAN" );

	lanTable.SetupColumn( 0, "Server Name", 0.45f );
	lanTable.SetupColumn( 1, "Map",         0.25f );
	lanTable.SetupColumn( 2, "Players",     0.15f );
	lanTable.SetupColumn( 3, "Ping",        0.15f );
	lanTable.bAllowSorting  = false;
	lanTable.bShowScrollBar = true;
	lanTable.SetRect( 8, 8, cw, tableH );
	lanTable.SetModel( &lanModel );
	AddItem( lanTable );

	SetActiveTab( 0 );
}

void CMenuWndServerBrowser::_VidInit()
{
	int w = (int)(uiStatic.width * 0.8f);
	int h = (int)(768 * 0.75f);
	int x = (uiStatic.width - w) / 2;
	int y = (768            - h) / 2;
	SetRect( x, y, w, h );
}

// ─── Public API ────────────────────────────────────────────────────────────────

// Called from UI_AddServerToList (ServerBrowser.cpp) for every server
// the engine reports, so WndServerBrowser stays in sync without polling.
void WndServerBrowser_AddServerToList( netadr_t adr, const char *info )
{
	if( s_pWndServerBrowser && s_pWndServerBrowser->IsVisible() )
		s_pWndServerBrowser->AddServer( adr, info );
}

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
