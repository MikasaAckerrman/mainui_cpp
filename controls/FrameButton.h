/*
FrameButton.h -- CS 1.6 / Source Engine style flat button with bevel border
Copyright (C) 2024 DragonSlayer Team

A simple text button for use inside CMenuFrame windows.
Draws with grey background + bright/dark bevel border + centered text.
*/
#ifndef MENU_FRAME_BUTTON_H
#define MENU_FRAME_BUTTON_H

#include "BaseItem.h"
#include "TrackerScheme.h"

class CMenuFrameButton : public CMenuBaseItem
{
public:
	typedef CMenuBaseItem BaseClass;

	CMenuFrameButton();

	void VidInit() override;
	bool KeyUp( int key ) override;
	bool KeyDown( int key ) override;
	void Draw() override;
};

#endif // MENU_FRAME_BUTTON_H
