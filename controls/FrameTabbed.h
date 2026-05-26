/*
FrameTabbed.h -- Source Engine-style framed window with tab strip
Copyright (C) 2024 DragonSlayer Team

A CMenuFrame with a row of tabs below the title bar.
Each tab shows/hides a different panel of content.
*/
#ifndef MENU_FRAME_TABBED_H
#define MENU_FRAME_TABBED_H

#include "Frame.h"

#define MAX_FRAME_TABS 8
#define FRAME_TAB_HEIGHT 22 // logical pixels - height of tab strip

class CMenuFrameTabbed : public CMenuFrame
{
public:
	typedef CMenuFrame BaseClass;

	CMenuFrameTabbed( const char *title = "Window" );

	void Draw() override;
	bool KeyUp( int key ) override;

	// Override to offset content below tabs
	Point GetPositionOffset() const override;

	// Add a tab. Items added after this call belong to this tab.
	void AddTab( const char *name );
	int GetActiveTab() const { return m_iActiveTab; }
	void SetActiveTab( int idx );

	// Track which tab each item belongs to
	void AddItem( CMenuBaseItem &item );

protected:
	void DrawTabs();
	int TabAtCursor();

	struct Tab
	{
		const char *name;
		int firstItem;  // index in m_pItems
		int lastItem;   // index in m_pItems (inclusive)
	};

	Tab m_tabs[MAX_FRAME_TABS];
	int m_iNumTabs;
	int m_iActiveTab;
	int m_iTabH; // scaled tab height
};

#endif // MENU_FRAME_TABBED_H
