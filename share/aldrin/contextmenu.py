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

"""
This module contains the context menu component for different zzub objects
such as plugins, patterns, and so on. based on the context object currently
selected, items can choose to append themselves or not.
"""

from gtkimport import gtk
import common
import aldrincom
from aldrincom import com

from utils import is_generator, is_root, is_effect, prepstr
from utils import PLUGIN_FLAGS_MASK, ROOT_PLUGIN_FLAGS, GENERATOR_PLUGIN_FLAGS, EFFECT_PLUGIN_FLAGS, CONTROLLER_PLUGIN_FLAGS

class ContextMenu(gtk.Menu):
	__aldrin__ = dict(
		id = 'aldrin.core.contextmenu',
		singleton = False,
		categories = [
		],
	)
	
	def __init__(self, contextid, context):
		gtk.Menu.__init__(self)
		self.__context_id = contextid
		self.__context = context
		
	def get_context(self):
		return self.__context
		
	def get_context_id(self):
		return self.__context_id
		
	context = property(get_context)
	context_id = property(get_context_id)
	
	def add_separator(self):
		self.append(gtk.SeparatorMenuItem())
		
	def add_submenu(self, label, submenu = None):
		if not submenu:
			submenu = ContextMenu(self.__context_id, self.__context)
		item = gtk.MenuItem(label=label)
		item.set_submenu(submenu)
		self.append(item)
		return item, submenu
		
	def add_item(self, label, func, *args):
		item = gtk.MenuItem(label=label)
		item.connect('activate', func, *args)
		self.append(item)
		return item
	
	def add_check_item(self, label, toggled, func, *args):
		item = gtk.CheckMenuItem(label=label)
		item.connect('toggled', func, *args)
		self.append(item)
		return item
		
	def add_image_item(self, label, iconpath, func, *args):
		item = gtk.ImageMenuItem(stock_id=label)
		if iconpath:
			image = gtk.Image()
			image.set_from_pixbuf(gtk.gdk.pixbuf_new_from_file(iconpath))
			item.set_image(image)
		item.connect('activate', func, *args)
		self.append(item)
		return item
		
	def popup(self, parent, event=None):
		for item in self.get_children():
			item.destroy()
		for item in com.get_from_category('contextmenu.handler'):
			item.populate_contextmenu(self)
		self.show_all()
		if not self.get_attach_widget():
			self.attach_to_widget(parent and parent.get_toplevel(), None)
		if event:
			event_button = event.button
			event_time = event.time
		else:
			event_button = 0
			event_time = 0
		gtk.Menu.popup(self, None, None, None, event_button, event_time)

