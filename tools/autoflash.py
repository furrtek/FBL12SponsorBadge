# Badge FBL12
# Sends an UF2 file to a FBL badge drive
# Logs UIDs, can skip file copy if UID already known

import os
import os.path
from os import listdir
from os.path import isfile, join
import sys
import struct
import time

file_path = "G:/readme.txt"

#uf2_file = "sounds.bin.uf2"
uf2_file = "BadgeFBL12-01.bin.uf2"
skip = False	# Set to True to skip known UIDs

# Load known UIDs
uids = []
with open("UIDs.txt", 'r') as f:
    for line in f:
    	if len(line) > 20:
    		uids.append(line.strip("\n").split("\t")[0])

print("Loaded %d UIDs" % len(uids))

while(True):
	print("Waiting for badge drive...")

	# Poll for file_path presence
	while not os.path.exists(file_path):
	    time.sleep(1)

	if os.path.isfile(file_path):
		with open(file_path, 'r') as f:
			# Search for UID line in file_path
		    for line in f:
		        if "UID: " in line:
		        	uid = line.replace("UID: ", "").strip("\t\n")
		        	print("Found badge with UID %s" % uid)

		        	if (uid in uids) and skip:
		        		print("\tAlready seen ! Skipping")
		        	else:
			        	print("\tFlashing %s..." % uf2_file)
			        	os.system("copy " + uf2_file + " G:\\" + uf2_file)
			        	if uid in uids:
			        		# Don't log already known UID
			        		print("\tDone !")
			        	else:
			        		# Update UID list
			        		uids.append(uid)
			        		with open("UIDs.txt", 'a') as f:
			        			f.write(uid + "\n")
			        		print("\tDone ! Badge has been added to UID list")
			        break
					
		# Wait for file_path to disappear
		print("\tRemove badge...")
		while os.path.exists(file_path):
		    time.sleep(1)
