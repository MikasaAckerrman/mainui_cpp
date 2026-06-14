/*
WndServerBrowser.cpp -- CS 1.6 / Source-Engine PC-style windowed server browser
Copyright (C) 2024 DragonSlayer Team

A CMenuFrameTabbed window reproducing the classic GoldSrc VGUI "Find Servers"
dialog for the DragonSlayer mod. Five tabs:

    Internet | NAT | LAN | Favorites | History

Feature parity with the legacy fullscreen CMenuServerBrowser (ServerBrowser.cpp):
  - live discovery (internetservers / localservers, cl_nat toggle)
  - per-server lock + favorite icons, sortable columns
  - ping / empty / full / map filters (Internet & NAT only)
  - favorites + history persisted to .lst files
  - password prompt for protected servers
  - "Add Server" dialog (IP:port + protocol)

Server data arrives asynchronously via WndServerBrowser_AddServerToList(), which
is invoked from UI_AddServerToList() in ServerBrowser.cpp for every server the
engine reports to the menu DLL. We route each incoming server to whichever tab
model was most recently refreshed.

NOTE: every file-local type below lives in an anonymous namespace. ServerBrowser.cpp
already declares globally-scoped types of the same family (server_t, filterMap_t,
favlist_entry_t, CMenuGameListModel). Giving our copies internal linkage keeps the
two translation units free of ODR / symbol collisions while still letting us mirror
the proven names from the reference implementation.
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "controls/FrameTabbed.h"
#include "controls/FrameButton.h"
#include "Table.h"
#include "Action.h"
#include "Field.h"
#include "SpinControl.h"
#include "DropDown.h"
#include "YesNoMessageBox.h"
#include "StringArrayModel.h"
#include "keydefs.h"
#include "Utils.h"
#include "TrackerScheme.h"
#include "utlvector.h"

#define ART_WND_LOCK     "gfx/shell/lock"
#define ART_WND_FAVORITE "gfx/shell/favorite"

#define WND_MAX_PING        9.999f
#define WND_FILTER_MAX_MAPS 16
#define WND_MAX_HISTORY     20

// The five tabs, in AddTab() order.
enum
{
	WND_TAB_INTERNET = 0,
	WND_TAB_NAT,
	WND_TAB_LAN,
	WND_TAB_FAVORITES,
	WND_TAB_HISTORY,
	WND_TAB_COUNT
};

// Table columns. The icon column renders a lock (Internet/NAT/LAN) or a star
// (Favorites/History) depending on the owning model's bFavColumn flag.
enum
{
	WND_COL_ICON = 0,
	WND_COL_NAME,
	WND_COL_MAP,
	WND_COL_PLAYERS,
	WND_COL_PING,
	WND_COL_LAST
};

namespace { class CMenuWndServerBrowser; }

namespace
{

//=============================================================================
// favlist_entry_t -- one persisted favorite/history record (mirrors legacy)
//=============================================================================
struct favlist_entry_t
{
	favlist_entry_t( const char *sadr, const char *prot, bool favorited = true )
	{
		Q_strncpy( this->sadr, sadr, sizeof( this->sadr ));
		Q_strncpy( this->prot, prot, sizeof( this->prot ));
		this->favorited = favorited;
	}

	void QueryServer( void ) const
	{
		EngFuncs::ClientCmdF( false, "ui_queryserver \"%s\" \"%s\"", sadr, prot );
	}

	char sadr[128];
	char prot[16];  // "49" (Xash) or "gs" (GoldSource)
	bool favorited;
};

//=============================================================================
// WndSrvEntry -- one row of the server table
//=============================================================================
struct WndSrvEntry
{
	netadr_t adr;
	char     name[64];
	char     mapname[64];
	char     clientsstr[16];   // "12/32"
	char     pingstr[16];      // "45 ms"
	char     proto[8];         // "49" or "gs"
	char     ipstr[32];        // "1.2.3.4:27015"

	int      numcl;
	int      maxcl;
	float    ping;             // seconds, bounded to WND_MAX_PING

	bool     havePassword;
	bool     favorite;
	bool     isGoldSrc;
	bool     pending;          // placeholder waiting for a query response

	double   pingStartTime;    // moment the query was issued (for ping calc)

	WndSrvEntry()
	{
		memset( this, 0, sizeof( *this ));
		ping = WND_MAX_PING;
	}

	WndSrvEntry( netadr_t a, const char *info, double startTime, bool is_favorite, bool is_pending = false )
	{
		memset( this, 0, sizeof( *this ));
		adr           = a;
		favorite      = is_favorite;
		pending       = is_pending;
		pingStartTime = startTime;
		Fill( info );
	}

	// (Re)parse an info string into the display fields and compute ping.
	void Fill( const char *info )
	{
		Q_strncpy( name,    Info_ValueForKey( info, "host" ), sizeof( name ));
		Q_strncpy( mapname, Info_ValueForKey( info, "map"  ), sizeof( mapname ));
		Q_strncpy( ipstr,   EngFuncs::NET_AdrToString( adr ), sizeof( ipstr ));

		numcl = atoi( Info_ValueForKey( info, "numcl" ));
		maxcl = atoi( Info_ValueForKey( info, "maxcl" ));
		snprintf( clientsstr, sizeof( clientsstr ), "%d/%d", numcl, maxcl );

		havePassword = !strcmp( Info_ValueForKey( info, "password" ), "1" );
		isGoldSrc    = !strcmp( Info_ValueForKey( info, "gs" ), "1" );
		Q_strncpy( proto, isGoldSrc ? "gs" : "49", sizeof( proto ));

		SetPingFromNow();
	}

	void SetPing( float seconds )
	{
		ping = bound( 0.0f, seconds, WND_MAX_PING );
		snprintf( pingstr, sizeof( pingstr ), "%.f ms", ping * 1000.0f );
	}

	void SetPingFromNow( void )
	{
		if( pingStartTime > 0.0 )
			SetPing( (float)( EngFuncs::DoubleTime() - pingStartTime ));
		else
			Q_strncpy( pingstr, "---", sizeof( pingstr ));
	}

	bool IsEmpty( void ) const { return numcl == 0; }
	bool IsFull( void )  const { return maxcl > 0 && numcl >= maxcl; }

	const char *ToProtocol( void ) const { return isGoldSrc ? "gs" : "49"; }

	// ---- comparators for qsort-based column sorting ----
	int NameCmp( const WndSrvEntry &o ) const { return colorstricmp( name, o.name ); }
	int MapCmp( const WndSrvEntry &o )  const { return stricmp( mapname, o.mapname ); }
	int ClientCmp( const WndSrvEntry &o ) const
	{
		if( numcl > o.numcl ) return 1;
		if( numcl < o.numcl ) return -1;
		return 0;
	}
	int PingCmp( const WndSrvEntry &o ) const
	{
		if( ping > o.ping ) return 1;
		if( ping < o.ping ) return -1;
		return 0;
	}

#define WND_GEN_COMPAR( method ) \
	static int method##Ascend( const void *a, const void *b ) \
	{ return (( const WndSrvEntry * )a)->method( *(( const WndSrvEntry * )b )); } \
	static int method##Descend( const void *a, const void *b ) \
	{ return (( const WndSrvEntry * )b)->method( *(( const WndSrvEntry * )a )); }

	WND_GEN_COMPAR( NameCmp )
	WND_GEN_COMPAR( MapCmp )
	WND_GEN_COMPAR( ClientCmp )
	WND_GEN_COMPAR( PingCmp )
#undef WND_GEN_COMPAR
};

//=============================================================================
// filterMap_t -- accumulator that feeds the per-tab "map" filter dropdown
//=============================================================================
struct filterMap_t
{
	char           name[64];
	unsigned short count;

	filterMap_t() { name[0] = 0; count = 0; display[0] = 0; }
	filterMap_t( const char *s, unsigned short c = 0 )
	{
		Q_strncpy( name, s, sizeof( name ));
		count = c;
		UpdateDisplay();
	}

	bool operator==( const char *s ) const { return colorstricmp( name, s ) == 0; }

	void AddCount( unsigned short c ) { count += c; UpdateDisplay(); }
	const char *GetDisplay( void ) const { return display; }

	static int CmpCountInvert( const filterMap_t *a, const filterMap_t *b )
	{
		if( a->count < b->count ) return 1;
		if( a->count > b->count ) return -1;
		return 0;
	}

private:
	char display[128];
	void UpdateDisplay( void ) { snprintf( display, sizeof( display ), "(%d) %s", count, name ); }
};

//=============================================================================
// CWndServerModel -- table data source backing one tab
//=============================================================================
class CWndServerModel : public CMenuBaseModel
{
public:
	CWndServerModel() :
		parent( NULL ), bFavColumn( false ), refreshStartTime( 0.0 ),
		m_iSortingColumn( WND_COL_PING ), m_bAscend( true )
	{
		filterPing  = WND_MAX_PING * 1000.0f;
		filterEmpty = 0;
		filterFull  = 0;
		filterMapName[0] = 0;
	}

	void SetParent( CMenuWndServerBrowser *p, bool favColumn )
	{
		parent     = p;
		bFavColumn = favColumn;
	}

	void Update() override
	{
		if( entries.Count() && m_iSortingColumn != -1 )
			Sort( m_iSortingColumn, m_bAscend );
	}

	int GetColumns() const override { return WND_COL_LAST; }
	int GetRows()    const override { return entries.Count(); }

	ECellType GetCellType( int line, int column ) override
	{
		if( column == WND_COL_ICON )
			return CELL_IMAGE_ADDITIVE;
		return CELL_TEXT;
	}

	const char *GetCellText( int line, int column ) override
	{
		if( !entries.IsValidIndex( line ))
			return NULL;

		const WndSrvEntry &e = entries[line];

		switch( column )
		{
		case WND_COL_ICON:
			if( bFavColumn )
				return e.favorite ? ART_WND_FAVORITE : NULL;
			return e.havePassword ? ART_WND_LOCK : NULL;
		case WND_COL_NAME:    return e.name;
		case WND_COL_MAP:     return e.mapname;
		case WND_COL_PLAYERS: return e.clientsstr;
		case WND_COL_PING:    return e.pingstr;
		}
		return NULL;
	}

	bool GetCellColors( int line, int column, unsigned int &textColor, bool &force ) const override
	{
		return false;
	}

	void OnActivateEntry( int line ) override; // defined after CMenuWndServerBrowser

	bool Sort( int column, bool ascend ) override
	{
		m_iSortingColumn = column;
		if( column == -1 )
			return false;

		m_bAscend = ascend;
		switch( column )
		{
		case WND_COL_NAME:
			qsort( entries.Base(), entries.Count(), sizeof( WndSrvEntry ),
				ascend ? WndSrvEntry::NameCmpAscend : WndSrvEntry::NameCmpDescend );
			return true;
		case WND_COL_MAP:
			qsort( entries.Base(), entries.Count(), sizeof( WndSrvEntry ),
				ascend ? WndSrvEntry::MapCmpAscend : WndSrvEntry::MapCmpDescend );
			return true;
		case WND_COL_PLAYERS:
			qsort( entries.Base(), entries.Count(), sizeof( WndSrvEntry ),
				ascend ? WndSrvEntry::ClientCmpAscend : WndSrvEntry::ClientCmpDescend );
			return true;
		case WND_COL_PING:
			qsort( entries.Base(), entries.Count(), sizeof( WndSrvEntry ),
				ascend ? WndSrvEntry::PingCmpAscend : WndSrvEntry::PingCmpDescend );
			return true;
		}
		return false;
	}

	// Reset for a fresh discovery pass. Records the start time so we can
	// derive ping from the round-trip of each reply.
	void Clear( void )
	{
		entries.RemoveAll();
		filterMaps.RemoveAll();
		refreshStartTime = EngFuncs::DoubleTime();
	}

	// Insert/refresh a server. Returns true if anything changed.
	bool AddServer( netadr_t adr, const char *info, bool is_favorite );

	// Append a placeholder row for a favorite/history entry we haven't heard
	// back from yet. The real data replaces it once the query reply arrives.
	void AddPlaceholder( netadr_t adr, const char *sadr, const char *prot, bool is_favorite )
	{
		for( int i = 0; i < entries.Count(); i++ )
			if( !EngFuncs::NET_CompareAdr( &entries[i].adr, &adr ))
				return;

		WndSrvEntry e;
		e.adr           = adr;
		e.favorite      = is_favorite;
		e.pending       = true;
		e.pingStartTime = refreshStartTime;
		Q_strncpy( e.name, sadr, sizeof( e.name ));
		Q_strncpy( e.ipstr, sadr, sizeof( e.ipstr ));
		Q_strncpy( e.mapname, "...", sizeof( e.mapname ));
		Q_strncpy( e.clientsstr, "0/0", sizeof( e.clientsstr ));
		Q_strncpy( e.pingstr, "---", sizeof( e.pingstr ));
		Q_strncpy( e.proto, ( !stricmp( prot, "gs" ) || !strcmp( prot, "48" )) ? "gs" : "49", sizeof( e.proto ));
		e.isGoldSrc = !strcmp( e.proto, "gs" );

		entries.AddToTail( e );
	}

	void SetFilterMap( const char *mapname )
	{
		Q_strncpy( filterMapName, mapname ? mapname : "", sizeof( filterMapName ));
	}

	// --- public state ---
	CMenuWndServerBrowser  *parent;
	bool                    bFavColumn;
	double                  refreshStartTime;

	CUtlVector<WndSrvEntry>  entries;
	CUtlVector<filterMap_t>  filterMaps;

	float filterPing;
	char  filterEmpty;
	char  filterFull;
	char  filterMapName[64];

private:
	int  m_iSortingColumn;
	bool m_bAscend;

	void AccumulateMap( const WndSrvEntry &e ); // needs parent, defined later
};

//=============================================================================
// CMenuWndServerBrowser -- the tabbed window itself
//=============================================================================
class CMenuWndServerBrowser : public CMenuFrameTabbed
{
public:
	CMenuWndServerBrowser();
	bool IsRoot() const override { return false; }

	void Show() override;
	void Hide() override;
	void Draw() override;

	// dispatch from the engine
	void AddServerToList( netadr_t adr, const char *info );

	// invoked by the active model when a row is double-clicked / activated
	void ConnectFromModel( CWndServerModel *model, int line );

	// invoked by a model when its map-set grows (rebuilds the right dropdown)
	void RebuildMapFilterFor( CWndServerModel *model );

	// filter dropdown handlers (public so SET_EVENT_MULTI lambdas can call them)
	void ApplyPingFilter( CMenuDropDownFloat *dd, float seconds );
	void ApplyEmptyFilter( CMenuDropDownInt *dd, char value );
	void ApplyFullFilter( CMenuDropDownInt *dd, char value );
	void ApplyMapFilter( CMenuDropDownStr *dd, const char *value );

private:
	void _Init()    override;
	void _VidInit() override;

	void ComputeWindowRect();
	void DoLayout();

	CWndServerModel *ActiveModel();
	CMenuTable      *ActiveTable();

	// refresh dispatchers
	void RefreshActive();
	void RefreshInternet();
	void RefreshNAT();
	void RefreshLAN();
	void RefreshFavorites();
	void RefreshHistory();

	// actions
	void ConnectToSelected();
	void DoConnect( const WndSrvEntry &e, bool fromInternet );
	void ConnectSaved();
	void FavoriteSelected();
	void ShowAddServerBox();
	void AddServerFromDialog();
	void OnSelectionChanged();
	void RefreshButtonsForSelection();

	// persistence
	void LoadLists();
	void SaveLists();
	void ParseServerListFromFile( const char *filename, CUtlVector<favlist_entry_t> &list );
	void SaveServerListToFile( const char *filename, const CUtlVector<favlist_entry_t> &list );
	void QueryServerList( const CUtlVector<favlist_entry_t> &list, CWndServerModel &model, bool favTab );

	void SetupServerTable( CMenuTable &table, CWndServerModel &model, bool favColumn );
	void SetupFilterDropdowns( CMenuDropDownFloat &ping, CMenuDropDownInt &empty,
		CMenuDropDownInt &full, CMenuDropDownStr &map );

	// --- always-visible bottom buttons (added before the first AddTab) ---
	CMenuFrameButton btnConnect;
	CMenuFrameButton btnRefresh;
	CMenuFrameButton btnFavorite;
	CMenuFrameButton btnAddServer;
	CMenuFrameButton btnDone;

	// --- per-tab tables + models ---
	CMenuTable       internetTable, natTable, lanTable, favoritesTable, historyTable;
	CWndServerModel  internetModel, natModel, lanModel, favoritesModel, historyModel;

	// --- Internet/NAT filters ---
	CMenuDropDownFloat internetPing,  natPing;
	CMenuDropDownInt   internetEmpty, natEmpty;
	CMenuDropDownInt   internetFull,  natFull;
	CMenuDropDownStr   internetMap,   natMap;

	// --- password dialog ---
	CMenuYesNoMessageBox askPassword;
	CMenuField           passwordField;

	// --- add-server dialog ---
	CMenuYesNoMessageBox addServerBox;
	CMenuField           addressField;
	CMenuSpinControl     protocolSpin;

	// --- persisted lists ---
	CUtlVector<favlist_entry_t> favoritesList;
	CUtlVector<favlist_entry_t> historyList;

	int  m_refreshTime;   // uiStatic.realTime gate for periodic auto-refresh
	int  m_lastTab;       // detect tab switches in Draw()
	bool m_savedValid;    // a password-protected connect is pending
	WndSrvEntry m_savedServer;
	bool m_savedFromInternet;
};

static CMenuWndServerBrowser *s_pWndServerBrowser = NULL;

//=============================================================================
// CWndServerModel methods that depend on the full browser type
//=============================================================================
bool CWndServerModel::AddServer( netadr_t adr, const char *info, bool is_favorite )
{
	// Skip duplicates, but let a real reply overwrite a placeholder row.
	for( int i = 0; i < entries.Count(); i++ )
	{
		if( !EngFuncs::NET_CompareAdr( &entries[i].adr, &adr ))
		{
			if( entries[i].pending )
			{
				bool fav  = entries[i].favorite || is_favorite;
				double st = entries[i].pingStartTime > 0.0 ? entries[i].pingStartTime : refreshStartTime;
				entries[i] = WndSrvEntry( adr, info, st, fav, false );
				return true;
			}
			return false;
		}
	}

	WndSrvEntry e( adr, info, refreshStartTime, is_favorite, false );

	// Apply the Internet/NAT filters (LAN/Favorites/History leave them at "any").
	if( e.ping > filterPing )
		return false;
	if( filterEmpty == '1' && !e.IsEmpty() ) return false;
	if( filterEmpty == '0' &&  e.IsEmpty() ) return false;
	if( filterFull  == '1' && !e.IsFull()  ) return false;
	if( filterFull  == '0' &&  e.IsFull()  ) return false;
	if( filterMapName[0] && colorstricmp( filterMapName, e.mapname ) != 0 )
		return false;

	AccumulateMap( e );

	entries.AddToTail( e );
	if( m_iSortingColumn != -1 )
		Sort( m_iSortingColumn, m_bAscend );
	return true;
}

void CWndServerModel::OnActivateEntry( int line )
{
	if( parent )
		parent->ConnectFromModel( this, line );
}

void CWndServerModel::AccumulateMap( const WndSrvEntry &e )
{
	if( !parent || !e.mapname[0] )
		return;

	for( int i = 0; i < filterMaps.Count(); ++i )
	{
		if( filterMaps[i] == e.mapname )
		{
			filterMaps[i].AddCount( (unsigned short)e.numcl );
			return;
		}
	}

	filterMaps.AddToTail( filterMap_t( e.mapname, (unsigned short)e.numcl ));
	filterMaps.Sort( filterMap_t::CmpCountInvert );
	parent->RebuildMapFilterFor( this );
}

//=============================================================================
// construction
//=============================================================================
CMenuWndServerBrowser::CMenuWndServerBrowser() :
	CMenuFrameTabbed( "Find Servers" ),
	m_refreshTime( 0 ), m_lastTab( -1 ),
	m_savedValid( false ), m_savedFromInternet( false )
{
}

//=============================================================================
// helpers: which tab is active?
//=============================================================================
CWndServerModel *CMenuWndServerBrowser::ActiveModel()
{
	switch( GetActiveTab() )
	{
	case WND_TAB_INTERNET:  return &internetModel;
	case WND_TAB_NAT:       return &natModel;
	case WND_TAB_LAN:       return &lanModel;
	case WND_TAB_FAVORITES: return &favoritesModel;
	case WND_TAB_HISTORY:   return &historyModel;
	}
	return &internetModel;
}

CMenuTable *CMenuWndServerBrowser::ActiveTable()
{
	switch( GetActiveTab() )
	{
	case WND_TAB_INTERNET:  return &internetTable;
	case WND_TAB_NAT:       return &natTable;
	case WND_TAB_LAN:       return &lanTable;
	case WND_TAB_FAVORITES: return &favoritesTable;
	case WND_TAB_HISTORY:   return &historyTable;
	}
	return &internetTable;
}

void CMenuWndServerBrowser::RebuildMapFilterFor( CWndServerModel *model )
{
	CMenuDropDownStr *dd;
	if( model == &internetModel )      dd = &internetMap;
	else if( model == &natModel )      dd = &natMap;
	else                               return;

	dd->Clear();
	for( int i = Q_min( WND_FILTER_MAX_MAPS, model->filterMaps.Count() ); i--; )
		dd->AddItem( model->filterMaps[i].GetDisplay(), model->filterMaps[i].name );
	dd->AddItem( L( "any map" ), "" );

	if( !model->filterMapName[0] )
		dd->SelectLast( false );
}

//=============================================================================
// filter dropdown handlers
//=============================================================================
void CMenuWndServerBrowser::ApplyPingFilter( CMenuDropDownFloat *dd, float seconds )
{
	if( dd == &internetPing )      { internetModel.filterPing = seconds; RefreshInternet(); }
	else if( dd == &natPing )      { natModel.filterPing = seconds;      RefreshNAT();      }
}

void CMenuWndServerBrowser::ApplyEmptyFilter( CMenuDropDownInt *dd, char value )
{
	if( dd == &internetEmpty )     { internetModel.filterEmpty = value; RefreshInternet(); }
	else if( dd == &natEmpty )     { natModel.filterEmpty = value;      RefreshNAT();      }
}

void CMenuWndServerBrowser::ApplyFullFilter( CMenuDropDownInt *dd, char value )
{
	if( dd == &internetFull )      { internetModel.filterFull = value; RefreshInternet(); }
	else if( dd == &natFull )      { natModel.filterFull = value;      RefreshNAT();      }
}

void CMenuWndServerBrowser::ApplyMapFilter( CMenuDropDownStr *dd, const char *value )
{
	if( dd == &internetMap )       { internetModel.SetFilterMap( value ); RefreshInternet(); }
	else if( dd == &natMap )       { natModel.SetFilterMap( value );      RefreshNAT();      }
}

//=============================================================================
// table + filter setup
//=============================================================================
void CMenuWndServerBrowser::SetupServerTable( CMenuTable &table, CWndServerModel &model, bool favColumn )
{
	model.SetParent( this, favColumn );

	table.SetCharSize( QM_SMALLFONT );
	table.SetupColumn( WND_COL_ICON,    NULL,              32.0f, true );
	table.SetupColumn( WND_COL_NAME,    L( "Name" ),       0.40f );
	table.SetupColumn( WND_COL_MAP,     L( "GameUI_Map" ), 0.20f );
	table.SetupColumn( WND_COL_PLAYERS, L( "Players" ),    100.0f, true );
	table.SetupColumn( WND_COL_PING,    L( "Ping" ),       120.0f, true );
	table.SetModel( &model );
	table.bAllowSorting   = true;
	table.bFramedHintText = true;
	table.SetSortingColumn( WND_COL_PING );
	table.onChanged = VoidCb( &CMenuWndServerBrowser::OnSelectionChanged );
}

void CMenuWndServerBrowser::SetupFilterDropdowns( CMenuDropDownFloat &ping, CMenuDropDownInt &empty,
	CMenuDropDownInt &full, CMenuDropDownStr &map )
{
	ping.AddItem( "500ms", 500.0f );
	ping.AddItem( "200ms", 200.0f );
	ping.AddItem( "100ms", 100.0f );
	ping.AddItem( "50ms",  50.0f );
	ping.AddItem( "20ms",  20.0f );
	ping.AddItem( L( "ping" ), WND_MAX_PING * 1000.0f );
	ping.SelectLast();
	ping.bDropUp = true;
	ping.eTextAlignment = QM_LEFT;
	ping.SetCharSize( QM_SMALLFONT );
	ping.SetSize( 80, 28 );
	SET_EVENT_MULTI( ping.onChanged,
	{
		CMenuDropDownFloat *self = (CMenuDropDownFloat *)pSelf;
		CMenuWndServerBrowser *p = pSelf->GetParent( CMenuWndServerBrowser );
		p->ApplyPingFilter( self, self->GetItem() / 1000.0f );
	});

	empty.AddItem( L( "empty" ), '1' );
	empty.AddItem( L( "not empty" ), '0' );
	empty.AddItem( L( "any" ), 0 );
	empty.SelectLast( false );
	empty.bDropUp = true;
	empty.eTextAlignment = QM_LEFT;
	empty.SetCharSize( QM_SMALLFONT );
	empty.SetSize( 120, 28 );
	SET_EVENT_MULTI( empty.onChanged,
	{
		CMenuDropDownInt *self = (CMenuDropDownInt *)pSelf;
		CMenuWndServerBrowser *p = pSelf->GetParent( CMenuWndServerBrowser );
		p->ApplyEmptyFilter( self, (char)self->GetItem() );
	});

	full.AddItem( L( "full" ), '1' );
	full.AddItem( L( "not full" ), '0' );
	full.AddItem( L( "any" ), 0 );
	full.SelectLast( false );
	full.bDropUp = true;
	full.eTextAlignment = QM_LEFT;
	full.SetCharSize( QM_SMALLFONT );
	full.SetSize( 120, 28 );
	SET_EVENT_MULTI( full.onChanged,
	{
		CMenuDropDownInt *self = (CMenuDropDownInt *)pSelf;
		CMenuWndServerBrowser *p = pSelf->GetParent( CMenuWndServerBrowser );
		p->ApplyFullFilter( self, (char)self->GetItem() );
	});

	map.AddItem( L( "any map" ), "" );
	map.bDropUp = true;
	map.eTextAlignment = QM_LEFT;
	map.SetCharSize( QM_SMALLFONT );
	map.SetSize( 200, 28 );
	SET_EVENT_MULTI( map.onChanged,
	{
		CMenuDropDownStr *self = (CMenuDropDownStr *)pSelf;
		CMenuWndServerBrowser *p = pSelf->GetParent( CMenuWndServerBrowser );
		p->ApplyMapFilter( self, self->GetItem() );
	});
}

//=============================================================================
// _Init -- build the whole window
//=============================================================================
void CMenuWndServerBrowser::_Init()
{
	ComputeWindowRect();

	// ---- always-visible bottom buttons (must precede the first AddTab) ----
	btnConnect.szName = L( "Connect" );
	SET_EVENT_MULTI( btnConnect.onReleased,
	{
		pSelf->GetParent( CMenuWndServerBrowser )->ConnectToSelected();
	});
	AddItem( btnConnect );

	btnRefresh.szName = L( "Refresh" );
	SET_EVENT_MULTI( btnRefresh.onReleased,
	{
		pSelf->GetParent( CMenuWndServerBrowser )->RefreshActive();
	});
	AddItem( btnRefresh );

	btnFavorite.szName = L( "Favorite" );
	SET_EVENT_MULTI( btnFavorite.onReleased,
	{
		pSelf->GetParent( CMenuWndServerBrowser )->FavoriteSelected();
	});
	AddItem( btnFavorite );

	btnAddServer.szName = L( "Add server" );
	SET_EVENT_MULTI( btnAddServer.onReleased,
	{
		pSelf->GetParent( CMenuWndServerBrowser )->ShowAddServerBox();
	});
	AddItem( btnAddServer );

	btnDone.szName = L( "Done" );
	SET_EVENT_MULTI( btnDone.onReleased,
	{
		pSelf->GetParent( CMenuWndServerBrowser )->Hide();
	});
	AddItem( btnDone );

	// ---- password dialog (link before tabs so it overlays every tab) ----
	passwordField.bHideInput = true;
	passwordField.bAllowColorstrings = false;
	passwordField.szName = L( "GameUI_Password" );
	passwordField.iMaxLength = 16;
	passwordField.SetRect( 188, 140, 270, 32 );

	SET_EVENT_MULTI( askPassword.onPositive,
	{
		CMenuWndServerBrowser *p = pSelf->GetParent( CMenuWndServerBrowser );
		EngFuncs::CvarSetString( "password", p->passwordField.GetBuffer() );
		p->passwordField.Clear();
		p->ConnectSaved();
	});
	SET_EVENT_MULTI( askPassword.onNegative,
	{
		CMenuWndServerBrowser *p = pSelf->GetParent( CMenuWndServerBrowser );
		EngFuncs::CvarSetString( "password", "" );
		p->passwordField.Clear();
		p->m_savedValid = false;
	});
	askPassword.SetMessage( L( "GameUI_PasswordPrompt" ));
	askPassword.Link( this );
	askPassword.Init();
	askPassword.AddItem( passwordField );

	// ---- add-server dialog ----
	addressField.bAllowColorstrings = false;
	addressField.szName = NULL;
	addressField.eTextAlignment = QM_LEFT;
	addressField.SetRect( 64, 150, 512, 32 );

	static const char *protlist[] = { "Xash3D 49", "GoldSource 48" };
	static CStringArrayModel protlistModel( protlist, V_ARRAYSIZE( protlist ));
	protocolSpin.Setup( &protlistModel );
	protocolSpin.SetCurrentValue( 0.0f );
	protocolSpin.eTextAlignment = QM_LEFT;
	protocolSpin.SetRect( 64, 100, 512, 32 );

	SET_EVENT_MULTI( addServerBox.onPositive,
	{
		pSelf->GetParent( CMenuWndServerBrowser )->AddServerFromDialog();
	});
	addServerBox.SetMessage( L( "Enter server Internet address\n(e.g., 209.255.10.255:27015)" ));
	addServerBox.dlgMessage1.SetCoord( 8, 8 );
	addServerBox.dlgMessage1.eTextAlignment = QM_TOPLEFT;
	addServerBox.Link( this );
	addServerBox.Init();
	addServerBox.AddItem( protocolSpin );
	addServerBox.AddItem( addressField );

	// server.dll is required to host a game; the legacy menu greys out the
	// add-server route for the Internet tab when it's missing.
	if( !EngFuncs::CheckGameDll() )
		btnAddServer.SetGrayed( true );

	// ===================== Tab: Internet =====================
	AddTab( L( "Internet" ));
	SetupServerTable( internetTable, internetModel, false );
	AddItem( internetTable );
	SetupFilterDropdowns( internetPing, internetEmpty, internetFull, internetMap );
	AddItem( internetPing );
	AddItem( internetEmpty );
	AddItem( internetFull );
	AddItem( internetMap );

	// ===================== Tab: NAT =====================
	AddTab( "NAT" ); // intentionally not localized, matches legacy
	SetupServerTable( natTable, natModel, false );
	AddItem( natTable );
	SetupFilterDropdowns( natPing, natEmpty, natFull, natMap );
	AddItem( natPing );
	AddItem( natEmpty );
	AddItem( natFull );
	AddItem( natMap );

	// ===================== Tab: LAN =====================
	AddTab( "LAN" );
	SetupServerTable( lanTable, lanModel, false );
	AddItem( lanTable );

	// ===================== Tab: Favorites =====================
	AddTab( L( "Favorites" ));
	SetupServerTable( favoritesTable, favoritesModel, true );
	AddItem( favoritesTable );

	// ===================== Tab: History =====================
	AddTab( L( "History" ));
	SetupServerTable( historyTable, historyModel, true );
	AddItem( historyTable );

	SetActiveTab( WND_TAB_INTERNET );

	DoLayout();
}

//=============================================================================
// geometry
//=============================================================================
void CMenuWndServerBrowser::ComputeWindowRect()
{
	int w = (int)( uiStatic.width * 0.8f );
	int h = (int)( 768 * 0.75f );
	int x = ( uiStatic.width - w ) / 2;
	int y = ( 768 - h ) / 2;
	SetRect( x, y, w, h );
}

void CMenuWndServerBrowser::DoLayout()
{
	int w = size.w;
	int h = size.h;

	int contentW = w - FRAME_BORDER_WIDTH * 2;
	int contentH = h - FRAME_TITLE_HEIGHT - FRAME_TAB_HEIGHT;

	int margin  = 8;
	int btnW    = 96;
	int btnH    = 26;
	int gap     = 8;

	int btnY    = contentH - btnH - 12;
	int filterH = 28;
	int filterY = btnY - filterH - gap;
	int tableX  = margin;
	int tableY  = margin;
	int tableW  = contentW - margin * 2;
	int tableH  = filterY - tableY - gap;
	if( tableH < 64 )
		tableH = 64;

	// tables (all five share the same content rectangle)
	internetTable.SetRect( tableX, tableY, tableW, tableH );
	natTable.SetRect( tableX, tableY, tableW, tableH );
	lanTable.SetRect( tableX, tableY, tableW, tableH );
	favoritesTable.SetRect( tableX, tableY, tableW, tableH );
	historyTable.SetRect( tableX, tableY, tableW, tableH );

	// filter row (Internet/NAT only; hidden automatically on other tabs)
	int fx = tableX;
	internetPing.SetCoord( fx, filterY );  fx += internetPing.size.w + 10;
	internetEmpty.SetCoord( fx, filterY ); fx += internetEmpty.size.w + 10;
	internetFull.SetCoord( fx, filterY );  fx += internetFull.size.w + 10;
	internetMap.SetCoord( fx, filterY );

	fx = tableX;
	natPing.SetCoord( fx, filterY );  fx += natPing.size.w + 10;
	natEmpty.SetCoord( fx, filterY ); fx += natEmpty.size.w + 10;
	natFull.SetCoord( fx, filterY );  fx += natFull.size.w + 10;
	natMap.SetCoord( fx, filterY );

	// keep dropdowns closed across relayout
	internetPing.MenuClose(); internetEmpty.MenuClose(); internetFull.MenuClose(); internetMap.MenuClose();
	natPing.MenuClose();      natEmpty.MenuClose();      natFull.MenuClose();      natMap.MenuClose();

	// bottom buttons, left to right; Done pinned to the right edge
	int bx = margin;
	btnConnect.SetRect( bx, btnY, btnW, btnH );   bx += btnW + gap;
	btnRefresh.SetRect( bx, btnY, btnW, btnH );    bx += btnW + gap;
	btnFavorite.SetRect( bx, btnY, btnW, btnH );   bx += btnW + gap;
	btnAddServer.SetRect( bx, btnY, btnW, btnH );
	btnDone.SetRect( contentW - margin - btnW, btnY, btnW, btnH );
}

void CMenuWndServerBrowser::_VidInit()
{
	ComputeWindowRect();
	DoLayout();
	m_refreshTime = uiStatic.realTime + 500;
}

//=============================================================================
// Draw -- periodic auto-refresh + instant refresh on tab switch
//=============================================================================
void CMenuWndServerBrowser::Draw()
{
	CMenuFrameTabbed::Draw();

	if( GetActiveTab() != m_lastTab )
	{
		m_lastTab = GetActiveTab();
		RefreshActive();
		m_refreshTime = uiStatic.realTime + 20000;
	}
	else if( uiStatic.realTime > m_refreshTime )
	{
		RefreshActive();
		m_refreshTime = uiStatic.realTime + 20000; // every 20s
	}
}

//=============================================================================
// Show / Hide
//=============================================================================
void CMenuWndServerBrowser::Show()
{
	CMenuFrameTabbed::Show();

	LoadLists();

	m_savedValid = false;
	m_lastTab    = -1;

	internetModel.Clear();
	natModel.Clear();
	lanModel.Clear();
	favoritesModel.Clear();
	historyModel.Clear();

	btnConnect.SetGrayed( true );
	btnFavorite.SetGrayed( true );

	m_refreshTime = uiStatic.realTime + 500;
}

void CMenuWndServerBrowser::Hide()
{
	SaveLists();
	CMenuFrameTabbed::Hide();
}

//=============================================================================
// refresh dispatchers
//=============================================================================
void CMenuWndServerBrowser::RefreshActive()
{
	switch( GetActiveTab() )
	{
	case WND_TAB_INTERNET:  RefreshInternet();  break;
	case WND_TAB_NAT:       RefreshNAT();       break;
	case WND_TAB_LAN:       RefreshLAN();       break;
	case WND_TAB_FAVORITES: RefreshFavorites(); break;
	case WND_TAB_HISTORY:   RefreshHistory();   break;
	}
	RefreshButtonsForSelection();
}

// Build the "\\empty\\1\\full\\0\\map\\de_dust" filter string for the engine.
static void BuildFilterString( const CWndServerModel &m, char *out, size_t outSize )
{
	out[0] = 0;
	char *buf = out;
	int remaining = (int)outSize;

	if( m.filterEmpty )
	{
		int n = snprintf( buf, remaining, "\\empty\\%c", m.filterEmpty );
		if( n > 0 ) { remaining -= n; buf += n; }
	}
	if( m.filterFull )
	{
		int n = snprintf( buf, remaining, "\\full\\%c", m.filterFull );
		if( n > 0 ) { remaining -= n; buf += n; }
	}
	if( m.filterMapName[0] )
		snprintf( buf, remaining, "\\map\\%s", m.filterMapName );
}

void CMenuWndServerBrowser::RefreshInternet()
{
	EngFuncs::CvarSetValue( "cl_nat", 0.0f );
	internetModel.Clear();
	internetTable.SetModel( &internetModel );

	char filter[256];
	BuildFilterString( internetModel, filter, sizeof( filter ));
	EngFuncs::ClientCmdF( false, "internetservers %s\n", filter );
}

void CMenuWndServerBrowser::RefreshNAT()
{
	EngFuncs::CvarSetValue( "cl_nat", 1.0f );
	natModel.Clear();
	natTable.SetModel( &natModel );

	char filter[256];
	BuildFilterString( natModel, filter, sizeof( filter ));
	EngFuncs::ClientCmdF( false, "internetservers %s\n", filter );
}

void CMenuWndServerBrowser::RefreshLAN()
{
	EngFuncs::CvarSetValue( "cl_nat", 0.0f );
	lanModel.Clear();
	lanTable.SetModel( &lanModel );
	EngFuncs::ClientCmd( false, "localservers\n" );
}

void CMenuWndServerBrowser::RefreshFavorites()
{
	EngFuncs::CvarSetValue( "cl_nat", 0.0f );
	favoritesModel.Clear();
	favoritesTable.SetModel( &favoritesModel );
	QueryServerList( favoritesList, favoritesModel, true );
}

void CMenuWndServerBrowser::RefreshHistory()
{
	EngFuncs::CvarSetValue( "cl_nat", 0.0f );
	historyModel.Clear();
	historyTable.SetModel( &historyModel );
	QueryServerList( historyList, historyModel, false );
}

//=============================================================================
// connect flow
//=============================================================================
void CMenuWndServerBrowser::ConnectFromModel( CWndServerModel *model, int line )
{
	if( !model->entries.IsValidIndex( line ))
		return;

	bool fromInternet = ( model == &internetModel );
	WndSrvEntry &e = model->entries[line];

	// ask for a password if the server is protected and we haven't entered one
	if( e.havePassword && !m_savedValid )
	{
		m_savedServer       = e;
		m_savedFromInternet = fromInternet;
		m_savedValid        = true;
		askPassword.Show();
		return;
	}

	if( !e.havePassword )
		EngFuncs::CvarSetString( "password", "" );

	m_savedValid = false;
	DoConnect( e, fromInternet );
}

void CMenuWndServerBrowser::ConnectToSelected()
{
	CMenuTable *tbl = ActiveTable();
	CWndServerModel *mdl = ActiveModel();
	ConnectFromModel( mdl, tbl->GetCurrentIndex() );
}

void CMenuWndServerBrowser::ConnectSaved()
{
	if( !m_savedValid )
		return;
	m_savedValid = false;
	DoConnect( m_savedServer, m_savedFromInternet );
}

void CMenuWndServerBrowser::DoConnect( const WndSrvEntry &e, bool fromInternet )
{
	// freeze the periodic refresh so it doesn't race the connect
	m_refreshTime = uiStatic.realTime + 999999;

	const char *sadr = EngFuncs::NET_AdrToString( e.adr );
	const char *prot = e.ToProtocol();

	// remember Internet connects in the history list
	if( fromInternet )
	{
		bool exists = false;
		FOR_EACH_VEC( historyList, i )
		{
			if( !strcmp( historyList[i].sadr, sadr ) && !strcmp( historyList[i].prot, prot ))
			{
				exists = true;
				break;
			}
		}
		if( !exists )
		{
			if( historyList.Count() >= WND_MAX_HISTORY )
				historyList.FastRemove( 0 );
			historyList.AddToTail( favlist_entry_t( sadr, prot, true ));
			SaveServerListToFile( "history_servers.lst", historyList );
		}
	}

	EngFuncs::ClientCmdF( false, "connect \"%s\" \"%s\"\n", sadr, prot );
	UI_ConnectionProgress_Connect( "" );
	Hide();
}

//=============================================================================
// favorites
//=============================================================================
void CMenuWndServerBrowser::FavoriteSelected()
{
	CMenuTable *tbl = ActiveTable();
	CWndServerModel *mdl = ActiveModel();

	int i = tbl->GetCurrentIndex();
	if( !mdl->entries.IsValidIndex( i ))
		return;

	WndSrvEntry &e = mdl->entries[i];
	const char *sadr = EngFuncs::NET_AdrToString( e.adr );

	e.favorite = !e.favorite;

	if( e.favorite )
	{
		bool exists = false;
		FOR_EACH_VEC( favoritesList, j )
		{
			if( !strcmp( favoritesList[j].sadr, sadr ))
			{
				favoritesList[j].favorited = true;
				exists = true;
				break;
			}
		}
		if( !exists )
			favoritesList.AddToTail( favlist_entry_t( sadr, e.ToProtocol(), true ));
	}
	else
	{
		FOR_EACH_VEC( favoritesList, j )
		{
			if( !strcmp( favoritesList[j].sadr, sadr ))
			{
				favoritesList.FastRemove( j );
				break;
			}
		}
	}

	SaveServerListToFile( "favorite_servers.lst", favoritesList );
	RefreshButtonsForSelection();
}

void CMenuWndServerBrowser::OnSelectionChanged()
{
	RefreshButtonsForSelection();
}

void CMenuWndServerBrowser::RefreshButtonsForSelection()
{
	CMenuTable *tbl = ActiveTable();
	CWndServerModel *mdl = ActiveModel();

	int i = tbl->GetCurrentIndex();
	bool valid = mdl->entries.IsValidIndex( i );

	btnConnect.SetGrayed( !valid );
	btnFavorite.SetGrayed( !valid );

	if( valid && mdl->entries[i].favorite )
		btnFavorite.szName = L( "Unfavorite" );
	else
		btnFavorite.szName = L( "Favorite" );
}

//=============================================================================
// add-server dialog
//=============================================================================
void CMenuWndServerBrowser::ShowAddServerBox()
{
	addServerBox.Show();
}

void CMenuWndServerBrowser::AddServerFromDialog()
{
	netadr_t adr;

	if( !EngFuncs::textfuncs.pNetAPI->StringToAdr( (char *)addressField.GetBuffer(), &adr ))
	{
		UI_ShowMessageBox( L( "Invalid address" ));
		return;
	}

	const char *proto;
	switch( (int)protocolSpin.GetCurrentValue() )
	{
	case 0:  proto = "49"; break;
	case 1:  proto = "gs"; break;
	default:
		UI_ShowMessageBox( L( "Invalid protocol" ));
		return;
	}

	// de-duplicate within favorites
	FOR_EACH_VEC( favoritesList, i )
	{
		if( !strcmp( favoritesList[i].sadr, addressField.GetBuffer() ) &&
		    !strcmp( favoritesList[i].prot, proto ))
		{
			UI_ShowMessageBox( L( "Server already in favorites" ));
			return;
		}
	}

	favoritesList.AddToTail( favlist_entry_t( addressField.GetBuffer(), proto, true ));
	SaveServerListToFile( "favorite_servers.lst", favoritesList );

	// jump to the Favorites tab and re-query so the new entry shows up live
	SetActiveTab( WND_TAB_FAVORITES );
	RefreshFavorites();
}

//=============================================================================
// persistence (mirrors ServerBrowser.cpp)
//=============================================================================
void CMenuWndServerBrowser::LoadLists()
{
	favoritesList.RemoveAll();
	historyList.RemoveAll();

	if( EngFuncs::FileExists( "favorite_servers.lst", true ))
		ParseServerListFromFile( "favorite_servers.lst", favoritesList );
	if( EngFuncs::FileExists( "history_servers.lst", true ))
		ParseServerListFromFile( "history_servers.lst", historyList );
}

void CMenuWndServerBrowser::SaveLists()
{
	SaveServerListToFile( "favorite_servers.lst", favoritesList );
	SaveServerListToFile( "history_servers.lst", historyList );
}

void CMenuWndServerBrowser::ParseServerListFromFile( const char *filename, CUtlVector<favlist_entry_t> &list )
{
	byte *pfile = EngFuncs::COM_LoadFile( filename );
	char *afile = (char *)pfile;

	if( !afile )
		return;

	while( true )
	{
		favlist_entry_t entry( "", "", true );

		afile = EngFuncs::COM_ParseFile( afile, entry.sadr, sizeof( entry.sadr ));
		if( !afile )
			break;

		afile = EngFuncs::COM_ParseFile( afile, entry.prot, sizeof( entry.prot ));
		if( !afile )
			break;

		// ignore legacy protocol
		if( !strcmp( entry.prot, "48" ))
			continue;

		list.AddToTail( entry );
	}

	EngFuncs::COM_FreeFile( pfile );
}

void CMenuWndServerBrowser::SaveServerListToFile( const char *filename, const CUtlVector<favlist_entry_t> &list )
{
	CUtlString s;

	if( list.Count() == 0 )
	{
		EngFuncs::DeleteFile( filename );
		return;
	}

	FOR_EACH_VEC( list, i )
	{
		if( !list[i].favorited )
			continue;
		s.AppendFormat( "%s %s\n", list[i].sadr, list[i].prot );
	}

	EngFuncs::COM_SaveFile( filename, s.Get(), s.Length() );
}

void CMenuWndServerBrowser::QueryServerList( const CUtlVector<favlist_entry_t> &list, CWndServerModel &model, bool favTab )
{
	FOR_EACH_VEC( list, i )
	{
		netadr_t adr;

		if( !EngFuncs::textfuncs.pNetAPI->StringToAdr( (char *)list[i].sadr, &adr ))
			continue;

		// placeholder until the real reply lands
		model.AddPlaceholder( adr, list[i].sadr, list[i].prot, favTab ? list[i].favorited : false );

		// fire the actual query
		list[i].QueryServer();
	}

	UI_MenuResetPing_f();
}

//=============================================================================
// incoming server dispatch
//=============================================================================
void CMenuWndServerBrowser::AddServerToList( netadr_t adr, const char *info )
{
#ifndef XASH_ALL_SERVERS
	if( stricmp( gMenu.m_gameinfo.gamefolder, Info_ValueForKey( info, "gamedir" )) != 0 )
		return;
#endif

	// is this server one of our favorites?
	const char *s = EngFuncs::NET_AdrToString( adr );
	bool is_favorite = false;
	FOR_EACH_VEC( favoritesList, i )
	{
		if( !strcmp( favoritesList[i].sadr, s ))
		{
			is_favorite = favoritesList[i].favorited;
			break;
		}
	}

	// Route to whichever model was cleared most recently (highest start time).
	CWndServerModel *models[WND_TAB_COUNT] = {
		&internetModel, &natModel, &lanModel, &favoritesModel, &historyModel
	};
	CMenuTable *tables[WND_TAB_COUNT] = {
		&internetTable, &natTable, &lanTable, &favoritesTable, &historyTable
	};

	int best = 0;
	for( int i = 1; i < WND_TAB_COUNT; i++ )
	{
		if( models[i]->refreshStartTime > models[best]->refreshStartTime )
			best = i;
	}

	if( models[best]->AddServer( adr, info, is_favorite ))
	{
		tables[best]->SetModel( models[best] );
		RefreshButtonsForSelection();
	}
}

} // anonymous namespace

//=============================================================================
// engine entry points (external linkage â referenced by ServerBrowser.cpp & macros)
//=============================================================================

// Called from UI_AddServerToList() in ServerBrowser.cpp for every reported server.
void WndServerBrowser_AddServerToList( netadr_t adr, const char *info )
{
	if( s_pWndServerBrowser && s_pWndServerBrowser->IsVisible() )
		s_pWndServerBrowser->AddServerToList( adr, info );
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
