# UI source assets

Inter variable font: https://github.com/google/fonts/tree/main/ofl/inter
Local source: fonts/Inter.ttf; license: fonts/OFL.txt (SIL OFL 1.1).
Runtime subsets: digits 36/48/56/64/72/112 px at weight 750; body 500; headings 700; optical size 32.

Google Material Symbols Outlined: https://fonts.google.com/icons
Official sources: https://github.com/google/material-design-icons
Selected original SVGs, codepoints and Apache-2.0 LICENSE are in material/.
Selected Google icons: local_cafe, stockpot, skillet, oven; Block for No icon (48 px); Replay for Repeat (24 px).
Kitchen SVGs are separate 40/48-px downloads with fill="#FFFFFF". The firmware uses
40-px kitchen glyphs in 64-px badges and 48-px kitchen glyphs in the picker.
Other Google control/navigation glyphs have been removed; Block and Replay are explicit UI exceptions.

Controls use LV_SYMBOL_OK/CLOSE/PLAY/PAUSE/STOP/PLUS/MINUS/UP/DOWN/EDIT/TRASH/LEFT. Repeat uses Google Replay.
Their exact codepoints are read from the local LVGL lv_symbol_def.h; 24/32-px glyphs
are rasterized from LVGL's bundled FontAwesome5-Solid+Brands+Regular.woff, without
network downloads or modifying vendor code. Inter falls back to the local native
24-px symbol subset, so host font flags do not need to change. No icon uses the restored crossed-circle Block, not CLOSE.

Regenerate with Pillow (not part of the firmware build):

1. Download the official variable font to a temporary file:
   https://raw.githubusercontent.com/google/material-design-icons/master/variablefont/MaterialSymbolsOutlined%5BFILL,GRAD,opsz,wght%5D.ttf
2. Verify SHA-256: f7837ba8573d9a7a74931618ced8307bc81f96d49af296bf6c8052c772ed5282
3. Run `python3 tools/generate_timer_fonts.py --material-font /tmp/kitchen-material-symbols.ttf`
   from the repository root.

kitchen_fonts.c and include/kitchen_symbols.h are generated project-owned subsets.
No vendor file, runtime font renderer, hardware configuration or host dependency is changed.
SHA256SUMS records the source files after the requested white-fill edit. Downloads were made on 2026-10-30;
regeneration against a changed upstream font requires visual review.

Replay is restored by rasterizing codepoint e042 at 24 px from the already cached
Google variable font (the SHA-256 above). No new SVG is required for generation;
kitchen_repeat_24 is in the Inter/native-symbol fallback chain.

Detailed timer cards use an additional 64-px kitchen subset, rasterized directly
from the cached original variable font at 64 px and optical size 48 (the font's
maximum optical-size axis). Compact 40/48-px bitmaps are not enlarged or replaced.
The 64-px glyph is shown in a 96-px badge. No extra runtime font renderer is needed.

## Supplemental UI assets

Current medium cards use kitchen_symbols_56 (56px, optical size 48), rasterized
directly from the official variable font, not enlarged from 40px bitmaps.
Settings uses kitchen_settings_36, Material Symbols Outlined U+E8B8, size 36
(33px ink height), replacing the unreliable LV_SYMBOL_SETTINGS transform path.
Reference SVG: material/settings_36px.svg, white fill, from the official
https://raw.githubusercontent.com/google/material-design-icons/master/symbols/web/settings/materialsymbolsoutlined/settings_48px.svg
Only height/width and white fill are changed; the vector path is unchanged.
Runtime glyphs come from the reviewed official TTF, not an SVG decoder.

Official TTF:
https://raw.githubusercontent.com/google/material-design-icons/master/variablefont/MaterialSymbolsOutlined%5BFILL,GRAD,opsz,wght%5D.ttf
SHA-256: f7837ba8573d9a7a74931618ced8307bc81f96d49af296bf6c8052c772ed5282.
The downloaded input matches the source used for existing kitchen glyphs.
PAUSED/DONE use Inter Bold (weight 700), 18/20px, only the required capital letters
and space, from the existing fonts/Inter.ttf under SIL OFL 1.1.

Generate ONLY the supplemental file (no firmware build):

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/generate_ui_extensions.py \
  --material-font /tmp/kitchen-material-symbols.ttf \
  --preview /tmp/kitchen-font-extensions.png
```

Output: kitchen_font_extensions.c. Existing generated kitchen_fonts.c and
include/kitchen_symbols.h are not regenerated or edited. CMake explicitly includes
the new translation unit. Material Symbols license remains material/LICENSE
(Apache-2.0); Inter license remains fonts/OFL.txt.

Status hierarchy: S1 uses the supplemental Inter700 kitchen_status_36. Medium cards
and overlay use the supplemental kitchen_status_24 from local Inter.ttf, and
small cards retain kitchen_status_18. Generate only the new file without
regenerating previous subsets or requiring the Material Symbols input:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/generate_ui_extensions.py --status24-only
```

Output: kitchen_status_24.c, glyphs restricted to PAUSED/DONE and space,
Inter Bold700, line-height30/base-line6, SIL OFL1.1. This is media asset
generation only, not a firmware/project compilation.

## Proportional large-card assets

Current kitchen icon tiers are 40/56/88px. The previous 64px large-card subset
is retained in the original generated source, but the main large card uses 88px.
New glyphs are rasterized directly from the same reviewed official variable TTF
(SHA-256 f7837ba8573d9a7a74931618ced8307bc81f96d49af296bf6c8052c772ed5282),
not enlarged from smaller bitmaps. Reusing this verified downloaded source
avoids replacing the icon designs with a different set. Optical size stays at
the font maximum 48 for the 88px render.

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/generate_ui_extensions.py \
  --material-font /tmp/kitchen-material-symbols.ttf --large-only \
  --preview /tmp/kitchen-icon-scales.png
```

Only kitchen_large_assets.c is generated: kitchen_symbols_88 (four kitchen
glyphs), kitchen_settings_52 (Google Settings), kitchen_controls_48 (native
LVGL FontAwesome Play/Pause/OK), kitchen_status_36 (Inter700). Existing generated
subsets remain untouched. Runtime has no TTF renderer, downloads or transform
zoom; native sizes are selected by the available dimensions of the card/button.
Licenses are listed in FONT_LICENSE.txt. The PNG is an asset preview, not a
panel screenshot or device validation.

## Overlay action weight

`kitchen_overlay_fonts.c` adds `kitchen_actions_20`, Inter Bold700 at20px,
ASCII plus middle dot for Repeat labels, using the existing local Inter.ttf
under SIL OFL1.1. The name button, textarea and placeholder reuse the existing
Inter Bold32 subset. No existing generated files are rewritten:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/generate_ui_extensions.py --overlay-only \
  --preview /tmp/kitchen-overlay-fonts.png
```

This generates media assets only, without a compiler or firmware build.
