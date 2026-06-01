/*
LogoPickerDialog.cpp - spraypaint logo picker dialog window
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

#include "LogoPickerDialog.h"
#include "BaseMenu.h"
#include "extdll_menu.h"

namespace LogoPickerLayout
{
	static const int DLG_W             = 420;
	static const int DLG_H             = 470;
	static const int PAD               = 20;
	static const int TITLE_H           = 28;
	static const int CONTENT_GAP       = 16;
	static const int BTN_W             = 120;
	static const int BTN_H             = 36;
	static const int BTN_GAP           = 16;
	static const int UI_VIRTUAL_HEIGHT = 768;
}

CMenuLogoPickerDialog::CMenuLogoPickerDialog()
	: CMenuBaseWindow( "CMenuLogoPickerDialog" )
{
}

void CMenuLogoPickerDialog::_Init()
{
	using namespace LogoPickerLayout;

	SetRect( ( uiStatic.width - DLG_W ) / 2, ( UI_VIRTUAL_HEIGHT - DLG_H ) / 2, DLG_W, DLG_H );

	const int listY = PAD + TITLE_H + CONTENT_GAP;
	const int listH = DLG_H - listY - PAD - BTN_H - CONTENT_GAP;

	m_table.iFlags |= QMF_DROPSHADOW;
	m_table.SetRect( PAD, listY, DLG_W - PAD * 2, listH );

	const int buttonY  = DLG_H - PAD - BTN_H;
	const int buttonsX = ( DLG_W - ( BTN_W * 2 + BTN_GAP ) ) / 2;

	m_btnOk.SetPicture( PC_OK );
	m_btnOk.szName = L( "OK" );
	m_btnOk.eTextAlignment = QM_CENTER;
	m_btnOk.SetRect( buttonsX, buttonY, BTN_W, BTN_H );
	SET_EVENT_MULTI( m_btnOk.onReleased,
	{
		CMenuLogoPickerDialog *dlg = (CMenuLogoPickerDialog *)pSelf->Parent();
		if( dlg->onOk )
			dlg->onOk( dlg );
		dlg->Hide();
	});

	m_btnCancel.SetPicture( PC_CANCEL );
	m_btnCancel.szName = L( "Cancel" );
	m_btnCancel.eTextAlignment = QM_CENTER;
	m_btnCancel.SetRect( buttonsX + BTN_W + BTN_GAP, buttonY, BTN_W, BTN_H );
	SET_EVENT_MULTI( m_btnCancel.onReleased,
	{
		((CMenuLogoPickerDialog *)pSelf->Parent())->Hide();
	});

	AddItem( m_table );
	AddItem( m_btnOk );
	AddItem( m_btnCancel );
}

void CMenuLogoPickerDialog::_VidInit()
{
	using namespace LogoPickerLayout;
	SetRect( ( uiStatic.width - DLG_W ) / 2, ( UI_VIRTUAL_HEIGHT - DLG_H ) / 2, DLG_W, DLG_H );
	pos.x += uiStatic.xOffset;
	pos.y += uiStatic.yOffset;
}

void CMenuLogoPickerDialog::Draw()
{
	using namespace LogoPickerLayout;

	// dim the background to convey modality
	UI_FillRect( 0, 0, gpGlobals->scrWidth, gpGlobals->scrHeight, 0x40000000 );

	// dialog body
	EngFuncs::FillRGBA( m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h, 20, 20, 20, 235 );
	UI_DrawRectangle( m_scPos, m_scSize, uiInputFgColor );

	// title + 1px engraved separator under it
	UI_DrawString( font, m_scPos.x, m_scPos.y + PAD * uiStatic.scaleY, m_scSize.w,
		TITLE_H * uiStatic.scaleY, L( "Spraypaint image" ), uiColorHelp, m_scChSize, QM_CENTER, ETF_SHADOW );
	EngFuncs::FillRGBA( m_scPos.x + PAD * uiStatic.scaleX, m_scPos.y + ( PAD + TITLE_H ) * uiStatic.scaleY,
		m_scSize.w - PAD * 2 * uiStatic.scaleX, 1, 255, 255, 255, 40 );

	BaseClass::Draw();
}

void CMenuLogoPickerDialog::Show( CMenuBaseModel *model, int current )
{
	// SetModel must happen before the table is added to the holder (done in
	// _Init, triggered by BaseClass::Show on first show) - see Table.h note #3.
	m_table.SetModel( model );

	BaseClass::Show();

	// Select after VidInit so iNumRows is known and the row is scrolled into view.
	if( current >= 0 )
		m_table.SetCurrentIndex( current );
}
