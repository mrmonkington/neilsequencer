# Anders' first program ever written in Python :)
# Creates zzub-wrappers for all Buzz DLL's in current folder

# Those DLLs that are Buzz-DLLs and do not have a "buzz"-prefix are renamed, and
# a zzub-wrapper template is copied from buzz2zzub/buzz2zzub.dll to the old name
# If a DLL already has a buzz-prefix, the wrapper dll is simply replaced.

import os
import shutil

def is_buzz_plugin(filename):
	# scan for known binary strings in the file to identify buzz machines
	f = open(filename, 'rb')
	f.seek(0, 2)
	size = f.tell()
	f.seek(0,0)
	binary_data = f.read(size)
	f.close()
	
	createmachine_index=binary_data.find('CreateMachine')
	getinfo_index=binary_data.find('GetInfo')

	zzubcreatemachine_index=binary_data.find('zzub_create_plugin')
	zzubgetinfo_index=binary_data.find('zzub_get_info')

	if createmachine_index!=-1 and getinfo_index!=-1 and zzubcreatemachine_index==-1 and zzubgetinfo_index==-1: return 1
	return 0

# find all files in current folder
for f in os.listdir('.'):
	if f.endswith('.dll') and os.path.isfile(f):
		# we know it ends with .dll and is a file
		if is_buzz_plugin(f):
			# its a buzz plugin indeed, lets rename it and copy the wrapper
			wrapper_file=f
			if f.startswith('buzz'):
				wrapper_file=f[4:None]
				print('Re-wrapping ' + f + ' for ' + wrapper_file )
			else:
				print('Renaming and creating wrapper for ' + f)
				shutil.move(f, 'buzz' + f)

			shutil.copyfile('buzz2zzub/buzz2zzub.dll', wrapper_file)
