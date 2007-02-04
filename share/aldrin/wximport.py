
def try_version(ver):
	try:
		import wxversion
		wxversion.select(ver)
		print "using wxPython " + ver
		return True
	except:
		return False

try_version('2.6') or try_version('2.8') or try_version('2.7')
import wx
