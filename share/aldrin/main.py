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
Provides application class and controls used in the aldrin main window.
"""

import sys, os

from gtkimport import gtk
import gobject

import time

from utils import format_time, ticks_to_time, prepstr, linear2db, filepath, \
	is_debug, question, error, add_scrollbars, file_filter, new_stock_image_toggle_button, \
	new_stock_image_button, message
import utils

from zzub import Player
import zzub
player = None
playstarttime = None

import sequencer, router, patterns, wavetable, preferences, hdrecorder, cpumonitor, info, common, rack
from sequencer import SequencerPanel
from router import RoutePanel
from patterns import PatternPanel
from wavetable import WavetablePanel
from rack import RackPanel
from info import InfoPanel
from preferences import show_preferences
import config
from config import get_plugin_aliases, get_plugin_blacklist
import about
import extman
import envelope
import driver
import cairo
from common import MARGIN, MARGIN2, MARGIN3, MARGIN0

import interface
from interface import IRootWindow

import os, sys

app = None

def get_stock_bmp(artid):
	fullpath = filepath('res/'+artid+'.png')
	if os.path.isfile(fullpath):
		return wx.Bitmap(fullpath, wx.BITMAP_TYPE_ANY)
	return wx.NullBitmap

class CancelException(Exception):
	"""
	Is being thrown when the user hits cancel in a sequence of
	modal UI dialogs.
	"""

class AboutDialog(gtk.AboutDialog):
	"""
	A simple about dialog with a text control and an OK button.
	"""
	def __init__(self, parent):
		"""
		Initialization.
		"""
		gtk.AboutDialog.__init__(self)
		self.set_name(about.NAME)
		self.set_version(about.VERSION)
		self.set_copyright(about.COPYRIGHT)
		self.set_comments(about.COMMENTS)
		self.set_license(about.LICENSE)
		self.set_wrap_license(True)
		self.set_website(about.WEBSITE)
		self.set_authors(about.AUTHORS)
		self.set_artists(about.ARTISTS)
		self.set_documenters(about.DOCUMENTERS)
		self.set_logo(gtk.gdk.pixbuf_new_from_file(filepath("res/splash.png")))

def show_about_dialog(parent):
	"""
	Displays the about dialog.
	
	@param parent: Parent window.
	@type parent: wx.Window
	"""
	dlg = AboutDialog(parent)
	dlg.run()
	dlg.destroy()


def about_visit_website(dialog, link, user_data):
	import webbrowser
	webbrowser.open_new(link)

def about_send_email(dialog, link, user_data):
	import webbrowser
	print link
	webbrowser.open_new('mailto:'+link)
	
gtk.about_dialog_set_url_hook(about_visit_website, None)
gtk.about_dialog_set_email_hook(about_send_email, None)

def make_submenu_item(submenu, name):
	item = gtk.MenuItem(label=name)
	item.set_submenu(submenu)
	return item

def make_stock_menu_item(stockid, func, frame=None, shortcut=None, *args):
	item = gtk.ImageMenuItem(stockid)
	if frame and shortcut:
		key, modifier = gtk.accelerator_parse(shortcut)
		item.add_accelerator("activate", frame.accelerators,  key,  modifier, gtk.ACCEL_VISIBLE)	
	if func:
		item.connect('activate', func, *args)
	return item

def make_stock_tool_item(stockid, func, *args):
	item = gtk.ToolButton(stockid)
	if func:
		item.connect('clicked', func, *args)
	return item

def make_stock_toggle_item(stockid, func, *args):
	item = gtk.ToggleToolButton(stockid)
	if func:
		item.connect('toggled', func, *args)
	return item

def make_stock_radio_item(stockid, func, *args):
	item = gtk.RadioToolButton(stock_id=stockid)
	if func:
		item.connect('toggled', func, *args)
	return item

def make_menu_item(label, desc, func, *args):
	item = gtk.MenuItem(label=label)
	if func:
		item.connect('activate', func, *args)
	return item

def make_check_item(label, desc, func, *args):
	item = gtk.CheckMenuItem(label=label)
	if func:
		item.connect('toggled', func, *args)
	return item

def make_radio_item(label, desc, func, *args):
	item = gtk.RadioMenuItem(label=label)
	if func:
		item.connect('toggled', func, *args)
	return item

	
STOCK_PATTERNS = "aldrin-patterns"
STOCK_ROUTER = "aldrin-router"
STOCK_SEQUENCER = "aldrin-sequencer"
STOCK_LOOP = "aldrin-loop"
STOCK_SOUNDLIB = "aldrin-soundlib"
STOCK_INFO = "aldrin-info"
STOCK_RACK = "aldrin-rack"
STOCK_PANIC = "aldrin-panic"

class AldrinFrame(gtk.Window, IRootWindow):
	"""
	The application main window class.
	"""

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
	
	PAGE_PATTERN = 0
	PAGE_ROUTE = 1
	PAGE_SEQUENCER = 2
	PAGE_WAVETABLE = 3
	PAGE_INFO = 4
	PAGE_RACK = 5
	
	style_rc = """
	#gtk-button-images=0
	gtk-icon-sizes="gtk-menu=16,16:gtk-button=16,16:gtk-small-toolbar=16,16:gtk-large-toolbar=16,16:panel-menu=16,16"
	"""
	
	gtk.rc_parse_string(style_rc)
	
	iconfactory = gtk.IconFactory()
	iconfactory.add_default()
	def make_iconset(path):
		return gtk.IconSet(gtk.gdk.pixbuf_new_from_file(path))

	iconfactory.add(gtk.STOCK_NEW, make_iconset(filepath('res/document-new.png')))
	iconfactory.add(gtk.STOCK_OPEN, make_iconset(filepath('res/document-open.png')))
	iconfactory.add(gtk.STOCK_SAVE, make_iconset(filepath('res/document-save.png')))

	iconfactory.add(gtk.STOCK_SAVE, make_iconset(filepath('res/document-save.png')))

	iconfactory.add(gtk.STOCK_MEDIA_PLAY, make_iconset(filepath('res/media-playback-start.png')))
	iconfactory.add(gtk.STOCK_MEDIA_RECORD, make_iconset(filepath('res/media-record.png')))
	iconfactory.add(gtk.STOCK_MEDIA_STOP, make_iconset(filepath('res/media-playback-stop.png')))

	iconfactory.add(STOCK_PATTERNS, make_iconset(filepath('res/patterns.png')))
	iconfactory.add(STOCK_ROUTER, make_iconset(filepath('res/machines.png')))
	iconfactory.add(STOCK_SEQUENCER, make_iconset(filepath('res/sequencer.png')))
	iconfactory.add(STOCK_LOOP, make_iconset(filepath('res/media-playlist-repeat.png')))
	iconfactory.add(STOCK_SOUNDLIB, make_iconset(filepath('res/wavetable.png')))
	iconfactory.add(STOCK_INFO, make_iconset(filepath('res/text-x-generic.png')))
	iconfactory.add(STOCK_PANIC, make_iconset(filepath('res/process-stop.png')))
	iconfactory.add(STOCK_RACK, make_iconset(filepath('res/rack.png')))
	
	gtk.stock_add((
		(STOCK_PATTERNS, "Pattern", 0, gtk.gdk.keyval_from_name('F2'), 'aldrin'),
		(STOCK_ROUTER, "Router", 0, gtk.gdk.keyval_from_name('F3'), 'aldrin'),
		(STOCK_SEQUENCER, "Sequencer", 0, gtk.gdk.keyval_from_name('F4'), 'aldrin'),
		(STOCK_LOOP, "Loop", 0, gtk.gdk.keyval_from_name('F8'), 'aldrin'),
		(STOCK_SOUNDLIB, "Sound Library", 0, gtk.gdk.keyval_from_name('F9'), 'aldrin'),
		(STOCK_INFO, "Info", 0, gtk.gdk.keyval_from_name('F10'), 'aldrin'),
		(STOCK_RACK, "Rack", 0, gtk.gdk.keyval_from_name('F11'), 'aldrin'),
		(STOCK_PANIC, "Panic", 0, gtk.gdk.keyval_from_name('F12'), 'aldrin'),
	))
	
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
		IRootWindow.__init__(self)
		
		self._cbtime = time.time()
		self._cbcalls = 0
		self._hevcalls = 0
		self._hevtimes = 0
		global player, playstarttime
		player = common.get_player()
		player.set_callback(self.player_callback)
		# load blacklist file and add blacklist entries
		for name in get_plugin_blacklist():
			player.blacklist_plugin(name)
		# load aliases file and add aliases
		for name,uri in get_plugin_aliases():
			player.add_plugin_alias(name, uri)
		pluginpath = os.environ.get('ALDRIN_PLUGIN_PATH',None)
		if pluginpath:
			pluginpaths = pluginpath.split(os.pathsep)
		else:
			pluginpaths = [
				'/usr/local/lib64/zzub',
				'/usr/local/lib/zzub',
				'/usr/lib64/zzub',
				'/usr/lib/zzub',
			]
		for pluginpath in pluginpaths:
			player.add_plugin_path(pluginpath + '/')
		inputname, outputname, samplerate, buffersize = config.get_config().get_audiodriver_config()
		player.initialize(samplerate)
		self.init_lunar()
		
		playstarttime = time.time()

		self.event_handlers = []

		# begin wxGlade: AldrinFrame.__init__
		gtk.Window.__init__(self, gtk.WINDOW_TOPLEVEL)
		#~ self.toolsmenu.hide()
		em = extman.get_extension_manager()
		em.register_service(interface.SERVICE_ROOTWINDOW, self, interface.IRootWindow)
		em.realize_extensions(self)

		self.set_size_request(500,400)
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
		
		wxglade_tmp_menu = gtk.Menu()
		wxglade_tmp_menu.append(make_stock_menu_item(gtk.STOCK_CUT, self.on_cut))
		wxglade_tmp_menu.append(make_stock_menu_item(gtk.STOCK_COPY, self.on_copy))
		wxglade_tmp_menu.append(make_stock_menu_item(gtk.STOCK_PASTE, self.on_paste))
		wxglade_tmp_menu.append(gtk.SeparatorMenuItem())
		wxglade_tmp_menu.append(make_stock_menu_item(gtk.STOCK_PREFERENCES, self.on_preferences))
		self.aldrinframe_menubar.append(make_submenu_item(wxglade_tmp_menu, "_Edit"))
		wxglade_tmp_menu = gtk.Menu()
		self.item_cpumon = make_check_item("_CPU monitor", "Show or hide CPU Monitor", self.on_toggle_cpu_monitor)
		wxglade_tmp_menu.append(self.item_cpumon)
		self.item_hdrec = make_check_item("Hard Disk Recorder", "Show or hide Hard Disk Recorder", self.on_toggle_hard_disk_recorder)
		wxglade_tmp_menu.append(self.item_hdrec)
		self.item_master = make_check_item("_Master", "Show or hide Master", self.on_toggle_mastertoolbar)
		wxglade_tmp_menu.append(self.item_master)
		self.item_transport = make_check_item("_Transport", "Show or hide transport bar", self.on_toggle_transport)
		wxglade_tmp_menu.append(self.item_transport)
		self.item_statusbar = make_check_item("_Status Bar", "Show or hide the status bar", self.on_toggle_statusbar)
		wxglade_tmp_menu.append(self.item_statusbar)
		wxglade_tmp_menu.append(make_check_item("S_kins", "Show or hide custom machine skins", None))
		self.item_standard = make_check_item("_Standard", "Show or hide the standard toolbar", self.on_toggle_toolbar)
		wxglade_tmp_menu.append(self.item_standard)
		self.viewmenu = wxglade_tmp_menu
		wxglade_tmp_menu_sub = gtk.Menu()
		defaultitem = gtk.RadioMenuItem(label="Default")
		wxglade_tmp_menu_sub.append(defaultitem)
		self.thememenu = wxglade_tmp_menu_sub
		cfg = config.get_config()
		if not cfg.get_active_theme():
			defaultitem.set_active(True)
		defaultitem.connect('toggled', self.on_select_theme, None)
		for name in sorted(cfg.get_theme_names()):
			item = gtk.RadioMenuItem(label=prepstr(name), group=defaultitem)
			if name == cfg.get_active_theme():
				item.set_active(True)
			item.connect('toggled', self.on_select_theme, name)
			wxglade_tmp_menu_sub.append(item)
		wxglade_tmp_menu.append(make_submenu_item(wxglade_tmp_menu_sub, "Themes"))
		self.aldrinframe_menubar.append(make_submenu_item(wxglade_tmp_menu, "_View"))
		self.toolsmenu = gtk.Menu()
		item = make_submenu_item(self.toolsmenu, "_Tools")
		self.aldrinframe_menubar.append(item)
		added = False
		for svc in em.enumerate_services(interface.CLASS_UI_BUILDER):
			added = svc.call_safe('extend_menu', False, interface.UIOBJECT_MAIN_MENU_TOOLS, self.toolsmenu) or added
		if not added:
			item.destroy()
		wxglade_tmp_menu = gtk.Menu()
		wxglade_tmp_menu.append(make_stock_menu_item(gtk.STOCK_HELP, self.on_help_contents))
		wxglade_tmp_menu.append(gtk.SeparatorMenuItem())
		wxglade_tmp_menu.append(make_stock_menu_item(gtk.STOCK_ABOUT, self.on_about))
		self.aldrinframe_menubar.append(make_submenu_item(wxglade_tmp_menu, "_Help"))
		#~ # Menu Bar end

		# Tool Bar
		def def_bmp(i):
			return get_stock_bmp(i)
		self.aldrinframe_toolbar = gtk.Toolbar()
		vbox.pack_start(self.aldrinframe_toolbar, expand=False)
		
		self.routeframe = RoutePanel(self)
		self.seqframe = SequencerPanel(self)
		self.patternframe = PatternPanel(self)
		self.wavetableframe = WavetablePanel(self)
		self.infoframe = InfoPanel(self)
		#~ self.rackframe = RackPanel(self)
		self.pages = {
			self.PAGE_PATTERN : (
				self.patternframe,
				STOCK_PATTERNS,
			),
			self.PAGE_ROUTE : (
				self.routeframe,
				STOCK_ROUTER,
			),
			self.PAGE_SEQUENCER : (
				self.seqframe,
				STOCK_SEQUENCER,
			),
			self.PAGE_WAVETABLE : (
				self.wavetableframe,
				STOCK_SOUNDLIB,
			),
			self.PAGE_INFO : (
				self.infoframe,
				STOCK_INFO,
			),
			#~ self.PAGE_RACK : (
				#~ self.rackframe,
				#~ STOCK_RACK,
			#~ ),
		}
		self.aldrinframe_toolbar.insert(make_stock_tool_item(gtk.STOCK_NEW, self.new),-1)
		self.aldrinframe_toolbar.insert(make_stock_tool_item(gtk.STOCK_OPEN, self.on_open),-1)
		self.aldrinframe_toolbar.insert(make_stock_tool_item(gtk.STOCK_SAVE, self.on_save),-1)
		extrasep = gtk.SeparatorToolItem()
		self.aldrinframe_toolbar.insert(extrasep,-1)
		added = False
		for svc in em.enumerate_services(interface.CLASS_UI_BUILDER):
			added = svc.call_safe('extend_toolbar', False, interface.UIOBJECT_MAIN_TOOLBAR, self.aldrinframe_toolbar) or added
		if not added:
			extrasep.destroy()

		self.mastertoolbar = MasterPanel(self)
		self.transport = TransportPanel(self)
		
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
		for k in sorted(self.pages):
			panel,stockid = self.pages[k]
			panel.show_all()
			self.framepanel.append_page(panel, gtk.image_new_from_stock(stockid, gtk.ICON_SIZE_SMALL_TOOLBAR))
		self.framepanel.connect('switch-page', self.on_activate_page)
			
		hbox = gtk.HBox()
		hbox.add(self.framepanel)
		hbox.pack_end(self.mastertoolbar, expand=False)
		vbox.add(hbox)

		self.aldrinframe_statusbar = gtk.Statusbar()
		
		vbox.pack_start(self.transport, expand=False)
		vbox.pack_end(self.aldrinframe_statusbar, expand=False)

		self.__set_properties()
		self.__do_layout()

		self.connect('key-press-event', self.on_key_down)
		self.connect('destroy', self.on_destroy)
		self.connect('delete-event', self.on_close)

		self.framepanel.set_current_page(self.PAGE_ROUTE)
		
		gobject.timeout_add(1000/25, self.on_handle_events)
		
		self.document_changed()
		self.show_all()
		self.load_view()

		import sys
		if len(app_args) > 1:
			self.open_file(app_args[1])
		if audiotrouble:
			error(self, "<b><big>Aldrin tried to guess an audio driver but that didn't work.</big></b>\n\nYou need to select your own. Hit OK to show the preferences dialog.")
			show_preferences(self,self)

	def init_lunar(self):
		"""
		Initializes the lunar dsp scripting system
		"""
		pc = player.get_plugincollection_by_uri("@zzub.org/plugincollections/lunar")

		# return if lunar is missing
		if not pc._handle:
			print >> sys.stderr, "not supporting lunar."
			return

		from xml.dom.minidom import parse
		userlunarpath = os.path.join(config.get_config().get_settings_folder(),'lunar')
		if not os.path.isdir(userlunarpath):
			print "folder %s does not exist, creating..." % userlunarpath
			os.makedirs(userlunarpath)
		pc.configure("local_storage_dir", userlunarpath)

	def update_filemenu(self, event=None):
		"""
		Updates the most recent files in the file menu.
		
		@param event: Event.
		@type event: wx.Event
		"""
		for item in self.filemenu:
			print item
			item.destroy()
		self.filemenu.append(make_stock_menu_item(gtk.STOCK_NEW, self.new, frame=self, shortcut="<Control>N"))
		self.filemenu.append(make_stock_menu_item(gtk.STOCK_OPEN, self.on_open, frame=self, shortcut="<Control>O"))
		self.filemenu.append(make_stock_menu_item(gtk.STOCK_SAVE, self.on_save, frame=self, shortcut="<Control>S"))
		self.filemenu.append(make_stock_menu_item(gtk.STOCK_SAVE_AS, self.on_save_as))
		recent_files = config.get_config().get_recent_files_config()
		if recent_files:
			self.filemenu.append(gtk.SeparatorMenuItem())
			for i,filename in enumerate(recent_files):
					self.filemenu.append(make_menu_item("_%i %s" % (i+1,os.path.basename(filename)), "", self.open_recent_file, filename))
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
		@type event: wx.MenuEvent
		"""
		view = self.get_active_view()
		if view and hasattr(view, 'on_copy'):
			view.on_copy(event)

	def on_cut(self, event):
		"""
		Sent when the cut function is selected from the menu.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		"""
		view = self.get_active_view()
		if view and hasattr(view, 'on_cut'):
			view.on_cut(event)

	def on_paste(self, event):
		"""
		Sent when the paste function is selected from the menu.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
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
		@type event: wx.Event
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
		@type event: wx.CommandEvent
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
		@type event: wx.CommandEvent
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
		@type event: wx.CommandEvent
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
		@type event: wx.CommandEvent
		"""
		self.show_mastertoolbar(widget.get_active())
		
	def on_toggle_automation(self, widget):
		"""
		handler triggered by the record toolbar button. Decides whether
		changes to parameters are recorded or not.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		if widget.get_active():
			player.set_automation(1)
		else:
			player.set_automation(0)

	def on_toggle_automation_accel(self, event):
		"""
		Handler triggered by the f7 accellerator. Enables/disables
		automation.
		
		@param event command event.
		@type event: wx.CommandEvent
		"""
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
		@type event: wx.CommandEvent
		"""
		if widget.get_active():
			player.set_loop_enabled(1)
		else:
			player.set_loop_enabled(0)
			
	def on_toggle_panic(self, widget):
		"""
		Handler triggered by the mute toolbar button. Deinits/reinits
		sound device.
		
		@param event command event.
		@type event: wx.CommandEvent
		"""
		if widget.get_active():
			player.audiodriver_enable(0)
		else:
			player.audiodriver_enable(1)

	def on_toggle_panic_accel(self, event):
		"""
		Handler triggered by the f12 accellerator. Deinits/reinits
		sound device.
		
		@param event command event.
		@type event: wx.CommandEvent
		"""
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
		return True
		
	def on_help_contents(self, event):
		"""
		Event handler triggered by the help menu option.
		
		@param event: menu event.
		@type event: wx.MenuEvent
		"""
		import webbrowser		
		webbrowser.open_new(filepath('../doc/aldrin/html/index.html'))
			
	def on_about(self, event):
		"""
		Event handler triggered by the "About" menu option.
		
		@param event: menu event.
		@type event: wx.MenuEvent
		"""
		show_about_dialog(self)
			
	def on_framepanel_size(self, event):
		"""
		Event handler that resizes the client panels, when the main window
		is being resized.
		
		@param event: size event.
		@type event: wx.SizeEvent
		"""
		#~ x,y,w,h = self.framepanel.GetClientRect()
		#~ for ctrlid, panel in self.pages.values():
			#~ panel.SetRect((x,y,w,h))
			
	def select_page(self, index):
		"""
		Selects a client panel. If the client panel has a view attribute,
		that view attribute is being interpreted as a window and will be
		focused, else the panel itself will be focused.
		
		@param index: Index of the panel (use one of the self.PAGE_* constants)
		@type index: int
		"""
		panel, stockid = self.pages[index]
		if self.framepanel.get_current_page() != index:
			self.framepanel.set_current_page(index)
		if hasattr(panel,'view'):
			print "grab focus",panel.view
			panel.view.grab_focus()
		else:
			print "not grab focus"
			panel.grab_focus()
			
	def on_preferences(self, event):
		"""
		Event handler triggered by the "Preferences" menu option.
		
		@param event: menu event.
		@type event: wx.MenuEvent
		"""
		show_preferences(self,self)

	def on_key_down(self, widget, event):
		"""
		Event handler for key events.
		"""
		kv = event.keyval
		k = gtk.gdk.keyval_name(event.keyval)
		if k == 'F3':
			self.select_page(self.PAGE_ROUTE)
		elif k == 'F2':
			self.select_page(self.PAGE_PATTERN)
		elif k == 'F4':
			self.select_page(self.PAGE_SEQUENCER)
		elif k == 'F5':
			self.btnplay.set_active(True)
		elif k == 'F6':
			self.play_from_cursor(event)
		elif k == 'F7':
			self.btnrecord.set_active(not self.btnrecord.get_active())
		elif k == 'F8':
			self.stop(event)
		elif k == 'F9':
			self.select_page(self.PAGE_WAVETABLE)
		elif k == 'F10':
			self.select_page(self.PAGE_INFO)
		#~ elif k == 'F11':
			#~ self.select_page(self.PAGE_RACK)
		elif k == 'F12':
			self.btnpanic.set_active(not self.btnpanic.get_active())
		else:
			return False
		return True
			
	def on_activate_page(self, notebook, page, page_num):
		gobject.timeout_add(10, self.select_page, page_num)

	def open_recent_file(self, widget, filename):
		"""
		Event handler triggered by recent file menu options.
		
		@param event: menu event.
		@type event: wx.MenuEvent
		"""
		self.save_changes()
		self.open_file(filename)
	
	def document_changed(self):
		"""
		Event handler triggered when the document has changed. You should
		call this on occasions where the entire document has changed, else
		there are specialized handlers in the panel classes.
		"""
		common.get_plugin_infos().update()
		self.routeframe.update_all()
		self.infoframe.update_all()
		#~ self.rackframe.update_all()
		self.routeframe.view.update_colors()
		self.routeframe.view.redraw()
		self.seqframe.seqview.redraw()
		self.seqframe.update_list()
		self.patternframe.update_all()
		self.wavetableframe.update_all()
		self.mastertoolbar.update_all()
		self.transport.update_all()
		self.btnloop.set_active(player.get_loop_enabled())
		self.btnrecord.set_active(player.get_automation())
		self.seqframe.adjust_seqscrollbars()
		
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
		if ext.lower() in ('.bmx','.bmw'):
			#~ progress = wx.ProgressDialog("Aldrin", "Loading BMX Song...")
			#~ wx.Yield()
			player.load_bmx(self.filename)
			#~ wx.Yield()
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
				print 'loading'
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
		self.update_filemenu()
		self.document_changed()
		self.select_page(self.PAGE_ROUTE)
		
	def save_file(self, filename):
		"""
		Saves a song to disk. The document will also be added to the
		recent file list.
		
		@param filename: Path to song.
		@type filename: str
		"""
		try:
			if not os.path.splitext(filename)[1]:
				filename += self.DEFAULT_EXTENSION
			self.filename = filename
			if os.path.isfile(filename):
				# rename incremental
				path,basename = os.path.split(filename)
				basename,ext = os.path.splitext(basename)
				i = 0
				while True:
					newpath = os.path.join(path,"%s%s.%03i.bak" % (basename,ext,i))
					if not os.path.isfile(newpath):
						break
					i += 1
				#print '%s => %s' % (filename, newpath)
				os.rename(filename, newpath)
			base,ext = os.path.splitext(self.filename)
			#~ progress = wx.ProgressDialog("Aldrin", "Saving '%s'..." % prepstr(self.filename))
			#~ wx.Yield()
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
		@type event: wx.MenuEvent
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
		@type event: wx.MenuEvent
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
		self.routeframe.reset()
		self.patternframe.reset()
		self.infoframe.reset()
		player.clear()
		player.set_loop_start(0)
		player.set_loop_end(self.seqframe.view.step)
		player.get_plugin(0).set_parameter_value(1, 0, 1, config.get_config().get_default_int('BPM', 126), 1)
		player.get_plugin(0).set_parameter_value(1, 0, 2, config.get_config().get_default_int('TPB', 4), 1)
		self.document_changed()
		self.update_title()
		
	def play(self, widget):
		"""
		Event handler triggered by the "Play" toolbar button.
		
		@param event: menu event.
		@type event: wx.MenuEvent
		"""
		global playstarttime
		if self.btnplay.get_active():
			playstarttime = time.time()
			player.play()
		elif player.get_state() == zzub.zzub_player_state_playing:
			# keep on
			self.btnplay.set_active(True)

	def play_from_cursor(self, event):
		"""
		Event handler triggered by the F6 key.
		
		@param event: menu event.
		@type event: wx.MenuEvent
		"""
		global playstarttime
		playstarttime = time.time()
		if not self.btnplay.get_active():
			self.btnplay.set_active(True)
		player.set_position(max(self.seqframe.view.row,0))
		player.play()		
		
	def on_select_theme(self, widget, data):
		"""
		Event handler for theme radio menu items.
		
		@param event: menu event.
		@type event: wx.MenuEvent
		"""
		print data
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
		@type event: wx.MenuEvent
		"""
		player.stop()
		self.btnplay.set_active(False)
		
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
		if response == int(gtk.RESPONSE_CANCEL):
			raise CancelException
		elif response == int(gtk.RESPONSE_YES):
			self.save()

	def new(self, event):
		"""
		Event handler triggered by the "New" menu option.
		
		@param event: menu event.
		@type event: wx.MenuEvent
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
		@type event: wx.MenuEvent
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
					
	def __set_properties(self):
		"""
		Assigns properties to dialog controls.
		"""
		# begin wxGlade: AldrinFrame.__set_properties
		self.set_title(self.title)
		gtk.window_set_default_icon_list(
			gtk.gdk.pixbuf_new_from_file(filepath("../icons/hicolor/48x48/apps/aldrin.png")),
			gtk.gdk.pixbuf_new_from_file(filepath("../icons/hicolor/32x32/apps/aldrin.png")),
			gtk.gdk.pixbuf_new_from_file(filepath("../icons/hicolor/24x24/apps/aldrin.png")),
			gtk.gdk.pixbuf_new_from_file(filepath("../icons/hicolor/22x22/apps/aldrin.png")),
			gtk.gdk.pixbuf_new_from_file(filepath("../icons/hicolor/16x16/apps/aldrin.png")))
		self.resize(750, 550)
		# statusbar fields
		self.aldrinframe_statusbar.push(0, "Ready to rok again")
		# end wxGlade

	def __do_layout(self):
		"""
		Layouts available controls.
		"""

	#########################
	# IMainFrame interface
	#########################
	
	def get_window(self):
		"""
		Returns the window object associated with the mainframe.
		
		@return: Window object.
		@rtype: wx.Window
		"""
		return self

