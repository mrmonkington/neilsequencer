import gtk
import gobject
import os
import pickle
import neil.com as com
from neil.utils import roundint, bn2mn, mn2bn, new_stock_image_button
from neil.utils import gettext, error
from random import *
from math import *
from neil.gtkcodebuffer import CodeBuffer, SyntaxLoader, add_syntax_path

class Expression():
    __neil__ = dict(
        id = 'neil.core.expression',
        categories = [
            ]
        )

    EXPRESSIONS_FILE = os.path.expanduser('~/.neil/expressions.txt')
    name = "Expression"
    expressions = {}

    def read_expressions(self):
        fd = open(self.EXPRESSIONS_FILE, 'r')
        self.expressions = pickle.load(fd)
        fd.close()

    def write_expressions(self):
        fd = open(self.EXPRESSIONS_FILE, 'w')
        pickle.dump(self.expressions, fd)
        fd.close()

    def add_expression(self, widget):
        model = self.selector.get_model()
        active = self.selector.get_active_iter()
        if active != None:
            old_name = model.get_value(active, 0)
        else:
            old_name = ''
        name = gettext(self.dialog, "Enter the name of your expression", 
                       old_name)
        if name != None:
            name = name.replace(',', ' ')
            self.expressions[name] = self.text.get_buffer().get_property('text')
            if name != old_name:
                model = self.selector.get_model()
                self.selector.set_active_iter(model.append([name]))
            self.write_expressions()

    def del_expression(self, widget):
        model = self.selector.get_model()
        active = self.selector.get_active_iter()
        if active == None:
            return
        name = model.get_value(active, 0)
        model.remove(active)
        del self.expressions[name]
        if self.selector.get_model().get_iter_first():
            self.selector.set_active(0)
        else:
            self.text.get_buffer().set_text('')
        self.write_expressions()

    def mov_expression(self, widget):
        model = self.selector.get_model()
        active = self.selector.get_active_iter()
        if active != None:
            old_name = model.get_value(active, 0)
        else:
            return
        new_name = gettext(self.dialog, "Enter new name for your expression",
                           old_name)
        if new_name != None:
            new_name = new_name.replace(',', ' ')
            name = model.get_value(active, 0)
            self.expressions[new_name] = str(self.expressions[name])
            del self.expressions[name]
            model.remove(active)
            self.selector.set_active_iter(model.append([new_name]))
            self.write_expressions()

    def active_expression_changed(self, combobox):
        model = combobox.get_model()
        active = combobox.get_active_iter()
        if active != None:
            name = model.get_value(active, 0)
            self.text.get_buffer().set_text(self.expressions[name])

    def transform(self, plugin, pattern, selection):
        try:
            self.read_expressions()
        except IOError:
            self.expressions = {}
        self.dialog = gtk.Dialog(
                "Expression",
                buttons=(gtk.STOCK_OK, True, gtk.STOCK_CANCEL, False)
                )
        hbox = gtk.HBox()
        self.selector = gtk.ComboBox(gtk.ListStore(str))
        cell = gtk.CellRendererText()
        self.selector.pack_start(cell, True)
        self.selector.add_attribute(cell, 'text', 0)
        self.selector.connect('changed', self.active_expression_changed)
        # Fill in the combobox expression selector with entries
        model = self.selector.get_model()
        for name, expression in self.expressions.items():
            model.append([name])
        add_button = new_stock_image_button(gtk.STOCK_OPEN, "Add Expression")
        del_button = new_stock_image_button(gtk.STOCK_REMOVE, "Remove Expression")
        mov_button = new_stock_image_button(gtk.STOCK_BOLD, "Rename Expression")
        hlp_button = new_stock_image_button(gtk.STOCK_HELP, "Help")
        add_button.connect('clicked', self.add_expression)
        del_button.connect('clicked', self.del_expression)
        mov_button.connect('clicked', self.mov_expression)
        add_button.set_size_request(30, 30)
        del_button.set_size_request(30, 30)
        mov_button.set_size_request(30, 30)
        hlp_button.set_size_request(30, 30)
        hbox.pack_start(self.selector)
        hbox.pack_start(add_button, expand=False)
        hbox.pack_start(del_button, expand=False)
        hbox.pack_start(mov_button, expand=False)
        hbox.pack_start(hlp_button, expand=False)
        scrolled_window = gtk.ScrolledWindow()
        lang = SyntaxLoader("python")
        buff = CodeBuffer(lang=lang)
        self.text = gtk.TextView(buff)
        scrolled_window.add(self.text)
        scrolled_window.set_size_request(500, 300)
        self.dialog.vbox.pack_start(hbox, expand=False)
        self.dialog.vbox.pack_start(scrolled_window)
        if self.selector.get_model().get_iter_first():
            self.selector.set_active(0)
        self.dialog.show_all()
        response = self.dialog.run()
        expr = self.text.get_buffer().get_property('text')
        self.dialog.destroy()
        if response:
            model = self.selector.get_model()
            active = self.selector.get_active_iter()
            if active != None:
                old_name = model.get_value(active, 0)
                self.expressions[old_name] = expr
                self.write_expressions()
            else:
                return
            try:
                def get_value(group, track, index, row):
                    return plugin.get_pattern_value(pattern, group, 
                                                    track, index, row)
                def set_value(group, track, index, row, value):
                    plugin.set_pattern_value(pattern, group, track, 
                                             index, row, value)
                def get_param(group, track, index):
                    return plugin.get_parameter(group, track, index)
                global_ = globals()
                new_global = {
                    '__builtins__' : global_['__builtins__'], 
                    '__name__' : global_['__name__'], 
                    '__doc__' : global_['__doc__'], 
                    '__package__' : global_['__package__'],
                    'get_value' : get_value,
                    'set_value' : set_value,
                    'get_param' : get_param,
                    'n' : plugin.get_pattern_length(pattern),
                    }
                exec expr in new_global
            except Exception as e:
                error(self.dialog, "There was a problem with your expression!", details=str(e))


__all__ = [
    'Expression',
    ]

__neil__ = dict(
    classes = [
        Expression,
        ]
    )
