# Embedded font source

This directory holds the **source TTF** that gets compiled into
`libmenu.so` as a static byte array. The font is always available at
runtime, regardless of game-directory state or APK asset extraction.

## Files

| File | Purpose |
|------|---------|
| `source.ttf` | The TTF whose bytes are embedded. **Currently Microsoft Tahoma 1995-1999** (see `LICENSE` for the conditions under which this is acceptable). |
| `LICENSE` | Notice covering the bundled `source.ttf`. The current Tahoma version is proprietary Microsoft software; only legitimate for **private** builds owned by users with a Microsoft licence. |
| `gen_embedded.py` | Regenerates `font/embedded_font_data.cpp` from `source.ttf`. |

## How the embedded font works

1. Engine asks `CFontManager::FindFontDataFile("Tahoma", ...)` for a path.
2. `FindFontDataFile` returns `gfx/fonts/tahoma.ttf` (the canonical path
   for the "Tahoma" slot in TrackerScheme.res).
3. `CFontManager::LoadFontDataFile("gfx/fonts/tahoma.ttf")` first tries
   the engine VFS (`COM_LoadFile`). If a user dropped their own
   `tahoma.ttf` into the gamedir, that wins.
4. If VFS has no file, `LoadFontDataFile` falls back to the embedded
   bytes via `Embedded_PathMatches` + `Embedded_GetData`. The pointer
   is `static const`; the caller never frees it.

Result: the menu **always has a font**, but a user can still override
with their own TTF on the device without recompiling.

## Why Tahoma here?

The VGUI1 Options dialog in CS 1.6 PC uses Tahoma. To match the canon
look pixel-for-pixel the actual Microsoft Tahoma TTF needs to be
rendered, not a substitute. Free fonts in the same design family
(Bitstream Vera / DejaVu, Noto Sans) come close but visibly differ in
the Cyrillic block.

For a public, redistributable build use a free substitute:

```sh
# Drop a freely-licensed TTF in source.ttf, e.g. DejaVu Sans Condensed
# from https://dejavu-fonts.github.io (Bitstream Vera + DejaVu licence).
cp /path/to/DejaVuSansCondensed.ttf source.ttf
python3 gen_embedded.py
# Update LICENSE to reflect the new font's licence terms.
# Rebuild libmenu.so.
```

The logical slot name stays `"Tahoma"` so `TrackerScheme.res` does not
need to change. Only the **bytes** inside `embedded_font_data.cpp`
change.

## Coverage of current source.ttf

The current Tahoma 1995-1999 version covers:

- 95/95 ASCII glyphs (full Latin)
- 122/255 of the Unicode Cyrillic block (U+0400-U+04FF)
- All 66 letters of basic Russian (А-Я а-я Ёё)
- All glyphs used in the Slayer3D Russian menu (Настройки,
  Мультиплеер, Клавиатура, Мышь, Звук, Видео, HUD, Аккаунт, Система,
  Аватар, Логотип, Загрузить, Изменить цвет, Имя игрока, Пароль,
  Применить, Отмена, Дополнительно, Показывать прицел, Фильтр мыши,
  Чувствительность, Прямой ввод)

Languages outside basic Russian (Ukrainian "ї", "є", "і"; Belarusian
"ў"; Kazakh "ұ"; etc.) are not in this Tahoma version. If you need
those, source a newer Tahoma (Vista/7 era) or fall back to a free
font with full Cyrillic coverage.
