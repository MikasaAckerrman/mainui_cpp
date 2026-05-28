// VguiLog -- diagnostic logger used to trace the Options dialog crash.
//
// Writes to two destinations:
//   * Engine console  (-> <gamedir>/qconsole.log via engine)
//   * <gamedir>/logs/vgui_diag.log via fopen("a")
//
// Both calls are guarded so they're safe before engfuncs is fully wired.

#include "BaseMenu.h"  // for EngFuncs and gpGlobals - heavy include first

#include <VGUI_Log.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern "C" void VguiLog( const char *fmt, ... )
{
	if ( !fmt ) return;

	char buf[1024];
	va_list args;
	va_start( args, fmt );
	vsnprintf( buf, sizeof( buf ), fmt, args );
	va_end( args );
	buf[sizeof( buf ) - 1] = 0;

	// Direct call to engfuncs.Con_Printf via the macro defined in
	// enginecallback_menu.h. By the time any menu cmd fires the engine
	// has populated this slot, so the call is safe without an explicit
	// null guard (and the field-access form clashes with the macro).
	Con_Printf( "[VGUI] %s\n", buf );
}

extern "C" void VguiLogFile( const char *fmt, ... )
{
	if ( !fmt ) return;

	char buf[1024];
	va_list args;
	va_start( args, fmt );
	vsnprintf( buf, sizeof( buf ), fmt, args );
	va_end( args );
	buf[sizeof( buf ) - 1] = 0;

	// Resolve <gamedir>/logs/vgui_diag.log. Engine getGameDir gives us the
	// absolute path on Android (writable scoped storage), so plain fopen
	// works without permission gymnastics.
	char gamedir[256] = { 0 };
	if ( EngFuncs::engfuncs.pfnGetGameDir )
		EngFuncs::engfuncs.pfnGetGameDir( gamedir );

	char path[512];
	if ( gamedir[0] )
		snprintf( path, sizeof( path ), "%s/logs/vgui_diag.log", gamedir );
	else
		snprintf( path, sizeof( path ), "logs/vgui_diag.log" );

	FILE *f = fopen( path, "a" );
	if ( !f )
	{
		// Logs/ dir might not exist -- fall back to the gamedir root.
		if ( gamedir[0] )
			snprintf( path, sizeof( path ), "%s/vgui_diag.log", gamedir );
		else
			snprintf( path, sizeof( path ), "vgui_diag.log" );
		f = fopen( path, "a" );
	}
	if ( f )
	{
		fprintf( f, "[VGUI] %s\n", buf );
		fclose( f );
	}
}
