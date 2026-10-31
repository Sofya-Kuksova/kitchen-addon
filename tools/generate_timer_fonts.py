"""Rasterize Inter and selected Google Material Symbols for LVGL 8; no firmware build.
Usage: python3 tools/generate_timer_fonts.py --material-font /tmp/kitchen-material-symbols.ttf
The official Material Symbols variable font is a generation input, never a firmware dependency.
"""
from pathlib import Path
import argparse
import io
import re
import struct
import zlib
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / 'components/kitchen_ui/assets'

def asset(name, font, codes, fallback=None):
    ascent, descent = font.getmetrics()
    bitmap, glyphs = [], [(0, 0, 0, 0, 0, 0)]
    for code in codes:
        char = chr(code)
        left, top, right, bottom = font.getbbox(char, anchor='ls')
        width, height = right-left, bottom-top
        im = Image.new('L', (max(1,width), max(1,height)))
        ImageDraw.Draw(im).text((-left,-top), char, font=font, fill=255, anchor='ls')
        pixels = [im.getpixel((x,y))//17 for y in range(height) for x in range(width)]
        glyphs.append((len(bitmap), round(font.getlength(char)*16), width, height, left, -bottom))
        pixels += [0] * ((-len(pixels))%2)
        bitmap.extend((pixels[i]<<4)|pixels[i+1] for i in range(0,len(pixels),2))
    out = [f'static const uint8_t {name}_bitmap[] = {{']
    for i in range(0,len(bitmap),24): out.append('    '+','.join(f'0x{x:02x}' for x in bitmap[i:i+24])+',')
    out += ['};', f'static const lv_font_fmt_txt_glyph_dsc_t {name}_glyphs[] = {{']
    for offset,adv,width,height,x,y in glyphs:
        out.append(f'    {{.bitmap_index={offset}, .adv_w={adv}, .box_w={width}, .box_h={height}, .ofs_x={x}, .ofs_y={y}}},')
    out += ['};', f'static const uint16_t {name}_unicode[] = {{'+','.join(str(c-codes[0]) for c in codes)+'};',
            f'static const lv_font_fmt_txt_cmap_t {name}_cmap = {{',
            f'    .range_start={codes[0]}, .range_length={codes[-1]-codes[0]+1}, .glyph_id_start=1,',
            f'    .unicode_list={name}_unicode, .list_length={len(codes)}, .type=LV_FONT_FMT_TXT_CMAP_SPARSE_TINY', '};',
            f'static const lv_font_fmt_txt_dsc_t {name}_dsc = {{',
            f'    .glyph_bitmap={name}_bitmap, .glyph_dsc={name}_glyphs, .cmaps=&{name}_cmap, .cmap_num=1, .bpp=4', '};',
            f'const lv_font_t {name} = {{',
            '    .get_glyph_dsc=lv_font_get_glyph_dsc_fmt_txt, .get_glyph_bitmap=lv_font_get_bitmap_fmt_txt,',
            f'    .line_height={ascent+descent}, .base_line={descent}, .dsc=&{name}_dsc'+
            (f', .fallback={fallback}' if fallback else ''), '};']
    return '\n'.join(out)

def inter(size, weight):
    f=ImageFont.truetype(str(ASSETS/'fonts/Inter.ttf'),size)
    f.set_variation_by_axes([32, weight])
    return f

def lvgl_symbol_source():
    # Restore the sfnt tables from LVGL's bundled WOFF, without a converter/build.
    path = ROOT/'managed_components/lvgl__lvgl/scripts/built_in_font/FontAwesome5-Solid+Brands+Regular.woff'
    data = path.read_bytes()
    # The LVGL 8.4 bundled .woff is already an sfnt font despite its extension.
    if data[:4] != b'wOFF': return data
    flavor, count = struct.unpack_from('>I', data, 4)[0], struct.unpack_from('>H', data, 12)[0]
    power = 1 << (count.bit_length()-1)
    result = bytearray(struct.pack('>IHHHH', flavor, count, power*16, power.bit_length()-1, count*16-power*16))
    result.extend(bytes(count*16))
    offset = len(result)
    for i in range(count):
        tag, start, packed, length, checksum = struct.unpack_from('>4sIIII',data,44+i*20)
        table = data[start:start+packed]
        if packed != length: table = zlib.decompress(table)
        if len(table) != length: raise ValueError('Invalid bundled WOFF table')
        struct.pack_into('>4sIII',result,12+i*16,tag,checksum,offset,length)
        result.extend(table)
        result.extend(bytes((-length)%4))
        offset = len(result)
    return bytes(result)

if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--material-font',required=True,type=Path)
    args=parser.parse_args()
    symbols={n:int(c,16) for n,c in (s.split() for s in (ASSETS/'material/codepoints.txt').read_text().splitlines())}
    repeat=ImageFont.truetype(str(args.material_font),24)
    repeat.set_variation_by_axes([0,0,24,400])
    assets=[asset('kitchen_repeat_24',repeat,[symbols['replay']],'LV_FONT_DEFAULT')]
    bundled_symbols=lvgl_symbol_source()
    symbol_header=(ROOT/'managed_components/lvgl__lvgl/src/font/lv_symbol_def.h').read_text()
    native_names={'OK','CLOSE','PLAY','PAUSE','STOP','PLUS','MINUS','UP','DOWN','EDIT','TRASH','LEFT','REFRESH'}
    native_codes=[]
    for name, escaped in re.findall(r'#define LV_SYMBOL_(\w+)\s+"([^"]+)"',symbol_header):
        if name in native_names:
            native_codes.append(ord(bytes(int(x,16) for x in re.findall(r'\\x([0-9A-Fa-f]{2})',escaped)).decode()))
    if len(native_codes) != len(native_names): raise ValueError('Required LVGL symbols absent')
    for size in (24,32):
        f=ImageFont.truetype(io.BytesIO(bundled_symbols),size)
        assets.append(asset(f'kitchen_lvgl_symbols_{size}',f,sorted(native_codes),'&kitchen_repeat_24'))
    kitchen_names = {'local_cafe', 'stockpot', 'skillet', 'oven'}
    for size in (40,48,64):
        names = kitchen_names | {'block'} if size == 48 else kitchen_names
        codes = sorted(symbols[n] for n in names)
        f=ImageFont.truetype(str(args.material_font),size)
        # Font axis order is FILL, GRAD, opsz, wght in this official font.
        axes=f.get_variation_axes()
        values=[{'Fill':0,'Grade':0,'Optical Size':min(size,48),'Weight':400}.get(a['name'].decode(),a['default']) for a in axes]
        f.set_variation_by_axes(values)
        assets.append(asset(f'kitchen_symbols_{size}',f,codes,'LV_FONT_DEFAULT'))
    for size in (36,48,56,64,72,112):
        assets.append(asset(f'kitchen_font_{size}',inter(size,750),list(range(48,59))))
    for size in (18,20,24,32):
        assets.append(asset(f'kitchen_text_{size}',inter(size,700 if size==32 else 500),list(range(32,127))+[0xb7,0x2022],'&kitchen_lvgl_symbols_24'))
    (ROOT/'components/kitchen_ui/kitchen_fonts.c').write_text(
        '/* Inter / LVGL symbols / Google kitchen symbols subsets. See assets/fonts/OFL.txt and assets/material/LICENSE. */\n#include "lvgl.h"\n\n'+'\n\n'.join(assets)+'\n')
    (ROOT/'components/kitchen_ui/include/kitchen_symbols.h').write_text(
        '/* Google Material Symbols Outlined. Source codepoints: assets/material/codepoints.txt. */\n#pragma once\n'+
        '\n'.join('#define KITCHEN_SYMBOL_'+n.upper()+' "'+''.join('\\x%02x'%b for b in chr(c).encode())+'"' for n,c in symbols.items())+'\n')
