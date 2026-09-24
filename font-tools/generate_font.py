from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import json, math

base = Path(__file__).resolve().parent
font = ImageFont.truetype(str(base / 'NotoSansKR.ttf'), 18)
font.set_variation_by_axes([500])
codes = list(range(32,127)) + list(range(0x3131,0x318f)) + list(range(0xac00,0xd7a4)) + [0xb7,0x2103]
glyphs = {}
rows = []
for code in codes:
    ch = chr(code)
    advance = min(20, math.ceil(font.getlength(ch)) + 1)
    im = Image.new('L', (20,24))
    ImageDraw.Draw(im).text((0,19), ch, font=font, fill=255, anchor='ls')
    packed = bytearray(60)
    for y in range(24):
        for x in range(20):
            if im.getpixel((x,y)) >= 100:
                bit=y*20+x
                packed[bit//8] |= 0x80 >> (bit%8)
    glyphs[ch] = [advance, list(packed)]
    rows.append('{'+str(advance)+','+','.join(map(str,packed))+'}')
header = '#pragma once\n#include <pgmspace.h>\n// Sangam18 bitmap derivative of Noto Sans KR, SIL OFL 1.1. See font-tools/OFL.txt.\n'
header += 'const uint8_t SANGAM18[][61] PROGMEM = {\n'+',\n'.join(rows)+'\n};\n'
header += '''inline int fontIndex(uint32_t c) {
  if (c>=32 && c<=126) return c-32;
  if (c>=0x3131 && c<=0x318e) return 95+c-0x3131;
  if (c>=0xac00 && c<=0xd7a3) return 189+c-0xac00;
  if (c==0xb7) return 11361;
  if (c==0x2103) return 11362;
  return '?' - 32;
}
inline int glyphAdvance(uint32_t c) { return pgm_read_byte(&SANGAM18[fontIndex(c)][0]); }
'''
(base.parent/'sangam_display'/'sangam18.h').write_text(header,encoding='utf-8')
(base/'glyphs.json').write_text(json.dumps(glyphs,ensure_ascii=False),encoding='utf-8')
print(f'Generated {len(rows)} glyphs, 18px native raster, {len(rows)*61} flash bytes.')
