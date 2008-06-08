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

import sys, os
from gtkimport import gtk
from utils import format_time, ticks_to_time, prepstr, linear2db, db2linear, filepath, \
	is_debug, question, error, add_scrollbars, file_filter, new_stock_image_toggle_button, \
	new_stock_image_button, message
import zzub
import gobject
import time
import common
import config
import errordlg
from common import MARGIN, MARGIN2, MARGIN3, MARGIN0
import driver
import sequencer, router, patterns, wavetable, preferences, hdrecorder, cpumonitor, info, common, rack
from utils import make_submenu_item, make_stock_menu_item, make_stock_tool_item, make_stock_toggle_item, \
	make_stock_radio_item, make_menu_item, make_check_item, make_radio_item

from sequencer import SequencerPanel
from router import RoutePanel
from patterns import PatternPanel
from wavetable import WavetablePanel
from rack import RackPanel
from info import InfoPanel
from preferences import show_preferences

from utils import CancelException
from config import get_plugin_aliases, get_plugin_blacklist

from aldrincom import com

class AldrinFrame(gtk.Window):
	"""
	The application main window class.
	"""
	
	__aldrin__ = dict(
		id = 'aldrin.core.window.root',
		singleton = True,
		categories = [
			'rootwindow',
		],
	)

	OPEN_SONG_FILTER = [
		file_filter("All songs (*.ccm,*.bmw,*.bmx)", "*.ccm", "*.bmw", "*.bmx"),
		file_filter("CCM Songs (*.ccm)", "*.ccm"),
		file_filter("BMX Songs with waves (*.bmx)","*.bmx"),
		file_filter("BMX Songs without waves (*.bmw)","*.bmw"),
	]
	
	SAVE_SONG_FILTER = [
		file_filter("CCM Songs (*.ccm)","*.ccm"),
	]
	
	DEFAULT_EXTENSION = '.ccm'
	
	title = "Aldrin"
	filename = ""

	event_to_name = dict([(getattr(zzub,x),x) for x in dir(zzub) if x.startswith('zzub_event_type_')])
	
	# IRootWindow.refresh_view
	def refresh_view(self, target):
		"""
		Refreshes a view.
		
		@param target: The uri of the target.
		@type target: str
		"""
		if target == interface.UIVIEW_ALL:
			self.document_changed()

		
	def __init__(self):
		"""
		Initializer.
		"""
		self._cbtime = time.time()
		self._cbcalls = 0
		self._hevcalls = 0
		self._hevtimes = 0		
		self.event_handlers = []
		
		player = com.get('aldrin.core.player')
		player.set_callback(self.player_callback)
		
		gtk.Window.__init__(self, gtk.WINDOW_TOPLEVEL)
		errordlg.install(self)
		self.set_geometry_hints(self,600,400)
		self.set_position(gtk.WIN_POS_CENTER)
		audiotrouble = False
		try:
			driver.get_audiodriver().init()
		except driver.AudioInitException:
			import traceback
			traceback.print_exc()
			audiotrouble = True
		miditrouble = False
		try:
			driver.get_mididriver().init()
		except driver.MidiInitException:
			import traceback
			traceback.print_exc()
			miditrouble = True
		
		self.cpumonitor = cpumonitor.CPUMonitorDialog(self)
		self.cpumonitor.connect('delete-event', self.on_close_cpumonitor)
		self.cpumonitor.realize()
		self.hdrecorder = hdrecorder.HDRecorderDialog(self)
		self.hdrecorder.connect('delete-event', self.on_close_hdrecorder)
		self.hdrecorder.realize()

		self.open_dlg = gtk.FileChooserDialog(title="Open", parent=self, action=gtk.FILE_CHOOSER_ACTION_OPEN,
			buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_OPEN, gtk.RESPONSE_OK)
		)
		self.open_dlg.add_shortcut_folder(filepath('demosongs'))
		for filter in self.OPEN_SONG_FILTER:
			self.open_dlg.add_filter(filter)
		self.save_dlg = gtk.FileChooserDialog(title="Save", parent=self, action=gtk.FILE_CHOOSER_ACTION_SAVE,
			buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_SAVE, gtk.RESPONSE_OK)
		)
		self.save_dlg.set_do_overwrite_confirmation(True)
		for filter in self.SAVE_SONG_FILTER:
			self.save_dlg.add_filter(filter)

		vbox = gtk.VBox()
		self.add(vbox)


		self.accelerators = gtk.AccelGroup()
		self.add_accel_group(self.accelerators)		
		
		# Menu Bar
		self.aldrinframe_menubar = gtk.MenuBar()
		vbox.pack_start(self.aldrinframe_menubar, expand=False)
		self.filemenu = gtk.Menu()
		self.aldrinframe_menubar.append(make_submenu_item(self.filemenu, "_File"))
		self.update_filemenu()
		
		tempmenu = gtk.Menu()
		tempmenu.append(make_stock_menu_item(gtk.STOCK_CUT, self.on_cut))
		tempmenu.append(make_stock_menu_item(gtk.STOCK_COPY, self.on_copy))
		tempmenu.append(make_stock_menu_item(gtk.STOCK_PASTE, self.on_paste))
		tempmenu.append(gtk.SeparatorMenuItem())
		tempmenu.append(make_stock_menu_item(gtk.STOCK_PREFERENCES, self.on_preferences))
		self.aldrinframe_menubar.append(make_submenu_item(tempmenu, "_Edit"))
		tempmenu = gtk.Menu()
		self.item_cpumon = make_check_item("_CPU monitor", "Show or hide CPU Monitor", self.on_toggle_cpu_monitor)
		tempmenu.append(self.item_cpumon)
		self.item_hdrec = make_check_item("Hard Disk Recorder", "Show or hide Hard Disk Recorder", self.on_toggle_hard_disk_recorder)
		tempmenu.append(self.item_hdrec)
		self.item_master = make_check_item("_Master", "Show or hide Master", self.on_toggle_mastertoolbar)
		tempmenu.append(self.item_master)
		self.item_transport = make_check_item("_Transport", "Show or hide transport bar", self.on_toggle_transport)
		tempmenu.append(self.item_transport)
		self.item_statusbar = make_check_item("_Status Bar", "Show or hide the status bar", self.on_toggle_statusbar)
		tempmenu.append(self.item_statusbar)
		tempmenu.append(make_check_item("S_kins", "Show or hide custom machine skins", None))
		self.item_standard = make_check_item("_Standard", "Show or hide the standard toolbar", self.on_toggle_toolbar)
		tempmenu.append(self.item_standard)
		self.viewmenu = tempmenu
		tempsubmenu = gtk.Menu()
		defaultitem = gtk.RadioMenuItem(label="Default")
		tempsubmenu.append(defaultitem)
		self.thememenu = tempsubmenu
		cfg = config.get_config()
		if not cfg.get_active_theme():
			defaultitem.set_active(True)
		defaultitem.connect('toggled', self.on_select_theme, None)
		for name in sorted(cfg.get_theme_names()):
			item = gtk.RadioMenuItem(label=prepstr(name), group=defaultitem)
			if name == cfg.get_active_theme():
				item.set_active(True)
			item.connect('toggled', self.on_select_theme, name)
			tempsubmenu.append(item)
		tempmenu.append(make_submenu_item(tempsubmenu, "Themes"))
		self.aldrinframe_menubar.append(make_submenu_item(tempmenu, "_View"))
		self.toolsmenu = gtk.Menu()
		item = make_submenu_item(self.toolsmenu, "_Tools")
		self.aldrinframe_menubar.append(item)
		added = False
		toolitems = com.get_from_category('menuitem.tool', self.toolsmenu)
		if not toolitems:
			item.destroy()
		tempmenu = gtk.Menu()
		tempmenu.append(make_stock_menu_item(gtk.STOCK_HELP, self.on_help_contents))
		tempmenu.append(gtk.SeparatorMenuItem())
		tempmenu.append(make_stock_menu_item(gtk.STOCK_ABOUT, self.on_about))
		self.aldrinframe_menubar.append(make_submenu_item(tempmenu, "_Help"))
		#~ # Menu Bar end

		# Tool Bar
		def def_bmp(i):
			return get_stock_bmp(i)
		self.aldrinframe_toolbar = gtk.Toolbar()
		vbox.pack_start(self.aldrinframe_toolbar, expand=False)
		
		def cmp_panel(a,b):
			a_order = (hasattr(a, '__view__') and a.__view__.get('order',0)) or 0
			b_order = (hasattr(b, '__view__') and b.__view__.get('order',0)) or 0
			return cmp(a_order, b_order)
		self.pages = sorted(com.get_from_category('aldrin.viewpanel', self), cmp=cmp_panel)
		self.aldrinframe_toolbar.insert(make_stock_tool_item(gtk.STOCK_NEW, self.new),-1)
		self.aldrinframe_toolbar.insert(make_stock_tool_item(gtk.STOCK_OPEN, self.on_open),-1)
		self.aldrinframe_toolbar.insert(make_stock_tool_item(gtk.STOCK_SAVE, self.on_save),-1)
		extrasep = gtk.SeparatorToolItem()
		self.aldrinframe_toolbar.insert(extrasep,-1)
		if not com.get_from_category('menuitem.toolbar', self.aldrinframe_toolbar):
			extrasep.destroy()

		self.mastertoolbar = com.get('aldrin.core.panel.master', self)
		self.transport = com.get('aldrin.core.panel.transport', self)
		
		for name in ('btnplay','btnrecord','btnstop','btnloop','btnpanic'):
			setattr(self, name, getattr(self.transport, name))
			
		self.btnplay.connect('clicked', self.play)
		self.btnrecord.connect('clicked', self.on_toggle_automation)
		self.btnstop.connect('clicked', self.stop)
		self.btnloop.connect('clicked', self.on_toggle_loop)
		self.btnpanic.connect('clicked', self.on_toggle_panic)

		self.framepanel = gtk.Notebook()
		self.framepanel.set_tab_pos(gtk.POS_LEFT)
		self.framepanel.set_show_border(False)
		#self.framepanel.set_show_tabs(False)
		
		icons = com.get("aldrin.core.icons")
		defaultpanelindex = -1
		for index,panel in enumerate(self.pages):
			if not hasattr(panel, '__view__'):
				print "panel",panel,"misses attribute __view__"
				continue
			options = panel.__view__
			stockid = options['stockid']
			label = options['label']
			key = options.get('shortcut', '')			
			if options.get('default'):
				defaultpanelindex = index
			icons.register_single(stockid=stockid, label=label, key=key, iconset=stockid)
			panel.show_all()
			header = gtk.VBox()
			labelwidget = gtk.Label(label)
			labelwidget.set_angle(90)
			header.pack_start(labelwidget)
			header.pack_start(gtk.image_new_from_stock(stockid, gtk.ICON_SIZE_SMALL_TOOLBAR))
			header.show_all()
			if key:
				header.set_tooltip_text("%s (%s)" % (label, key))
			else:
				header.set_tooltip_text(label)
			self.framepanel.append_page(panel, header)
		
		hbox = gtk.HBox()
		hbox.add(self.framepanel)
		hbox.pack_end(self.mastertoolbar, expand=False)
		vbox.add(hbox)
	
		self.aldrinframe_statusbar = gtk.Statusbar()
		
		vbox.pack_start(self.transport, expand=False)
		vbox.pack_end(self.aldrinframe_statusbar, expand=False)

		self.set_title(self.title)
		gtk.window_set_default_icon_list(
			gtk.gdk.pixbuf_new_from_file(filepath("../icons/hicolor/48x48/apps/aldrin.png")),
			gtk.gdk.pixbuf_new_from_file(filepath("../icons/hicolor/32x32/apps/aldrin.png")),
			gtk.gdk.pixbuf_new_from_file(filepath("../icons/hicolor/24x24/apps/aldrin.png")),
			gtk.gdk.pixbuf_new_from_file(filepath("../icons/hicolor/22x22/apps/aldrin.png")),
			gtk.gdk.pixbuf_new_from_file(filepath("../icons/hicolor/16x16/apps/aldrin.png")))
		self.resize(750, 550)
		self.aldrinframe_statusbar.push(0, "Ready to rok again")

		self.connect('key-press-event', self.on_key_down)
		self.connect('destroy', self.on_destroy)
		self.connect('delete-event', self.on_close)

		if defaultpanelindex != -1:
			self.framepanel.set_current_page(defaultpanelindex)
		
		gobject.timeout_add(1000/25, self.on_handle_events)
		self.framepanel.connect('switch-page', self.on_activate_page)
		self.framepanel.connect('button-release-event', self.button_up)
		self.activated=0
		
		self.document_changed()
		self.show_all()
		self.load_view()
		
		eventbus = com.get('aldrin.core.eventbus')
		eventbus.print_mapping()

		import sys
		options, args = com.get('aldrin.core.options').get_options_args()
		if len(args) > 1:
			self.open_file(args[1])
		if audiotrouble:
			error(self, "<b><big>Aldrin tried to guess an audio driver but that didn't work.</big></b>\n\nYou need to select your own. Hit OK to show the preferences dialog.")
			show_preferences(self,self)

	def update_filemenu(self, event=None):
		"""
		Updates the most recent files in the file menu.
		
		@param event: Event.
		@type event: Event
		"""
		for item in self.filemenu:
			item.destroy()
		self.filemenu.append(make_stock_menu_item(gtk.STOCK_NEW, self.new, frame=self, shortcut="<Control>N"))
		self.filemenu.append(make_stock_menu_item(gtk.STOCK_OPEN, self.on_open, frame=self, shortcut="<Control>O"))
		self.filemenu.append(make_stock_menu_item(gtk.STOCK_SAVE, self.on_save, frame=self, shortcut="<Control>S"))
		self.filemenu.append(make_stock_menu_item(gtk.STOCK_SAVE_AS, self.on_save_as))
		recent_files = config.get_config().get_recent_files_config()
		if recent_files:
			self.filemenu.append(gtk.SeparatorMenuItem())
			for i,filename in enumerate(recent_files):
					filetitle=os.path.basename(filename).replace("_","__")
					self.filemenu.append(make_menu_item("_%i %s" % (i+1,filetitle), "", self.open_recent_file, filename))
		self.filemenu.append(gtk.SeparatorMenuItem())
		self.filemenu.append(make_stock_menu_item(gtk.STOCK_QUIT, self.on_exit))
		self.filemenu.show_all()

	def player_callback(self, player, plugin, data):
		"""
		Default callback for ui events sent by zzub.
		
		@param player: player instance.
		@type player: zzub.Player
		@param plugin: plugin instance
		@type plugin: zzub.Plugin
		@param data: event data.
		@type data: zzub_event_data_t
		"""
		result = False
		for handler in self.event_handlers:
			result = handler(player,plugin,data) or result
		if data.type == zzub.zzub_event_type_player_state_changed:
			state = getattr(data,'').player_state_changed.player_state
			if state == zzub.zzub_player_state_playing:
				self.btnplay.set_active(True)
			else:
				self.btnplay.set_active(False)
		self._cbcalls += 1
		return result
		
	def get_active_view(self):
		"""
		Returns the active panel view.
		"""
		for pindex,(ctrlid,(panel,menuitem)) in self.pages.iteritems():
			if panel.window and panel.window.is_visible() and hasattr(panel,'view'):
				return getattr(panel,'view')

	def on_copy(self, event):
		"""
		Sent when the copy function is selected from the menu.
		
		@param event: Menu event.
		@type event: MenuEvent
		"""
		view = self.get_active_view()
		if view and hasattr(view, 'on_copy'):
			view.on_copy(event)

	def on_cut(self, event):
		"""
		Sent when the cut function is selected from the menu.
		
		@param event: Menu event.
		@type event: MenuEvent
		"""
		view = self.get_active_view()
		if view and hasattr(view, 'on_cut'):
			view.on_cut(event)

	def on_paste(self, event):
		"""
		Sent when the paste function is selected from the menu.
		
		@param event: Menu event.
		@type event: MenuEvent
		"""
		view = self.get_active_view()
		if view and hasattr(view, 'on_paste'):
			view.on_paste(event)
		
	def load_view(self):
		"""
		Called to load view settings from config
		"""
		cfg = config.get_config()
		cfg.load_window_pos("MainFrameWindow", self)
		cfg.load_window_pos("Toolbar", self.aldrinframe_toolbar)
		cfg.load_window_pos("MasterToolbar", self.mastertoolbar)
		cfg.load_window_pos("Transport", self.transport)
		cfg.load_window_pos("StatusBar", self.aldrinframe_statusbar)
		
		self.update_view()
		
	def save_view(self):
		"""
		Called to store view settings to config
		"""
		cfg = config.get_config()
		cfg.save_window_pos("MainFrameWindow", self)
		cfg.save_window_pos("Toolbar", self.aldrinframe_toolbar)
		cfg.save_window_pos("MasterToolbar", self.mastertoolbar)
		cfg.save_window_pos("Transport", self.transport)
		cfg.save_window_pos("StatusBar", self.aldrinframe_statusbar)
		
	def update_view(self):
		"""
		Called to update all viewstates.
		"""
		self.item_hdrec.set_active(self.hdrecorder.get_property('visible'))
		self.item_cpumon.set_active(self.cpumonitor.get_property('visible'))
		self.item_master.set_active(self.mastertoolbar.get_property('visible'))
		self.item_statusbar.set_active(self.aldrinframe_statusbar.get_property('visible'))
		self.item_standard.set_active(self.aldrinframe_toolbar.get_property('visible'))
		self.item_transport.set_active(self.transport.get_property('visible'))
		self.save_view()
		
	def on_close_cpumonitor(self, widget, event):
		"""
		Called when the cpu monitor is closed manually.
		
		@param event: event.
		@type event: Event
		"""
		self.cpumonitor.hide_all()
		self.update_view()
		return True
		
	def on_close_hdrecorder(self, widget, event):
		"""
		Called when the hd recorder is closed manually.
		"""
		self.hdrecorder.hide_all()
		self.update_view()
		return True
		
	def on_toggle_cpu_monitor(self, widget):
		"""
		Handler triggered by the "Toggle CPU Monitor" menu option.
		Shows and hides the CPU Monitor panel.
		"""
		if widget.get_active():
			self.cpumonitor.show_all()
		else:
			self.cpumonitor.hide_all()

	def on_toggle_hard_disk_recorder(self, widget):
		"""
		Handler triggered by the "Toggle Hard Disk Recorder" menu option.
		Shows and hides the Hard Disk Recorder panel.
		"""
		if widget.get_active():
			self.hdrecorder.show_all()
		else:
			self.hdrecorder.hide_all()

	def show_toolbar(self, enable):
		"""
		Shows or hides the toolbar.
		
		@param enable: If True, toolbar will be shown.
		@type enable: bool
		"""
		if enable:
			self.aldrinframe_toolbar.show_all()
		else:
			self.aldrinframe_toolbar.hide_all()
		self.update_view()
		
	def on_toggle_toolbar(self, widget):
		"""
		Handler triggered by the "Toggle Default" menu option.
		Shows and hides the toolbar.
		
		@param event: command event.
		@type event: CommandEvent
		"""
		self.show_toolbar(widget.get_active())

	def show_statusbar(self, enable):
		"""
		Shows or hides the status bar.
		
		@param enable: If True, status bar will be shown.
		@type enable: bool
		"""
		if enable:
			self.aldrinframe_statusbar.show_all()
		else:
			self.aldrinframe_statusbar.hide_all()
		self.update_view()
		
	def on_toggle_statusbar(self, widget):
		"""
		Handler triggered by the "Toggle Status Bar" menu option.
		Shows and hides the status bar.
		
		@param event: command event.
		@type event: CommandEvent
		"""
		self.show_statusbar(widget.get_active())

	def show_mastertoolbar(self, enable):
		"""
		Shows or hides the mastertoolbar.
		
		@param enable: If True, toolbar will be shown.
		@type enable: bool
		"""
		if enable:
			self.mastertoolbar.show_all()
		else:
			self.mastertoolbar.hide_all()
		self.update_view()
		
	def on_toggle_transport(self, widget):
		"""
		Handler triggered by the "Toggle Master Toolbar" menu option.
		Shows and hides the master toolbar.
		
		@param event: command event.
		@type event: CommandEvent
		"""
		if widget.get_active():
			self.transport.show_all()
		else:
			self.transport.hide_all()
		self.update_view()

	def on_toggle_mastertoolbar(self, widget):
		"""
		Handler triggered by the "Toggle Master Toolbar" menu option.
		Shows and hides the master toolbar.
		
		@param event: command event.
		@type event: CommandEvent
		"""
		self.show_mastertoolbar(widget.get_active())
		
	def on_toggle_automation(self, widget):
		"""
		handler triggered by the record toolbar button. Decides whether
		changes to parameters are recorded or not.
		
		@param event: Command event.
		@type event: CommandEvent
		"""
		player = com.get('aldrin.core.player')
		if widget.get_active():
			player.set_automation(1)
		else:
			player.set_automation(0)
		self.mastertoolbar.button_up(1,1)

	def on_toggle_automation_accel(self, event):
		"""
		Handler triggered by the f7 accellerator. Enables/disables
		automation.
		
		@param event command event.
		@type event: CommandEvent
		"""
		player = com.get('aldrin.core.player')
		if not self.aldrinframe_toolbar.GetToolState(self.RECORD):
			self.aldrinframe_toolbar.ToggleTool(self.RECORD, True)
			player.set_automation(1)
		else:
			self.aldrinframe_toolbar.ToggleTool(self.RECORD, False)
			player.set_automation(0)
		
	def on_toggle_loop(self, widget):
		"""
		Handler triggered by the loop toolbar button. Decides whether
		the song loops or not.
		
		@param event command event.
		@type event: CommandEvent
		"""
		player = com.get('aldrin.core.player')
		if widget.get_active():
			player.set_loop_enabled(1)
		else:
			player.set_loop_enabled(0)
		self.mastertoolbar.button_up(1,1)
			
	def on_toggle_panic(self, widget):
		"""
		Handler triggered by the mute toolbar button. Deinits/reinits
		sound device.
		
		@param event command event.
		@type event: CommandEvent
		"""
		player = com.get('aldrin.core.player')
		if widget.get_active():
			player.audiodriver_enable(0)
		else:
			player.audiodriver_enable(1)
		self.mastertoolbar.button_up(1,1)

	def on_toggle_panic_accel(self, event):
		"""
		Handler triggered by the f12 accellerator. Deinits/reinits
		sound device.
		
		@param event command event.
		@type event: CommandEvent
		"""
		player = com.get('aldrin.core.player')
		if not self.aldrinframe_toolbar.GetToolState(self.PANIC):
			self.aldrinframe_toolbar.ToggleTool(self.PANIC, True)
			player.audiodriver_enable(0)
		else:
			self.aldrinframe_toolbar.ToggleTool(self.PANIC, False)
			player.audiodriver_enable(1)


	def on_handle_events(self):
		"""
		Handler triggered by the default timer. Calls player.handle_events()
		to work off the players message queue.
		"""
		player = com.get('aldrin.core.player')
		t1 = time.time()
		player.handle_events()
		t2 = time.time() - t1
		self._hevtimes = (self._hevtimes * 0.9) + (t2 * 0.1)
		self._hevcalls += 1
		t = time.time()
		if (t - self._cbtime) > 1:
			#print self._hevcalls, self._cbcalls, "%.2fms" % (self._hevtimes*1000)
			self._cbcalls = 0
			self._hevcalls = 0
			self._cbtime = t
		#called only if loop pattern is off when song ends:
		if player.get_state() != zzub.zzub_player_state_playing and self.btnplay.get_active():
			self.btnplay.set_active(False)
		return True
		
	def on_help_contents(self, event):
		"""
		Event handler triggered by the help menu option.
		
		@param event: menu event.
		@type event: MenuEvent
		"""
		import webbrowser		
		webbrowser.open_new(filepath('../doc/aldrin/html/index.html'))
			
	def on_about(self, event):
		"""
		Event handler triggered by the "About" menu option.
		
		@param event: menu event.
		@type event: MenuEvent
		"""
		com.get('aldrin.core.dialog.about', self).show()
			
	def on_framepanel_size(self, event):
		"""
		Event handler that resizes the client panels, when the main window
		is being resized.
		
		@param event: size event.
		@type event: SizeEvent
		"""
		#~ x,y,w,h = self.framepanel.GetClientRect()
		#~ for ctrlid, panel in self.pages.values():
			#~ panel.SetRect((x,y,w,h))
			
	def get_current_panel(self):
		return self.pages[self.framepanel.get_current_page()]
		
	def select_panel(self, panel):
		for index,page in enumerate(self.pages):
			if page == panel:
				self.select_page(index)
				return
			
	def select_page(self, index):
		"""
		Selects a client panel. If the client panel has a view attribute,
		that view attribute is being interpreted as a window and will be
		focused, else the panel itself will be focused.
		
		@param index: Index of the panel (use one of the self.PAGE_* constants)
		@type index: int
		"""
		if not(self.activated):
			self.activated=1
			self.index=index
			panel = self.pages[self.index]
			if self.framepanel.get_current_page() != self.index:
				self.framepanel.set_current_page(self.index)
			panel.handle_focus()
		self.activated=0
		
	def button_up(self, widget, event):
		"""
		selects panel after button up
		"""
		panel = self.get_current_panel()		
		panel.handle_focus()
			
	def on_preferences(self, event):
		"""
		Event handler triggered by the "Preferences" menu option.
		
		@param event: menu event.
		@type event: MenuEvent
		"""
		show_preferences(self,self)

	def on_key_down(self, widget, event):
		"""
		Event handler for key events.
		"""
		kv = event.keyval
		k = gtk.gdk.keyval_name(event.keyval)
		for index,panel in enumerate(self.pages):
			if not hasattr(panel, '__view__'):
				continue
			shortcut = panel.__view__.get('shortcut', '')
			if not shortcut:
				continue
			if k == shortcut:
				self.select_page(index)
				return True
		if k == 'F5':
			self.btnplay.set_active(True)
		elif k == 'F6':
			self.play_from_cursor(event)
		elif k == 'F7':
			self.btnrecord.set_active(not self.btnrecord.get_active())
		elif k == 'F8':
			self.stop(event)
		elif k == 'F12':
			self.btnpanic.set_active(not self.btnpanic.get_active())
		else:
			return False
		return True
			
	def on_activate_page(self, notebook, page, page_num):
		self.select_page(page_num)
			

	def open_recent_file(self, widget, filename):
		"""
		Event handler triggered by recent file menu options.
		
		@param event: menu event.
		@type event: MenuEvent
		"""
		try:
			self.save_changes()
			self.open_file(filename)
		except CancelException:
			pass

	def document_changed(self):
		"""
		Event handler triggered when the document has changed. You should
		call this on occasions where the entire document has changed, else
		there are specialized handlers in the panel classes.
		"""
		common.get_plugin_infos().update()
		for panel in self.pages:
			if hasattr(panel, 'update_all'):
				panel.update_all()
		self.mastertoolbar.update_all()
		self.transport.update_all()
		player = com.get('aldrin.core.player')
		self.btnloop.set_active(player.get_loop_enabled())
		self.btnrecord.set_active(player.get_automation())
		self.select_page(self.framepanel.get_current_page())
		
	def update_title(self):
		"""
		Updates the title to display the filename of the currently
		loaded document.
		"""
		self.set_title("%s - %s" % (self.title, os.path.basename(self.filename)))
		
	def open_file(self, filename):
		"""
		Loads a song from disk. The old document will be wiped, and
		the song will be added to the recent file list.
		
		@param filename: Path to song.
		@type filename: str
		"""
		if not os.path.isfile(filename):
			return
		self.clear()
		self.filename = filename
		base,ext = os.path.splitext(self.filename)
		player = com.get('aldrin.core.player')
		if ext.lower() in ('.bmx','.bmw'):
			#~ progress = ProgressDialog("Aldrin", "Loading BMX Song...")
			#~ Yield()
			player.load_bmx(self.filename)
			#~ Yield()
			#~ progress.Update(100)
		elif ext.lower() in ('.ccm'):
			# XXX: TODO
			dlg = gtk.Dialog('Aldrin', flags=gtk.DIALOG_MODAL)
			progBar = gtk.ProgressBar()
			progBar.set_text('Loading CCM Song...')
			progBar.set_size_request(300, 40)
			progBar.set_fraction(0)
			progBar.show()
			dlg.vbox.pack_start(progBar)			
			dlg.show()
			while gtk.events_pending():
				gtk.main_iteration()
			def progress_callback():	
				progBar.pulse()
				while gtk.events_pending():
					gtk.main_iteration()
				return  progBar.get_fraction() == 1.0
			progBar.pulse()
			while gtk.events_pending():
				gtk.main_iteration()
			gobject.timeout_add(1000/25, progress_callback)
			progBar.pulse()
			player.load_ccm(self.filename)
			progBar.set_fraction(1.0)
			dlg.destroy()
		else:
			message(self, "'%s' is not a supported file format." % ext)
			return
		self.update_title()
		config.get_config().add_recent_file_config(self.filename)
		self.btnplay.set_active(False)
		self.update_filemenu()
		self.document_changed()
		
	def save_file(self, filename):
		"""
		Saves a song to disk. The document will also be added to the
		recent file list.
		
		@param filename: Path to song.
		@type filename: str
		"""
		player = com.get('aldrin.core.player')
		try:
			if not os.path.splitext(filename)[1]:
				filename += self.DEFAULT_EXTENSION
			self.filename = filename
			if os.path.isfile(filename):
				if config.get_config().get_incremental_saving():
					# rename incremental
					path,basename = os.path.split(filename)
					basename,ext = os.path.splitext(basename)
					i = 0
					while True:
						newpath = os.path.join(path,"%s%s.%03i.bak" % (basename,ext,i))
						if not os.path.isfile(newpath):
							break
						i += 1
					print '%s => %s' % (filename, newpath)
					os.rename(filename, newpath)
				else:
					# store one backup copy
					path,basename = os.path.split(filename)
					basename,ext = os.path.splitext(basename)
					newpath = os.path.join(path,"%s%s.bak" % (basename,ext))
					if os.path.isfile(newpath):
						os.remove(newpath)
					print '%s => %s' % (filename, newpath)
					os.rename(filename, newpath)
			base,ext = os.path.splitext(self.filename)
			#~ progress = ProgressDialog("Aldrin", "Saving '%s'..." % prepstr(self.filename))
			#~ Yield()
			assert player.save_ccm(self.filename) == 0
		except:
			import traceback
			text = traceback.format_exc()
			traceback.print_exc()
			error(self, "<b><big>Error saving file:</big></b>\n\n%s" % text)
		#~ progress.Update(100)
		self.update_title()
		config.get_config().add_recent_file_config(self.filename)
		self.update_filemenu()
		
	def on_open(self, event):
		"""
		Event handler triggered by the "Open File" menu option.
		
		@param event: menu event.
		@type event: MenuEvent
		"""
		try:
			self.save_changes()
			self.open()
		except CancelException:
			pass

	def open(self):
		"""
		Shows the open file dialog and if successful, loads the
		selected song from disk.
		"""
		response = self.open_dlg.run()
		self.open_dlg.hide()
		if response == gtk.RESPONSE_OK:
			self.open_file(self.open_dlg.get_filename())
			
	def on_save(self, event):
		"""
		Event handler triggered by the "Save" menu option.
		"""
		try:
			self.save()
		except CancelException:
			pass
			
	def save(self):
		"""
		Shows a save file dialog if filename is unknown and saves the file.
		"""
		if not self.filename:
			self.save_as()
		else:
			self.save_file(self.filename)
		
	def save_as(self):
		"""
		Shows a save file dialog and saves the file.
		"""
		self.save_dlg.set_filename(self.filename)
		response = self.save_dlg.run()
		self.save_dlg.hide()
		if response == gtk.RESPONSE_OK:
			filepath = self.save_dlg.get_filename()
			self.save_file(filepath)
		else:
			raise CancelException
		
	def on_save_as(self, event):
		"""
		Event handler triggered by the "Save As" menu option.
		
		@param event: menu event.
		@type event: MenuEvent
		"""
		try:
			self.save_as()
		except CancelException:
			pass
		
	def clear(self):
		"""
		Clears the current document.
		"""
		self.filename = ""
		common.get_plugin_infos().reset()
		for panel in self.pages:
			if hasattr(panel, 'reset'):
				panel.reset()
		player = com.get('aldrin.core.player')
		player.clear()
		player.set_loop_start(0)
		player.set_loop_end(com.get('aldrin.core.sequencerpanel').view.step)
		player.get_plugin(0).set_parameter_value(1, 0, 1, config.get_config().get_default_int('BPM', 126), 1)
		player.get_plugin(0).set_parameter_value(1, 0, 2, config.get_config().get_default_int('TPB', 4), 1)
		self.document_changed()
		self.update_title()
		
	def play(self, widget):
		"""
		Event handler triggered by the "Play" toolbar button.
		
		@param event: menu event.
		@type event: MenuEvent
		"""
		player = com.get('aldrin.core.player')
		if self.btnplay.get_active():
			player.playstarttime = time.time()
			player.play()
		elif player.get_state() == zzub.zzub_player_state_playing:
			# keep on
			self.btnplay.set_active(True)
		self.mastertoolbar.button_up(1,1)

	def play_from_cursor(self, event):
		"""
		Event handler triggered by the F6 key.
		
		@param event: menu event.
		@type event: MenuEvent
		"""
		if not self.btnplay.get_active():
			self.btnplay.set_active(True)
		player = com.get('aldrin.core.player')
		player.playstarttime = time.time()
		player.set_position(max(com.get('aldrin.core.sequencerpanel').view.row,0))
		player.play()		
		
	def on_select_theme(self, widget, data):
		"""
		Event handler for theme radio menu items.
		
		@param event: menu event.
		@type event: MenuEvent
		"""
		cfg = config.get_config()
		if not data:
			cfg.select_theme(None)
		else:
			cfg.select_theme(data)
		self.document_changed()
		
	def stop(self, event):
		"""
		Event handler triggered by the "Stop" toolbar button.
		
		@param event: menu event.
		@type event: MenuEvent
		"""
		player = com.get('aldrin.core.player')
		player.stop()
		if self.btnplay.get_active() == False:
			player.set_position(0)
		else:
			self.btnplay.set_active(False)
		self.mastertoolbar.button_up(1,1)
		
	def save_changes(self):
		"""
		Asks whether to save changes or not. Throws a {CancelException} if
		cancelled.
		"""
		if self.filename:
			text = "<big><b>Save changes to <i>%s</i>?</b></big>" % os.path.basename(self.filename)
		else:
			text = "<big><b>Save changes?</b></big>"
		response = question(self, text)
		if response == int(gtk.RESPONSE_CANCEL) or response == int(gtk.RESPONSE_DELETE_EVENT):
			raise CancelException
		elif response == int(gtk.RESPONSE_YES):
			self.save()

	def new(self, event):
		"""
		Event handler triggered by the "New" menu option.
		
		@param event: menu event.
		@type event: MenuEvent
		"""
		try:
			self.save_changes()
			self.set_title(self.title)
			self.clear()
		except CancelException:
			pass
			
	def on_destroy(self, widget):
		"""
		Event handler triggered when the window is being destroyed.
		"""
		gtk.main_quit()
			
	def on_exit(self, widget):
		"""
		Event handler triggered by the "Exit" menu option.
		
		@param event: menu event.
		@type event: MenuEvent
		"""
		if not self.on_close(None, None):
			self.destroy()
			
	def on_close(self, widget, event):
		"""
		Event handler triggered when the window is being closed.
		"""
		self.save_view()
		try:
			self.save_changes()
			self.hide_all()
			return False
		except CancelException:
			return True
					

	#########################
	# IMainFrame interface
	#########################
	
	def get_window(self):
		"""
		Returns the window object associated with the mainframe.
		
		@return: Window object.
		@rtype: Window
		"""
		return self


__aldrin__ = dict(
	classes = [
		AldrinFrame,
	],
)

if __name__ == '__main__':
	pass