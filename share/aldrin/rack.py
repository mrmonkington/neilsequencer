#encoding: latin-1

# Aldrin
# Modular Sequencer
# Copyright (C) 2006 The Aldrin Development Team
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
Contains all classes and functions needed to render the rack view.
"""

from gtkimport import gtk
import gobject
import cairo
import pangocairo
from utils import prepstr, filepath, db2linear, linear2db, is_debug, filenameify, \
	get_item_count, question, error, new_listview, add_scrollbars, get_clipboard_text, set_clipboard_text, \
	gettext, new_stock_image_button, diff
import config
import zzub
import sys,os
import indexer
import fnmatch
import ctypes
import time
import extman, interface
import Queue
from preset import PresetCollection, Preset
from patterns import key_to_note
import common
player = common.get_player()
from common import MARGIN, MARGIN2, MARGIN3, MARGIN0

class ParameterView(gtk.VBox):
	"""
	Displays parameter sliders for a plugin in a new Dialog.
	"""
	
	UNBIND_ALL = 'unbind-all'
	CONTROLLER = 'controller'
	
	def __init__(self, rootwindow, plugin):
		"""
		Initializer.
		
		@param plugin: The plugin object for which to display parameters.
		@type plugin: zzub.Plugin
		"""
		gtk.VBox.__init__(self)
		self.set_flags(gtk.CAN_FOCUS)
		self.plugin = plugin
		name = prepstr(self.plugin.get_name())
		pl = self.plugin.get_pluginloader()
		classname = prepstr(pl.get_name())
		self.rootwindow = rootwindow
		title = "%s - %s" % (name,classname)
		oc = self.plugin.get_output_channels()
		if oc  == 2:
			title += " (Stereo Output)"
		elif oc == 1:
			title += " (Mono Output)"
		self._title = title

		self.presetbox = gtk.combo_box_entry_new_text()
		self.presetbox.set_size_request(100,-1)
		self.presetbox.set_wrap_width(4)
		self.btnadd = new_stock_image_button(gtk.STOCK_ADD)
		self.btnremove = new_stock_image_button(gtk.STOCK_REMOVE)
		self.btncopy = new_stock_image_button(gtk.STOCK_COPY)
		self.btnrandom = gtk.Button("_Random")
		self.btnhelp = new_stock_image_button(gtk.STOCK_HELP)
		self.btnsnap= gtk.Button("Snapshot")
		menugroup = gtk.HBox(False, MARGIN)
		menugroup.pack_start(self.presetbox)
		menugroup.pack_start(self.btnadd, expand=False)
		menugroup.pack_start(self.btnremove, expand=False)
		menugroup.pack_start(self.btncopy, expand=False)
		menugroup.pack_start(self.btnrandom, expand=False)
		menugroup.pack_start(self.btnsnap, expand=False)
		menugroup.pack_start(self.btnhelp, expand=False)
		toplevelgroup = gtk.VBox(False, MARGIN)
		toplevelgroup.set_border_width(MARGIN)
		toplevelgroup.pack_start(menugroup, expand=False)
		
		scrollwindow = gtk.ScrolledWindow()
		scrollwindow.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		
		self.pluginloader = pl

		self.update_presets()

		rowgroup = gtk.VBox()
		rowgroup.set_border_width(MARGIN)
		
		self.id2pid = {}
		self.pid2ctrls = {}
		
		snamegroup = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
		sslidergroup = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
		svaluegroup = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
		
		def add_slider(g,t,i):
			p = pl.get_parameter(g,i)
			if not (p.get_flags() & zzub.zzub_parameter_flag_state):
				return
			if g == 1:
				name = prepstr(p.get_name())
			else:
				name = "%i-%s" % (t,prepstr(p.get_name()))
			namelabel = gtk.Label()
			namelabel._default_name = name
			slider = gtk.HScale()
			slider.set_property('draw-value', False)
			slider.set_range(p.get_value_min(),p.get_value_max())
			# set increment size for left and right arrow and mouse clicks
			increment = (p.get_value_max() - p.get_value_min()) / 10
			slider.set_increments(1, increment)
			v = plugin.get_parameter_value(g,t,i)
			slider.set_value(v)
			valuelabel = gtk.Label("")
			valuelabel.set_alignment(0, 0.5)
			valuelabel.set_size_request(80, -1)
			
			snamegroup.add_widget(namelabel)
			namelabel.set_alignment(1, 0.5)
			sslidergroup.add_widget(slider)
			svaluegroup.add_widget(valuelabel)
			
			slidergroup = gtk.HBox(False, MARGIN)
			slidergroup.pack_start(namelabel, expand=False)	
			slidergroup.add(slider)	
			slidergroup.pack_end(valuelabel, expand=False)	
			rowgroup.pack_start(slidergroup, expand=False)
			self.pid2ctrls[(g,t,i)] = [namelabel,slider,valuelabel]
			slider.connect('button-press-event', self.on_context_menu, (g,t,i))
			namelabel.add_events(gtk.gdk.ALL_EVENTS_MASK)
			namelabel.connect('button-press-event', self.on_context_menu, (g,t,i))
			valuelabel.add_events(gtk.gdk.ALL_EVENTS_MASK)
			valuelabel.connect('button-press-event', self.on_context_menu, (g,t,i))
			slider.connect('scroll-event', self.on_mousewheel, (g,t,i))
			slider.connect('change-value', self.on_scroll_changed, (g,t,i))
			slider.connect('key-press-event', self.on_key_down, (g,t,i))
			self.update_valuelabel(g,t,i)
			self.update_namelabel(g,t,i)
			
		for i in range(pl.get_parameter_count(1)): # globals
			add_slider(1,0,i)
		# tracks
		for t in range(plugin.get_track_count()):
			for i in range(pl.get_parameter_count(2)):
				add_slider(2,t,i)
				
		self.btnadd.connect('clicked', self.on_button_add)
		self.btnremove.connect('clicked', self.on_button_remove)
		self.btncopy.connect('clicked', self.on_button_copy)
		self.btnrandom.connect('clicked', self.on_button_random)
		self.btnsnap.connect('clicked', self.on_button_snapshot)
		self.btnhelp.connect('clicked', self.on_button_help)
		self.connect('destroy', self.on_destroy)
		if rootwindow.routeframe:
			routeview = rootwindow.routeframe.view
			self.connect('key-press-event', routeview.on_key_jazz, self.plugin)		
			self.connect('key-release-event', routeview.on_key_jazz_release, self.plugin)
		self.connect('button-press-event', self.on_left_down)

		self.presetbox.set_active(0)
		self.presetbox.connect('changed', self.on_select_preset)
		
		scrollwindow.add_with_viewport(rowgroup)
		self.scrollwindow = scrollwindow
		self.rowgroup = rowgroup
		toplevelgroup.add(scrollwindow)

		self.add(toplevelgroup)		
		self.rootwindow.event_handlers.append(self.on_callback)
		self.update_preset_buttons()
	
	def on_left_down(self, widget, event, data=None):
		self.grab_focus()

	def get_best_size(self):
		rc = self.get_allocation()
		cdx,cdy,cdw,cdh = rc.x, rc.y, rc.width, rc.height
		rc = self.rowgroup.get_allocation()
		svx, svy = rc.width, rc.height
		rc = self.scrollwindow.get_allocation()
		swx,swy = rc.width, rc.height
		ofsy = cdh - swy # size without scrollwindow
		return max(swx,400), min((svy+20+ofsy),(3*gtk.gdk.screen_height())/4)
		
	def get_title(self):
		return self._title
		
	def on_unbind(self, widget, (g,t,i)):
		"""
		Unbinds all controllers from the selected parameter.
		
		@param event: Event.
		@type event: wx.Event
		"""
		player.remove_midimapping(self.plugin, g, t, i)
		self.update_namelabel(g,t,i)
		
	def on_learn_controller(self, widget, (g,t,i)):
		"""
		Handles the learn entry from the context menu. Associates
		a controller with a plugin parameter.
		"""
		import controller
		res = controller.learn_controller(self, self.rootwindow)
		if res:
			name, channel, ctrlid = res
			player.add_midimapping(self.plugin, g, t, i, channel, ctrlid)
		self.update_namelabel(g,t,i)

	def on_bind_controller(self, widget, (g,t,i), (name,channel,ctrlid)):
		"""
		Handles clicks on controller names in the context menu. Associates
		a controller with a plugin parameter.
		
		@param event: Event.
		@type event: wx.Event
		"""
		player.add_midimapping(self.plugin, g, t, i, channel, ctrlid)
		self.update_namelabel(g,t,i)
		
	def update_namelabel(self, g,t,i):
		nl,s,vl = self.pid2ctrls[(g,t,i)]
		markup = "<b>%s</b>" % nl._default_name
		for mm in player.get_midimapping_list():
			mp,mg,mt,mi = mm.get_plugin(), mm.get_group(), mm.get_track(), mm.get_column()
			if (mp == self.plugin) and ((mg,mt,mi) == (g,t,i)):
				markup = "<u>%s</u>" % markup
				break
		nl.set_markup(markup)
		
	def on_context_menu(self, widget, event, (g,t,i)):
		"""
		Event handler for requests to show the context menu.
		
		@param event: event.
		@type event: wx.Event
		"""
		if event.button == 1:
			nl,s,vl = self.pid2ctrls[(g,t,i)]
			s.grab_focus()
		elif event.button == 3:
			nl,s,vl = self.pid2ctrls[(g,t,i)]
			menu = gtk.Menu()
			submenu = gtk.Menu()
			index = 0
			def make_submenu_item(submenu, name):
				item = gtk.MenuItem(label=name)
				item.set_submenu(submenu)
				return item
			def make_menu_item(label, desc, func, *args):
				item = gtk.MenuItem(label=label)
				if func:
					item.connect('activate', func, *args)
				return item
			def cmp_nocase(a,b):
				return cmp(a[0].lower(),b[0].lower())
			for name,channel,ctrlid in sorted(config.get_config().get_midi_controllers(), cmp_nocase):
				submenu.append(make_menu_item(prepstr(name), "", self.on_bind_controller, (g,t,i), (name,channel,ctrlid)))
				index += 1
			controllers = 0
			for mm in player.get_midimapping_list():
				mp,mg,mt,mi = mm.get_plugin(), mm.get_group(), mm.get_track(), mm.get_column()
				if (mp == self.plugin) and ((mg,mt,mi) == (g,t,i)):
					label = "Bound to CC #%03i (CH%02i)" % (mm.get_controller(), mm.get_channel()+1)
					item = gtk.MenuItem(label=label)
					item.set_sensitive(False)
					menu.append(item)
					controllers +=1
			if controllers:
				menu.append(gtk.SeparatorMenuItem())
			if index:
				menu.append(make_submenu_item(submenu, "_Bind to MIDI Controller"))
			menu.append(make_menu_item("_Learn MIDI Controller", "", self.on_learn_controller, (g,t,i)))
			if controllers:
				menu.append(gtk.SeparatorMenuItem())
				menu.append(make_menu_item("_Unbind Parameter", "", self.on_unbind, (g,t,i)))
			menu.show_all()
			menu.attach_to_widget(self, None)
			menu.popup(None, None, None, event.button, event.time)
			return True
		
	def on_callback(self, player, plugin, data):
		"""
		parameter window callback for ui events sent by zzub.
		
		@param player: player instance.
		@type player: zzub.Player
		@param plugin: plugin instance
		@type plugin: zzub.Plugin
		@param data: event data.
		@type data: zzub_event_data_t
		"""
		if plugin == self.plugin:
			if data.type == zzub.zzub_event_type_parameter_changed:
				data = getattr(data,'').change_parameter
				g,t,i,v = data.group, data.track, data.param, data.value
				p = self.pluginloader.get_parameter(g,i)
				if p.get_flags() & zzub.zzub_parameter_flag_state:
					nl,s,vl = self.pid2ctrls[(g,t,i)]
					v = self.plugin.get_parameter_value(g,t,i)
					s.set_value(v)
					self.update_valuelabel(g,t,i)
					
	def update_presets(self):
		"""
		Updates the preset box.
		"""
		self.presets = config.get_config().get_plugin_presets(self.pluginloader)
		s = self.presetbox.child.get_text()
		self.presetbox.get_model().clear()
		self.presetbox.append_text('<default>')
		for preset in self.presets.presets:
			self.presetbox.append_text(prepstr(preset.name))
		self.presetbox.child.set_text(s)
					
	def apply_preset(self, preset=None):
		if not preset:
			for g in range(1,3):
				for t in range(self.plugin.get_group_track_count(g)):
						for i in range(self.pluginloader.get_parameter_count(g)):
							p = self.pluginloader.get_parameter(g,i)
							if p.get_flags() & zzub.zzub_parameter_flag_state:
								self.plugin.set_parameter_value(g,t,i,p.get_value_default(),0)
		else:			
			preset.apply(self.plugin)
		self.update_all_sliders()
	
	def on_button_add(self, widget):
		"""
		Handler for the Add preset button
		"""
		name = self.presetbox.child.get_text()
		presets = [preset for preset in self.presets.presets if prepstr(preset.name) == name]
		if presets:
			preset = presets[0]
		else:
			preset = Preset()
			self.presets.presets.append(preset)
		preset.name = name
		preset.comment = ''
		preset.pickup(self.plugin)
		self.presets.sort()
		config.get_config().set_plugin_presets(self.pluginloader, self.presets)
		self.update_presets()
		self.update_preset_buttons()

	def on_button_remove(self, widget):
		"""
		Handler for the Remove preset button
		"""
		name = self.presetbox.child.get_text()
		presets = [preset for preset in self.presets.presets if prepstr(preset.name) == name]
		if presets:
			self.presets.presets.remove(presets[0])
		config.get_config().set_plugin_presets(self.pluginloader, self.presets)
		self.update_presets()
		self.update_preset_buttons()
		
	def update_preset_buttons(self):
		"""
		Update presets.
		"""
		if self.presetbox.child.get_text() == "<default>":
			self.btnadd.set_sensitive(False)
			self.btnremove.set_sensitive(False)
		elif [preset for preset in self.presets.presets if prepstr(preset.name) == self.presetbox.child.get_text()]:
			self.btnadd.set_sensitive(True)
			self.btnremove.set_sensitive(True)
		else:
			self.btnadd.set_sensitive(True)
			self.btnremove.set_sensitive(True)

	def on_select_preset(self, widget):
		"""
		Handler for changes of the choice box. Changes the current parameters
		according to preset.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		if widget.get_active() == -1:
			pass
		else:
			sel = max(self.presetbox.get_active() - 1,-1)
			if sel == -1:
				self.apply_preset(None)
			else:
				self.apply_preset(self.presets.presets[sel])
		self.update_preset_buttons()
			
	def update_all_sliders(self):
		"""
		Updates all sliders. Should only be called when most sliders
		have been changed at once, e.g. after a preset change.
		"""
		for g in range(1,3):
			for t in range(self.plugin.get_group_track_count(g)):
				for i in range(self.pluginloader.get_parameter_count(g)):
					p = self.pluginloader.get_parameter(g,i)
					if p.get_flags() & zzub.zzub_parameter_flag_state:
						nl,s,vl = self.pid2ctrls[(g,t,i)]
						v = self.plugin.get_parameter_value(g,t,i)						
						s.set_value(v)
						self.update_valuelabel(g,t,i)
		
	def on_button_edit(self, event):
		"""
		Handler for clicks on the 'Edit' button. Opens the
		preset dialog.
		"""
		dlg = PresetDialog(self.plugin, self)
		dlg.run()
		dlg.destroy()
		self.update_presets()

	def on_button_copy(self, event):
		"""
		Handler for clicks on the 'Copy' button. Constructs a paste buffer
		which can be used for pasting in the pattern editor.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		import patterns
		CLIPBOARD_MAGIC = patterns.PatternView.CLIPBOARD_MAGIC
		data = CLIPBOARD_MAGIC
		data += "%01x" % patterns.SEL_ALL
		for g in range(1,3):
			for t in range(self.plugin.get_group_track_count(g)):
				for i in range(self.pluginloader.get_parameter_count(g)):
					p = self.pluginloader.get_parameter(g,i)
					if p.get_flags() & zzub.zzub_parameter_flag_state:
						v = self.plugin.get_parameter_value(g,t,i)
						data += "%04x%01x%02x%02x%04x" % (0,g,t,i,v)
		set_clipboard_text(data)
		
	def on_button_random(self, event):
		"""
		Handler for clicks on the 'Random' button.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		import random
		for g in range(1,3):
			for t in range(self.plugin.get_group_track_count(g)):
				for i in range(self.pluginloader.get_parameter_count(g)):
					p = self.pluginloader.get_parameter(g,i)
					if p.get_flags() & zzub.zzub_parameter_flag_state:
						nl,s,vl = self.pid2ctrls[(g,t,i)]
						v = random.randint(p.get_value_min(), p.get_value_max())
						self.plugin.set_parameter_value(g,t,i,v,0)
						s.set_value(v)
						self.update_valuelabel(g,t,i)
	
	def on_button_snapshot(self, event):
		"""
		Handler for clicks on the 'Snapshot' button.
		"""
		#for g in range(1,3):
		#	for t in range(self.plugin.get_group_track_count(g)):
		automation = player.get_automation()
		player.set_automation(1)
		for i in range(self.pluginloader.get_parameter_count(1)):
			p = self.pluginloader.get_parameter(1,i)
			nl,s,vl = self.pid2ctrls[(1,0,i)]
			v=int(s.get_value())
			self.plugin.set_parameter_value(1,0,i,v,1)
		player.set_automation(automation)
		self.rootwindow.patternframe.view.update_line(player.get_position())
		self.rootwindow.patternframe.view.redraw()
			
	def on_button_help(self, event):
		"""
		Handler for clicks on the 'Help' button.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		uri = filenameify(self.pluginloader.get_uri())
		name = filenameify(self.pluginloader.get_name())		
		helpfilepaths = [
			filepath('../doc/zzub/plugins/' + uri + '/index.html'),
			filepath('../doc/zzub/plugins/' + name + '/index.html'),
		]
		for path in helpfilepaths:
			print "searching for '%s'..." % path
			if os.path.isfile(path):
				import webbrowser
				webbrowser.open_new(path)
				return
		wx.MessageDialog(self, message="Sorry, there's no help for this plugin yet.", caption = "Help", style = wx.ICON_WARNING|wx.OK|wx.CENTER).ShowModal()
		
	def on_key_down(self, widget, event, (g,t,i)):
		"""
		Callback that responds to key stroke.
		"""		
		kv = event.keyval
		if gtk.gdk.keyval_from_name('KP_0') <= kv <= gtk.gdk.keyval_from_name('KP_9'):
			kv = kv - gtk.gdk.keyval_from_name('KP_0')  + gtk.gdk.keyval_from_name('0') 
			p = self.pluginloader.get_parameter(g,i)
			minv = p.get_value_min()
			maxv = p.get_value_max()
			data_entry = DataEntry((minv,maxv,chr(kv)), self)
			x,y,state = self.window.get_toplevel().get_pointer()
			px,py = self.get_toplevel().get_position()
			data_entry.move(px+x, py+y)
			if data_entry.run() == gtk.RESPONSE_OK:
				try:
					value = int(data_entry.edit.get_text())
					nl,s,vl = self.pid2ctrls[(g,t,i)]
					s.set_value(value)
					self.plugin.set_parameter_value(g,t,i,int(s.get_value()),0)
					self.update_valuelabel(g,t,i)
				except:
					import traceback
					traceback.print_exc()
			data_entry.destroy()
			return True
		return False
	
	def on_destroy(self, event):
		"""
		Handles destroy events.
		"""
		print "rack:on_destroy"
		self.rootwindow.event_handlers.remove(self.on_callback)
		
	def on_mousewheel(self, widget, event, (g,t,i)):
		"""
		Sent when the mousewheel is used on a slider.

		@param event: A mouse event.
		@type event: wx.MouseEvent
		"""
		nl,s,vl = self.pid2ctrls[(g,t,i)]
		v = self.plugin.get_parameter_value(g,t,i)
		if event.direction == gtk.gdk.SCROLL_UP:
			v += 1
		elif event.direction == gtk.gdk.SCROLL_DOWN:
			v -= 1
		self.plugin.set_parameter_value(g,t,i,v,1)
		v = self.plugin.get_parameter_value(g,t,i)
		s.set_value(v)
		self.update_valuelabel(g,t,i)
		
	def update_valuelabel(self, g, t, i):
		"""
		Updates the right label for a parameter slider.
		
		@param g: The group this parameter belongs to.
		@type g: int
		@param t: The track of the group this parameter belongs to.
		@type t: int
		@param i: The parameter index within the track.
		@type i: int
		"""
		nl,s,vl = self.pid2ctrls[(g,t,i)]
		v = self.plugin.get_parameter_value(g,t,i)
		text = prepstr(self.plugin.describe_value(g,i,v))
		if not text:
			text = "%i" % v
		vl.set_label(text)
		
	def on_scroll_changed(self, widget, scroll, value, (g,t,i)):
		"""
		Event handler for changes in slider movements.
		
		@param event: A scroll event.
		@type event: wx.ScrollEvent
		"""
		nl,s,vl = self.pid2ctrls[(g,t,i)]
		p = self.pluginloader.get_parameter(g,i)
		minv = p.get_value_min()
		maxv = p.get_value_max()
		value = int(max(min(value, maxv), minv) + 0.5)
		s.set_value(value) # quantize slider position
		self.plugin.set_parameter_value(g,t,i,value,1)
		self.update_valuelabel(g,t,i)
		return True

class DataEntry(gtk.Dialog):
	"""
	A data entry control meant for numerical input of slider values.
	"""
	def __init__(self, (minval,maxval,v), parent):
		"""
		Initializer.
		"""
		gtk.Dialog.__init__(self,
			"Edit Value",
			parent.get_toplevel(),
			gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
			(gtk.STOCK_OK, gtk.RESPONSE_OK, gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL),
		)	
		self.label = gtk.Label("Enter Value:")
		self.edit = gtk.Entry()
		self.edit.set_text(v)
		s = gtk.HBox()
		s.pack_start(self.label, expand=False)
		s.pack_start(self.edit)
		self.vbox.pack_start(s, expand=False)
		label = gtk.Label(prepstr("%s - %s" % (minval,maxval)))
		label.set_alignment(0,0.5)
		self.vbox.pack_start(label, expand=False)
		self.edit.grab_focus()
		self.edit.select_region(1,-1)
		self.show_all()
		self.edit.connect('activate', self.on_text_enter)
	
	def on_text_enter(self, widget):
		self.response(gtk.RESPONSE_OK)

class RackPanel(gtk.VBox):
	"""
	Rack panel.
	
	Displays controls for individual plugins.
	"""
	def __init__(self, rootwindow):
		"""
		Initialization.
		"""
		gtk.VBox.__init__(self)
		self.rootwindow = rootwindow
		self.panels = {}
		scrollwindow = gtk.ScrolledWindow()
		scrollwindow.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		rowgroup = gtk.VBox()
		scrollwindow.add_with_viewport(rowgroup)
		self.scrollwindow = scrollwindow
		self.rowgroup = rowgroup
		self.connect('realize', self.on_realize)
		self.add(self.scrollwindow)
		self.rootwindow.event_handlers.append(self.on_player_callback)

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
		if data.type == zzub.zzub_event_type_delete_plugin:
			self.update_all()

	def on_realize(self, widget):
		self.update_all()

	def update_all(self):
		"""
		Updates the full view.
		"""
		print "rack:update_all"
		addlist, rmlist = diff(self.panels.keys(), common.get_plugin_infos().keys())
		print addlist, rmlist
		for plugin in rmlist:
			self.panels[plugin].destroy()
			del self.panels[plugin]
		for plugin in addlist:
			view = ParameterView(self.rootwindow, plugin)
			view.show_all()
			self.panels[plugin] = view
			self.rowgroup.pack_start(view, expand=False)
			#~ view.set_size_request(*view.get_best_size())

if __name__ == '__main__':
	import testplayer, utils
	player = testplayer.get_player()
	player.load_ccm(utils.filepath('demosongs/paniq-knark.ccm'))
	window = testplayer.TestWindow()
	rack = RackPanel(window)
	window.add(rack)
	window.show_all()
	
	gtk.main()
