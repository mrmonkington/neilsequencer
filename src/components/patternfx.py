import gtk
import gobject
import os
import pickle
import neil.com as com
from neil.utils import roundint, bn2mn, mn2bn, new_stock_image_button
from neil.utils import gettext, warning
from neil.envelope import SimpleEnvelope
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

class Envelope():
    __neil__ = dict(
        id = 'neil.core.patternfx.envelope',
        categories = [
            'patternfx'
            ]
        )

    name = "Envelope"

    def transform(self, data, parameter):
        dialog = gtk.Dialog(
            "Envelope",
            buttons=(gtk.STOCK_OK, True, gtk.STOCK_CANCEL, False)
            )
        if len(data) < 2:
            warning(dialog, "The selection is too short! Select more rows.")
            return data
        a = parameter.get_value_min()
        b = parameter.get_value_max()
        envelope = SimpleEnvelope()
        envelope.envelope = []
        last_point = (0.0, 0.0)
        for row, value in enumerate(data):
            if value != parameter.get_value_none():
                last_point = (float(row) / (len(data) - 1), 
                              (value - a) / float(b - a))
                envelope.envelope.append(last_point)
            else:
                last_point = (float(row) / (len(data) - 1), 
                              last_point[1])
                envelope.envelope.append(last_point)
        # Prune points that are linear interpolations of the points
        # on both sides of said points.
        points = envelope.envelope
        if len(points) > 2:
            triples = [(i, i + 1, i + 2) for i, point in enumerate(points[:-2])]
            to_delete = []
            for triple in triples:
                p1, p2, p3 = [points[i] for i in triple]
                if abs((p1[1] + p3[1]) * 0.5 - p2[1]) <= (1.0 / (b - a)):
                    to_delete.append(triple[1])
            envelope.envelope = [point for index, point in enumerate(points) 
                                 if index not in to_delete]
        envelope.set_size_request(600, 200)
        dialog.vbox.pack_start(envelope)
        dialog.show_all()
        response = dialog.run()
        dialog.destroy()
        if response:
            env = envelope.envelope
            index = 0
            for row, value in enumerate(data):
                phase = float(row) / (len(data) - 1)
                while env[index + 1][0] < phase:
                    index += 1
                p = ((phase - env[index][0]) / 
                     (env[index + 1][0] - env[index][0]))
                value = env[index][1] + p * (env[index + 1][1] - env[index][1])
                data[row] = int(a + value * (b - a))
        return data

__all__ = [
    'SimpleRandom',
    'RandomWalk',
    'LinearTransform',
    'Envelope',
    ]

__neil__ = dict(
    classes = [
        SimpleRandom,
        RandomWalk,
        LinearTransform,
        Envelope,
        ]
    )
