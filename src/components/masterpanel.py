#encoding: latin-1

# Neil
# Modular Sequencer
# Copyright (C) 2006,2007,2008 The Neil Development Team
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

if __name__ == '__main__':
    import os
    os.system('../../bin/neil-combrowser neil.core.panel.master')
    raise SystemExit

import gtk
import cairo
from neil.common import MARGIN
import gobject
import neil.utils as utils
import neil.com as com
import array
import numpy as np


pattern = cairo.SurfacePattern(cairo.ImageSurface.create_for_data(
          array.array('B', [0, 0, 0, 255] * 2 + [0, 0, 0, 0] * 4),
          cairo.FORMAT_ARGB32, 1, 6, 4))
pattern.set_extend(cairo.EXTEND_REPEAT)


class AmpView(gtk.DrawingArea):
    """
    A simple control rendering a Buzz-like master VU bar.
    """

    __gsignals__ = {
       'clip': (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_FLOAT,))
    }

    def __init__(self, parent, channel):
        """
        Initializer.
        """
        gtk.DrawingArea.__init__(self)
        self.linear = None
        self.range = 80
        self.amp = 0.0
        self.channel = channel
        self.stops = (0.0, 6.0 / self.range, 48.0 / self.range)  # red, yellow, green
        self.peaks = np.zeros(300)
        self.hold = 15
        self.set_size_request(20, -1)
        self.connect("expose_event", self.expose)
        self.connect("configure_event", self.configure)
        gobject.timeout_add(33, self.on_update)

    def on_update(self):
        """
        Event handler triggered by a 30fps timer event.
        """
        player = com.get('neil.core.player')
        master = player.get_plugin(0)
        # vol = master.get_parameter_value(1, 0, 0)
        self.amp = min(master.get_last_peak()[self.channel], 1.0)

        self.peaks = np.roll(self.peaks, self.hold)
        std = self.peaks.std()
        self.peaks[:self.hold] = [self.amp - std] * self.hold

        rect = self.get_allocation()
        if self.window:
            self.window.invalidate_rect((0, 0, rect.width, rect.height), False)
        return True

    def draw(self, ctx):
        """
        Draws the VU bar client region.
        """
        rect = self.get_allocation()
        w, h = rect.width, rect.height
        # y = 0
        ctx.set_source(self.linear)
        ctx.rectangle(0, 0, w, h)
        ctx.fill_preserve()
        ctx.set_source_rgba(0, 0, 0, .9)
        ctx.fill()

        ctx.set_source(self.linear)

        # db
        bh = int((h * (utils.linear2db(self.amp, limit= -self.range) +
                       self.range)) / self.range)
        ctx.rectangle(1 + (0 if self.channel else w / 2), h - bh - 1, w / 2 - 2, bh)
        ctx.fill()

        # linear
        bhl = int(h * self.amp)
        ctx.set_source(self.linear)
        ctx.rectangle(1 + (w / 2 if self.channel else 0), h - bhl - 1, w / 2 - 2, bhl)
        ctx.fill()

        # border
        ctx.set_source_rgb(0, 0, 0)
        ctx.rectangle(0, 0, w, h)
        ctx.stroke()

        ctx.mask(pattern)

        rms = lambda x: 0 if len(x) == 0 else ((sum(x) / len(x)) ** 2) ** 0.5
        db = lambda p: utils.linear2db(p, limit= -self.range) + self.range
        # db rms
        dbp = [x for x in [db(p) for p in self.peaks] if x > 0]
        ph = max(bh, int(h * rms(dbp) / self.range))
        ctx.set_source_rgb(1, 1, 1)
        ctx.move_to(1 + (0 if self.channel else w / 2), h - ph)
        ctx.line_to(w / 2 - 1 + (0 if self.channel else w / 2), h - ph)
        ctx.stroke()

        # linear rms
        phl = max(bhl, int(h * rms(self.peaks)))
        ctx.move_to(1 + (w / 2 if self.channel else 0), h - phl)
        ctx.line_to(w / 2 - 1 + (w / 2 if self.channel else 0), h - phl)
        ctx.stroke()

        # db peak
        p = int(h * (utils.linear2db(max(self.peaks), limit= -self.range) + self.range) / self.range)
        ctx.set_source(self.linear)
        ctx.move_to(1 + (0 if self.channel else w / 2), h - p)
        ctx.line_to(w / 2 - 1 + (0 if self.channel else w / 2), h - p)
        ctx.stroke()

        # linear peak
        p = int(h * max(self.peaks))
        ctx.move_to(1 + (w / 2 if self.channel else 0), h - p)
        ctx.line_to(w / 2 - 1 + (w / 2 if self.channel else 0), h - p)
        ctx.stroke()

        if self.amp >= 1.0:
            ctx.set_source_rgb(1, 0, 0)
            ctx.rectangle(1, 1, w - 2, h - 2)
            ctx.stroke_preserve()
            ctx.set_source_rgba(1, 0, 0, .5)
            ctx.fill()
            self.emit('clip', self.amp)

    def configure(self, widget, event):
        self.linear = cairo.LinearGradient(0, 0, 0, self.get_allocation().height)
        self.linear.add_color_stop_rgb(self.stops[0], 1, 0, 0)
        self.linear.add_color_stop_rgb(self.stops[1], 1, 1, 0)
        self.linear.add_color_stop_rgb(self.stops[2], 0, 1, 0)
        self.linear.add_color_stop_rgb(1, 0, .3, 0)

    def expose(self, widget, event):
        context = widget.window.cairo_create()
        self.draw(context)
        return False


