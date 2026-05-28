#ifndef VGUI_LOG_H
#define VGUI_LOG_H

// Diagnostic logging for VGUI1 dialog flow.
// Two channels are used together so a crash always leaves a trail:
//
//   1. VLOG()  -> EngFuncs::engfuncs.Con_Printf
//      Engine captures Con_Printf output to <gamedir>/qconsole.log,
//      which is the canonical place to look first.
//
//   2. VLOGF() -> appends one line to <gamedir>/logs/vgui_diag.log
//      via fopen("a"). Survives even if engine console is unavailable
//      (e.g. very early init or after a hard crash that lost console).
//
// Both channels are no-ops when the cvar bridge / engfuncs are not yet
// wired, so it's safe to call them from any constructor.

#ifdef __cplusplus
extern "C" {
#endif

void VguiLog( const char *fmt, ... );      // Con_Printf
void VguiLogFile( const char *fmt, ... );  // separate file

#ifdef __cplusplus
}
#endif

// Convenience: log to BOTH places at once. In release builds VLOG compiles
// to a no-op so the per-call fopen + Con_Printf cost (~12 calls per click
// in the Options dialog) is fully eliminated. Define VGUI_DIAG at build
// time to re-enable diagnostic logging.
#ifdef VGUI_DIAG
#  define VLOG(...)  do { VguiLog(__VA_ARGS__); VguiLogFile(__VA_ARGS__); } while (0)
#else
#  define VLOG(...)  ((void)0)
#endif

#endif // VGUI_LOG_H
