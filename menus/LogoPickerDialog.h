/*
LogoPickerDialog.h - spraypaint logo picker dialog window
Copyright (C) 2026 armorberserk

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#pragma once
#ifndef LOGOPICKERDIALOG_H
#define LOGOPICKERDIALOG_H

#include "Framework.h"
#include "PicButton.h"
#include "Table.h"

// PC-style modal picker window for the spraypaint logo: a scrollable list of
// available logos + OK/Cancel. Replaces the blind cycle-spin selection UI.
class CMenuLogoPickerDialog : public CMenuBaseWindow
{
public:
	typedef CMenuBaseWindow BaseClass;
	CMenuLogoPickerDialog();

	// model - generic string model with the logo names (owned by the caller)
	// current - index to preselect (<0 for none)
	void Show( CMenuBaseModel *model, int current );

	int  GetSelectedIndex() const { return m_table.GetCurrentIndex(); }

	CEventCallback onOk;

private:
	void _Init() override;
	void _VidInit() override;
	void Draw() override;

	CMenuTable     m_table;
	CMenuPicButton m_btnOk;
	CMenuPicButton m_btnCancel;
};

#endif // LOGOPICKERDIALOG_H
