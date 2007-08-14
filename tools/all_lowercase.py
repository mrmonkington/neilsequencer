#!/usr/bin/python

import sys, os

if not len(sys.argv) == 2:
	print "syntax:"
	print "\tall_lowercase.py <path>"
	print "\t"
	print "converts all files and folders to lowercase and replaces blanks with underscores."
	raise SystemExit

for root,folders,files in os.walk(sys.argv[1]):
	if not '.svn' in root:
		for filename in folders + files:
			if not '.svn' in filename:
				fullpath_old = os.path.join(root, filename)
				fullpath_new = os.path.join(root, filename.lower().replace(' ','_'))
				if fullpath_old != fullpath_new:
					print "%s => %s" % (fullpath_old, fullpath_new)
					os.rename(fullpath_old, fullpath_new)
