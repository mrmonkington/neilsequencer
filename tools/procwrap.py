
import ctypes

def dlopen(*args,**kwds):
	"""
	Opens a library by name and returns a handle object. See 
	{library.load} for more information.
	"""
	import library
	return library.load(*args,**kwds)

def dlsym(lib, name, restype, *args):
	if not lib:
		return None
	proc = getattr(lib,name)
	proc.restype = strip_ctype(restype)
	proc.argtypes = [strip_ctype(argtype) for argname,argtype in args]
	proc.o_restype = restype
	proc.o_args = args
	return proc
def strip_ctype(obj):
	return obj
