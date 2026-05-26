/*
Frame.h -- Source Engine-style framed window with title bar
Copyright (C) 2024 DragonSlayer Team

A non-fullscreen draggable window with:
- Dark background (from TrackerScheme)
- Title bar with text and close button (X glyph)
- Double bevel border (1px outer dark + 1px inner bright)
- Drag from anywhere in the window
- Resize from bottom edge / bottom corners
- All drag/resize state updates happen synchronously inside MouseMove(x,y),
  so they work consistently on both PC mouse and Android touch.
*/
#ifndef MENU_FRAME_H
#define MENU_FRAME_H

#include "BaseWindow.h"
#include "TrackerScheme.h"

// CS 1.6 PC reference: ~22px title bar, 20px tab strip, 11-12px text font.
#define FRAME_TITLE_HEIGHT 22  // logical pixels
#define FRAME_BORDER_WIDTH 2
#define FRAME_RESIZE_GRIP  10  // logical pixels - corner grab zone size
#define FRAME_MIN_W        220 // minimum logical width
#define FRAME_MIN_H        140 // minimum logical height
#define FRAME_TEXT_HEIGHT  12  // logical pixels — Tahoma 11px feel

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

	// Drag/resize update primitives — apply state given an absolute cursor (x,y).
	// Called from MouseMove() (primary) and from ApplyDrag/ApplyResize in Draw()
	// (idempotent fallback for the case where MouseMove was missed).
	void UpdateDrag( int x, int y );
	void UpdateResize( int x, int y );

	// Legacy hooks that re-apply state from uiStatic.cursorX/Y. Kept so subclasses
	// like CMenuWndConsole that override Draw can still call them; they are
	// idempotent with the MouseMove path (same start state → same final state).
	void ApplyResize();
	void ApplyDrag();

	// State
	bool m_bDragging;
	bool m_bResizing;
	int  m_iResizeEdge;

	// Captured at KeyDown — start cursor position and start window rect.
	// Used identically for drag and resize so we never accumulate floating error.
	Point m_actionStartCursor;
	Point m_actionStartPos;
	Size  m_actionStartSize;

	// Scaled sizes computed in Draw
	int m_iTitleH;
	int m_iBorderW;
};

#endif // MENU_FRAME_H