class MasterPanel(gtk.VBox):
    """
    A panel containing the master machine controls.
    """

    __neil__ = dict(
            id='neil.core.panel.master',
            singleton=True,
    )

    def __init__(self):
        gtk.VBox.__init__(self)
        self.latency = 0
        self.ohg = utils.ObjectHandlerGroup()
        eventbus = com.get('neil.core.eventbus')
        eventbus.zzub_parameter_changed += self.on_zzub_parameter_changed
        eventbus.document_loaded += self.update_all
        #eventbus.zzub_player_state_changed += self.on_zzub_player_state_changed
        self.masterslider = gtk.VScale()
        self.masterslider.set_draw_value(False)
        self.masterslider.set_range(0, 16384)
        self.masterslider.set_size_request(-1, 333)
        self.masterslider.set_increments(500, 500)
        self.masterslider.set_inverted(True)
        #self.ohg.connect(self.masterslider, 'scroll-event', self.on_mousewheel)
        self.ohg.connect(self.masterslider, 'change-value', self.on_scroll_changed)
        self.ampl = AmpView(self, 0)
        self.ampr = AmpView(self, 1)

        self.ampl.connect('clip', self.on_clipped)
        self.ampr.connect('clip', self.on_clipped)

        hbox = gtk.HBox()
        hbox.set_border_width(MARGIN)
        hbox.pack_start(self.ampl)
        hbox.pack_start(self.masterslider)
        hbox.pack_start(self.ampr)
        vbox = gtk.VBox()

        self.clipbtn = gtk.Button()
        self.clipbtn.connect('clicked', self.on_clip_button_clicked)
        self.clipbtn.set_property("can-focus", gtk.FALSE)

        self.volumelabel = gtk.Label()

        vbox.pack_start(self.clipbtn, expand=False, fill=False)
        vbox.pack_start(hbox, expand=True, fill=True)
        vbox.pack_start(self.volumelabel, expand=False, fill=False)

        self.pack_start(vbox)

        self.update_all()

        # Send keystrokes back to main window
        # Lambda needed because root window isn't initialized here
        self.connect('key-press-event', lambda w, e: com.get('neil.core.window.root').on_key_down(w, e))
        self.connect('realize', self.on_realize)

    def on_realize(self, widget):
        self.clipbtn_org_color = self.clipbtn.get_style().bg[gtk.STATE_NORMAL]

    def on_zzub_parameter_changed(self, plugin, group, track, param, value):
        player = com.get('neil.core.player')
        if plugin == player.get_plugin(0):
            self.update_all()

    def on_scroll_changed(self, widget, scroll, value):
        """
        Event handler triggered when the master slider has been dragged.

        @param event: event.
        @type event: wx.Event
        """
        vol = int(min(max(value, 0), 16384) + 0.5)
        player = com.get('neil.core.player')
        master = player.get_plugin(0)
        master.set_parameter_value_direct(1, 0, 0, 16384 - vol, 1)
        #self.masterslider.set_value(vol)

    def on_mousewheel(self, widget, event):
        """
        Sent when the mousewheel is used on the master slider.

        @param event: A mouse event.
        @type event: wx.MouseEvent
        """
        vol = self.masterslider.get_value()
        step = 16384 / 48
        if event.direction == gtk.gdk.SCROLL_UP:
            vol += step
        elif event.direction == gtk.gdk.SCROLL_DOWN:
            vol -= step
        vol = min(max(0, vol), 16384)
        self.on_scroll_changed(None, None, vol)

    def on_clip_button_clicked(self, widget):
        player = com.get('neil.core.player')
        master = player.get_plugin(0)
        maxL, maxR = master.get_last_peak()
        self.clipbtn.set_label("%.1f dbFS" % utils.linear2db(min(maxL, maxR, 1.)))
        self.clipbtn.modify_bg(gtk.STATE_NORMAL, self.clipbtn_org_color)

    def on_clipped(self, widget, level):
        # db = utils.linear2db(level, widget.range)
        self.clipbtn.set_label('CLIP')
        self.clipbtn.modify_bg(gtk.STATE_NORMAL, gtk.gdk.Color("#f00"))

    def update_all(self):
        """
        Updates all controls.
        """
        # block = self.ohg.autoblock()
        player = com.get('neil.core.player')
        master = player.get_plugin(0)
        vol = master.get_parameter_value(1, 0, 0)
        self.masterslider.set_value(16384 - vol)
        if vol == 16384:
            text = "(muted)"
        else:
            db = (-vol * self.ampl.range / 16384.0)
            if db == 0.0:
                db = 0.0
            text = "%.1f dBFS" % db
        self.volumelabel.set_markup("<small>%s</small>" % text)
        self.latency = com.get('neil.core.driver.audio').get_latency()

__neil__ = dict(
        classes=[
                MasterPanel,
        ],
)
