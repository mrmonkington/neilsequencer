#encoding: latin-1

# Aldrin
# Modular Sequencer
# Copyright (C) 2006,2007,2008 The Aldrin Development Team
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

from gtkimport import gtk
import cairo
from common import MARGIN, MARGIN2, MARGIN3, MARGIN0
import gobject
from utils import filepath
import utils
from aldrincom import com
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
		self.range = 76
		self.amp = 0.0
		self.channel = channel
		self.pixbuf = gtk.gdk.pixbuf_new_from_file(filepath('res/vubar.svg'))
		self.stops = (0.0, 6.0 / self.range, 12.0 / self.range) # red, yellow, green
		self.index = 0
		gtk.DrawingArea.__init__(self)
		self.set_size_request(self.pixbuf.get_width(),self.pixbuf.get_height())
		self.connect("expose_event", self.expose)
		gobject.timeout_add(100, self.on_update)
	
	def on_update(self):
		"""
		Event handler triggered by a 25fps timer event.
		"""
		player = com.get('aldrin.core.player')
		master = player.get_plugin(0)
		vol=master.get_parameter_value(1, 0, 0)
		self.amp = min(master.get_last_peak()[self.channel],1.0)
		rect = self.get_allocation()
		if self.window:
			self.window.invalidate_rect((0,0,rect.width,rect.height), False)
		return True
		
	def draw(self, ctx):
		"""
		Draws the VU bar client region.
		"""
		rect = self.get_allocation()
		w,h = rect.width,rect.height
		if self.amp >= 1.0:
			ctx.set_source_rgb(1,0,0)
			ctx.rectangle(0,0,w,h)
			ctx.fill()
		else:
			y = 0
			ctx.set_source_pixbuf(self.pixbuf, 0,0)
			ctx.rectangle(0, 0, w, h)
			ctx.fill()
			bh = int((h * (utils.linear2db(self.amp,limit=-self.range) + self.range)) / self.range)
			ctx.set_source_rgba(0,0,0,0.6)
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
	
	__aldrin__ = dict(
		id = 'aldrin.core.panel.master',
	)
	
	def __init__(self):
		gtk.VBox.__init__(self)
		self.latency = 0
		self.ohg = utils.ObjectHandlerGroup()
		eventbus = com.get('aldrin.core.eventbus')
		eventbus.zzub_parameter_changed += self.on_zzub_parameter_changed
		self.masterslider = gtk.VScale()
		self.masterslider.set_draw_value(False)
		self.masterslider.set_range(0,16384)
		self.masterslider.set_size_request(-1,200)
		self.masterslider.set_increments(500, 500)
		self.masterslider.set_inverted(True)
		#self.ohg.connect(self.masterslider, 'scroll-event', self.on_mousewheel)
		self.ohg.connect(self.masterslider, 'change-value', self.on_scroll_changed)
		self.ampl = AmpView(self, 0)
		self.ampr = AmpView(self, 1)
		hbox = gtk.HBox()
		hbox.set_border_width(MARGIN)
		hbox.pack_start(self.ampl)
		hbox.pack_start(self.masterslider)
		hbox.pack_start(self.ampr)
		self.pack_start(hbox, expand=False, fill=False)
		self.update_all()

	def on_zzub_parameter_changed(self, plugin,group,track,param,value):
		player = com.get('aldrin.core.player')
		if plugin == player.get_plugin(0):
			self.update_all()
			
	def on_scroll_changed(self, widget, scroll, value):
		"""
		Event handler triggered when the master slider has been dragged.
		
		@param event: event.
		@type event: wx.Event
		"""
		import time
		block = self.ohg.autoblock()
		vol = int(min(max(value,0), 16384) + 0.5)
		player = com.get('aldrin.core.player')
		master = player.get_plugin(0)
		master.set_parameter_value_direct(1, 0, 0, 16384 - vol, 1)
		self.masterslider.set_value(vol)
		return True

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
		player = com.get('aldrin.core.player')
		master = player.get_plugin(0)
		vol = master.get_parameter_value(1, 0, 0)
		self.masterslider.set_value(16384 - vol)
		self.latency = com.get('aldrin.core.driver.audio').get_latency()

__aldrin__ = dict(
	classes = [
		MasterPanel,
	],
)

if __name__ == '__main__':
	import contextlog, aldrincom
	contextlog.init()
	aldrincom.init()
	view = com.get('aldrin.core.panel.master')
	dlg = com.get('aldrin.test.dialog', view)
	# running standalone
	browser = com.get('aldrin.pythonconsole.dialog', False)
	browser.show_all()
	player = com.get('aldrin.core.player')
	gtk.main()

