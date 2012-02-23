
import gtk
import gobject
from traceback import format_exception, print_exc as traceback_print_exc
import sys

Parent = None


def error(parent, msg, msg2=None, details=None, offer_quit=False):
    """
    Shows an error message dialog.
    """
    dialog = gtk.MessageDialog(parent and parent.get_toplevel(),
        gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
        gtk.MESSAGE_ERROR, gtk.BUTTONS_NONE)
    dialog.set_markup(msg)
    dialog.set_resizable(True)
    if msg2:
        dialog.format_secondary_text(msg2)
    if details:
        expander = gtk.Expander("Details")
        dialog.vbox.pack_start(expander, expand=False, fill=True)
        label = gtk.TextView()
        label.set_editable(False)
        label.get_buffer().set_property('text', details)
        label.set_wrap_mode(gtk.WRAP_NONE)

        sw = gtk.ScrolledWindow()
        sw.set_shadow_type(gtk.SHADOW_ETCHED_IN)
        sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        sw.set_size_request(-1, 200)

        sw.add(label)
        expander.add(sw)
        dialog.show_all()
    dialog.add_button(gtk.STOCK_OK, gtk.RESPONSE_OK)
    if offer_quit:
        dialog.add_button(gtk.STOCK_QUIT, gtk.RESPONSE_REJECT)

        def delayed_quit():
            try:
                gtk.main_quit()
            except RuntimeError:
                raise SystemExit

        def quit_on_cancel(dlg, response_id):
            if response_id == gtk.RESPONSE_REJECT:
                gobject.timeout_add(1, delayed_quit)
        dialog.connect('response', quit_on_cancel)
    response = dialog.run()
    dialog.destroy()
    return response

last_exc = None


def show_exc_dialog(exc_type, value, traceback):
    global last_exc
    exc = ''.join(format_exception(exc_type, value, traceback))
    if exc == last_exc:
        return
    last_exc = exc
    error(Parent, "<b>An exception (<i>%s</i>) occurred.</b>" % exc_type.__name__, str(value), exc, offer_quit=True)


def print_exc():
    traceback_print_exc()
    show_exc_dialog(*sys.exc_info())


def local_excepthook(exc_type, value, traceback):
    sys.__excepthook__(exc_type, value, traceback)
    show_exc_dialog(exc_type, value, traceback)


def install(parent=None):
    global Parent
    Parent = parent
    sys.excepthook = local_excepthook

if __name__ == '__main__':
    install()
