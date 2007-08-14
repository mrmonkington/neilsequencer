#encoding: latin-1

# pyzzub
# Python bindings for libzzub
# Copyright (C) 2006 The libzzub Development Team
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

"""
library provides a dynamic library loader that works location
independent across all ctypes versions.
"""

import sys, os

def load(*names,**kw):
	"""
	searches for a library with given names and returns a ctypes 
	.so/.dll library object if successful. if the library can not
	be loaded, an assertion error will be thrown.
	
	@type  names: list of strings
	@param names: one or more aliases for required libraries, e.g.
				  'SDL','SDL-1.2'.
	@rtype: ctypes CDLL handle
	"""
	import ctypes, os, sys
	searchpaths = []
	if os.name in ('posix', 'mac'):
		if os.environ.has_key('LD_LIBRARY_PATH'):
			searchpaths += os.environ['LD_LIBRARY_PATH'].split(os.pathsep)
		searchpaths += [
			'/usr/local/lib64',
			'/usr/local/lib',
			'/usr/lib64',
			'/usr/lib',
		]
	elif os.name == 'nt':
		searchpaths += ['.']
		if 'PATH' in os.environ:
			searchpaths += os.environ['PATH'].split(os.pathsep)
	else:
		assert 0, "Unknown OS: %s" % os.name
	if 'paths' in kw:
		searchpaths += kw['paths']
	for name in names:
		if os.name == 'nt':
			libname = name + '.dll'
		elif sys.platform == 'darwin':
			libname = 'lib' + name + '.dylib'
			if 'version' in kw:
				libname += '.' + kw['version']			
		else:
			libname = 'lib' + name + '.so'
			if 'version' in kw:
				libname += '.' + kw['version']
		m = None
		for path in searchpaths:
			if os.path.isdir(path):
				libpath = os.path.join(path,libname)
				if os.path.isfile(libpath):
					m = ctypes.CDLL(libpath)
					break
				for filename in reversed(sorted(os.listdir(path))):
					if filename.startswith(libname):						
						m = ctypes.CDLL(os.path.join(path,filename))
						break
				if m:
					break
		if m:
			break
	assert m, "libraries %s not found in %s" % (','.join(["'%s'" % a for a in names]),','.join(searchpaths))
	return m
	
__all__ = [
	'load',
]

if __name__ == '__main__':
	print load('SDL', 'SDL-1.2')
	print load('zzub',version='0.1')
	print load('zzub',version='0.2')
