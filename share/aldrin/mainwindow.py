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

import os
from gtkimport import gtk
from utils import format_time, ticks_to_time, prepstr, linear2db, db2linear, filepath, \
	is_debug, question, error, add_scrollbars, file_filter, new_stock_image_toggle_button, \
	new_stock_image_button, message
import zzub
import gobject
import config
import errordlg

import common
MARGIN = common.MARGIN
MARGIN2 = common.MARGIN2
MARGIN3 = common.MARGIN3
MARGIN0 = common.MARGIN0

import hdrecorder, cpumonitor

from utils import make_submenu_item, make_stock_menu_item, make_stock_tool_item, make_stock_toggle_item, \
	make_stock_radio_item, make_menu_item, make_check_item, make_radio_item, new_theme_image, add_accelerator

import preferences
show_preferences = preferences.show_preferences

from utils import CancelException

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
	
	def __init__(self):
		"""
		Initializer.
		"""
		
		gtk.Window.__init__(self, gtk.WINDOW_TOPLEVEL)
		errordlg.install(self)
		self.set_geometry_hints(self,600,400)
		self.set_position(gtk.WIN_POS_CENTER)
		
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
		for filefilter in self.OPEN_SONG_FILTER:
			self.open_dlg.add_filter(filefilter)
		self.save_dlg = gtk.FileChooserDialog(title="Save", parent=self, action=gtk.FILE_CHOOSER_ACTION_SAVE,
			buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_SAVE, gtk.RESPONSE_OK)
		)
		self.save_dlg.set_do_overwrite_confirmation(True)
		for filefilter in self.SAVE_SONG_FILTER:
			self.save_dlg.add_filter(filefilter)

		vbox = gtk.VBox()
		self.add(vbox)


		self.accelerators = gtk.AccelGroup()
		self.add_accel_group(self.accelerators)		
		
		# Menu Bar
		self.aldrinframe_menubar = gtk.MenuBar()
		vbox.pack_start(self.aldrinframe_menubar, expand=False)
		self.filemenu = gtk.Menu()
		filemenuitem = make_submenu_item(self.filemenu, "_File")
		filemenuitem.connect('activate', self.update_filemenu)
		self.aldrinframe_menubar.append(filemenuitem)
		self.update_filemenu(None)
		
		self.editmenu = gtk.Menu()
		editmenuitem = make_submenu_item(self.editmenu, "_Edit")
		editmenuitem.connect('activate', self.update_editmenu)
		self.update_editmenu(None)
		
		self.aldrinframe_menubar.append(editmenuitem)
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
		self.aldrinframe_toolbar = gtk.Toolbar()
		vbox.pack_start(self.aldrinframe_toolbar, expand=False)
		
		def cmp_panel(a,b):
			a_order = (hasattr(a, '__view__') and a.__view__.get('order',0)) or 0
			b_order = (hasattr(b, '__view__') and b.__view__.get('order',0)) or 0
			return cmp(a_order, b_order)
		self.pages = sorted(com.get_from_category('aldrin.viewpanel'), cmp=cmp_panel)
		self.aldrinframe_toolbar.insert(make_stock_tool_item(gtk.STOCK_NEW, self.new),-1)
		self.aldrinframe_toolbar.insert(make_stock_tool_item(gtk.STOCK_OPEN, self.on_open),-1)
		self.aldrinframe_toolbar.insert(make_stock_tool_item(gtk.STOCK_SAVE, self.on_save),-1)
		extrasep = gtk.SeparatorToolItem()
		self.aldrinframe_toolbar.insert(extrasep,-1)
		if not com.get_from_category('menuitem.toolbar', self.aldrinframe_toolbar):
			extrasep.destroy()

		self.mastertoolbar = com.get('aldrin.core.panel.master', self)
		self.transport = com.get('aldrin.core.panel.transport', self)

		self.framepanel = gtk.Notebook()
		self.framepanel.set_tab_pos(gtk.POS_LEFT)
		self.framepanel.set_show_border(False)
		#self.framepanel.set_show_tabs(False)
		
		com.get("aldrin.core.icons") # make sure theme icons are loaded
		
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
			panel.show_all()
			header = gtk.VBox()
			labelwidget = gtk.Label(label)
			labelwidget.set_angle(90)
			header.pack_start(labelwidget)
			header.pack_start(new_theme_image(stockid, gtk.ICON_SIZE_MENU))
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

		self.update_title()
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
		
		gobject.timeout_add(500, self.update_title)
		self.framepanel.connect('switch-page', self.on_activate_page)
		self.activated=0
		
		self.document_changed()
		self.show_all()
		self.load_view()
		
		eventbus = com.get('aldrin.core.eventbus')
		eventbus.print_mapping()

		options, args = com.get('aldrin.core.options').get_options_args()
		if len(args) > 1:
			self.open_file(args[1])
		for driver in com.get_from_category('driver'):
			if driver.init_failed:
				gobject.timeout_add(50, show_preferences, self)
				break
			
	def on_undo(self, *args):
		"""
		Called when an undo item is being called.
		"""
		print "UNDO"
		com.get('aldrin.core.player').undo()
		self.print_history()
			
	def on_redo(self, *args):
		"""
		Called when an undo item is being called.
		"""
		print "REDO"
		com.get('aldrin.core.player').redo()
		self.print_history()
		
	def print_history(self):
		"""
		Dumps the current undo history to console.
		"""
		player = com.get('aldrin.core.player')
		pos = player.history_get_position()		
		historysize = player.history_get_size()
		print "----"
		for index in xrange(historysize):
			desc = str(player.history_get_description(index))
			s = '#%i: "%s"' % (index,desc)
			if pos == index:
				s += ' <-'
			print s
		print "----"
		
	def can_activate_undo(self, *args):
		"""
		handler for can-activate-accel signal by Undo menuitem. Checks if undo can be executed.
		"""
		player = com.get('aldrin.core.player')
		return player.can_undo()
		
	def can_activate_redo(self, *args):
		"""
		handler for can-activate-accel signal by Redo menuitem. Checks if redo can be executed.
		"""
		player = com.get('aldrin.core.player')
		return player.can_redo()
		
	def update_editmenu(self, *args):
		"""
		Updates the edit menu, including the undo menu.
		"""
		for item in self.editmenu:
			item.destroy()
		player = com.get('aldrin.core.player')
		
		pos = player.history_get_position()		
		self.print_history()
		
		item = add_accelerator(make_menu_item("Undo", "", self.on_undo), self, "<Control>Z")
		if player.can_undo():
			item.get_children()[0].set_label('Undo "%s"' % player.history_get_description(pos-1))
		else:
			item.set_sensitive(False)
		item.connect('can-activate-accel', self.can_activate_undo)
		self.editmenu.append(item)
		
		item = add_accelerator(make_menu_item("Redo", "", self.on_redo), self, "<Control>Y")
		if player.can_redo():
			item.get_children()[0].set_label('Redo "%s"' % player.history_get_description(pos))
		else:
			item.set_sensitive(False)
		item.connect('can-activate-accel', self.can_activate_redo)
		self.editmenu.append(item)
		
		self.editmenu.append(gtk.SeparatorMenuItem())
		self.editmenu.append(make_stock_menu_item(gtk.STOCK_CUT, self.on_cut))
		self.editmenu.append(make_stock_menu_item(gtk.STOCK_COPY, self.on_copy))
		self.editmenu.append(make_stock_menu_item(gtk.STOCK_PASTE, self.on_paste))
		self.editmenu.append(gtk.SeparatorMenuItem())
		self.editmenu.append(make_stock_menu_item(gtk.STOCK_PREFERENCES, self.on_preferences))
		self.editmenu.show_all()
			
	def update_filemenu(self, *args):
		"""
		Updates the most recent files in the file menu.
		
		@param widget: the Menu item.
		@type widget: gtk.MenuItem
		"""
		print "update_filemenu"
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

	def get_active_view(self):
		"""
		Returns the active panel view.
		"""
		for pindex,(ctrlid,(panel,menuitem)) in self.pages.iteritems():
			if panel.window and panel.window.is_visible() and hasattr(panel,'view'):
				return panel.view

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
		
	def on_close_cpumonitor(self, *args):
		"""
		Called when the cpu monitor is closed manually.
		
		@param event: event.
		@type event: Event
		"""
		self.cpumonitor.hide_all()
		self.update_view()
		return True
		
	def on_close_hdrecorder(self, *args):
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
		
	def on_help_contents(self, *args):
		"""
		Event handler triggered by the help menu option.
		
		@param event: menu event.
		@type event: MenuEvent
		"""
		import webbrowser		
		webbrowser.open_new(filepath('../doc/aldrin/html/index.html'))
			
	def on_about(self, *args):
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
			if not panel:
				return
			if self.framepanel.get_current_page() != self.index:
				self.framepanel.set_current_page(self.index)
			panel.handle_focus()
		self.activated=0
		
	def on_preferences(self, *args):
		"""
		Event handler triggered by the "Preferences" menu option.
		
		@param event: menu event.
		@type event: MenuEvent
		"""
		show_preferences(self)

	def on_key_down(self, widget, event):
		"""
		Event handler for key events.
		"""
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
		if k == 'F6':
			self.play_from_cursor(event)
		else:
			return False
		return True
			
	def on_activate_page(self, widget, unused, page_num):
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
		self.select_page(self.framepanel.get_current_page())
		
	def update_title(self):
		"""
		Updates the title to display the filename of the currently
		loaded document.
		"""
		player = com.get('aldrin.core.player')
		filename = os.path.basename(self.filename)
		if not filename:
			filename = 'Unsaved'
		if player.document_changed():
			filename = '*'+filename
		if filename:
			title = filename + ' - ' + self.title
		else:
			title = self.title
		self.set_title(title)
		return True
		
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
			player.clear()
			player.load_bmx(self.filename)
			player.document_unchanged()
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
			gobject.timeout_add(int(1000/25), progress_callback)
			progBar.pulse()
			player.clear()
			player.load_ccm(self.filename)
			player.document_unchanged()
			progBar.set_fraction(1.0)
			dlg.destroy()
		else:
			message(self, "'%s' is not a supported file format." % ext)
			return
		self.update_title()
		config.get_config().add_recent_file_config(self.filename)		
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
			result = player.save_ccm(self.filename)
			assert result == 0
			player.document_unchanged()
		except:
			import traceback
			text = traceback.format_exc()
			traceback.print_exc()
			error(self, "<b><big>Error saving file:</big></b>\n\n%s" % text)
		#~ progress.Update(100)
		self.update_title()
		config.get_config().add_recent_file_config(self.filename)
		
	def on_open(self, *args):
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
			
	def on_save(self, *args):
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
		
	def on_save_as(self, *args):
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
		player = com.get('aldrin.core.player')
		player.clear()
		player.set_loop_start(0)
		player.set_loop_end(com.get('aldrin.core.sequencerpanel').view.step)
		player.get_plugin(0).set_parameter_value(1, 0, 1, config.get_config().get_default_int('BPM', 126), 1)
		player.get_plugin(0).set_parameter_value(1, 0, 2, config.get_config().get_default_int('TPB', 4), 1)
		player.history_flush()
		self.document_changed()
		self.update_title()
		
	def play(self, *args):
		"""
		Event handler triggered by the "Play" toolbar button.
		
		@param event: menu event.
		@type event: MenuEvent
		"""
		player = com.get('aldrin.core.player')
		player.play()

	def play_from_cursor(self, *args):
		"""
		Event handler triggered by the F6 key.
		
		@param event: menu event.
		@type event: MenuEvent
		"""
		player = com.get('aldrin.core.player')
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
		
	def stop(self, *args):
		"""
		Event handler triggered by the "Stop" toolbar button.
		
		@param event: menu event.
		@type event: MenuEvent
		"""
		player = com.get('aldrin.core.player')
		player.stop()
		
	def save_changes(self):
		"""
		Asks whether to save changes or not. Throws a {CancelException} if
		cancelled.
		"""
		player = com.get('aldrin.core.player')
		if not player.document_changed():
			return
		if self.filename:
			text = "<big><b>Save changes to <i>%s</i>?</b></big>" % os.path.basename(self.filename)
		else:
			text = "<big><b>Save changes?</b></big>"
		response = question(self, text)
		if response == int(gtk.RESPONSE_CANCEL) or response == int(gtk.RESPONSE_DELETE_EVENT):
			raise CancelException
		elif response == int(gtk.RESPONSE_YES):
			self.save()

	def new(self, *args):
		"""
		Event handler triggered by the "New" menu option.
		
		@param event: menu event.
		@type event: MenuEvent
		"""
		try:
			self.save_changes()
			self.clear()
			self.update_title()
			com.get('aldrin.core.player').document_unchanged()
		except CancelException:
			pass
			
	def on_destroy(self, *args):
		"""
		Event handler triggered when the window is being destroyed.
		"""
		eventbus = com.get('aldrin.core.eventbus')
		eventbus.shutdown()
			
	def on_exit(self, *args):
		"""
		Event handler triggered by the "Exit" menu option.
		
		@param event: menu event.
		@type event: MenuEvent
		"""
		if not self.on_close(None, None):
			self.destroy()
			
	def on_close(self, *args):
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
					


__aldrin__ = dict(
	classes = [
		AldrinFrame,
	],
)

if __name__ == '__main__':
	pass
