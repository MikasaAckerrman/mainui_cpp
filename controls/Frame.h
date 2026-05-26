/*
Frame.h -- Source Engine-style framed window with title bar
Copyright (C) 2024 DragonSlayer Team

A non-fullscreen draggable window with:
- Dark background (from TrackerScheme)
- Title bar with text and close button
- Bevel border (bright top-left, dark bottom-right)
- Touch-friendly drag via title bar
*/
#ifndef MENU_FRAME_H
#define MENU_FRAME_H

#include "BaseWindow.h"
#include "TrackerScheme.h"

#define FRAME_TITLE_HEIGHT 24  // logical pixels (scaled)
#define FRAME_BORDER_WIDTH 2
#define FRAME_RESIZE_GRIP  20  // logical pixels for resize grab zone
#define FRAME_MIN_W        200 // minimum logical width
#define FRAME_MIN_H        150 // minimum logical height

enum EResizeEdge
{
	RESIZE_NONE = 0,
	RESIZE_LEFT,
	RESIZE_RIGHT,
	RESIZE_TOP,
	RESIZE_BOTTOM,
	RESIZE_TOPLEFT,
	RESIZE_TOPRIGHT,
	RESIZE_BOTTOMLEFT,
	RESIZE_BOTTOMRIGHT
};

class CMenuFrame : public CMenuBaseWindow
{
public:
	typedef CMenuBaseWindow BaseClass;

	CMenuFrame( const char *title = "Window" );

	void Draw() override;
	bool KeyDown( int key ) override;
	bool KeyUp( int key ) override;
	bool MouseMove( int x, int y ) override;

	void SetRect( int x, int y, int w, int h );

	// Override to place content below title bar
	Point GetPositionOffset() const override;

	// Title text
	const char *m_szTitle;

	// Allow/disallow resize
	bool bAllowResize;

protected:
	void DrawTitleBar();
	void DrawBorder();
	void DrawBackground();

	bool IsInTitleBar( int x, int y );
	bool IsOnCloseButton( int x, int y );

	// Returns which edge/corner the cursor is on (inside grab zone)
	int HitTestResize( int x, int y );

	// Drag state
	bool m_bDragging;
	Point m_dragOffset;

	// Resize state
	bool m_bResizing;
	int  m_iResizeEdge;
	Point m_resizeStartCursor;
	Point m_resizeStartPos;
	Size  m_resizeStartSize;

	// Scaled sizes computed in Draw
	int m_iTitleH;
	int m_iBorderW;
};

#endif // MENU_FRAME_H
