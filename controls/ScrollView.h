#ifndef SCROLLVIEW_H
#define SCROLLVIEW_H

#include "ItemsHolder.h"

#define SCROLLBAR_WIDTH 16  // logical pixels

class CMenuScrollView : public CMenuItemsHolder
{
	typedef CMenuItemsHolder BaseClass;
public:
	CMenuScrollView();

	void VidInit() override;
	void Draw() override;
	bool KeyDown( int key ) override;
	bool KeyUp( int key ) override;
	bool MouseMove( int x, int y ) override;

	Point GetPositionOffset() const override;

private:
	bool IsRectVisible( Point pt, Size sz );
	void DrawScrollBar();

	bool  m_bDisableScrolling; // can't actually scroll due to item placement
	bool  m_bHoldingMouse1;
	bool  m_bGestureOnContent; // true if gesture started in content area (not scrollbar)
	Point m_HoldingPoint;

	int m_iPos;
	int m_iMax;

	// Visual scrollbar state
	int m_iScrollBarWidth;        // scaled scrollbar width
	bool m_bScrollBarDragging;    // thumb is being dragged
	int m_iScrollBarDragStartY;   // cursor Y when drag started
	int m_iScrollBarDragStartPos; // m_iPos when drag started
	// float m_flOverScrolling;
};

#endif // SCROLLVIEW_H
