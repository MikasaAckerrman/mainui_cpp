/*
embedded_font.h - access to the TTF bytes compiled into libmenu.so
Copyright (c) 2026 DragonSlayer Team

The byte array itself lives in font/embedded_font_data.cpp, which is
auto-generated from font/embedded_source/source.ttf by
font/embedded_source/gen_embedded.py. CFontManager::LoadFontDataFile
falls back to it when the on-disk lookup misses, so the engine always
has a usable font without depending on the game directory or APK assets
being unpacked correctly.
*/

#ifndef MAINUI_EMBEDDED_FONT_H
#define MAINUI_EMBEDDED_FONT_H

// True if `vfspath` matches the gfx/fonts/<name>.ttf path that
// FindFontDataFile() generates for the embedded font's logical name
// (e.g. "Tahoma" -> "gfx/fonts/tahoma.ttf"). When this returns true,
// LoadFontDataFile is allowed to substitute the embedded bytes.
bool Embedded_PathMatches( const char *vfspath );

// Returns a pointer to the embedded TTF bytes. The buffer is read-only
// static memory owned by libmenu.so; the caller MUST NOT free it. Length
// is written to *plen if non-null. Returns NULL when no font is
// embedded (g_embeddedFontLen == 0).
//
// Returned as `unsigned char*` rather than the engine's `byte` typedef
// so this header has no dependency on the engine SDK chain - it can be
// included anywhere.
unsigned char *Embedded_GetData( int *plen );

#endif // MAINUI_EMBEDDED_FONT_H
