# Badge FBL12
# Generates an UF2 update file
# Returns 0 on success, 1 on any error

# gen_update.py binfile type

# Main format is UF2 compatible, see design doc for info about the custom fields

import os
import os.path
from os import listdir
from os.path import isfile, join
import sys
import struct

def print_usage():
	print("Generates an update file for the FBL12 badge")
	print("Usage:")
	print("\t%s binfile type" % sys.argv[0])
	print("\tbinfile: Input raw data file or sound directory")
	print("\ttype: 0 for app image, 1 for ext flash sound data, 2 for user data")
	exit(1)

# This MUST be 256
block_data_size = 256

def gen_block(mem_index, dest_addr, block_number, total_blocks, payload, overall_checksum):
	block = bytearray()

	# UF2 header
	block += (0x0A324655).to_bytes(4, byteorder='little')	# "UF2\n"
	block += (0x9E5D5157).to_bytes(4, byteorder='little')	# Start magic
	block += (0x00000001).to_bytes(4, byteorder='little')	# UF2 flags: not main flash
	block += dest_addr.to_bytes(4, byteorder='little')
	block += block_data_size.to_bytes(4, byteorder='little')	# In bytes
	block += block_number.to_bytes(4, byteorder='little')
	block += total_blocks.to_bytes(4, byteorder='little')
	block += (0).to_bytes(4, byteorder='little')

	block += payload

	sum_payload = 0
	for byte in payload:
		sum_payload += byte
	sum_payload &= 0xFFFF

	# Special fields, not part of the payload
	block += (0x2A).to_bytes(1, byteorder='little')			# Update file magic
	block += mem_index.to_bytes(1, byteorder='little')		# Destination memory index
	block += sum_payload.to_bytes(2, byteorder='little')	# Payload byte sum
	block += overall_checksum.to_bytes(4, byteorder='little')		# Complete payload for memory index byte sum

	block += bytes(512 - len(block) - 4)		# Padding

	block += (0x0AB16F30).to_bytes(4, byteorder='little')	# End magic

	return block

def generate(file_path, update_type):
	out_path = file_path + ".uf2"

	if not os.path.isfile(file_path):
		print("ERROR: File %s not found" % file_path)
		exit(1)

	file_size = os.path.getsize(file_path)

	# Compute overall checksum
	# This needs to be done before UF2 blocks are generated because they ALL contain the overall checksum

	with open(file_path, "rb") as f_in:
		data = bytearray(f_in.read())

	data_remaining = len(data) % 256
	if data_remaining:
		data += bytes(256 - data_remaining)		# Padding
	data_remaining = len(data)

	total_blocks = data_remaining // block_data_size

	overall_checksum = 0
	for byte in data:
		overall_checksum += byte	# Byte sum for UF2 overall checksum

	if update_type == 0:
		# App, we need to add the app size and 32-bit checksum to the end of flash (additionnal UF2 block)
		mem_index = 0
		dest_addr = 0x00000000			# Relative to ADDR_APP
		max_size = 0x00020000 - 32768 - 2048 - 2048	# Reserve last app flash page for app size and checksum, could be merged with data if app ever grows to this size

		total_blocks += 1

		app_checksum = 0
		for i in range(0, len(data), 4):
			app_checksum += int.from_bytes(data[i:i+4], byteorder='little', signed=False)
		app_checksum &= 0xFFFFFFFF
		print("App size: 0x%08x" % data_remaining)
		print("App checksum: 0x%08x" % app_checksum)

		last_block_payload = bytearray()
		last_block_payload += bytes(256 - 4 - 4)		# Padding
		last_block_payload += data_remaining.to_bytes(4, byteorder='little')
		last_block_payload += app_checksum.to_bytes(4, byteorder='little')

		# Update UF2 overall checksum with additional data
		for byte in last_block_payload:
			overall_checksum += byte
	elif update_type == 1:
		# Ext flash, this is much simpler
		mem_index = 1
		dest_addr = 0x00040000	# Preset samples area
		max_size = 0x00040000	# 256kB
	else:
		# User data, this is much simpler
		mem_index = 0
		dest_addr = 0x00020000 - 32768 - 2048			# Relative to ADDR_APP
		max_size = 2048

	if file_size > max_size:
		print("ERROR: File %s won't fit" % file_name)
		exit(1)

	src_addr = 0
	blocks = []
	block_number = 0

	# Generate UF2 blocks
	while(data_remaining > 0):
		payload = data[src_addr:src_addr + block_data_size]
		block = gen_block(mem_index, dest_addr, block_number, total_blocks, payload, overall_checksum)
		blocks.append(block)

		src_addr += block_data_size
		dest_addr += block_data_size
		data_remaining -= block_data_size
		block_number += 1

	if update_type == 0:
		# Generate last app block with app size and checksum
		block = gen_block(mem_index, 0x00017700, block_number, total_blocks, last_block_payload, overall_checksum)
		blocks.append(block)

	with open(out_path, "wb") as f_out:
		for block in blocks:
			f_out.write(block)

	print("Total blocks: %d" % total_blocks)
	print("Overall checksum: 0x%08x" % overall_checksum)
	print("Generated %s update file" % out_path)

	exit(0)

if len(sys.argv) == 3:
	update_type = int(sys.argv[2])
	if not update_type in [0, 1, 2]:
		print_usage()
	
	mypath = sys.argv[1]

	if update_type == 1:
		# Process wav files
		# Assumes they're already u8 8kHz mono
		onlyfiles = [f for f in listdir(mypath) if isfile(join(mypath, f))]

		data_out = bytearray()
		for c in range(8):
			for f in onlyfiles:
				if f[0] == str(c):
					print("Processing %s" % f)
					break
			with open(join(mypath, f), "rb") as f_in:
				data = bytearray(f_in.read())
			sample_count = int.from_bytes(data[0x28:0x28+4], byteorder='little', signed=False)
			print("Sample count 0x%X" % sample_count)

			sample_data = data[0x2C:0x2C + sample_count]
			mod = len(sample_data) % 256
			if mod:
				sample_data += bytes(256 - mod)		# Padding to 256 byte page
			page_count = len(sample_data) // 256
			print("page_count 0x%X" % page_count)

			data_slot = bytearray()
			data_slot += page_count.to_bytes(2, byteorder='little')
			data_slot += bytes(256 - 2)		# Padding to 256 byte page
			data_slot += sample_data

			#with open(f + ".bin", "wb") as f_out:
			#	f_out.write(data_out)

			data_slot += bytes(32768 - len(data_slot))		# Padding to 32768 byte sample slot
			data_out += data_slot

		with open("sounds.bin", "wb") as f_out:
			f_out.write(data_out)
		mypath = "sounds.bin"

	generate(mypath, update_type)
else:
	print_usage()
