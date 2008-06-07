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
from common import MARGIN, MARGIN2, MARGIN3, MARGIN0
import gobject
from utils import new_stock_image_toggle_button, new_stock_image_button, format_time, \
	ticks_to_time
import audiogui
import time
import config
from aldrincom import com
import zzub
import common

STOCK_PATTERNS = "aldrin-patterns"
STOCK_ROUTER = "aldrin-router"
STOCK_SEQUENCER = "aldrin-sequencer"
STOCK_LOOP = "aldrin-loop"
STOCK_SOUNDLIB = "aldrin-soundlib"
STOCK_INFO = "aldrin-info"
STOCK_RACK = "aldrin-rack"
STOCK_PANIC = "aldrin-panic"

class TransportPanel(gtk.HBox):
	"""
	A panel containing the BPM/TPB spin controls.
	"""
	
	__aldrin__ = dict(
		id = 'aldrin.core.panel.transport',
	)
	
	def __init__(self, rootwindow):
		"""
		Initializer.
		"""		
		gtk.HBox.__init__(self)
		self.rootwindow = rootwindow
		self.rootwindow.event_handlers.append(self.on_player_callback)
		self.cpulabel = gtk.Label("CPU")
		self.cpu = gtk.ProgressBar()
		self.cpu.set_size_request(80,-1)
		self.cpuvalue = gtk.Label("100%")
		self.cpuvalue.set_size_request(32,-1)
		self.bpmlabel = gtk.Label("BPM")
		self.bpm = gtk.SpinButton()
		self.bpm.set_range(16,500)
		self.bpm.set_value(126)
		self.bpm.set_increments(1, 10)
		self.tpblabel = gtk.Label("TPB")
		self.tpb = gtk.SpinButton()
		self.tpb.set_range(1,32)
		self.tpb.set_value(4)
		self.tpb.set_increments(1, 2)
		self.btnplay = new_stock_image_toggle_button(gtk.STOCK_MEDIA_PLAY)
		self.btnrecord = new_stock_image_toggle_button(gtk.STOCK_MEDIA_RECORD)
		self.btnstop = new_stock_image_button(gtk.STOCK_MEDIA_STOP)
		self.btnloop = new_stock_image_toggle_button(STOCK_LOOP)
		self.btnpanic = new_stock_image_toggle_button(STOCK_PANIC)
		
		vbox = gtk.VBox(False, 0)
		sg1 = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
		sg2 = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
		def add_row(name):
			c1 = gtk.Label()
			c1.set_markup("<b>%s</b>" % name)
			c1.set_alignment(1, 0.5)
			c2 = gtk.Label()
			lcd = audiogui.LCD()
			#~ lcd.set_fg_color(0.4, 0.5, 1.0)
			#~ lcd.set_bg_color(0.4, 0.05, 1.0)
			#~ lcd.set_contrast(0.05)
			#~ lcd.set_brightness(0.8)
			lcd.set_contrast(0.1)
			lcd.set_dimensions(9,1)
			lcd.set_border(2)
			#lcd.set_scale(2)
			#~ lcd.set_text("Slider 2\n 41 CC12  1  U12")
			#~ c2.set_alignment(1, 0.5)
			hbox = gtk.HBox(False, MARGIN)
			hbox.pack_start(c1, expand=False)
			hbox.pack_start(lcd, expand=False)
			sg1.add_widget(c1)
			sg2.add_widget(lcd)
			vbox.add(hbox)
			return lcd
		self.elapsed = add_row("Elapsed")
		self.current = add_row("Current")
		self.loop = add_row("Loop")
		self.starttime = time.time()
		self.update_label()		


		combosizer = gtk.HBox(False, MARGIN)
		combosizer.pack_start(vbox, expand=False)
		combosizer.pack_start(gtk.VSeparator(), expand=False)

		hbox = gtk.HBox(False, MARGIN0)
		hbox.pack_start(self.btnplay,expand=False)
		hbox.pack_start(self.btnrecord,expand=False)
		hbox.pack_start(self.btnstop,expand=False)
		hbox.pack_start(self.btnloop,expand=False)
		self.transport_buttons = hbox.get_children() + [self.btnpanic]
		def on_realize(self):
			for e in self.transport_buttons:
				rc = e.get_allocation()
				w = max(rc.width, rc.height)
				e.set_size_request(w,w)
		self.connect('realize', on_realize)
		
		
		combosizer.pack_start(hbox, expand=False)
		combosizer.pack_start(gtk.VSeparator(), expand=False)
		
		combosizer.pack_start(self.bpmlabel,expand=False)
		combosizer.pack_start(self.bpm,expand=False)
		combosizer.pack_start(self.tpblabel,expand=False)
		combosizer.pack_start(self.tpb,expand=False)

		combosizer.pack_start(gtk.VSeparator(), expand=False)
		cpubox = gtk.HBox(False, MARGIN)
		cpubox.pack_start(self.cpulabel, expand=False)
		cpubox.pack_start(self.cpu, expand=False)
		cpubox.pack_start(self.cpuvalue, expand=False)
		cpuvbox = gtk.VBox(False, MARGIN0)
		cpuvbox.pack_start(gtk.VBox())
		cpuvbox.pack_start(cpubox, expand=False)
		cpuvbox.pack_end(gtk.VBox())
		combosizer.pack_start(cpuvbox, expand=False)
		combosizer.pack_start(gtk.VSeparator(), expand=False)
		combosizer.pack_start(self.btnpanic, expand=False)
		
		self.pack_start(gtk.HBox())
		self.pack_start(combosizer, expand=False)
		self.pack_end(gtk.HBox())
		self.set_border_width(MARGIN)
		player = com.get('aldrin.core.player')
		player.get_plugin(0).set_parameter_value(1, 0, 1, config.get_config().get_default_int('BPM', 126), 1)
		player.get_plugin(0).set_parameter_value(1, 0, 2, config.get_config().get_default_int('TPB', 4), 1)
		self.bpm_value_changed = self.bpm.connect('value-changed', self.on_bpm)
		self.tpb_value_changed = self.tpb.connect('value-changed', self.on_tpb)
		self.bpm.connect('focus-in-event', self.spin_focus)
		self.tpb.connect('focus-in-event', self.spin_focus)
		gobject.timeout_add(100, self.update_label)
		gobject.timeout_add(500, self.update_cpu)
		self.update_all()
		
	def update_cpu(self):
		player = com.get('aldrin.core.player')
		cpu = min(player.audiodriver_get_cpu_load(), 100)
		self.cpu.set_fraction(cpu / 100.0)
		self.cpuvalue.set_label("%i%%" % int(cpu + 0.5))
		return True
		
	def update_label(self):
		"""
		Event handler triggered by a 10fps timer event.
		"""
		player = com.get('aldrin.core.player')
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
		lb,le = player.get_song_loop()		
		l = format_time(ticks_to_time(le-lb,bpm,tpb))
		for text,control in [(e,self.elapsed),(c,self.current),(l,self.loop)]:
			#~ if text != control.get_text():
			control.set_text(text)
		return True

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
				data = getattr(data,'').change_parameter
				g,t,i,v = data.group, data.track, data.param, data.value
				if (g,t) == (1,0):
					if i == 1:
						self.update_bpm()
						try:
							com.get('aldrin.core.wavetablepanel').waveedit.view.view_changed()
						except AttributeError:
							pass
					
					elif i == 2:
						self.update_tpb()
		
	def on_bpm(self, widget):
		"""
		Event handler triggered when the bpm spin control value is being changed.
		
		@param event: event.
		@type event: wx.Event
		"""
		player = com.get('aldrin.core.player')
		player.get_plugin(0).set_parameter_value(1, 0, 1, int(self.bpm.get_value()), 1)
		config.get_config().set_default_int('BPM', int(self.bpm.get_value()))

	def on_tpb(self, widget):
		"""
		Event handler triggered when the tpb spin control value is being changed.
		
		@param event: event.
		@type event: wx.Event
		"""
		player = com.get('aldrin.core.player')
		player.get_plugin(0).set_parameter_value(1, 0, 2, int(self.tpb.get_value()), 1)
		config.get_config().set_default_int('TPB', int(self.tpb.get_value()))
		
	def spin_focus(self, widget, event):
		"""
		Event handler triggered when tbp and bpm get focus
		"""
		routeview=com.get('aldrin.core.routerpanel').view
		if routeview.selected_plugin:
			last = routeview.selected_plugin
			routeview.selected_plugin = None			
			common.get_plugin_infos().get(last).reset_plugingfx()
		
	def update_bpm(self):
		self.bpm.handler_block(self.bpm_value_changed)
		player = com.get('aldrin.core.player')
		master = player.get_plugin(0)
		bpm = master.get_parameter_value(1, 0, 1)
		self.bpm.set_value(bpm)
		self.bpm.handler_unblock(self.bpm_value_changed)
		
	def update_tpb(self):
		self.tpb.handler_block(self.tpb_value_changed)
		player = com.get('aldrin.core.player')
		master = player.get_plugin(0)
		tpb = master.get_parameter_value(1, 0, 2)
		self.tpb.set_value(tpb)
		self.tpb.handler_unblock(self.tpb_value_changed)

	def update_all(self):
		"""
		Updates all controls.
		"""
		self.update_bpm()
		self.update_tpb()

__aldrin__ = dict(
	classes = [
		TransportPanel,
	],
)

if __name__ == '__main__':
	pass