# end of class AldrinFrame
import random

import Queue, time

class EventPlayer:
	"""
	replays timed events.
	"""
	def __init__(self):
		self.reset()
		
	def reset(self):
		self.offset = None
		self.starttime = None
		self.queue = []
		
	def put(self, v, t):
		if (not self.offset) or (self.offset > t):
			self.offset = t #+ driver.get_audiodriver().get_latency()
			self.starttime = time.time()
			self.lasttime = t
		t -= self.offset
		ts = self.starttime + t
		if len(self.queue) > 100:
			self.reset()
			return
		self.queue.insert(0, (ts,v))
	
	def get_next(self):
		if not len(self.queue):
			self.reset()
			return
		r = 0.0
		while len(self.queue):
			ct = time.time()
			t,v = self.queue[-1]
			if t >= ct:
				break
			r = max(r,self.queue.pop()[1])
		return r

class AmpView(gtk.DrawingArea):
	"""
	A simple control rendering a Buzz-like master VU bar.
	"""
	def __init__(self):
		"""
		Initializer.
		"""
		import Queue
		self.range = 76
		self.amp = 0.0
		self.amps = EventPlayer()
		self.stops = (0.0, 6.0 / self.range, 12.0 / self.range) # red, yellow, green
		self.index = 0
		gtk.DrawingArea.__init__(self)
		self.set_size_request(MARGIN,100)
		self.connect("expose_event", self.expose)
		gobject.timeout_add(1000/25, self.on_update)
		
	def set_amp(self, amp, t):
		self.amps.put(amp, t)
	
	def on_update(self):
		"""
		Event handler triggered by a 25fps timer event.
		"""
		v = self.amps.get_next()
		if v != None:
			self.amp = v
			rect = self.get_allocation()
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
			bh = int((h * (linear2db(self.amp,limit=-self.range) + self.range)) / self.range)
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
		self.ampl = AmpView()
		self.ampr = AmpView()
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
		if plugin == player.get_plugin(0):
			if data.type == zzub.zzub_event_type_parameter_changed:
				self.update_all()
			elif data.type == zzub.zzub_event_type_vu:
				vu = getattr(data,'').vu
				self.ampl.set_amp(vu.left_amp, vu.time)
				self.ampr.set_amp(vu.right_amp, vu.time)

	def on_scroll_changed(self, widget, scroll, value):
		"""
		Event handler triggered when the master slider has been dragged.
		
		@param event: event.
		@type event: wx.Event
		"""
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
		master = player.get_plugin(0)
		vol = master.get_parameter_value(1, 0, 0)
		self.masterslider.set_value(16384 - vol)
		self.latency = driver.get_audiodriver().get_latency()
		self.ampl.amps.reset()
		self.ampr.amps.reset()

