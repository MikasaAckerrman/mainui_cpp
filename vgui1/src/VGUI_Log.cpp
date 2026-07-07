// VguiLog -- diagnostic logger used to trace the VGUI1 dialog flow.
//
// Writes to two destinations:
//   * Engine console  (-> <gamedir>/qconsole.log via engine)
//   * <gamedir>/logs/vgui_diag.log -- BUFFERED (see below)
//
// PERFORMANCE (why buffered):
//   The old VguiLogFile did fopen/fprintf/fclose on EVERY call. On a touch
//   drag the engine fires hundreds of cursorMoved events per second, so the
//   per-event fopen caused visible drag/resize lag on device -- which is why
//   diagnostic logging had to be compiled out entirely.
//   This version accumulates lines in an in-memory buffer and flushes to disk
//   only when the buffer fills (VGUI_LOG_FLUSH_LINES) or on an explicit
//   VguiLogFlush() (called on shutdown and at key diagnostic moments). One
//   fopen per N lines instead of per line -> negligible cost, no drag lag,
//   safe to leave enabled while debugging.
//
// All calls are guarded so they're safe before engfuncs is fully wired.

#include "BaseMenu.h"  // for EngFuncs and gpGlobals - heavy include first

#include <VGUI_Log.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// --- in-memory line buffer ---------------------------------------------
// Flush to disk once this many lines have accumulated.
#define VGUI_LOG_FLUSH_LINES  64
// Max chars per buffered line (including trailing newline + NUL).
#define VGUI_LOG_LINE_MAX     1024

static char s_logBuf[VGUI_LOG_FLUSH_LINES][VGUI_LOG_LINE_MAX];
static int  s_logCount = 0;

// Resolve <gamedir>/logs/vgui_diag.log (fallback: <gamedir>/vgui_diag.log,
// or plain relative path if gamedir is not yet known). Returns via out.
static void VguiLog_ResolvePath( char *out, int outSize, bool useLogsDir )
{
	char gamedir[256] = { 0 };
	if ( EngFuncs::engfuncs.pfnGetGameDir )
		EngFuncs::engfuncs.pfnGetGameDir( gamedir );

	if ( gamedir[0] )
		snprintf( out, outSize, useLogsDir ? "%s/logs/vgui_diag.log" : "%s/vgui_diag.log", gamedir );
	else
		snprintf( out, outSize, useLogsDir ? "logs/vgui_diag.log" : "vgui_diag.log" );
}

// Write the whole in-memory buffer to disk in ONE fopen/fclose, then clear it.
extern "C" void VguiLogFlush( void )
{
	if ( s_logCount <= 0 )
		return;

	char path[512];
	VguiLog_ResolvePath( path, sizeof( path ), true );

	FILE *f = fopen( path, "a" );
	if ( !f )
	{
		// logs/ dir might not exist -- fall back to the gamedir root.
		VguiLog_ResolvePath( path, sizeof( path ), false );
		f = fopen( path, "a" );
	}
	if ( f )
	{
		for ( int i = 0; i < s_logCount; i++ )
			fputs( s_logBuf[i], f );
		fclose( f );
	}

	// Clear the buffer whether or not the write succeeded, so a persistently
	// unwritable path can't make the buffer grow unbounded (it's fixed-size
	// anyway, but this keeps line ordering sane).
	s_logCount = 0;
}

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

	// Leave headroom so the "[VGUI] " prefix + "\n" can't overflow the buffered
	// line (avoids format-truncation and keeps every message intact).
	char buf[VGUI_LOG_LINE_MAX - 16];
	va_list args;
	va_start( args, fmt );
	vsnprintf( buf, sizeof( buf ), fmt, args );
	va_end( args );
	buf[sizeof( buf ) - 1] = 0;

	// Append "[VGUI] <msg>\n" into the in-memory buffer. No disk I/O here --
	// that happens in VguiLogFlush() once the buffer fills or on demand.
	snprintf( s_logBuf[s_logCount], VGUI_LOG_LINE_MAX, "[VGUI] %s\n", buf );
	s_logCount++;

	if ( s_logCount >= VGUI_LOG_FLUSH_LINES )
		VguiLogFlush();
}
