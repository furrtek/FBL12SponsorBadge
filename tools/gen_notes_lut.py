# Badge FBL
# Generates note frequency lookup table

samplerate = 8000
acc_max = 65536

hertz = acc_max / samplerate

# Octaves 2, 3, 4 C -> B
str_out = ""
for c in range(12 * 3):
	freq = 220 * (2 ** ((c - 9 - 12) / 12))
	str_out += str(int(freq * hertz)) + ", "
print(str_out)