class TransportPanel(gtk.HBox):
	"""
	A panel containing the BPM/TPB spin controls.
	"""
	def __init__(self, rootwindow):
		"""
		Initializer.
		"""		
		gtk.HBox.__init__(self)
		self.rootwindow = rootwindow
		self.rootwindow.event_handlers.append(self.on_player_callback)
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
			c2.set_alignment(1, 0.5)
			hbox = gtk.HBox(False, MARGIN)
			hbox.pack_start(c1, expand=False)
			hbox.pack_start(c2, expand=False)
			sg1.add_widget(c1)
			sg2.add_widget(c2)
			vbox.add(hbox)
			return c2
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
				print w
				e.set_size_request(w,w)
		self.connect('realize', on_realize)
		
		
		combosizer.pack_start(hbox, expand=False)
		combosizer.pack_start(gtk.VSeparator(), expand=False)
		
		combosizer.pack_start(self.bpmlabel,expand=False)
		combosizer.pack_start(self.bpm,expand=False)
		combosizer.pack_start(self.tpblabel,expand=False)
		combosizer.pack_start(self.tpb,expand=False)

		combosizer.pack_start(gtk.VSeparator(), expand=False)
		combosizer.pack_start(self.btnpanic, expand=False)
		
		self.pack_start(gtk.HBox())
		self.pack_start(combosizer, expand=False)
		self.pack_end(gtk.HBox())
		self.set_border_width(MARGIN)
		player.get_plugin(0).set_parameter_value(1, 0, 1, config.get_config().get_default_int('BPM', 126), 1)
		player.get_plugin(0).set_parameter_value(1, 0, 2, config.get_config().get_default_int('TPB', 4), 1)
		self.bpm_value_changed = self.bpm.connect('value-changed', self.on_bpm)
		self.tpb_value_changed = self.tpb.connect('value-changed', self.on_tpb)
		gobject.timeout_add(100, self.update_label)
		self.update_all()
		
	def update_label(self):
		"""
		Event handler triggered by a 10fps timer event.
		"""
		p = player.get_position()
		m = player.get_plugin(0)
		bpm = m.get_parameter_value(1, 0, 1)
		tpb = m.get_parameter_value(1, 0, 2)
		time.time() - self.starttime
		if player.get_state() == 0: # playing
			e = format_time(time.time() - playstarttime)
		else:
			e = format_time(0.0)
		c = format_time(ticks_to_time(p,bpm,tpb))
		lb,le = player.get_song_loop()		
		l = format_time(ticks_to_time(le-lb,bpm,tpb))
		for text,control in [(e,self.elapsed),(c,self.current),(l,self.loop)]:
			if text != control.get_text():
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
		if plugin == player.get_plugin(0):
			if data.type == zzub.zzub_event_type_parameter_changed:
				data = getattr(data,'').change_parameter
				g,t,i,v = data.group, data.track, data.param, data.value
				if (g,t) == (1,0):
					if i == 1:
						self.update_bpm()
					elif i == 2:
						self.update_tpb()
		
	def on_bpm(self, widget):
		"""
		Event handler triggered when the bpm spin control value is being changed.
		
		@param event: event.
		@type event: wx.Event
		"""
		player.get_plugin(0).set_parameter_value(1, 0, 1, int(self.bpm.get_value()), 1)
		config.get_config().set_default_int('BPM', int(self.bpm.get_value()))

	def on_tpb(self, widget):
		"""
		Event handler triggered when the tpb spin control value is being changed.
		
		@param event: event.
		@type event: wx.Event
		"""
		player.get_plugin(0).set_parameter_value(1, 0, 2, int(self.tpb.get_value()), 1)
		config.get_config().set_default_int('TPB', int(self.tpb.get_value()))
		
	def update_bpm(self):
		self.bpm.handler_block(self.bpm_value_changed)
		master = player.get_plugin(0)
		bpm = master.get_parameter_value(1, 0, 1)
		self.bpm.set_value(bpm)
		self.bpm.handler_unblock(self.bpm_value_changed)
		
	def update_tpb(self):
		self.tpb.handler_block(self.tpb_value_changed)
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

