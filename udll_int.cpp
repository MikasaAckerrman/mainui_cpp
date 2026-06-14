/*
dll_int.cpp - dll entry point
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/


#include "extdll_menu.h"
#include "BaseMenu.h"
#include "Utils.h"

ui_enginefuncs_t EngFuncs::engfuncs;
ui_extendedfuncs_t EngFuncs::textfuncs;
ui_globalvars_t	*gpGlobals;
CMenu gMenu;

static UI_FUNCTIONS gFunctionTable = 
{
	UI_VidInit,
	UI_Init,
	UI_Shutdown,
	UI_UpdateMenu,
	UI_KeyEvent,
	UI_MouseMove,
	UI_SetActiveMenu,
	UI_AddServerToList,
	UI_GetCursorPos,
	UI_SetCursorPos,
	UI_ShowCursor,
	UI_CharEvent,
	UI_MouseInRect,
	UI_IsVisible,
	UI_CreditsActive,
	UI_FinalCredits
};

//=======================================================================
//			GetApi
//=======================================================================
extern "C" EXPORT int GetMenuAPI(UI_FUNCTIONS *pFunctionTable, ui_enginefuncs_t* pEngfuncsFromEngine, ui_globalvars_t *pGlobals)
{
	// ── Early diagnostic log — no EngFuncs yet, pure C stdio ───────────────
	{
		const char *_logPaths[] = {
			"/sdcard/Slayer3D_menu.log",
			"/storage/emulated/0/Slayer3D_menu.log",
			"/sdcard/Android/data/su.xash.engine.test/files/Slayer3D_menu.log",
			"/data/local/tmp/Slayer3D_menu.log",
			nullptr
		};
		FILE *_f = nullptr;
		for (int _i = 0; _logPaths[_i]; _i++) {
			_f = fopen(_logPaths[_i], "w");  // 'w' = fresh log each run
			if (_f) break;
		}
		if (_f) {
			fprintf(_f, "=== GetMenuAPI called ===\n");
			fprintf(_f, "  pFunctionTable  = %p\n", (void*)pFunctionTable);
			fprintf(_f, "  pEngfuncs       = %p\n", (void*)pEngfuncsFromEngine);
			fprintf(_f, "  pGlobals        = %p\n", (void*)pGlobals);
			fprintf(_f, "  UI_FUNCTIONS sz = %zu bytes\n", sizeof(UI_FUNCTIONS));
			fflush(_f);
			fclose(_f);
		}
	}
	// ────────────────────────────────────────────────────────────────────────
	if( !pFunctionTable || !pEngfuncsFromEngine )
	{
		return false;
	}

	// copy HUD_FUNCTIONS table to engine, copy engfuncs table from engine
	memcpy( pFunctionTable, &gFunctionTable, sizeof( UI_FUNCTIONS ));
	memcpy( &EngFuncs::engfuncs, pEngfuncsFromEngine, sizeof( ui_enginefuncs_t ));
	memset( &EngFuncs::textfuncs, 0, sizeof( ui_extendedfuncs_t ));

	gpGlobals = pGlobals;

	return true;
}

static UI_EXTENDED_FUNCTIONS gExtendedTable =
{
	AddTouchButtonToList,
	UI_MenuResetPing_f,
	UI_ConnectionWarning_f,
	UI_UpdateDialog,
	UI_ShowMessageBox,
	UI_ConnectionProgress_Disconnect,
	UI_ConnectionProgress_Download,
	UI_ConnectionProgress_DownloadEnd,
	UI_ConnectionProgress_Precache,
	UI_ConnectionProgress_Connect,
	UI_ConnectionProgress_ChangeLevel,
	UI_ConnectionProgress_ParseServerInfo
};

extern "C" EXPORT int GetExtAPI( int version, UI_EXTENDED_FUNCTIONS *pFunctionTable, ui_extendedfuncs_t *pEngfuncsFromEngine )
{
	{
		FILE *_ef = fopen("/sdcard/Slayer3D_menu.log", "a");
		if (!_ef) _ef = fopen("/storage/emulated/0/Slayer3D_menu.log", "a");
		if (!_ef) _ef = fopen("/data/local/tmp/Slayer3D_menu.log", "a");
		if (_ef) { fprintf(_ef, "[GetExtAPI] version=%d\n", version); fflush(_ef); fclose(_ef); }
	}
	if( !pFunctionTable || !pEngfuncsFromEngine )
	{
		return false;
	}

	if( version != MENU_EXTENDED_API_VERSION )
	{
		Con_Printf( "Error: failed to initialize extended menu API. Expected by DLL: %d. Got from engine: %d\n", MENU_EXTENDED_API_VERSION, version );
		return false;
	}

	memcpy( &EngFuncs::textfuncs, pEngfuncsFromEngine, sizeof( ui_extendedfuncs_t ) );
	memcpy( pFunctionTable, &gExtendedTable, sizeof( UI_EXTENDED_FUNCTIONS ));

	return true;
}
