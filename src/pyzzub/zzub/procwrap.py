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
Provides an ld-like interface for retrieving functions from C libraries.

This is used in conjunction with the ./tools scripts, which generate
the libzzub bindings.
"""

import ctypes

def dlopen(*args,**kwds):
	"""
	Opens a library by name and returns a handle object. See 
	{library.load} for more information.
	"""
	import library
	return library.load(*args,**kwds)

def dlsym(lib, name, restype, *args):
	"""
	Retrieves a symbol from a library loaded by dlopen and
	assigns correct result and argument types.
	
	@param lib: Library object.
	@type lib: ctypes.CDLL
	@param name: Name of symbol.
	@type name: str
	@param restype: Type of function return value.
	@param args: Types of function arguments.	
	"""
	if not lib:
		return None
	proc = getattr(lib,name)
	proc.restype = strip_ctype(restype)
	proc.argtypes = [strip_ctype(argtype) for argname,argtype in args]
	proc.o_restype = restype
	proc.o_args = args
	return proc

def strip_ctype(obj):
	"""
	Stub.
	"""
	return obj


__all__ = [
'dlopen',
'dlsym',
]