class PluginContextMenu(gtk.Menu):
	__aldrin__=dict(id='aldrin.core.popupmenu',singleton=True,categories=['contextmenu.handler'])
	def populate_contextmenu(self, menu):
		if menu.context_id == 'plugin':
			self.populate_pluginmenu(menu)
		elif menu.context_id == 'connection':
			self.populate_connectionmenu(menu)
		elif menu.context_id == 'router':
			self.populate_routermenu(menu)
			
	def populate_routermenu(self, menu):
		self.get_plugin_menu(menu=menu)
		com.get_from_category('menuitem.route', menu)
			
	def populate_connectionmenu(self, menu):
		mp, index = menu.context
		menu.append(make_menu_item("_Disconnect plugins", "Disconnect plugins", self.on_popup_disconnect, mp, index))
		conntype = mp.get_input_connection_type(index)
		if conntype == zzub.zzub_connection_type_audio:
			menu.append(gtk.SeparatorMenuItem())
			menu.append(make_submenu_item(self.get_plugin_menu(include_generators=False,include_controllers=False, conn=(mp, index)), "_Insert Effect"))
			menu.append(gtk.SeparatorMenuItem())
			menu.append(make_menu_item("_Signal Analysis", "Signal Analysis", self.on_popup_show_signalanalysis, mp, index))
		elif conntype == zzub.zzub_connection_type_event:
			menu.append(gtk.SeparatorMenuItem())
			mi = conn.get_input()
			for param in mi.get_pluginloader().get_parameter_list(3):
				print param
		com.get_from_category('menuitem.connection', menu, connection=(mp,index))
			
	def populate_pluginmenu(self, menu):
		mp = menu.context
		menu.add_check_item("_Mute", common.get_plugin_infos().get(mp).muted, self.on_popup_mute, mp)
		if is_generator(mp):
			menu.add_check_item("_Solo", self.solo_plugin == mp, self.on_popup_solo, mp)
		menu.add_separator()
		menu.add_item("_Parameters...", self.on_popup_show_params, mp)
		menu.add_item("_Attributes...", self.on_popup_show_attribs, mp)
		menu.add_item("P_resets...", self.on_popup_show_presets, mp)
		menu.add_separator()
		menu.add_item("_Rename...", self.on_popup_rename, mp)
		if not is_root(mp):
			menu.add_item("_Delete", self.on_popup_delete, mp)
		if is_effect(mp) or is_root(mp):
			menu.add_separator()
			menu.add_check_item("Default Target",self.autoconnect_target == mp,self.on_popup_set_target, mp)
			menu.add_submenu("_Prepend Effect",self.get_plugin_menu(include_generators=False, include_controllers=False, plugin=mp))
		commands = mp.get_commands()
		if commands:
			menu.add_separator()
			submenuindex = 0
			for index in range(len(commands)):
				cmd = commands[index]
				if cmd.startswith('/'):
					item, submenu = menu.add_submenu(prepstr(cmd[1:]))
					subcommands = mp.get_sub_commands(index)
					submenuindex += 1
					for subindex in range(len(subcommands)):
						subcmd = subcommands[subindex]
						submenu.add_item(prepstr(subcmd), self.on_popup_command, mp, submenuindex, subindex)
				else:
					menu.add_item(prepstr(cmd), self.on_popup_command, mp, 0, index)
		com.get_from_category('menuitem.plugin', menu, plugin=mp)

	def on_popup_rename(self, widget, mp):
		text = gettext(self, "Enter new plugin name:", prepstr(mp.get_name()))
		if text:
			mp.set_name(text)
			common.get_plugin_infos().get(mp).reset_plugingfx()
			self.redraw()
	
	def on_popup_solo(self, widget, mp):
		"""
		Event handler for the "Mute" context menu option.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		"""		
		if self.solo_plugin != mp:
			self.solo(mp)
		else:
			self.solo(None)		
		self.redraw()
	
	def on_popup_mute(self, widget, mp):
		"""
		Event handler for the "Mute" context menu option.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		"""
		self.toggle_mute(mp)		
		self.redraw()
		
	def on_popup_delete(self, widget, mp):
		"""
		Event handler for the "Delete" context menu option.
		"""
		res = question(self, "<b><big>Remove plugin?</big></b>\n\nThis action can not be reversed.", allowcancel = False)
		if res == gtk.RESPONSE_YES:
			inplugs = []
			outplugs = []
			# record all connections
			while True:
				conns = mp.get_input_connection_list()
				if not conns:
					break
				conn = conns.pop()
				input = conn.get_input()
				for i in range(conn.get_output().get_input_connection_count()):
						if conn.get_output().get_input_connection(i)==conn:
							break
				try:
					aconn = conn.get_audio_connection()
					amp = aconn.get_amplitude()
					pan = aconn.get_panning()
					inplugs.append((input,amp,pan))
				except:
					import traceback
					print traceback.format_exc()
				mp.delete_input(input)
			while True:
				conns = mp.get_output_connection_list()
				if not conns:
					break
				conn = conns.pop()
				output = conn.get_output()
				for i in range(conn.get_output().get_input_connection_count()):
						if conn.get_output().get_input_connection(i)==conn:
							break
				try:
					aconn = conn.get_audio_connection()
					amp = aconn.get_amplitude()
					pan = aconn.get_panning()
					outplugs.append((output,amp,pan))
				except:
					import traceback
					print traceback.format_exc()
				output.delete_input(mp)
			# and now restore them
			for inplug,iamp,ipan in inplugs:
				for outplug,oamp,opan in outplugs:
					newamp = (iamp*oamp)/16384
					newpan = ipan
					outplug.add_audio_input(inplug, newamp, newpan)
			del common.get_plugin_infos()[mp]
			mp.destroy()
			self.redraw()
		
	def on_popup_disconnect(self, widget, mp, index):
		"""
		Event handler for the "Disconnect" context menu option.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		"""
		plugin = mp.get_input_connection_plugin(index)
		conntype = mp.get_input_connection_type(index)
		mp.delete_input(plugin,conntype)
		player = com.get('aldrin.core.player')
		player.history_commit("disconnect")
		self.redraw()

	def on_popup_show_signalanalysis(self, widget, conn):
		"""
		Event handler for the "Signal Analysis" context menu option.
		"""
		dlg = SignalAnalysisDialog(self.rootwindow, conn.get_input(), self)
		dlg.show_all()
		
	def on_popup_show_attribs(self, widget, mp):
		"""
		Event handler for the "Attributes..." context menu option.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		"""
		dlg = AttributesDialog(mp, self)
		dlg.run()
		dlg.destroy()
		
		
	def on_popup_show_presets(self, widget, plugin):
		"""
		Event handler for the "Presets..." context menu option.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		"""		
		dlg = PresetDialog(self.rootwindow, plugin, self)
		dlg.show_all()
		
	def on_popup_show_params(self, widget, mp):
		"""
		Event handler for the "Parameters..." context menu option.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		"""
		self.show_parameter_dialog(mp)
		
	def on_popup_new_plugin(self, widget, pluginloader, kargs={}):
		"""
		Event handler for "new plugin" context menu options.
		"""
		player = com.get('aldrin.core.player')
		basename = pluginloader.get_short_name()
		name = pluginloader.get_short_name()
		basenumber = 2
		mask=gtk.get_current_event_state()
		while True:
			found = False
			for mp in player.get_plugin_list():
				if mp.get_name() == name:
					found = True
					name = "%s%i" % (basename, basenumber)
					basenumber += 1
					break
			if not found:
				break
		print "create_plugin: ",name
		mp = player.create_plugin(None, 0, name, pluginloader)
		assert mp
		if ((mp.get_flags() & PLUGIN_FLAGS_MASK) == GENERATOR_PLUGIN_FLAGS) and \
			(pluginloader.get_parameter_count(1) or pluginloader.get_parameter_count(2)):
			pattern = mp.create_pattern(com.get('aldrin.core.sequencerpanel').view.step)
			pattern.set_name('00')
			seq = player.get_current_sequencer()
			t=seq.create_sequence(mp)
			t.set_event(0,16)
			if not(mask & gtk.gdk.SHIFT_MASK):
				if player.autoconnect_target:
					player.autoconnect_target.add_input(mp, zzub.zzub_connection_type_audio)
				else:
					player.get_plugin(0).add_input(mp, zzub.zzub_connection_type_audio)
		mp.set_position(*self.pixel_to_float(self.contextmenupos))
		# if we have a context plugin, prepend connections
		if 'plugin' in kargs:
			plugin = kargs['plugin']
			inplugs = []
			# record all connections
			while True:
				conns = plugin.get_input_connection_list()
				if not conns:
					break
				conn = conns.pop()
				input = conn.get_input()
				for i in range(conn.get_output().get_input_connection_count()):
						if conn.get_output().get_input_connection(i)==conn:
							break
				try:
					aconn = conn.get_audio_connection()
					amp = aconn.get_amplitude()
					pan = aconn.get_panning()
					inplugs.append((input,amp,pan))
				except:
					import traceback
					print traceback.format_exc()
				plugin.delete_input(input)
			# restore
			for inplug,amp,pan in inplugs:
				mp.add_audio_input(inplug, amp, pan)
			plugin.add_audio_input(mp, 16384, 16384)
		# if we have a context connection, replace that one
		elif 'conn' in kargs:
			conn = kargs['conn']
			for i in range(conn.get_output().get_input_connection_count()):
					if conn.get_output().get_input_connection(i)==conn:
						break
			try:
				aconn = conn.get_audio_connection()
				amp = aconn.get_amplitude()
				pan = aconn.get_panning()
				minput = conn.get_input()
				moutput = conn.get_output()
				moutput.delete_input(minput)
				mp.add_audio_input(minput, amp, pan)
				moutput.add_audio_input(mp, 16384, 16384)
			except:
				import traceback
				print traceback.format_exc()
		player.history_commit("new plugin")
		self.rootwindow.document_changed()
		# add plugin information
		common.get_plugin_infos().add_plugin(mp)
		# open parameter view if its an effect
		if is_effect(mp):
			self.show_parameter_dialog(mp)
		
	def get_plugin_menu(self, include_generators = True, include_effects = True, include_controllers = True, **kargs):
		"""
		Generates and returns a new plugin menu.
		
		@return: A menu containing commands to instantiate new plugins.
		@rtype: wx.Menu
		"""
		cfg = com.get('aldrin.core.config')
		def make_submenu_item(submenu, name):
			item = gtk.MenuItem(label=name)
			item.set_submenu(submenu)
			return item
		def make_menu_item(label, desc, func, *args):
			item = gtk.ImageMenuItem(stock_id=label)
			if func:
				item.connect('activate', func, *args)
			return item
		def fill_menu(menu,node):
			add_separator = False
			for child in node.children:
				if child.is_directory() and not child.is_empty():
					if add_separator:
						add_separator = False
						if menu.get_children():
							menu.append(gtk.SeparatorMenuItem())
					submenu = gtk.Menu()
					fill_menu(submenu, child)
					menu.append(make_submenu_item(submenu, prepstr(child.name)))
				elif child.is_reference():
					if child.pluginloader:
						if not include_generators and is_generator(child.pluginloader):
							continue
						if not include_effects and is_effect(child.pluginloader):
							continue
						if not include_controllers and is_controller(child.pluginloader):
							continue
					if add_separator:
						add_separator = False
						if menu.get_children():
							menu.append(gtk.SeparatorMenuItem())
					item = make_menu_item(prepstr(child.name), "", self.on_popup_new_plugin, child.pluginloader, kargs)
					if child.icon:
						iconpath = cfg.get_plugin_icon_path(child.icon)
						if iconpath:
							image = gtk.Image()
							image.set_from_pixbuf(gtk.gdk.pixbuf_new_from_file(iconpath))
							item.set_image(image)
					if not child.pluginloader:
						item.set_sensitive(False)
					menu.append(item)
				elif child.is_separator():
					add_separator = True
		if 'menu' in kargs:
			plugin_menu = kargs['menu']
		else:
			plugin_menu = gtk.Menu()
		plugin_tree = com.get('aldrin.core.plugintree')
		fill_menu(plugin_menu, plugin_tree)
		plugin_menu.append(gtk.SeparatorMenuItem())
		plugin_menu.append(make_menu_item("Unmute All", "", self.on_popup_unmute_all))
		return plugin_menu
		
	def on_popup_unmute_all(self, widget):
		"""
		Event handler for unmute all menu option
		"""
		player = com.get('aldrin.core.player')
		for mp in reversed(player.get_plugin_list()):
			info = common.get_plugin_infos().get(mp)
			info.muted=False
			mp.set_mute(info.muted)
			info.reset_plugingfx()
		
	def on_popup_command(self, widget, plugin, subindex, index):
		"""
		Event handler for plugin commands
		"""
		plugin.command((subindex<<8) | index)
	
	def on_popup_set_target(self, widget, plugin):
		"""
		Event handler for menu option to set machine as target for default connection
		"""
		self.autoconnect_target = plugin

__aldrin__ = dict(
	classes = [
		ContextMenu,
		PluginContextMenu,
	],
)

