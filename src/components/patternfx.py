import gtk
import gobject
import os
import pickle
import neil.com as com
from neil.utils import roundint, bn2mn, mn2bn, new_stock_image_button
from neil.utils import gettext
from random import *
from math import *
from neil.gtkcodebuffer import CodeBuffer, SyntaxLoader, add_syntax_path

class SimpleRandom():
    __neil__ = dict(
        id = 'neil.core.patternfx.simplerandom',
        categories = [
            'patternfx'
            ]
        )

    name = "Simple Random"

    def transform(self, data, parameter):
        for row in range(len(data)):
            a = parameter.get_value_min()
            b = parameter.get_value_max()
            value = randint(a, b)
            data[row] = value
        return data

class RandomWalk():
    __neil__ = dict(
        id = 'neil.core.patternfx.randomwalk',
        categories = [
            'patternfx'
            ]
        )

    name = "Random Walk"

    def transform(self, data, parameter):
        dialog = gtk.Dialog(
            "Random Walk",
            buttons=(gtk.STOCK_OK, True, gtk.STOCK_CANCEL, False)
            )
        grid = gtk.Table(3, 2, True)
        grid.attach(gtk.Label("Start:"), 0, 1, 0, 1)
        grid.attach(gtk.Label("Min Step:"), 0, 1, 1, 2)
        grid.attach(gtk.Label("Max Step:"), 0, 1, 2, 3)
        start_box = gtk.SpinButton(gtk.Adjustment(
                parameter.get_value_min(),
                parameter.get_value_min(),
                parameter.get_value_max(),
                1))
        min_box = gtk.SpinButton(gtk.Adjustment(
                1,
                1,
                parameter.get_value_max(),
                1))
        max_box = gtk.SpinButton(gtk.Adjustment(
                1,
                1,
                parameter.get_value_max(),
                1))
        grid.attach(start_box, 1, 2, 0, 1)
        grid.attach(min_box, 1, 2, 1, 2)
        grid.attach(max_box, 1, 2, 2, 3)
        dialog.vbox.add(grid)
        dialog.show_all()
        response = dialog.run()
        start = start_box.get_value()
        min_step = min_box.get_value()
        max_step = max_box.get_value()
        dialog.destroy()
        value = start
        if response:
            for row in range(len(data)):
                data[row] = int(value)
                if randint(0, 1) == 0:
                    value += randint(min_step, max_step)
                else:
                    value -= randint(min_step, max_step)
                while (value > parameter.get_value_max() or
                       value < parameter.get_value_min()):
                    if value > parameter.get_value_max():
                        value = 2 * parameter.get_value_max() - value
                    if value < parameter.get_value_min():
                        value = 2 * parameter.get_value_min() - value
        return data

class LinearTransform():
    __neil__ = dict(
        id = 'neil.core.patternfx.linear',
        categories = [
            'patternfx'
            ]
        )

    name = "Linear Transform"

    def transform(self, data, parameter):
        dialog = gtk.Dialog(
            "Linear Transform",
            buttons=(gtk.STOCK_OK, True, gtk.STOCK_CANCEL, False)
            )
        grid = gtk.Table(2, 2, True)
        grid.attach(gtk.Label("Add"), 0, 1, 0, 1)
        grid.attach(gtk.Label("Mul:"), 0, 1, 1, 2)
        add = gtk.Entry()
        mul = gtk.Entry()
        add.set_size_request(10, -1)
        mul.set_size_request(10, -1)
        add.set_text("0")
        mul.set_text("1.0")
        grid.attach(add, 1, 2, 0, 1)
        grid.attach(mul, 1, 2, 1, 2)
        dialog.vbox.add(grid)
        dialog.show_all()
        response = dialog.run()
        try:
            add = int(add.get_text())
            mul = float(mul.get_text())
        except(ValueError):
            return data
        dialog.destroy()
        if response:
            for row in range(len(data)):
                if data[row] != parameter.get_value_none():
                    data[row] = int(data[row] * mul + add)
                    data[row] = min(data[row], parameter.get_value_max())
                    data[row] = max(data[row], parameter.get_value_min())
        return data

class Expression():
    __neil__ = dict(
        id = 'neil.core.patternfx.expression',
        categories = [
            'patternfx'
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
                model.append([name])
            self.write_expressions()

    def del_expression(self, widget):
        model = self.selector.get_model()
        active = self.selector.get_active_iter()
        name = model.get_value(active, 0)
        model.remove(active)
        del self.expressions[name]
        self.write_expressions()

    def mov_expression(self, widget):
        new_name = gettext(self.dialog, "Enter new name for your expression")
        if new_name != None:
            new_name = new_name.replace(',', ' ')
            model = self.selector.get_model()
            active = self.selector.get_active_iter()
            name = model.get_value(active, 0)
            self.expressions[new_name] = str(self.expressions[name])
            del self.expressions[name]
            model.remove(active)
            self.write_expressions()

    def active_expression_changed(self, combobox):
        model = combobox.get_model()
        active = combobox.get_active_iter()
        if active != None:
            name = model.get_value(active, 0)
            self.text.get_buffer().set_text(self.expressions[name])

    def transform(self, data, parameter):
        try:
            self.expressions = self.read_expressions()
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
        scrolled_window.add_with_viewport(self.text)
        scrolled_window.set_size_request(300, 300)
        self.dialog.vbox.add(hbox)
        self.dialog.vbox.add(scrolled_window)
        self.dialog.show_all()
        response = self.dialog.run()
        expr = self.text.get_buffer().get_property('text')
        self.dialog.destroy()
        if response:
            try:
                a = parameter.get_value_min()
                b = parameter.get_value_max()
                z = parameter.get_value_none()
                n = len(data)
                exec expr
            except Exception:
                error_box = gtk.Dialog(
                    "Expression Error",
                    buttons=(gtk.STOCK_OK, True)
                    )
                error_box.vbox.add(gtk.Label("There was some problem with your expression"))
                error_box.show_all()
                error_box.run()
                error_box.destroy()
        return [int(value) for value in data]


__all__ = [
    'SimpleRandom',
    'RandomWalk',
    'LinearTransform',
    'Expression',
    ]

__neil__ = dict(
    classes = [
        SimpleRandom,
        RandomWalk,
        LinearTransform,
        Expression,
        ]
    )
