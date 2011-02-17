#!/usr/bin/python

"""
removes all kind of build clutter from subdirectories.
"""

import os, sys

print "enumerating files registered by svn..."
svnfiles = map(lambda x: './'+x, os.popen('svn ls -R').read().split('\n'))

print "enumerating all files..."
files = os.popen('find').read().split('\n')

goodfiles = [
	'./options.conf',
	'.',
	'',
]
badfiles = []

for file in files:
	if os.path.isdir(file):
		file += '/'
	if not '.svn' in file:
		if not file in svnfiles and not file in goodfiles:
			badfiles.append(file)

if not 'sure' in sys.argv:
	print "i would delete these files:"
	for file in badfiles:
		print file
	print
	print "run cleanup.py again with 'sure' as argument to delete those files."
else:
	for file in badfiles:
		print "removing %s..." % file
		os.remove(file)
