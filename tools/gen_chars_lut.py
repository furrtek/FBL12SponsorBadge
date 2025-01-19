# Badge FBL12
# Generates LED lookup table for each char defined in chars.txt

import json
from PIL import Image, ImageDraw

charmap = {}
with open('chars.txt') as char_file:
	for line in char_file:
		if ':' in line:
			charmap[line[0]] = int(line.split(':')[1].split(',')[0].strip())

print("const uint8_t char_lut[64] = {")
for c in range(32, 96):
	char = chr(c)
	if char in charmap:
		code = charmap[char]
	else:
		code = 0
	
	# Bit reordering
	out = 0
	out += 1 if code & 1 else 0
	out += 2 if code & 128 else 0
	out += 4 if code & 64 else 0
	out += 8 if code & 32 else 0
	out += 16 if code & 16 else 0
	out += 32 if code & 8 else 0
	out += 64 if code & 4 else 0
	out += 128 if code & 2 else 0

	print("\t%d,\t// %s" % (out, char if char != "\\" else "Backslash"))
print("};")
