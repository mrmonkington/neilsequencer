
from utils import error
from traceback import format_exception
import sys

Parent = None

def local_excepthook(type, value, traceback):
	sys.__excepthook__(type, value, traceback)
	exc = ''.join(format_exception(type, value, traceback))
	error(Parent, "<b>An exception (<i>%s</i>) occurred.</b>" % type.__name__, str(value), exc)

def install(parent=None):
	global Parent
	Parent = parent
	sys.excepthook = local_excepthook

if __name__ == '__main__':
	install()
	raise 5
