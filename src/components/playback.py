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

"""
Provides dialog class for cpu monitor.
"""

import gtk
import gobject
from neil.utils import prepstr, add_scrollbars
import neil.utils as utils, os, stat
import neil.common as common
from neil.common import MARGIN, MARGIN2, MARGIN3
import neil.com as com
import time
from neil.utils import new_stock_image_toggle_button, new_stock_image_button
from neil.utils import format_time, ticks_to_time, new_theme_image_toggle_button
from neil.utils import new_image_button, new_image_toggle_button, imagepath

class PlaybackInfo(gtk.Dialog):
    """
    This dialog show playback information.
    """
    
    __neil__ = dict(
        id = 'neil.core.playback',
        singleton = True,
        categories = [
            'viewdialog',
            'view',
            ]
	)
    
    __view__ = dict(
        label = "Playback Info",
        order = 0,
        toggle = True,
	)
    
    def __init__(self):
        """
        Initializer.
        """
        gtk.Dialog.__init__(self)
        self.set_title("Playback")
        sg1 = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
        sg2 = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
        def add_row(name):
            import pango
            c1 = gtk.Label()
            c1.modify_font(pango.FontDescription("Monospace 10"))
            c1.set_markup("<b>%s</b>" % name)
            c1.set_alignment(1, 0.5)
            c2 = gtk.Label()
            c2.modify_font(pango.FontDescription("Monospace 10"))
            c2.set_alignment(1, 0.5)
            hbox = gtk.HBox(False, MARGIN)
            hbox.pack_start(c1, expand=False)
            hbox.pack_start(c2, expand=False)
            sg1.add_widget(c1)
            sg2.add_widget(c2)
            self.vbox.add(hbox)
            return c2
        self.elapsed = add_row("Elapsed")
        self.current = add_row("Current")
        self.loop = add_row("Loop")
        self.starttime = time.time()
        self.update_label()
        self.set_deletable(False)
        self.set_resizable(False)
        gobject.timeout_add(100, self.on_timer)

    def on_timer(self):
        """
        Called by timer event. Updates CPU usage statistics.
        """
        self.update_label()
        return True

    def update_label(self):
        """
        Event handler triggered by a 10fps timer event.
        """
        player = com.get('neil.core.player')
        p = player.get_position()
        m = player.get_plugin(0)
        bpm = m.get_parameter_value(1, 0, 1)
        tpb = m.get_parameter_value(1, 0, 2)
        time.time() - self.starttime
        if player.get_state() == 0: # playing
            e = format_time(time.time() - player.playstarttime)
        else:
            e = format_time(0.0)
        c = format_time(ticks_to_time(p,bpm,tpb))
        lb,le = player.get_loop()
        l = format_time(ticks_to_time(le-lb,bpm,tpb))
        for text,control in [(e,self.elapsed),(c,self.current),(l,self.loop)]:
            #~ if text != control.get_text():
            control.set_markup("%s" % text)
        return True

__all__ = [
    'PlaybackInfo',
    ]

__neil__ = dict(
    classes = [
        PlaybackInfo,
	],
    )

