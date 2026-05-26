/*
WndServerBrowser.cpp -- Source Engine-style windowed server browser
Copyright (C) 2024 DragonSlayer Team

A CMenuFrameTabbed with Internet/LAN/Favorites tabs and the existing
server list table. This replaces the fullscreen ServerBrowser when
called from the new-style menu.
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "controls/FrameTabbed.h"
#include "Table.h"
#include "Field.h"
#include "keydefs.h"
#include "Utils.h"
#include "TrackerScheme.h"

// Forward declaration - we'll hook into engine server queries
// For now, a minimal working browser frame

class CMenuWndServerBrowser : public CMenuFrameTabbed
{
public:
	CMenuWndServerBrowser();

	bool IsRoot() const override { return false; }

private:
	void _Init() override;
	void _VidInit() override;

	void RefreshServers();
	void JoinServer();

	// Simple placeholder items per tab
	CMenuAction internetLabel;
	CMenuAction lanLabel;
	CMenuAction favLabel;
};

static CMenuWndServerBrowser *s_pWndServerBrowser = NULL;

CMenuWndServerBrowser::CMenuWndServerBrowser() : CMenuFrameTabbed( "Servers" )
{
}

void CMenuWndServerBrowser::_Init()
{
	// Window size: 80% width, 70% height, centered
	int w = (int)(uiStatic.width * 0.8f);
	int h = (int)(768 * 0.7f);
	int x = (uiStatic.width - w) / 2;
	int y = (768 - h) / 2;
	SetRect( x, y, w, h );

	// Add tabs
	AddTab( "Internet" );
	internetLabel.szName = "Internet server list - connect to refresh";
	internetLabel.iFlags |= QMF_INACTIVE;
	internetLabel.SetCoord( 16, 16 );
	AddItem( internetLabel );

	AddTab( "LAN" );
	lanLabel.szName = "LAN server list";
	lanLabel.iFlags |= QMF_INACTIVE;
	lanLabel.SetCoord( 16, 16 );
	AddItem( lanLabel );

	AddTab( "Favorites" );
	favLabel.szName = "Favorite servers";
	favLabel.iFlags |= QMF_INACTIVE;
	favLabel.SetCoord( 16, 16 );
	AddItem( favLabel );

	SetActiveTab( 0 );
}

void CMenuWndServerBrowser::_VidInit()
{
	int w = (int)(uiStatic.width * 0.8f);
	int h = (int)(768 * 0.7f);
	int x = (uiStatic.width - w) / 2;
	int y = (768 - h) / 2;
	SetRect( x, y, w, h );
}

void CMenuWndServerBrowser::RefreshServers()
{
	EngFuncs::ClientCmd( false, "ui_internetservers\n" );
}

void CMenuWndServerBrowser::JoinServer()
{
	// placeholder
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
