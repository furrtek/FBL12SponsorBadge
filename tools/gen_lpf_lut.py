# Badge FBL12
# Generates LPF alpha LUT

import math

# Linear fc from 1kHz to 100Hz in 256 steps (15.2 Hz)
start_freq = 1000
stop_freq = 100
delta = (start_freq - stop_freq) / 256.0
dt = 1 / 8000.0

# C
fc = start_freq
with open("lpf_lut.c", "w") as outfile:
	outfile.write("const uint8_t lpf_lut[256] = {\n")
	for step in range(256):
		fc -= delta
		e = 2 * math.pi * fc * dt
		a = int((e / (e + 1)) * 256)
		outfile.write("\t%d,\t// %dHz\n" % (a, fc))
	outfile.write("};")
