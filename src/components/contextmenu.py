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

import gtk
import aldrin.common as common
from aldrin.com import com
import zzub

from aldrin.utils import is_generator, is_root, is_controller, is_effect, \
	prepstr, Menu, new_theme_image
from aldrin.utils import PLUGIN_FLAGS_MASK, ROOT_PLUGIN_FLAGS, \
	GENERATOR_PLUGIN_FLAGS, EFFECT_PLUGIN_FLAGS, CONTROLLER_PLUGIN_FLAGS

class ContextMenu(Menu):
	__aldrin__ = dict(
		id = 'aldrin.core.contextmenu',
		singleton = False,
		categories = [
		],
	)
	
	def __init__(self, contextid, context):
		Menu.__init__(self)
		self.__context_id = contextid
		self.__context = context
		
	def get_context(self):
		return self.__context
		
	def get_context_id(self):
		return self.__context_id
		
	context = property(get_context)
	context_id = property(get_context_id)
	
	def add_submenu(self, label, submenu = None):
		if not submenu:
			submenu = ContextMenu(self.__context_id, self.__context)
		return Menu.add_submenu(self, label, submenu)
		
	def popup(self, parent, event=None):
		for item in self.get_children():
			item.destroy()
		for item in com.get_from_category('contextmenu.handler'):
			item.populate_contextmenu(self)
		return Menu.popup(self, parent, event)

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
			
	def populate_connectionmenu(self, menu):
		mp, index = menu.context
		menu.add_item("Disconnect plugins", self.on_popup_disconnect, mp, index)
		conntype = mp.get_input_connection_type(index)
		if conntype == zzub.zzub_connection_type_audio:
			menu.add_separator()
			menu.add_submenu("_Insert Effect",self.get_plugin_menu(include_generators=False,include_controllers=False, conn=(mp, index)))
			menu.add_separator()
			menu.add_item("_Signal Analysis", self.on_popup_show_signalanalysis, mp, index)
		elif conntype == zzub.zzub_connection_type_event:
			menu.add_separator()
			mi = conn.get_input()
			for param in mi.get_pluginloader().get_parameter_list(3):
				print param
		
	def populate_pluginmenu(self, menu):
		mp = menu.context
		player = com.get('aldrin.core.player')
		menu.add_check_item("_Mute", common.get_plugin_infos().get(mp).muted, self.on_popup_mute, mp)
		if is_generator(mp):
			menu.add_check_item("_Solo", player.solo_plugin == mp, self.on_popup_solo, mp)
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
			menu.add_check_item("Default Target",player.autoconnect_target == mp,self.on_popup_set_target, mp)
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

	def on_popup_rename(self, widget, mp):
		text = gettext(self, "Enter new plugin name:", prepstr(mp.get_name()))
		if text:
			mp.set_name(text)
			common.get_plugin_infos().get(mp).reset_plugingfx()
	
	def on_popup_solo(self, widget, mp):
		"""
		Event handler for the "Mute" context menu option.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		"""		
		player = com.get('aldrin.core.player')
		if player.solo_plugin != mp:
			player.solo(mp)
		else:
			player.solo(None)
	
	def on_popup_mute(self, widget, mp):
		"""
		Event handler for the "Mute" context menu option.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		"""
		player = com.get('aldrin.core.player')
		player.toggle_mute(mp)		
		
	def on_popup_delete(self, widget, mp):
		"""
		Event handler for the "Delete" context menu option.
		"""
		player = com.get('aldrin.core.player')
		player.delete_plugin(mp)
		
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
		manager = com.get('aldrin.core.parameterdialog.manager')
		manager.show(mp, widget)
		
	def on_popup_new_plugin(self, widget, pluginloader, kargs={}):
		"""
		Event handler for "new plugin" context menu options.
		"""
		player = com.get('aldrin.core.player')
		player.create_plugin(pluginloader)
		
	def get_plugin_menu(self, include_generators = True, include_effects = True, include_controllers = True, **kargs):
		"""
		Generates and returns a new plugin menu.
		
		@return: A menu containing commands to instantiate new plugins.
		@rtype: wx.Menu
		"""
		cfg = com.get('aldrin.core.config')
		def fill_menu(menu,node):
			add_separator = False
			for child in node.children:
				if child.is_directory() and not child.is_empty():
					if add_separator:
						add_separator = False
						if menu.get_children():
							menu.add_separator()
					submenu = Menu()
					fill_menu(submenu, child)
					menu.add_submenu(prepstr(child.name), submenu)
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
							menu.add_separator()
					if child.icon:
						image = new_theme_image(child.icon, gtk.ICON_SIZE_MENU)
						item = menu.add_image_item(prepstr(child.name), image, self.on_popup_new_plugin, child.pluginloader, kargs)
					else:
						item = menu.add_item(prepstr(child.name), self.on_popup_new_plugin, child.pluginloader, kargs)
					if not child.pluginloader:
						item.set_sensitive(False)
				elif child.is_separator():
					add_separator = True
		if 'menu' in kargs:
			plugin_menu = kargs['menu']
		else:
			plugin_menu = Menu()
		plugin_tree = com.get('aldrin.core.plugintree')
		fill_menu(plugin_menu, plugin_tree)
		plugin_menu.add_separator()
		plugin_menu.add_item("Unmute All", self.on_popup_unmute_all)
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

