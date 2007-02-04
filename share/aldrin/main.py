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

from wximport import wx


import time

from utils import format_time, ticks_to_time, prepstr, linear2db, filepath, is_debug
import utils

from zzub import Player
import zzub
player = None
playstarttime = None

import sequencer, router, patterns, wavetable, preferences, hdrecorder, cpumonitor, info
from sequencer import SequencerPanel
from router import RoutePanel
from patterns import PatternPanel
from wavetable import WavetablePanel
from info import InfoPanel
from preferences import show_preferences
import config
from config import get_plugin_aliases, get_plugin_blacklist
from about import abouttext
import extman
import envelope
import driver

import interface
from interface import IMainFrame, \
					MAINFRAME_SERVICE

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

class AboutDialog(wx.Dialog):
	"""
	A simple about dialog with a text control and an OK button.
	"""
	def __init__(self, *args, **kwds):
		"""
		Initialization.
		"""
		wx.Dialog.__init__(self, *args, **kwds)
		self.btnok = wx.Button(self, wx.ID_OK, "OK")
		rowgroup = wx.BoxSizer(wx.VERTICAL)
		self.image = wx.StaticBitmap(self, -1, wx.Bitmap(filepath("res/splash.png"), wx.BITMAP_TYPE_ANY))
		self.info = wx.TextCtrl(self, -1, style=wx.TE_MULTILINE | wx.TE_READONLY | wx.TE_AUTO_URL)
		self.info.SetValue(abouttext)
		self.info.SetMinSize((300,100))
		rowgroup.Add(self.image, 0, wx.ALIGN_LEFT, 0)
		rowgroup.Add(self.info, 0, wx.EXPAND | wx.ALL, 5)
		rowgroup.Add(self.btnok, 0, wx.ALIGN_RIGHT | wx.ALIGN_TOP | wx.LEFT | wx.RIGHT | wx.BOTTOM, 5)
		self.SetAutoLayout(True)
		self.SetSizerAndFit(rowgroup)
		self.Layout()
		self.Centre()

def show_about_dialog(parent):
	"""
	Displays the about dialog.
	
	@param parent: Parent window.
	@type parent: wx.Window
	"""
	dlg = AboutDialog(parent, -1)
	dlg.ShowModal()	
	dlg.Destroy()

