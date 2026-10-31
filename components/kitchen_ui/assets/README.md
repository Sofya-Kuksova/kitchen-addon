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
