#!/usr/bin/python

import sys, os

if not len(sys.argv) == 2:
	print "syntax:"
	print "\tfix_newlines.py <path>"
	print "\t"
	print "makes sure all source files have a newline."
	raise SystemExit

for root,folders,files in os.walk(sys.argv[1]):
	if not '.svn' in root:
		for filename in folders + files:
			if not '.svn' in filename:
				base,ext = os.path.splitext(filename)
				fullpath = os.path.join(root, filename)
				if ext in ('.c','.cpp','.h','.hpp'):
					data = file(fullpath,'rb').read()
					if not data.endswith('\n'):
						print 'FIX: %s' % fullpath
						data += '\n'
						file(fullpath,'wb').write(data)
