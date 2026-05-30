# Embedded font source

This directory holds the **source TTF** that gets compiled into
`libmenu.so` as a static byte array. The font is always available at
runtime, regardless of game-directory state or APK asset extraction.

## Files

| File | Purpose |
|------|---------|
| `source.ttf` | The TTF whose bytes are embedded. Currently DejaVu Sans Condensed (Book) v2.37. |
| `LICENSE` | License of `source.ttf`. Currently the Bitstream Vera + DejaVu license (free to redistribute). |
| `gen_embedded.py` | Regenerates `font/embedded_font_data.cpp` from `source.ttf`. |

## How the embedded font works

1. Engine asks `CFontManager::FindFontDataFile("Tahoma", ...)` for a path.
2. `FindFontDataFile` returns `gfx/fonts/tahoma.ttf` (the canonical path
   for the "Tahoma" slot in TrackerScheme.res).
3. `CFontManager::LoadFontDataFile("gfx/fonts/tahoma.ttf")` first tries
   the engine VFS (`COM_LoadFile`). If a user dropped their own
   `tahoma.ttf` into the gamedir, that wins.
4. If VFS has no file, `LoadFontDataFile` falls back to the embedded
   bytes via `Embedded_PathMatches` + `Embedded_GetData`. The pointer is
   `static const`; the caller never frees it.

Result: the menu **always has a font**, but a user can still override
with their own TTF on the device without recompiling.

## Why DejaVu Sans Condensed (and not Tahoma)?

- Tahoma is proprietary Microsoft font software. We cannot legally embed
  the actual Tahoma TTF in a public binary.
- DejaVu Sans Condensed comes from Carter's Bitstream Vera lineage - the
  same designer who later made Tahoma for Microsoft. Among free fonts
  with full Cyrillic coverage, this is the closest match in design DNA
  and proportions (condensed, low-x-height-ratio, UI-tuned).
- ~680 KB on disk. ~700 KB after embedding into the .so.

## Swapping in a different font

```sh
cd mainui_cpp/font/embedded_source
# Replace source.ttf with whatever you want (your own licensed Tahoma,
# Verdana, Liberation Sans, etc.) - keep the filename source.ttf.
cp /path/to/your/tahoma.ttf source.ttf
# Regenerate embedded_font_data.cpp:
python3 gen_embedded.py
# Rebuild libmenu.so. Done.
```

The logical slot name stays "Tahoma" so TrackerScheme.res does not need
to change. Only the *bytes* inside `embedded_font_data.cpp` change.

If you are doing this in a private fork for personal use with a font you
own (e.g. Tahoma from a Windows licence), you are within the bounds of
the Microsoft EULA. Do not push such a fork to a public repo.
