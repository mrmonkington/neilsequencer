
from gtkimport import gtk
import gobject
from traceback import format_exception
import sys

Parent = None

def error(parent, msg, msg2=None, details=None):
	"""
	Shows an error message dialog.
	"""
	dialog = gtk.MessageDialog(parent and parent.get_toplevel(),
		gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
		gtk.MESSAGE_ERROR , gtk.BUTTONS_NONE)
	dialog.set_markup(msg)
	dialog.set_resizable(True)
	if msg2:
		dialog.format_secondary_text(msg2)
	if details:
		expander = gtk.Expander("Details")
		dialog.vbox.pack_start(expander, False, False, 0)
		label = gtk.TextView()
		label.set_editable(False)
		label.get_buffer().set_property('text', details)
		label.set_wrap_mode(gtk.WRAP_NONE)
		
		sw = gtk.ScrolledWindow()
		sw.set_shadow_type(gtk.SHADOW_ETCHED_IN)
		sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		
		sw.add(label)		
		expander.add(sw)
		dialog.show_all()
	dialog.add_buttons(gtk.STOCK_OK, gtk.RESPONSE_OK)
	response = dialog.run()
	dialog.destroy()
	return response

last_exc = None
def local_excepthook(type, value, traceback):
	global last_exc
	sys.__excepthook__(type, value, traceback)
	exc = ''.join(format_exception(type, value, traceback))
	if exc == last_exc:
		return
	last_exc = exc
	error(Parent, "<b>An exception (<i>%s</i>) occurred.</b>" % type.__name__, str(value), exc)

def install(parent=None):
	global Parent
	Parent = parent
	sys.excepthook = local_excepthook

if __name__ == '__main__':
	install()