class AldrinApplication:
	"""
	Application class. This one will be instantiated as a singleton.
	"""
	def main(self):
		"""
		Called when the main loop initializes.
		"""
		self.frame = AldrinFrame()
		gtk.main()
		return 1

from optparse import OptionParser
parser = OptionParser()
parser.add_option("--profile", metavar="profile", default='', help="Start Aldrin with profiling enabled, save results to <profile>.")
app_options = None
app_args = None

def main():
	global app
	app = AldrinApplication()
	app.main()
	if player:
		driver.get_mididriver().destroy()
		driver.get_audiodriver().destroy()

def run(argv):
	"""
	Starts the application and runs the mainloop.
	
	@param argv: command line arguments as passed by sys.argv.
	@type argv: str list
	"""
	global app
	global app_options
	global app_args
	app_options, app_args = parser.parse_args(argv)
	if app_options.profile:
		import cProfile
		cProfile.runctx('main()', globals(), locals(), app_options.profile)
	else:
		main()

__all__ = [
'CancelException',
'AboutDialog',
'show_about_dialog',
'AldrinFrame',
'AmpView',
'MasterPanel',
'TimePanel',
'AldrinApplication',
'main',
'run',
]

if __name__ == '__main__':
	#~ import profile
	#~ profile.run('run(sys.argv)', 'aldrinprofile')
	run(sys.argv)
