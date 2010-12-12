import gtk
import random
import neil.com as com
from neil.utils import roundint, bn2mn, mn2bn

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
            value = random.randint(a, b)
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
                if random.randint(0, 1) == 0:
                    value += random.randint(min_step, max_step)
                else:
                    value -= random.randint(min_step, max_step)
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

    name = "Expression"

    def transform(self, data, parameter):
        dialog = gtk.Dialog(
            "Expression",
            buttons=(gtk.STOCK_OK, True, gtk.STOCK_CANCEL, False)
            )
        text = gtk.TextView()
        dialog.vbox.add(text)
        dialog.show_all()
        response = dialog.run()
        expr = text.get_buffer().get_property('text')
        dialog.destroy()
        if response:
            try:
                for i in range(len(data)):
                    from random import *
                    from math import *
                    x = data[i]
                    a = parameter.get_value_min()
                    b = parameter.get_value_max()
                    z = parameter.get_value_none()
                    p = i / float(n)
                    n = len(data)
                    exec expr
                    data[i] = int(y)
            except Exception:
                error_box = gtk.Dialog(
                    "Expression Error",
                    buttons=(gtk.STOCK_OK, True)
                    )
                error_box.vbox.add(gtk.Label("There was some problem with your expression"))
                error_box.show_all()
                error_box.run()
                error_box.detroy()
        return data


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