class AldrinFrame(wx.Frame, IMainFrame):
	"""
	The application main window class.
	"""
	
	SONG_WILDCARD = '|'.join([
		"All songs (*.ccm,*.bmw,*.bmx)","*.ccm;*.bmw;*.bmx",
		"CCM Songs (*.ccm)","*.ccm",
		"BMX Songs with waves (*.bmx)","*.bmx",
		"BMX Songs without waves (*.bmw)","*.bmw",
	])
	
	SAVE_SONG_WILDCARD = '|'.join([
		"CCM Songs (*.ccm)","*.ccm",
	])
	
	DEFAULT_EXTENSION = '.ccm'
	
	NEW = wx.ID_NEW
	OPEN = wx.ID_OPEN
	SAVE = wx.ID_SAVE
	SAVE_AS = wx.ID_SAVEAS
	EXIT = wx.ID_EXIT
	CUT = wx.ID_CUT
	COPY = wx.ID_COPY
	PASTE = wx.ID_PASTE
	TOGGLE_CPU_MONITOR = wx.NewId()
	TOGGLE_HARD_DISK_RECORDER = wx.NewId()
	TOGGLE_MASTER = wx.NewId()
	TOGGLE_STATUS_BAR = wx.NewId()
	TOGGLE_TIME = wx.NewId()
	TOGGLE_SKINS = wx.NewId()
	TOGGLE_STANDARD = wx.NewId()
	THEMES = wx.NewId()
	PREFERENCES = wx.NewId() #wx.ID_PREFERENCES
	CONTENTS = wx.ID_HELP
	TOTD = wx.NewId()
	ABOUT = wx.NewId()
	PATTERNS = wx.NewId()
	MACHINES = wx.NewId()
	SEQUENCER = wx.NewId()
	PLAY = wx.NewId()
	PLAY_FROM_CURSOR = wx.NewId()
	RECORD = wx.NewId()
	RECORD_ACCEL = wx.NewId()
	STOP = wx.NewId()
	LOOP = wx.NewId()
	WAVETABLE = wx.NewId()
	INFO = wx.NewId()
	PANIC = wx.NewId()
	PANIC_ACCEL = wx.NewId()
	THEMEBASE = wx.NewId()
	for i in range(128):
		wx.RegisterId(THEMEBASE+i)
	RECENTFILE = wx.ID_FILE1
	
	PAGE_PATTERN = 0
	PAGE_ROUTE = 1
	PAGE_SEQUENCER = 2
	PAGE_WAVETABLE = 3
	PAGE_INFO = 4
	
	title = "Aldrin"
	filename = ""

	event_to_name = dict([(getattr(zzub,x),x) for x in dir(zzub) if x.startswith('zzub_event_type_')])
	
	def __init__(self, *args, **kwds):
		"""
		Initializer.
		"""
		IMainFrame.__init__(self)
		global player, playstarttime
		
		player = Player()
		player.set_callback(self.player_callback)
		# load blacklist file and add blacklist entries
		for name in get_plugin_blacklist():
			player.blacklist_plugin(name)
		# load aliases file and add aliases
		for name,uri in get_plugin_aliases():
			player.add_plugin_alias(name, uri)
		#~ pluginpath = filepath('../../lib/zzub') + os.sep
		#~ print "pluginpath = '%s'" % pluginpath
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
		
		sequencer.player = player
		router.player = player
		patterns.player = player
		wavetable.player = player
		preferences.player = player
		hdrecorder.player = player
		cpumonitor.player = player
		info.player = player
		extman.player = player
		envelope.player = player
		driver.player = player

		self.event_handlers = []

		# begin wxGlade: AldrinFrame.__init__
		kwds["style"] = wx.DEFAULT_FRAME_STYLE
		wx.Frame.__init__(self, *args, **kwds)
		audiotrouble = False
		try:
			driver.get_audiodriver().init()
		except driver.AudioInitException:
			import traceback
			traceback.print_exc()
			audiotrouble = True
		midiinputs = config.get_config().get_mididriver_inputs()
		miditrouble = False
		# disable it, if it crashes, we're not using it again
		config.get_config().set_mididriver_inputs([])
		for i in range(player.mididriver_get_count()):
			if player.mididriver_is_input(i):
				drivername = player.mididriver_get_name(i)
				if drivername in midiinputs:
					print "Opening MIDI device '%s'..." % drivername
					if player.mididriver_open(i) != 0:
						miditrouble = True
		config.get_config().set_mididriver_inputs(midiinputs)
		self.recent_files = []
		
		self.cpumonitor = cpumonitor.CPUMonitorDialog(self, -1)
		self.hdrecorder = hdrecorder.HDRecorderDialog(self, -1)

		self.open_dlg = wx.FileDialog(
			self, 
			message="Open", 
			wildcard = self.SONG_WILDCARD,
			style=wx.OPEN | wx.FILE_MUST_EXIST)
		self.save_dlg = wx.FileDialog(
			self, 
			message="Save", 
			wildcard = self.SAVE_SONG_WILDCARD,
			style=wx.SAVE | wx.OVERWRITE_PROMPT)
		self.save_dlg.SetFilterIndex(0)

		# Menu Bar
		self.aldrinframe_menubar = wx.MenuBar()
		self.SetMenuBar(self.aldrinframe_menubar)
		self.filemenu = wx.Menu()
		self.aldrinframe_menubar.Append(self.filemenu, "&File")
		self.update_filemenu()
		wxglade_tmp_menu = wx.Menu()
		wxglade_tmp_menu.Append(self.CUT, "Cu&t", "Cut the selection and put it on the Clipboard", wx.ITEM_NORMAL)
		wxglade_tmp_menu.Append(self.COPY, "&Copy", "Copy the selection and put it on the Clipboard", wx.ITEM_NORMAL)
		wxglade_tmp_menu.Append(self.PASTE, "&Paste", "Insert clipboard contents", wx.ITEM_NORMAL)
		self.aldrinframe_menubar.Append(wxglade_tmp_menu, "&Edit")
		wxglade_tmp_menu = wx.Menu()
		wxglade_tmp_menu.Append(self.TOGGLE_CPU_MONITOR, "&CPU monitor", "Show or hide CPU Monitor", wx.ITEM_CHECK)
		wxglade_tmp_menu.Append(self.TOGGLE_HARD_DISK_RECORDER, "Hard Disk Recorder", "Show or hide Hard Disk Recorder", wx.ITEM_CHECK)
		wxglade_tmp_menu.Append(self.TOGGLE_MASTER, "&Master", "Show or hide Master", wx.ITEM_CHECK)
		wxglade_tmp_menu.Append(self.TOGGLE_STATUS_BAR, "&Status Bar", "Show or hide the status bar", wx.ITEM_CHECK)
		wxglade_tmp_menu.Append(self.TOGGLE_TIME, "T&ime", "Show or hide Time", wx.ITEM_CHECK)
		wxglade_tmp_menu.Append(self.TOGGLE_SKINS, "S&kins", "Show or hide custom machine skins", wx.ITEM_CHECK)
		wxglade_tmp_menu.Append(self.TOGGLE_STANDARD, "&Standard", "Show or hide the standard toolbar", wx.ITEM_CHECK)
		self.viewmenu = wxglade_tmp_menu
		wxglade_tmp_menu_sub = wx.Menu()
		wxglade_tmp_menu_sub.Append(self.THEMEBASE, "<default>", "", wx.ITEM_RADIO)
		self.thememenu = wxglade_tmp_menu_sub
		index = 1
		wx.EVT_MENU(self, self.THEMEBASE, self.on_select_theme)
		selid = None		
		cfg = config.get_config()
		for name in cfg.get_theme_names():
			self.thememenu.Append(self.THEMEBASE + index, prepstr(name), "", wx.ITEM_RADIO)
			wx.EVT_MENU(self, self.THEMEBASE+index, self.on_select_theme)
			if name == cfg.get_active_theme():
				selid = self.THEMEBASE + index
			index += 1
		if selid != None:
			self.thememenu.Check(selid, True)
		wxglade_tmp_menu.AppendMenu(self.THEMES, "Themes", wxglade_tmp_menu_sub, "")
		wxglade_tmp_menu.AppendSeparator()
		wxglade_tmp_menu.Append(self.PREFERENCES, "&Preferences...", "View preferences", wx.ITEM_NORMAL)
		self.aldrinframe_menubar.Append(wxglade_tmp_menu, "&View")
		self.toolsmenu = wx.Menu()
		self.toolsmenuid = self.aldrinframe_menubar.GetMenuCount()
		self.aldrinframe_menubar.Append(self.toolsmenu, "&Tools")
		wxglade_tmp_menu = wx.Menu()
		wxglade_tmp_menu.Append(self.CONTENTS, "&Contents...", "", wx.ITEM_NORMAL)
		#wxglade_tmp_menu.Append(self.TOTD, "&Tip of the day...", "Displays a Tip of the Day.", wx.ITEM_NORMAL)
		wxglade_tmp_menu.AppendSeparator()
		wxglade_tmp_menu.Append(self.ABOUT, "&About Aldrin...", "Display program information, version number and copyright", wx.ITEM_NORMAL)
		self.aldrinframe_menubar.Append(wxglade_tmp_menu, "&Help")
		# Menu Bar end
		self.aldrinframe_statusbar = self.CreateStatusBar(1, wx.ST_SIZEGRIP)
		
		# Tool Bar
		def def_bmp(i):
			#~ bmp = wx.ArtProvider.GetBitmap(i, wx.ART_TOOLBAR)
			#~ if bmp == wx.NullBitmap:
				#~ return get_stock_bmp(i)
			#~ return bmp
			return get_stock_bmp(i)
		self.aldrinframe_toolbar = wx.ToolBar(self, -1, style=wx.TB_HORIZONTAL|wx.TB_FLAT)
		self.SetToolBar(self.aldrinframe_toolbar)
		self.aldrinframe_toolbar.AddLabelTool(self.NEW, "New", def_bmp('document-new'), wx.NullBitmap, wx.ITEM_NORMAL, "New", "Create a new song")
		self.aldrinframe_toolbar.AddLabelTool(self.OPEN, "Open", def_bmp('document-open'), wx.NullBitmap, wx.ITEM_NORMAL, "Open", "Open an existing song")
		self.aldrinframe_toolbar.AddLabelTool(self.SAVE, "Save", def_bmp('document-save'), wx.NullBitmap, wx.ITEM_NORMAL, "Save", "Save the active song")
		#~ self.aldrinframe_toolbar.AddSeparator()
		#~ self.aldrinframe_toolbar.AddLabelTool(self.CUT, "Cut", def_bmp('edit-cut'), wx.NullBitmap, wx.ITEM_NORMAL, "Cut", "Cut the selection and put it on the Clipboard")
		#~ self.aldrinframe_toolbar.AddLabelTool(self.COPY, "Copy", def_bmp('edit-copy'), wx.NullBitmap, wx.ITEM_NORMAL, "Copy", "Copy the selection and put it on the Clipboard")
		#~ self.aldrinframe_toolbar.AddLabelTool(self.PASTE, "Paste", def_bmp('edit-paste'), wx.NullBitmap, wx.ITEM_NORMAL, "Paste", "Insert Clipboard contents")
		self.aldrinframe_toolbar.AddSeparator()
		self.aldrinframe_toolbar.AddLabelTool(self.PATTERNS, "Patterns", def_bmp('patterns'), wx.NullBitmap, wx.ITEM_CHECK, "View pattern editor (F2)", "View pattern editor")
		self.aldrinframe_toolbar.AddLabelTool(self.MACHINES, "Router", def_bmp('machines'), wx.NullBitmap, wx.ITEM_CHECK, "View router (F3)", "View plugin router")
		self.aldrinframe_toolbar.AddLabelTool(self.SEQUENCER, "Sequencer", def_bmp('sequencer'), wx.NullBitmap, wx.ITEM_CHECK,  "View sequence editor (F4)", "View sequence editor")
		self.aldrinframe_toolbar.AddSeparator()
		self.aldrinframe_toolbar.AddLabelTool(self.PLAY, "Play", def_bmp('media-playback-start'), wx.NullBitmap, wx.ITEM_CHECK, "Play (F5)", "Play")
		self.aldrinframe_toolbar.AddLabelTool(self.RECORD, "Record", def_bmp('media-record'), wx.NullBitmap, wx.ITEM_CHECK, "Record (F7)", "Record")
		self.aldrinframe_toolbar.AddLabelTool(self.STOP, "Stop", def_bmp('media-playback-stop'), wx.NullBitmap, wx.ITEM_NORMAL, "Stop (F8)", "Stop")
		self.aldrinframe_toolbar.AddLabelTool(self.LOOP, "Loop", def_bmp('media-playlist-repeat'), wx.NullBitmap, wx.ITEM_CHECK, "Enable/disable looping", "Enable/disable looping")
		self.aldrinframe_toolbar.AddSeparator()
		self.aldrinframe_toolbar.AddLabelTool(self.WAVETABLE, "Sound Library", def_bmp('wavetable'), wx.NullBitmap, wx.ITEM_CHECK, "View sound library (F9)", "View sound library")
		self.aldrinframe_toolbar.AddLabelTool(self.INFO, "Info", def_bmp('text-x-generic'), wx.NullBitmap, wx.ITEM_CHECK, "View info (F10)", "View info")
		self.aldrinframe_toolbar.AddSeparator()
		self.aldrinframe_toolbar.AddLabelTool(self.PANIC, "Panic", def_bmp('process-stop'), wx.NullBitmap, wx.ITEM_CHECK, "Free wave output device (F12)", "Free wave output device for other applications")
		# Tool Bar end

		self.timetoolbar = TimePanel(self, -1)
		self.mastertoolbar = MasterPanel(self, self, -1)
		
		#~ self.framepanel = wx.Notebook(self, -1, style = wx.NB_BOTTOM)
		self.framepanel = wx.Panel(self, -1)
		wx.EVT_SIZE(self.framepanel, self.on_framepanel_size)		
		self.routeframe = RoutePanel(self, self.framepanel, -1)
		self.seqframe = SequencerPanel(self, self.framepanel, -1)
		self.patternframe = PatternPanel(self, self.framepanel, -1)
		self.wavetableframe = WavetablePanel(self, self.framepanel, -1)
		self.infoframe = InfoPanel(self, self.framepanel, -1)
		self.pages = {
			self.PAGE_PATTERN : (self.PATTERNS, self.patternframe),
			self.PAGE_ROUTE : (self.MACHINES, self.routeframe),
			self.PAGE_SEQUENCER : (self.SEQUENCER, self.seqframe),
			self.PAGE_WAVETABLE : (self.WAVETABLE, self.wavetableframe),
			self.PAGE_INFO : (self.INFO, self.infoframe),
		}
		#~ self.framepanel.InsertPage(self.PAGE_PATTERN, self.patternframe, "Patterns")
		#~ self.framepanel.InsertPage(self.PAGE_ROUTE, self.routeframe, "Machines")
		#~ self.framepanel.InsertPage(self.PAGE_SEQUENCER, self.seqframe, "Sequencer")
		#~ self.framepanel.InsertPage(self.PAGE_WAVETABLE, self.wavetableframe, "Wavetable")
		self.__set_properties()
		self.__do_layout()
		# end wxGlade
		
		for i in range(10):
			wx.EVT_MENU(self, self.RECENTFILE+i, self.open_recent_file)

		wx.EVT_MENU(self, self.COPY, self.on_copy)
		wx.EVT_MENU(self, self.CUT, self.on_cut)
		wx.EVT_MENU(self, self.PASTE, self.on_paste)
		
		wx.EVT_MENU(self, self.NEW, self.new)
		wx.EVT_MENU(self, self.OPEN, self.on_open)
		wx.EVT_MENU(self, self.SAVE, self.on_save)
		wx.EVT_MENU(self, self.SAVE_AS, self.on_save_as)
		wx.EVT_MENU(self, self.EXIT, self.on_exit)
		
		wx.EVT_MENU(self, self.TOGGLE_HARD_DISK_RECORDER, self.on_toggle_hard_disk_recorder)
		wx.EVT_MENU(self, self.TOGGLE_CPU_MONITOR, self.on_toggle_cpu_monitor)
		wx.EVT_MENU(self, self.TOGGLE_MASTER, self.on_toggle_mastertoolbar)
		wx.EVT_MENU(self, self.TOGGLE_TIME, self.on_toggle_timetoolbar)
		wx.EVT_MENU(self, self.TOGGLE_STATUS_BAR, self.on_toggle_statusbar)
		wx.EVT_MENU(self, self.TOGGLE_STANDARD, self.on_toggle_toolbar)

		wx.EVT_MENU(self, self.PATTERNS, self.show_patterns)
		wx.EVT_MENU(self, self.MACHINES, self.show_machines)		
		wx.EVT_MENU(self, self.SEQUENCER, self.show_sequencer)
		wx.EVT_MENU(self, self.WAVETABLE, self.show_wavetable)
		wx.EVT_MENU(self, self.INFO, self.show_info)

		wx.EVT_MENU(self, self.PLAY, self.play)
		wx.EVT_MENU(self, self.PLAY_FROM_CURSOR, self.play_from_cursor)
		wx.EVT_MENU(self, self.STOP, self.stop)
		wx.EVT_MENU(self, self.RECORD, self.on_toggle_automation)
		wx.EVT_MENU(self, self.RECORD_ACCEL, self.on_toggle_automation_accel)
		wx.EVT_MENU(self, self.LOOP, self.on_toggle_loop)
		
		wx.EVT_MENU(self, self.ABOUT, self.on_about)
		
		wx.EVT_MENU(self, self.CONTENTS, self.on_help_contents)

		wx.EVT_KEY_DOWN(self, self.on_key_down)
		wx.EVT_MENU(self, self.PREFERENCES, self.on_preferences)
		
		wx.EVT_MENU(self, self.PANIC_ACCEL, self.on_toggle_panic_accel)
		wx.EVT_TOOL(self, self.PANIC, self.on_toggle_panic)
		
		wx.EVT_CLOSE(self, self.on_close)
		wx.EVT_WINDOW_DESTROY(self, self.on_destroy)
		
		wx.EVT_CLOSE(self.hdrecorder, self.on_close_hdrecorder)
		wx.EVT_CLOSE(self.cpumonitor, self.on_close_cpumonitor)

		aTable = wx.AcceleratorTable([
			(wx.ACCEL_NORMAL,  wx.WXK_F2, self.PATTERNS),
			(wx.ACCEL_NORMAL,  wx.WXK_F3, self.MACHINES),
			(wx.ACCEL_NORMAL,  wx.WXK_F4, self.SEQUENCER),
			(wx.ACCEL_NORMAL,  wx.WXK_F5, self.PLAY),
			(wx.ACCEL_NORMAL,  wx.WXK_F6, self.PLAY_FROM_CURSOR),
			(wx.ACCEL_NORMAL,  wx.WXK_F7, self.RECORD_ACCEL),
			(wx.ACCEL_NORMAL,  wx.WXK_F8, self.STOP),
			(wx.ACCEL_NORMAL,  wx.WXK_F9, self.WAVETABLE),
			(wx.ACCEL_NORMAL,  wx.WXK_F10, self.INFO),
			(wx.ACCEL_NORMAL,  wx.WXK_F12, self.PANIC_ACCEL),
		])
		self.SetAcceleratorTable(aTable)
		self.show_machines(None)
		
		self.timer = wx.Timer(self, -1)
		self.timer.Start(100)
		wx.EVT_TIMER(self, self.timer.GetId(), self.on_handle_events)
		
		self.document_changed()
		self.load_view()
		
		em = extman.get_extension_manager()
		em.register_service(MAINFRAME_SERVICE, self, interface.IMainFrame)
		em.realize_extensions(self)
		
		if not self.toolsmenu.GetMenuItemCount():
			#self.aldrinframe_menubar.EnableTop(self.toolsmenuid, False)
			self.aldrinframe_menubar.Remove(self.toolsmenuid)

		import sys
		if len(app_args) > 1:
			self.open_file(app_args[1])
		if audiotrouble:
			wx.MessageDialog(self, message="Aldrin tried to guess an audio driver but that didn't work. You need to select your own. Hit OK to show the preferences dialog.", caption = "Aldrin", style = wx.ICON_ERROR|wx.OK|wx.CENTER).ShowModal()
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
		for item in self.filemenu.GetMenuItems():
			self.filemenu.DeleteItem(item)
		self.filemenu.Append(self.NEW, "&New\tCTRL+N", "Create a new song", wx.ITEM_NORMAL)
		self.filemenu.Append(self.OPEN, "&Open...\tCTRL+O", "Open an existing song", wx.ITEM_NORMAL)
		self.filemenu.Append(self.SAVE, "&Save\tCTRL+S", "Save the active song", wx.ITEM_NORMAL)
		self.filemenu.Append(self.SAVE_AS, "&Save As...", "Save the active song with a new name", wx.ITEM_NORMAL)
		self.recent_files = config.get_config().get_recent_files_config()
		if self.recent_files:
			self.filemenu.AppendSeparator()
			for i in range(len(self.recent_files)):
					filename = self.recent_files[i]
					id = self.RECENTFILE + i
					self.filemenu.Append(id, "&%i %s" % (i+1,os.path.basename(filename)), "", wx.ITEM_NORMAL)
		self.filemenu.AppendSeparator()
		self.filemenu.Append(self.EXIT, "E&xit", "Quit the application; prompts to save document", wx.ITEM_NORMAL)

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
				self.aldrinframe_toolbar.ToggleTool(self.PLAY, True)
			else:
				self.aldrinframe_toolbar.ToggleTool(self.PLAY, False)
		return result
		
	def get_active_view(self):
		"""
		Returns the active panel view.
		"""
		for pindex,(ctrlid,panel) in self.pages.iteritems():
			if panel.IsShown() and hasattr(panel,'view'):
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
		cfg.load_window_pos("TimeToolbar", self.timetoolbar)
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
		cfg.save_window_pos("TimeToolbar", self.timetoolbar)
		cfg.save_window_pos("StatusBar", self.aldrinframe_statusbar)
		
	def update_view(self):
		"""
		Called to update all viewstates.
		"""
		self.aldrinframe_menubar.Check(self.TOGGLE_HARD_DISK_RECORDER, self.hdrecorder.IsShown())
		self.aldrinframe_menubar.Check(self.TOGGLE_CPU_MONITOR, self.cpumonitor.IsShown())
		self.viewmenu.Check(self.TOGGLE_MASTER, self.mastertoolbar.IsShown())
		self.viewmenu.Check(self.TOGGLE_TIME, self.timetoolbar.IsShown())
		self.viewmenu.Check(self.TOGGLE_STATUS_BAR, self.aldrinframe_statusbar.IsShown())
		self.viewmenu.Check(self.TOGGLE_STANDARD, self.aldrinframe_toolbar.IsShown())
		self.Layout()
		# status bar refuses to resize properly, so we need to fake
		# a size event in order to make it behave.
		w,h = self.GetSize()		
		self.SetSize((w-1,h-1))
		self.SetSize((w,h))
		self.save_view()
		
	def on_close_cpumonitor(self, event):
		"""
		Called when the cpu monitor is closed manually.
		
		@param event: event.
		@type event: wx.Event
		"""
		event.Skip()
		self.cpumonitor.Hide()
		self.update_view()
		
	def on_close_hdrecorder(self, event):
		"""
		Called when the hd recorder is closed manually.
		
		@param event: event.
		@type event: wx.Event
		"""
		event.Skip()
		self.hdrecorder.Hide()
		self.update_view()
		
	def on_toggle_cpu_monitor(self, event):
		"""
		Handler triggered by the "Toggle CPU Monitor" menu option.
		Shows and hides the CPU Monitor panel.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		if event.IsChecked():
			self.cpumonitor.Show()
		else:
			self.cpumonitor.Hide()

	def on_toggle_hard_disk_recorder(self, event):
		"""
		Handler triggered by the "Toggle Hard Disk Recorder" menu option.
		Shows and hides the Hard Disk Recorder panel.
		
		@param event: command event.
		@type event: wx.CommandEvent
		"""
		if event.IsChecked():
			self.hdrecorder.Show()
		else:
			self.hdrecorder.Hide()

	def show_toolbar(self, enable):
		"""
		Shows or hides the toolbar.
		
		@param enable: If True, toolbar will be shown.
		@type enable: bool
		"""
		if enable:
			self.aldrinframe_toolbar.Show()
		else:
			self.aldrinframe_toolbar.Hide()
		self.update_view()
		
	def on_toggle_toolbar(self, event):
		"""
		Handler triggered by the "Toggle Default" menu option.
		Shows and hides the toolbar.
		
		@param event: command event.
		@type event: wx.CommandEvent
		"""
		self.show_toolbar(event.IsChecked())

	def show_statusbar(self, enable):
		"""
		Shows or hides the status bar.
		
		@param enable: If True, status bar will be shown.
		@type enable: bool
		"""
		if enable:
			self.aldrinframe_statusbar.Show()
		else:
			self.aldrinframe_statusbar.Hide()
		self.update_view()
		
	def on_toggle_statusbar(self, event):
		"""
		Handler triggered by the "Toggle Status Bar" menu option.
		Shows and hides the status bar.
		
		@param event: command event.
		@type event: wx.CommandEvent
		"""
		self.show_statusbar(event.IsChecked())

	def show_timetoolbar(self, enable):
		"""
		Shows or hides the timetoolbar.
		
		@param enable: If True, toolbar will be shown.
		@type enable: bool
		"""
		if enable:
			self.timetoolbar.Show()
		else:
			self.timetoolbar.Hide()
		self.update_view()
		
	def on_toggle_timetoolbar(self, event):
		"""
		Handler triggered by the "Toggle Time Toolbar" menu option.
		Shows and hides the time toolbar.
		
		@param event: command event.
		@type event: wx.CommandEvent
		"""
		self.show_timetoolbar(event.IsChecked())

	def show_mastertoolbar(self, enable):
		"""
		Shows or hides the mastertoolbar.
		
		@param enable: If True, toolbar will be shown.
		@type enable: bool
		"""
		if enable:
			self.mastertoolbar.Show()
		else:
			self.mastertoolbar.Hide()
		self.update_view()

	def on_toggle_mastertoolbar(self, event):
		"""
		Handler triggered by the "Toggle Master Toolbar" menu option.
		Shows and hides the master toolbar.
		
		@param event: command event.
		@type event: wx.CommandEvent
		"""
		self.show_mastertoolbar(event.IsChecked())
		
	def on_toggle_automation(self, event):
		"""
		handler triggered by the record toolbar button. Decides whether
		changes to parameters are recorded or not.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		if event.IsChecked():
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
		
	def on_toggle_loop(self, event):
		"""
		Handler triggered by the loop toolbar button. Decides whether
		the song loops or not.
		
		@param event command event.
		@type event: wx.CommandEvent
		"""
		if event.IsChecked():
			player.set_loop_enabled(1)
		else:
			player.set_loop_enabled(0)
			
	def on_toggle_panic(self, event):
		"""
		Handler triggered by the mute toolbar button. Deinits/reinits
		sound device.
		
		@param event command event.
		@type event: wx.CommandEvent
		"""
		if self.aldrinframe_toolbar.GetToolState(self.PANIC):
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


	def on_handle_events(self, event):
		"""
		Handler triggered by the default timer. Calls player.handle_events()
		to work off the players message queue.
		
		@param event: timer event
		@type event: wx.TimerEvent
		"""
		player.handle_events()
		
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
		x,y,w,h = self.framepanel.GetClientRect()
		for ctrlid, panel in self.pages.values():
			panel.SetRect((x,y,w,h))
			
	def select_page(self, index):
		"""
		Selects a client panel. If the client panel has a view attribute,
		that view attribute is being interpreted as a window and will be
		focused, else the panel itself will be focused.
		
		@param index: Index of the panel (use one of the self.PAGE_* constants)
		@type index: int
		"""
		for pindex,(ctrlid,panel) in self.pages.iteritems():
			panel.Show(index == pindex)
			self.aldrinframe_toolbar.ToggleTool(ctrlid, index == pindex)
			if index == pindex:
				if hasattr(panel,'view'):
					getattr(panel,'view').SetFocus()
				else:
					panel.SetFocus()
			
	def on_preferences(self, event):
		"""
		Event handler triggered by the "Preferences" menu option.
		
		@param event: menu event.
		@type event: wx.MenuEvent
		"""
		show_preferences(self,self)

	def on_key_down(self, event):
		"""
		Event handler for key events.
		
		@param event: key event.
		@type event: wx.KeyEvent
		"""
		k = event.GetKeyCode()
		if k == wx.WXK_F3:
			self.show_machines(event)
		elif k == wx.WXK_F2:
			self.show_patterns(event)
		elif k == wx.WXK_F4:
			self.show_sequencer(event)
		elif k == wx.WXK_F5:
			self.play(event)
		elif k == wx.WXK_F6:
			self.play_from_cursor(event)
		elif k == wx.WXK_F7:
			self.on_toggle_automation_accel(event)
		elif k == wx.WXK_F8:
			self.stop(event)
		elif k == wx.WXK_F9:
			self.show_wavetable(event)
		elif k == wx.WXK_F10:
			self.show_info(event)
		elif k == wx.WXK_F12:
			self.on_toggle_panic_accel(event)
		else:
			event.Skip()

	def show_info(self, event):		
		"""
		Event handler triggered by the "Info" toolbar button.
		
		@param event: menu event.
		@type event: wx.MenuEvent
		"""
		self.select_page(self.PAGE_INFO)
		
	def show_wavetable(self, event):		
		"""
		Event handler triggered by the "Wavetable" toolbar button.
		
		@param event: menu event.
		@type event: wx.MenuEvent
		"""
		self.select_page(self.PAGE_WAVETABLE)
		self.wavetableframe.samplelist.SetFocus()

	def show_patterns(self, event):
		"""
		Event handler triggered by the "Patterns" toolbar button.
		
		@param event: menu event.
		@type event: wx.MenuEvent
		"""
		self.select_page(self.PAGE_PATTERN)

	def show_machines(self, event):
		"""
		Event handler triggered by the "Machines" toolbar button.
		
		@param event: menu event.
		@type event: wx.MenuEvent
		"""
		self.select_page(self.PAGE_ROUTE)

	def show_sequencer(self, event):
		"""
		Event handler triggered by the "Sequencer" toolbar button.
		
		@param event: menu event.
		@type event: wx.MenuEvent
		"""
		self.select_page(self.PAGE_SEQUENCER)

	def open_recent_file(self, event):
		"""
		Event handler triggered by recent file menu options.
		
		@param event: menu event.
		@type event: wx.MenuEvent
		"""
		self.save_changes()
		self.open_file(self.recent_files[event.GetId() - self.RECENTFILE])
	
	def document_changed(self):
		"""
		Event handler triggered when the document has changed. You should
		call this on occasions where the entire document has changed, else
		there are specialized handlers in the panel classes.
		"""
		self.routeframe.update_all()
		self.infoframe.update_all()
		self.routeframe.view.update_colors()
		self.routeframe.view.ReDraw()		
		self.seqframe.seqview.ReDraw()
		self.seqframe.update_list()
		self.patternframe.update_all()
		self.wavetableframe.update_all()
		self.mastertoolbar.update_all()
		self.aldrinframe_toolbar.ToggleTool(self.LOOP, player.get_loop_enabled())
		self.aldrinframe_toolbar.ToggleTool(self.RECORD, player.get_automation())
		
	def update_title(self):
		"""
		Updates the title to display the filename of the currently
		loaded document.
		"""
		self.SetTitle("%s - %s" % (self.title, os.path.basename(self.filename)))
		
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
			progress = wx.ProgressDialog("Aldrin", "Loading BMX Song...")
			wx.Yield()
			player.load_bmx(self.filename)
			wx.Yield()
			progress.Update(100)
		elif ext.lower() in ('.ccm'):
			progress = wx.ProgressDialog("Aldrin", "Loading CCM Song...")
			wx.Yield()
			player.load_ccm(self.filename)
			wx.Yield()
			progress.Update(100)
		else:
			wx.MessageDialog(self, message="'%s' is not a supported file format." % ext, caption = "Aldrin", style = wx.ICON_ERROR|wx.OK|wx.CENTER).ShowModal()
			return
		
		self.update_title()
		config.get_config().add_recent_file_config(self.filename)
		self.update_filemenu()
		self.document_changed()
		
	def save_file(self, filename):
		"""
		Saves a song to disk. The document will also be added to the
		recent file list.
		
		@param filename: Path to song.
		@type filename: str
		"""
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
		progress = wx.ProgressDialog("Aldrin", "Saving '%s'..." % prepstr(self.filename))
		wx.Yield()
		player.save_ccm(self.filename)
		progress.Update(100)
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
		if self.open_dlg.ShowModal() == wx.ID_OK:
			self.open_file(self.open_dlg.GetPath())
			
	def on_save(self, event):
		"""
		Event handler triggered by the "Save" menu option.
		
		@param event: menu event.
		@type event: wx.MenuEvent
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
		self.save_dlg.SetDirectory(os.path.dirname(self.filename))
		self.save_dlg.SetFilename(os.path.basename(self.filename))
		if self.save_dlg.ShowModal() == wx.ID_OK:
			filepath = self.save_dlg.GetPath()
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
		
	def play(self, event):
		"""
		Event handler triggered by the "Play" toolbar button.
		
		@param event: menu event.
		@type event: wx.MenuEvent
		"""
		global playstarttime
		playstarttime = time.time()
		if not self.aldrinframe_toolbar.GetToolState(self.PLAY):
			self.aldrinframe_toolbar.ToggleTool(self.PLAY, True)
		player.play()		

	def play_from_cursor(self, event):
		"""
		Event handler triggered by the F6 key.
		
		@param event: menu event.
		@type event: wx.MenuEvent
		"""
		global playstarttime
		playstarttime = time.time()
		if not self.aldrinframe_toolbar.GetToolState(self.PLAY):
			self.aldrinframe_toolbar.ToggleTool(self.PLAY, True)
		player.set_position(max(self.seqframe.view.row,0))
		player.play()		
		
	def on_select_theme(self, event):
		"""
		Event handler for theme radio menu items.
		
		@param event: menu event.
		@type event: wx.MenuEvent
		"""
		index = event.GetId() - self.THEMEBASE - 1
		cfg = config.get_config()
		if index == -1:
			cfg.select_theme(None)
		else:
			cfg.select_theme(cfg.get_theme_names()[index])
		self.document_changed()
		
	def stop(self, event):
		"""
		Event handler triggered by the "Stop" toolbar button.
		
		@param event: menu event.
		@type event: wx.MenuEvent
		"""
		player.stop()
		self.aldrinframe_toolbar.ToggleTool(self.PLAY, False)
		
	def save_changes(self):
		"""
		Asks whether to save changes or not. Throws a {CancelException} if
		cancelled.
		"""
		if self.filename:
			text = "Save changes to %s?" % os.path.basename(self.filename)
		else:
			text = "Save changes?"
		dlg = wx.MessageDialog(self, message=text, caption = self.title, style = wx.ICON_EXCLAMATION|wx.YES_NO|wx.CANCEL|wx.CENTER)
		res = dlg.ShowModal()
		if res == wx.ID_CANCEL:
			raise CancelException
		elif res == wx.ID_YES:
			self.save()

	def new(self, event):
		"""
		Event handler triggered by the "New" menu option.
		
		@param event: menu event.
		@type event: wx.MenuEvent
		"""
		try:
			self.save_changes()
			self.SetTitle(self.title)
			self.clear()
		except CancelException:
			pass
			
	def on_destroy(self, event):
		"""
		Event handler triggered when the window is being destroyed.
		
		@param event: event.
		@type event: wx.Event
		"""
			
	def on_exit(self, event):
		"""
		Event handler triggered by the "Exit" menu option.
		
		@param event: menu event.
		@type event: wx.MenuEvent
		"""
		self.Close()
			
	def on_close(self, event):		
		"""
		Event handler triggered when the window is being closed.
		
		@param event: event.
		@type event: wx.Event
		"""
		self.save_view()
		if event.CanVeto():
			try:
				self.save_changes()
				event.Skip()
				self.Destroy()
				if app:
					app.ExitMainLoop()
			except CancelException:
				pass
		else:
			event.Skip()
			self.Destroy()
			app.ExitMainLoop()
					
	def __set_properties(self):
		"""
		Assigns properties to dialog controls.
		"""
		# begin wxGlade: AldrinFrame.__set_properties
		self.SetTitle(self.title)
		if os.name == 'nt':
			iconbundle = wx.IconBundle()
			iconbundle.AddIcon(wx.Icon(filepath("res/aldrin.ico"),wx.BITMAP_TYPE_ICO,48,48))
			iconbundle.AddIcon(wx.Icon(filepath("res/aldrin.ico"),wx.BITMAP_TYPE_ICO,32,32))
			iconbundle.AddIcon(wx.Icon(filepath("res/aldrin.ico"),wx.BITMAP_TYPE_ICO,16,16))
			self.SetIcons(iconbundle)
		else:
			iconbundle = wx.IconBundle()
			iconbundle.AddIcon(wx.Icon(filepath("../icons/hicolor/48x48/apps/aldrin.png"),wx.BITMAP_TYPE_PNG,48,48))
			iconbundle.AddIcon(wx.Icon(filepath("../icons/hicolor/32x32/apps/aldrin.png"),wx.BITMAP_TYPE_PNG,32,32))
			iconbundle.AddIcon(wx.Icon(filepath("../icons/hicolor/24x24/apps/aldrin.png"),wx.BITMAP_TYPE_PNG,24,24))
			iconbundle.AddIcon(wx.Icon(filepath("../icons/hicolor/22x22/apps/aldrin.png"),wx.BITMAP_TYPE_PNG,22,22))
			iconbundle.AddIcon(wx.Icon(filepath("../icons/hicolor/16x16/apps/aldrin.png"),wx.BITMAP_TYPE_PNG,16,16))
			self.SetIcons(iconbundle)
		self.SetSize((750, 550))
		self.aldrinframe_statusbar.SetStatusWidths([-1])
		# statusbar fields
		aldrinframe_statusbar_fields = ["Ready to rok again"]
		for i in range(len(aldrinframe_statusbar_fields)):
		    self.aldrinframe_statusbar.SetStatusText(aldrinframe_statusbar_fields[i], i)
		self.aldrinframe_toolbar.SetToolBitmapSize((16, 15))
		self.aldrinframe_toolbar.Realize()
		# end wxGlade

	def __do_layout(self):
		"""
		Layouts available controls.
		"""
		# begin wxGlade: AldrinFrame.__do_layout
		sizer = wx.BoxSizer(wx.VERTICAL)
		sizer.Add(self.mastertoolbar, 0, wx.EXPAND)
		sizer.Add(self.timetoolbar, 0, wx.EXPAND)
		sizer.Add(self.framepanel, 1, wx.EXPAND)
		self.SetAutoLayout(True)
		self.SetSizer(sizer)		
		self.Layout()
		self.Centre()		
		# end wxGlade

	#########################
	# IMainFrame interface
	#########################
	
	def get_window(self):
		"""
		Returns the window object associated with the mainframe.
		
		@return: Window object.
		@rtype: wx.Window
		"""
		return wx.Window(self)
		
	MENUITEMBASE = wx.NewId()
	for i in range(32):
		wx.RegisterId(MENUITEMBASE+i)
	nextmenuitemid = MENUITEMBASE
	
	def add_menuitem(self, label, description = "", kind = wx.ITEM_NORMAL):
		"""
		Adds a new menuitem to the tools menu and returns the identifier.
		
		@param label: Label of the item.
		@type label: str
		@param description: Description for Status bar.
		@type description: str
		@param kind: One of wx.ITEM_NORMAL, wx.ITEM_CHECK or wx.ITEM_RADIO
		@type kind: int
		@return: Identifier of the menuitem.
		@rtype: int
		"""
		menuid = self.nextmenuitemid
		self.nextmenuitemid += 1
		self.toolsmenu.Append(menuid, label, description, kind)
		return menuid
		
	def add_submenu(self, label, submenu, description = ""):
		"""
		Adds a new submenu to the tools menu and returns the identifier.
		
		@param label: Label of the item.
		@type label: str
		@param submenu: The submenu which to add.
		@type submenu: wx.Menu
		@param description: Description for Status bar.
		@type description: str
		@return: Identifier of the menuitem.
		@rtype: int
		"""
		menuid = self.nextmenuitemid
		self.nextmenuitemid += 1
		self.toolsmenu.AppendMenu(menuid, label, submenu, description)
		return menuid

	TOOLBASE = wx.NewId()
	for i in range(32):
		wx.RegisterId(TOOLBASE+i)
	nexttoolid = TOOLBASE
		
	def add_tool_button(self, label, bitmap1, bitmap2 = wx.NullBitmap, kind = wx.ITEM_NORMAL, tooltip = "", description = ""):
		"""
		Adds a new tool to the toolbar.
		
		@param label: Label of the button. Will not be visible on all systems.
		@type label: str
		@param bitmap1: Bitmap for the button.
		@type bitmap1: wx.Bitmap
		@param bitmap2: Bitmap for disabled button.
		@type bitmap2: wx.Bitmap
		@param kind: One of wx.ITEM_NORMAL, wx.ITEM_CHECK or wx.ITEM_RADIO
		@type kind: int
		@param tooltip: Tooltip Text
		@type tooltip: str
		@param description: Description for Status bar.
		@type description: str
		"""
		if self.nexttoolid == self.TOOLBASE:
			self.aldrinframe_toolbar.AddSeparator()
		toolid = self.nexttoolid
		self.nexttoolid += 1
		self.aldrinframe_toolbar.AddLabelTool(toolid,label,bitmap1,bitmap2,kind,tooltip,description)
		return toolid
		
	def add_click_handler(self, toolid, func):
		"""
		Adds a handler for when a tool is being clicked by the user.
		
		@param toolid: Id of the tool as returned by add_tool()
		@type toolid: int
		@param func: Function to call. The function should take
					an additional event parameter.
		@type func: callable
		"""		
		wx.EVT_MENU(self, toolid, func)

# end of class AldrinFrame
from canvas import Canvas
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

class AmpView(Canvas):
	"""
	A simple control rendering a Buzz-like master VU bar.
	"""
	def __init__(self, *args, **kwds):
		"""
		Initializer.
		"""
		import Queue
		kwds['style'] = wx.SUNKEN_BORDER
		self.parent = args[0]
		self.bgbrush = wx.Brush("#000000",wx.SOLID)
		self.gbrush = wx.Brush("#00ff00",wx.SOLID)
		self.ybrush = wx.Brush("#ffff00",wx.SOLID)
		self.rbrush = wx.Brush("#ff0000",wx.SOLID)
		self.range = 76
		self.amp = 0.0
		self.amps = EventPlayer()
		self.rl = 6.0 / self.range
		self.yl = 6.0 / self.range
		self.gl = (self.range - 12.0) / self.range
		self.index = 0
		Canvas.__init__(self, *args, **kwds)
		self.timer = wx.Timer(self, -1)
		self.timer.Start(1000 / 25)
		wx.EVT_TIMER(self, self.timer.GetId(), self.on_update)
		
	def set_amp(self, amp, t):
		self.amps.put(amp, t)
	
	def on_update(self, event):
		"""
		Event handler triggered by a 25fps timer event.
		
		@param event: Timer event.
		@type event: wx.Event
		"""
		v = self.amps.get_next()
		if v == None:
			return
		#self.amp += 0.3 * (v - self.amp)
		self.amp = v
		self.ReDraw()

	def DrawBuffer(self):
		"""
		Draws the VU bar client region.
		"""
		# lowpass		
		w,h = self.GetSize()
		dc = self.buffer
		dc.SetPen(wx.TRANSPARENT_PEN)
		if self.amp >= 1.0:
			dc.SetBrush(self.rbrush)
			dc.DrawRectangle(0, 0, w, h)
		else:
			x = 0
			dc.SetBrush(self.gbrush)
			dc.DrawRectangle(x, 0, int(self.gl * w), h)
			x += int(self.gl * w)
			dc.SetBrush(self.ybrush)
			dc.DrawRectangle(x, 0, int(self.yl * w), h)
			x += int(self.yl * w)
			dc.SetBrush(self.rbrush)
			dc.DrawRectangle(x, 0, int(self.rl * w), h)
			bw = int((w * (linear2db(self.amp,limit=-self.range) + self.range)) / self.range)
			dc.SetBrush(self.bgbrush)
			dc.DrawRectangle(bw, 0, w - bw, h)

class MasterPanel(wx.Panel):
	"""
	A panel containing the master slider and BPM/TPB spin controls.
	"""
	def __init__(self, rootwindow, *args, **kwds):
		"""
		Initializer.
		"""		
		wx.Panel.__init__(self, *args, **kwds)
		self.latency = 0
		self.rootwindow = rootwindow
		self.rootwindow.event_handlers.append(self.on_player_callback)
		self.bpmlabel = wx.StaticText(self, -1, label ="BPM:")
		self.bpm = wx.SpinCtrl(self, -1)
		w,h = self.bpm.GetMinSize()
		self.bpm.SetMinSize((50,h))
		self.bpm.SetRange(16,500)
		self.bpm.SetValue(126)
		self.tpblabel = wx.StaticText(self, -1, label ="TPB:")
		self.tpb = wx.SpinCtrl(self, -1)
		self.tpb.SetMinSize((50,h))
		self.tpb.SetRange(1,32)
		self.tpb.SetValue(4)
		self.masterslider = wx.Slider(self, -1)
		msx,msy = self.masterslider.GetMinSize()
		self.masterslider.SetMinSize((200,msy))
		self.masterslider.SetRange(0,16384)
		self.ampl = AmpView(self, -1)
		self.ampl.SetMinSize((200,10))
		self.ampr = AmpView(self, -1)
		self.ampr.SetMinSize((200,10))
		masterslider = wx.BoxSizer(wx.VERTICAL)
		masterslider.Add(self.ampl, 0, wx.TOP, 2)
		masterslider.Add(self.masterslider, 0, wx.ALIGN_LEFT, 0)
		masterslider.Add(self.ampr, 0, wx.BOTTOM, 2)
		combosizer = wx.BoxSizer(wx.HORIZONTAL)
		combosizer.Add(self.bpmlabel, 0, wx.ALIGN_CENTER_VERTICAL|wx.LEFT|wx.TOP|wx.BOTTOM, 5)
		combosizer.Add(self.bpm, 0, wx.ALIGN_CENTER_VERTICAL|wx.LEFT|wx.TOP|wx.BOTTOM, 5)
		combosizer.Add(self.tpblabel, 0, wx.ALIGN_CENTER_VERTICAL|wx.LEFT|wx.TOP|wx.BOTTOM, 5)
		combosizer.Add(self.tpb, 0, wx.ALIGN_CENTER_VERTICAL|wx.LEFT|wx.TOP|wx.BOTTOM, 5)
		sizer = wx.BoxSizer(wx.HORIZONTAL)
		sizer.Add(masterslider, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_LEFT|wx.LEFT, 5)
		sizer.Add(combosizer, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_LEFT, 0)
		self.SetAutoLayout(True)
		self.SetSizerAndFit(sizer)
		self.Layout()
		player.get_plugin(0).set_parameter_value(1, 0, 1, config.get_config().get_default_int('BPM', 126), 1)
		player.get_plugin(0).set_parameter_value(1, 0, 2, config.get_config().get_default_int('TPB', 4), 1)
		self.update_all()
		wx.EVT_SCROLL(self.masterslider, self.on_scroll_changed)
		wx.EVT_MOUSEWHEEL(self.masterslider, self.on_mousewheel)
		wx.EVT_SPINCTRL(self, self.bpm.GetId(), self.on_bpm)
		wx.EVT_SPINCTRL(self, self.tpb.GetId(), self.on_tpb)

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
		
	def on_bpm(self, event):
		"""
		Event handler triggered when the bpm spin control value is being changed.
		
		@param event: event.
		@type event: wx.Event
		"""
		player.get_plugin(0).set_parameter_value(1, 0, 1, self.bpm.GetValue(), 1)
		config.get_config().set_default_int('BPM', self.bpm.GetValue())

	def on_tpb(self, event):
		"""
		Event handler triggered when the tpb spin control value is being changed.
		
		@param event: event.
		@type event: wx.Event
		"""
		player.get_plugin(0).set_parameter_value(1, 0, 2, self.tpb.GetValue(), 1)
		config.get_config().set_default_int('TPB', self.tpb.GetValue())

	def on_scroll_changed(self, event):
		"""
		Event handler triggered when the master slider has been dragged.
		
		@param event: event.
		@type event: wx.Event
		"""
		vol = self.masterslider.GetValue()
		master = player.get_plugin(0)
		master.set_parameter_value(1, 0, 0, 16384 - vol, 1)

	def on_mousewheel(self, event):
		"""
		Sent when the mousewheel is used on the master slider.

		@param event: A mouse event.
		@type event: wx.MouseEvent
		"""
		vol = self.masterslider.GetValue()
		step = 16384 / 48
		if event.m_wheelRotation > 0:
			vol += step
		else:
			vol -= step
		vol = min(max(self.masterslider.GetMin(),vol), self.masterslider.GetMax())
		self.masterslider.SetValue(vol)
		self.on_scroll_changed(event)
		
	def update_all(self):
		"""
		Updates all controls.
		"""
		master = player.get_plugin(0)
		vol = master.get_parameter_value(1, 0, 0)
		bpm = master.get_parameter_value(1, 0, 1)
		tpb = master.get_parameter_value(1, 0, 2)
		self.bpm.SetValue(bpm)
		self.tpb.SetValue(tpb)
		self.masterslider.SetValue(16384 - vol)
		self.latency = driver.get_audiodriver().get_latency()
		self.ampl.amps.reset()
		self.ampr.amps.reset()

class TimePanel(wx.Panel):
	"""
	A toolbar displaying elapsed, current and loop time values.
	"""
	def __init__(self, *args, **kwds):
		"""
		Initializer.
		"""
		# begin wxGlade: SequencerFrame.__init__
		wx.Panel.__init__(self, *args, **kwds)
		self.timelabel = wx.StaticText(self, -1)
		self.starttime = time.time()
		self.update_label()		
		sizer = wx.BoxSizer(wx.HORIZONTAL)
		sizer.Add(self.timelabel, 0, wx.LEFT|wx.TOP|wx.BOTTOM, 5)
		self.SetAutoLayout(True)
		self.SetSizerAndFit(sizer)
		self.Layout()
		self.timer = wx.Timer(self, -1)
		self.timer.Start(100)
		wx.EVT_TIMER(self, self.timer.GetId(), self.update_label)
		
	def update_label(self, event = None):
		"""
		Event handler triggered by a 10fps timer event.
		
		@param event: Timer event.
		@type event: wx.Event
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
		text = 'Elapsed %s   Current %s   Loop %s' % (e,c,l)
		if text != self.timelabel.GetLabel():
			self.timelabel.SetLabel(text)
		#self.timelabel.SetLabel('%i' % player.get_position())


class AldrinApplication(wx.App):
	"""
	Application class. This one will be instantiated as a singleton.
	"""
	def OnInit(self):
		"""
		Called when the main loop initializes.
		"""
		self.frame=AldrinFrame(None,-1,'')
		self.SetTopWindow(self.frame)
		self.frame.Show()
		return 1

from optparse import OptionParser
parser = OptionParser()
parser.add_option("--profile", metavar="profile", default='', help="Start Aldrin with profiling enabled, save results to <profile>.")
app_options = None
app_args = None

def main():
	global app
	app = AldrinApplication(0)
	app.MainLoop()
	if player:
		import sys
		print >> sys.stderr, "uninitializing midi driver..."
		player.mididriver_close_all()
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
		import profile
		profile.runctx('main()', globals(), locals(), app_options.profile)
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
