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
import utils
from aldrincom import com
import driver
import zzub

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
		self.stops = (0.0, 6.0 / self.range, 12.0 / self.range) # red, yellow, green
		self.index = 0
		gtk.DrawingArea.__init__(self)
		self.set_size_request(MARGIN,100)
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
			p = cairo.LinearGradient(0.0, 0.0, 0, h)
			p.add_color_stop_rgb(self.stops[0],1,0,0)
			p.add_color_stop_rgb(self.stops[1],1,1,0)
			p.add_color_stop_rgb(self.stops[2],0,1,0)
			ctx.set_source(p)
			ctx.rectangle(0, 0, w, h)
			ctx.fill()
			bh = int((h * (utils.linear2db(self.amp,limit=-self.range) + self.range)) / self.range)
			ctx.set_source_rgb(0,0,0)
			ctx.rectangle(0, 0, w, h - bh)
			ctx.fill()

	def expose(self, widget, event):
		self.context = widget.window.cairo_create()
		self.draw(self.context)
		return False

class MasterPanel(gtk.HBox):
	"""
	A panel containing the master machine controls.
	"""
	
	__aldrin__ = dict(
		id = 'aldrin.core.panel.master',
	)
	
	def __init__(self, rootwindow):
		gtk.HBox.__init__(self)
		self.set_border_width(MARGIN)
		self.latency = 0
		self.rootwindow = rootwindow
		self.rootwindow.event_handlers.append(self.on_player_callback)
		self.masterslider = gtk.VScale()
		self.masterslider.set_draw_value(False)
		self.masterslider.set_range(0,16384)
		self.masterslider.set_size_request(-1,200)
		self.masterslider.set_increments(500, 500)
		self.masterslider.set_inverted(True)
		self.masterslider.connect('scroll-event', self.on_mousewheel)
		self.masterslider.connect('change-value', self.on_scroll_changed)
		self.masterslider.connect('button-release-event', self.button_up)
		self.ampl = AmpView(self, 0)
		self.ampr = AmpView(self, 1)
		self.add(self.ampl)
		self.add(self.masterslider)
		self.add(self.ampr)
		self.update_all()

	def on_player_callback(self, player, plugin, data):
		"""
		callback for ui events sent by zzub.
		
		@param player: player instance.
		@type player: zzub.Player
		@param plugin: plugin instance
		@type plugin: zzub.Plugin
		@param data: event data.
		@type data: zzub_event_data_t
		"""
		player = com.get('aldrin.core.player')
		if plugin == player.get_plugin(0):
			if data.type == zzub.zzub_event_type_parameter_changed:
				self.update_all()

	def on_scroll_changed(self, widget, scroll, value):
		"""
		Event handler triggered when the master slider has been dragged.
		
		@param event: event.
		@type event: wx.Event
		"""
		player = com.get('aldrin.core.player')
		vol = int(min(max(value,0), 16384) + 0.5)
		master = player.get_plugin(0)
		master.set_parameter_value(1, 0, 0, 16384 - vol, 1)
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
		player = com.get('aldrin.core.player')
		master = player.get_plugin(0)
		vol = master.get_parameter_value(1, 0, 0)
		self.masterslider.set_value(16384 - vol)
		self.latency = driver.get_audiodriver().get_latency()

	def button_up(self, widget, event):
		"""
		refocus panel
		"""
		page=self.rootwindow.framepanel.get_current_page()
		panel = self.rootwindow.pages[page]
		try: panel.view.grab_focus()
		except: panel.grab_focus()

__aldrin__ = dict(
	classes = [
		MasterPanel,
	],
)

if __name__ == '__main__':
	pass