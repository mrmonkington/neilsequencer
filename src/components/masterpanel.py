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
from neil.common import MARGIN, MARGIN2, MARGIN3, MARGIN0
import gobject
from neil.utils import filepath, imagepath
import neil.utils as utils
import neil.com as com
import driver
import zzub
import sys

class AmpView(gtk.DrawingArea):
    """
    A simple control rendering a Buzz-like master VU bar.
    """
    def __init__(self, parent, channel):
        """
        Initializer.
        """
        import Queue
        self.range = 80
        self.amp = 0.0
        self.channel = channel
        self.stops = (0.0, 6.0 / self.range, 12.0 / self.range) # red, yellow, green
        self.index = 0
        gtk.DrawingArea.__init__(self)
        self.set_size_request(10, -1)
        self.connect("expose_event", self.expose)
        gobject.timeout_add(100, self.on_update)

    def on_update(self):
        """
        Event handler triggered by a 25fps timer event.
        """
        player = com.get('neil.core.player')
        master = player.get_plugin(0)
        vol = master.get_parameter_value(1, 0, 0)
        self.amp = min(master.get_last_peak()[self.channel], 1.0)
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
        if self.amp >= 1.0:
            ctx.set_source_rgb(1, 0, 0)
            ctx.rectangle(0, 0, w, h)
            ctx.fill()
        else:
            y = 0
            ctx.set_source_rgb(0, 1, 0)
            ctx.rectangle(0, 0, w, h)
            ctx.fill()
            bh = int((h * (utils.linear2db(self.amp, limit =- self.range) + self.range)) / self.range)
            ctx.set_source_rgba(0, 0, 0, 0.8)
            ctx.rectangle(0, 0, w, h - bh)
            ctx.fill()

    def expose(self, widget, event):
        self.context = widget.window.cairo_create()
        self.draw(self.context)
        return False

class MasterPanel(gtk.VBox):
    """
    A panel containing the master machine controls.
    """

    __neil__ = dict(
            id = 'neil.core.panel.master',
            singleton = True,
            categories = [
                    'view',
            ],
    )

    __view__ = dict(
                    label = "Master",
                    order = 0,
                    toggle = True,
    )

    def __init__(self):
        gtk.VBox.__init__(self)
        self.latency = 0
        self.ohg = utils.ObjectHandlerGroup()
        eventbus = com.get('neil.core.eventbus')
        eventbus.zzub_parameter_changed += self.on_zzub_parameter_changed
        eventbus.document_loaded += self.update_all
        self.masterslider = gtk.VScale()
        self.masterslider.set_draw_value(False)
        self.masterslider.set_range(0, 16384)
        self.masterslider.set_size_request(-1, 200)
        self.masterslider.set_increments(500, 500)
        self.masterslider.set_inverted(True)
        #self.ohg.connect(self.masterslider, 'scroll-event', self.on_mousewheel)
        self.ohg.connect(self.masterslider, 'change-value',
                         self.on_scroll_changed)
        self.ampl = AmpView(self, 0)
        self.ampr = AmpView(self, 1)
        hbox = gtk.HBox()
        hbox.set_border_width(MARGIN)
        hbox.pack_start(self.ampl)
        hbox.pack_start(self.masterslider)
        hbox.pack_start(self.ampr)
        vbox = gtk.VBox()
        self.volumelabel = gtk.Label()
        vbox.pack_start(hbox)
        vbox.pack_start(self.volumelabel, expand=False, fill=False)
        self.pack_start(vbox)
        self.update_all()

    def on_zzub_parameter_changed(self, plugin,group,track,param,value):
        player = com.get('neil.core.player')
        if plugin == player.get_plugin(0):
            self.update_all()

    def on_scroll_changed(self, widget, scroll, value):
        """
        Event handler triggered when the master slider has been dragged.

        @param event: event.
        @type event: wx.Event
        """
        import time
        vol = int(min(max(value,0), 16384) + 0.5)
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
        vol = min(max(0,vol), 16384)
        self.on_scroll_changed(None, None, vol)

    def update_all(self):
        """
        Updates all controls.
        """
        block = self.ohg.autoblock()
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
            text = "%.1f dB" % db
        self.volumelabel.set_markup("<small>%s</small>" % text)
        self.latency = com.get('neil.core.driver.audio').get_latency()

__neil__ = dict(
        classes = [
                MasterPanel,
        ],
)

