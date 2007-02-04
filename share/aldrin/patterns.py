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
Contains all classes and functions needed to render the pattern
editor and its associated dialogs.
"""

import os, sys
from wximport import wx
from canvas import Canvas, ScrolledCanvas, BitmapBuffer
from utils import prepstr, filepath
import pickle
import zzub
import time

from utils import NOTES, roundint
PATLEFTMARGIN = 48
CONN = 0
GLOBAL = 1
TRACK = 2

patternsizes = [
1,4,8,12,16,24,32,48,64,96,128,192,256,512
]

class PatternDialog(wx.Dialog):
	"""
	Pattern Dialog Box.
	
	This dialog is used to create a new pattern or a copy of a pattern, and to modify existent patterns.
	"""
	def __init__(self, *args, **kwds):
		"""
		Initialization.
		"""
		wx.Dialog.__init__(self, *args, **kwds)
		self.btnok = wx.Button(self, wx.ID_OK, "OK")
		self.btncancel = wx.Button(self, wx.ID_CANCEL, "Cancel")
		self.namelabel = wx.StaticText(self, -1, "Name")
		self.edtname = wx.TextCtrl(self, -1, style=wx.TE_PROCESS_ENTER)
		self.lengthlabel = wx.StaticText(self, -1, "Length")
		self.lengthbox = wx.ComboBox(self, -1, style=wx.TE_PROCESS_ENTER)
		for s in patternsizes:
			self.lengthbox.Append(str(s))
		self.rowslabel = wx.StaticText(self, -1, "rows")		
		topgrid = wx.FlexGridSizer(2,3,5,5)
		#~ topgrid.AddGrowableCol(0, 0)
		#~ topgrid.AddGrowableCol(1, 0)
		#~ topgrid.AddGrowableRow(0, 0)
		#~ topgrid.AddGrowableRow(1, 0)
		rowgroup = wx.BoxSizer(wx.HORIZONTAL)
		rowgroup.Add(self.lengthbox, 1, wx.ALIGN_CENTER_VERTICAL | wx.ALL, 5)		
		rowgroup.Add(self.rowslabel, 1, wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT | wx.ALL, 5)		
		topgrid.Add(self.namelabel, 1, wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT | wx.ALL, 5)		
		topgrid.Add(self.edtname, 1, wx.ALIGN_CENTER_VERTICAL, 0)
		topgrid.Add(self.btnok, 1, wx.ALL, 2)
		topgrid.Add(self.lengthlabel, 1, wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT, 5)		
		topgrid.Add(rowgroup, 1, 0, 0)
		topgrid.Add(self.btncancel, 1, wx.ALL, 2)
		self.SetAutoLayout(True)
		self.SetSizerAndFit(topgrid)
		self.Layout()
		wx.EVT_TEXT_ENTER(self, self.edtname.GetId(), self.on_enter)
		wx.EVT_TEXT_ENTER(self, self.lengthbox.GetId(), self.on_enter)
	def on_enter(self, event):
		self.EndModal(wx.ID_OK)
		
		
# pattern dialog modes
DLGMODE_NEW = 0
DLGMODE_COPY = 1
DLGMODE_CHANGE = 2
def show_pattern_dialog(parent, name, length, dlgmode):
	"""
	Shows the pattern creation/modification dialog.
	
	@param parent: Parent container
	@type parent: wx.Window
	@param name: Pattern name
	@type name: string
	@param length: Pattern name
	@type length: int
	@param dlgmode: Dialog mode (DLGMODE_NEW: create new pattern, 
	DLGMODE_COPY: create copy of pattern, DLGMODE_CHANGE: modify pattern)
	@type dlgmode: int
	@return: Tuple containing pattern name and length
	@rtype: (string, int)
	"""
	dlg = PatternDialog(parent)
	dlg.dlgmode = dlgmode
	if dlgmode == DLGMODE_NEW:
		dlg.SetTitle("New Pattern")
	elif dlgmode == DLGMODE_COPY:
		dlg.SetTitle("Create copy of pattern")
		dlg.lengthbox.Enable(False)
	elif dlgmode == DLGMODE_CHANGE:
		dlg.SetTitle("Pattern Properties")
	dlg.edtname.SetValue(name)
	dlg.lengthbox.SetValue(str(length))
	dlg.edtname.SetSelection(-1,-1)
	dlg.edtname.SetFocus()
	if dlg.ShowModal() != wx.ID_OK:
		return
	dlg.Destroy()
	return str(dlg.edtname.GetValue()), int(dlg.lengthbox.GetValue())
		
class PatternToolBar(wx.Panel):
	"""
	Pattern Toolbar
	
	Contains lists of the plugins, patterns, waves and octaves available.
	"""
	def __init__(self, *args, **kwds):
		"""
		Initialization.
		"""
		# begin wxGlade: SequencerFrame.__init__
		self.parent = args[0]
		wx.Panel.__init__(self, *args, **kwds)
		self.pluginlabel = wx.StaticText(self, -1, label="&Plugin")
		self.pluginselect = wx.Choice(self, -1)
		self.patternlabel = wx.StaticText(self, -1, label="&Pattern")
		self.patternselect = wx.Choice(self, -1)
		self.wavelabel = wx.StaticText(self, -1, label="&Wave")
		self.waveselect = wx.Choice(self, -1)
		self.octavelabel = wx.StaticText(self, -1, label="&Base octave")
		self.octaveselect = wx.Choice(self, -1)
		self.playnotes = wx.CheckBox(self, -1, label="Play &notes")

		wx.EVT_CHOICE(self, self.pluginselect.GetId(), self.on_pluginselect)
		wx.EVT_CHOICE(self, self.patternselect.GetId(), self.on_patternselect)
		wx.EVT_CHOICE(self, self.waveselect.GetId(), self.on_waveselect)
		wx.EVT_CHOICE(self, self.octaveselect.GetId(), self.on_octaveselect)

		self.playnotes.SetValue(True)

		self.plugin = 0
		self.pattern = 0
		self.wave = 0
		self.cb2w = {} # combobox index to wave index
		
		mx,my = self.waveselect.GetMinSize()
		self.waveselect.SetMinSize((150,my))
		self.octaveselect.SetMinSize((50,my))
		
		sizer = wx.BoxSizer(wx.HORIZONTAL)
		sizer.Add(self.pluginlabel, 0, wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.TOP | wx.BOTTOM, 5)
		sizer.Add(self.pluginselect, 0, wx.EXPAND | wx.LEFT | wx.TOP | wx.BOTTOM, 5)
		sizer.Add(self.patternlabel, 0, wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.TOP | wx.BOTTOM, 5)
		sizer.Add(self.patternselect, 0, wx.EXPAND | wx.LEFT | wx.TOP | wx.BOTTOM, 5)
		sizer.Add(self.wavelabel, 0, wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.TOP | wx.BOTTOM, 5)
		sizer.Add(self.waveselect, 0, wx.EXPAND | wx.LEFT | wx.TOP | wx.BOTTOM, 5)
		sizer.Add(self.octavelabel, 0, wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.TOP | wx.BOTTOM, 5)
		sizer.Add(self.octaveselect, 0, wx.EXPAND | wx.LEFT | wx.TOP | wx.BOTTOM, 5)
		sizer.Add(self.playnotes, 0, wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.TOP | wx.BOTTOM, 5)
		self.SetAutoLayout(True)
		self.SetSizer(sizer)
		self.Layout()
		self.Fit()
	
	def reset(self):
		self.plugin = 0
		self.pattern = 0
		self.wave = 0
		
	def on_pluginselect(self, event):		
		"""
		Callback to handle selection of the pluginselect list.
		
		@param event: Selection event.
		@type event: wx.CommandEvent
		"""
		self.select_plugin(event.GetSelection())
		
	def on_patternselect(self, event):
		"""
		Callback to handle selection of the patternselect list.
		
		@param event: Selection event.
		@type event: wx.CommandEvent
		"""
		self.pattern = event.GetSelection()
		self.parent.view.pattern_changed()
		
	def on_waveselect(self, event):
		"""
		Callback to handle selection of the waveselect list.
		
		@param event: Selection event.
		@type event: wx.CommandEvent
		"""
		self.wave = self.cb2w[event.GetSelection()]
		self.parent.view.SetFocus()
		
	def on_octaveselect(self, event):
		"""
		Callback to handle selection of different octaves.
		
		@param event: Selection event.
		@type event: wx.CommandEvent
		"""
		self.parent.view.octave = event.GetSelection()
		self.parent.view.SetFocus()
		
	def update_pluginselect(self):
		"""
		Updates the plugin selection box.
		"""
		self.plugin = min(max(self.plugin, 0), player.get_plugin_count()-1)
		self.pluginselect.Clear()
		for plugin in player.get_plugin_list():
			self.pluginselect.Append(prepstr(plugin.get_name()))
		if self.plugin != -1:
			self.pluginselect.SetSelection(self.plugin)
		
	def next_wave(self):
		"""
		Selects the next wave.
		"""
		self.waveselect.SetSelection(min(self.waveselect.GetSelection()+1,self.waveselect.GetCount()-1))
		self.wave = self.cb2w[self.waveselect.GetSelection()]

	def prev_wave(self):
		"""
		Selects the previous wave.
		"""
		self.waveselect.SetSelection(max(self.waveselect.GetSelection()-1,0))
		self.wave = self.cb2w[self.waveselect.GetSelection()]
		
	def select_plugin(self, i):
		"""
		Selects a plugin
		
		@param i: Plugin index.
		@type i: int
		"""
		self.parent.view.selection = None
		self.parent.view.start_col = 0
		self.plugin = min(max(i, 0), player.get_plugin_count()-1)
		self.update_pluginselect()
		self.update_patternselect()
		self.parent.view.pattern_changed()

	def select_pattern(self, i):
		"""
		Selects a pattern.
		
		@param i: Pattern index.
		@type i: int
		"""
		if self.plugin != -1:
			plugin = player.get_plugin(self.plugin)
			self.pattern = min(max(i, 0),plugin.get_pattern_count()-1)
		else:
			self.pattern = -1
		self.update_patternselect()
		self.parent.view.pattern_changed()
		
	def prev_pattern(self):
		"""
		Selects the previous pattern.
		"""
		self.select_pattern(self.pattern-1)
		
	def next_pattern(self):
		"""
		Selects the next pattern.
		"""
		self.select_pattern(self.pattern+1)
			
	def update_patternselect(self):
		"""
		Rebuilds and updates the patternselect list.
		"""
		self.patternselect.Clear()
		if self.plugin != -1:
			mp = player.get_plugin(self.plugin)
			for p in mp.get_pattern_list():
				self.patternselect.Append(prepstr(p.get_name()))
			self.pattern = min(max(self.pattern, 0),mp.get_pattern_count()-1)
			if self.pattern != -1:
				self.patternselect.SetSelection(self.pattern)
		
	def update_waveselect(self):
		"""
		Rebuilds and updates the waveselect list.
		"""
		self.cb2w = {}
		self.waveselect.Clear()
		count = 0
		wi = 0
		for i in range(player.get_wave_count()):
			w = player.get_wave(i)
			if w.get_level_count() >= 1:
				self.waveselect.Append("%02X. %s" % (i+1, prepstr(w.get_name())))
				if i == self.wave:
					wi = count
				self.cb2w[count] = i
				count += 1
		if not wi:
			self.wave = 0
		self.waveselect.SetSelection(wi)
		
	def update_octaves(self):
		"""
		Rebuilds and updates the octaveselect list.
		"""
		self.octaveselect.Clear()
		for i in range(10):
			self.octaveselect.Append("%i" % i)
		self.octaveselect.SetSelection(self.parent.view.octave)
		
	def update_all(self):
		"""
		Updates the toolbar to reflect a pattern change.
		"""
		self.update_pluginselect()
		self.update_patternselect()
		self.update_waveselect()
		self.update_octaves()

class PatternPanel(wx.Panel):
	"""
	Panel containing the pattern toolbar and pattern view.
	"""
	def __init__(self, rootwindow, *args, **kwds):
		"""
		Initialization.
		
		@param rootwindow: Window that contains the component.
		@type rootwindow: main.AldrinFrame
		"""
		# begin wxGlade: SequencerFrame.__init__
		#kwds["style"] = wx.DEFAULT_PANEL_STYLE
		wx.Panel.__init__(self, *args, **kwds)
		self.rootwindow = rootwindow
		self.rootwindow.event_handlers.append(self.on_player_callback)
		self.toolbar = PatternToolBar(self, -1)		
		self.statusbar = wx.StatusBar(self, style=0)				 
		self.view = PatternView(rootwindow, self, -1)
		self.toolbar.update_all()
		self.__set_properties()
		self.__do_layout()
		# end wxGlade

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
		elif data.type == zzub.zzub_event_type_new_plugin:
			self.update_all()

	def reset(self):
		"""
		Resets the toolbar
		"""
		self.toolbar.reset()
		
	def update_all(self):
		"""
		Updates the toolbar and the pattern view to reflect a pattern change.
		"""
		self.toolbar.update_all()
		self.view.pattern_changed()

	def __set_properties(self):
		"""
		Sets properties during initialization.
		"""
		# begin wxGlade: SequencerFrame.__set_properties
		self.statusbar.SetFieldsCount(3)		
		self.statusbar.SetStatusWidths([-1, -1, -6])
		self.statusbar.SetSize((100, 20))
		# end wxGlade

	def __do_layout(self):
		"""
		Arranges children components during initialization.
		"""
		# begin wxGlade: SequencerFrame.__do_layout
		sizer = wx.BoxSizer(wx.VERTICAL)
		sizer.Add(self.toolbar, 0, wx.EXPAND, 0)
		sizer.Add(self.view, 1, wx.EXPAND, 0)
		sizer.Add(self.statusbar, 0,  wx.EXPAND, 0)
		self.SetAutoLayout(True)
		self.SetSizer(sizer)
		self.Layout()

from utils import fixbn, bn2mn, mn2bn, note2str, switch2str, byte2str, word2str

t2c = [
	note2str,
	switch2str,
	byte2str,
	word2str,
]
t2w = [3,1,2,4]
t2si = [2, 1, 2, 4]
t2siofs = [[0,2], [0], [0,1], [0,1,2,3]]


import config

def key_to_note(k):
	"""
	uses the active keymap to determine note and
	octave from a pressed key.
	
	@param k: Pressed key.
	@type k: int
	@return: a tuple consisting of octave and note or None.
	@rtype: (int,int)
	"""
	keymap = config.get_config().get_keymap()
	rows = keymap.split('|')
	k = chr(k)
	for row,index in zip(rows,range(len(rows))):
		if k in row:
			note = row.index(k)
			return index+(note/12), note%12
	return None

sc2note = {
	90:(0,0),
	83:(0,1),
	88:(0,2),
	68:(0,3),
	67:(0,4),
	86:(0,5),
	71:(0,6),
	66:(0,7),
	72:(0,8),
	78:(0,9),
	74:(0,10),
	77:(0,11),
	44:(1,0),
	81:(1,0),
	50:(1,1),
	87:(1,2),
	51:(1,3),
	69:(1,4),
	82:(1,5),
	53:(1,6),
	84:(1,7),
	54:(1,8),
	89:(1,9),
	55:(1,10),
	85:(1,11),
	73:(2,0),
	57:(2,1),
	79:(2,2),
	48:(2,3),
	80:(2,4),
}

def get_str_from_param(p,v):
	"""
	Extracts a string representation from value in context of a parameter.
	
	@param p: Parameter.
	@type p: zzub.Parameter
	@param v: Value.	
	@type v: int
	"""		
	return t2c[p.get_type()](p,v)

def get_length_from_param(p):
	"""
	Gets length of a parameter.
	
	@param p: Parameter.
	@type p: zzub.Parameter	
	"""	
	return t2w[p.get_type()]
	
def get_subindexcount_from_param(p):
	"""
	Gets subindex count of a parameter.
	
	@param p: Parameter.
	@type p: zzub.Parameter	
	"""	
	return t2si[p.get_type()]
	
def get_subindexoffsets_from_param(p):
	return t2siofs[p.get_type()]

# selection modes: column, track, tracks, all
SEL_COLUMN = 0
SEL_TRACK = 1
SEL_GROUP = 2
SEL_ALL = 3
SEL_COUNT = 4

class PatternView(Canvas):
	"""
	Pattern viewer class.
	"""	
	CLIPBOARD_MAGIC = "PATTERNDATA"
	
	class Selection:
		"""
		Selection class.
		
		Container for selection range and the selection mode.
		"""
		begin = -1
		end = -1
		group = 0
		track = 0		
		index = 0
		mode = SEL_COLUMN
	
	def __init__(self, rootwindow, *args, **kwds):
		"""
		Initialization.
		
		@param rootwindow: Window that contains the component.
		@type rootwindow: main.AldrinFrame
		"""
		kwds['style'] = wx.SUNKEN_BORDER | wx.WANTS_CHARS | wx.VSCROLL | wx.HSCROLL
		self.rootwindow = rootwindow
		self.patternsize = 16
		self.parent = args[0]
		self.pattern = None
		self.plugin = None
		self.row = 0
		self.index = 0
		self.track = 0
		self.group = 0
		self.subindex = 0
		self.row_step = 1
		self.octave = 4
		self.start_row = 0
		self.selection = None
		self.playpos = 0
		self.plugin_info = self.rootwindow.routeframe.view.plugin_info
		self.font = wx.Font(8, wx.FONTFAMILY_TELETYPE, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_BOLD)
		dc = wx.ScreenDC()
		dc.SetFont(self.font)
		fw, fh, fd, fel = dc.GetFullTextExtent("W")
		self.row_height = fh # row height
		self.top_margin = fh # top margin
		self.column_width = fw # column width
		# implements horizontal scrolling
		self.start_col = 0
		# prepare bitmap font
		self.bmpfont = BitmapBuffer(self.column_width*128,self.row_height,'#ffffff')
		self.bmpfont.SetFont(self.font)
		for c in range(32,128):
			self.bmpfont.DrawLabel(chr(c), wx.Rect(c*self.column_width, 0, self.column_width, self.row_height), 
			wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL)
		Canvas.__init__(self, *args, **kwds)
		self.timer = wx.Timer(self, -1)
		self.timer.Start(100)
		wx.EVT_TIMER(self, self.timer.GetId(), self.update_position)
		wx.EVT_MOUSEWHEEL(self, self.on_mousewheel)	
		wx.EVT_LEFT_DOWN(self, self.on_left_down)	
		wx.EVT_KEY_DOWN(self, self.on_key_down)	
		wx.EVT_CHAR(self, self.on_char)
		wx.EVT_CONTEXT_MENU(self, self.on_context_menu)
		wx.EVT_SCROLLWIN(self, self.on_scroll_window)
		self.pattern_changed()
		
	def on_copy(self, event):
		"""
		Sent when the copy function is selected from the menu.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		"""
		self.copy()

	def on_cut(self, event):
		"""
		Sent when the cut function is selected from the menu.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		"""
		self.cut()

	def on_paste(self, event):
		"""
		Sent when the paste function is selected from the menu.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		"""
		self.paste()

	def on_context_menu(self, event):
		"""
		Callback that constructs and displays the popup menu
		
		@param event: Menu event.
		@type event: wx.CommandEvent
		"""
		# create main items only once
		if not hasattr(self, "popup_add_track"):
			self.popup_add_track = wx.NewId()
			self.popup_delete_track = wx.NewId()
			self.popup_create_pattern = wx.NewId()
			self.popup_pattern_properties = wx.NewId()
			self.popup_remove_pattern = wx.NewId()
			self.popup_create_copy = wx.NewId()
			self.popup_solo = wx.NewId()

		self.Bind(wx.EVT_MENU, self.on_popup_add_track, id=self.popup_add_track)
		self.Bind(wx.EVT_MENU, self.on_popup_delete_track, id=self.popup_delete_track)
		self.Bind(wx.EVT_MENU, self.on_popup_create_pattern, id=self.popup_create_pattern)
		self.Bind(wx.EVT_MENU, self.on_popup_properties, id=self.popup_pattern_properties)
		self.Bind(wx.EVT_MENU, self.on_popup_remove_pattern, id=self.popup_remove_pattern)
		self.Bind(wx.EVT_MENU, self.on_popup_create_copy, id=self.popup_create_copy)
		self.Bind(wx.EVT_MENU, self.on_popup_solo, id=self.popup_solo)
			
		menu = wx.Menu()			
		menu.Append(self.popup_add_track, "Add track\tCtrl +")
		menu.Append(self.popup_delete_track, "Delete last track\tCtrl -")
		menu.AppendSeparator()
		menu.Append(self.popup_create_pattern, "New pattern...\tCtrl+Return")
		menu.Append(self.popup_pattern_properties, "Pattern properties...\tCtrl+Backspace")
		menu.Append(self.popup_remove_pattern, "Remove pattern...\tCtrl+Del")
		menu.Append(self.popup_create_copy, "Create copy...\tCtrl+Shift+Return")
		menu.AppendSeparator()
		menu.Append(self.popup_solo, "Solo Plugin\tCtrl+L", "Toggle solo", wx.ITEM_CHECK)
		item = menu.FindItemById(self.popup_solo)
		if self.rootwindow.routeframe.view.solo_plugin == self.get_plugin():
			item.Check()

		# Popup the menu.  If an item is selected then its handler
		# will be called before PopupMenu returns.
		self.PopupMenu(menu)
		menu.Destroy()
	
	def update_position(self, event):
		"""
		Updates the position.
		"""
		if not self.parent.IsShown(): return 	# Only needed by OSX
		playpos = player.get_position()
		if self.playpos != playpos:
			self.playpos = playpos
			self.Refresh()
			
	def get_new_pattern_name(self):
		"""
		Finds an unused pattern name.
		"""
		m = self.get_plugin()
		if not m:
			return
		patternid = 0
		while True:
			s = "%02i" % patternid
			found = False
			for p in m.get_pattern_list():
				if p.get_name() == s:
					found = True
					patternid += 1
					break
			if not found:
				break
		return s
		
	def init_values(self):
		"""
		Initializes pattern storage and information.
		"""
		# plugin
		self.plugin = None
		self.pattern = None
		# parameter count
		self.parameter_count = [0,0,0]
		self.parameter_width = [[],[],[]]
		self.lines = None
		self.row_count = 0
		self.parameter_type = None
		self.subindex_count = None
		self.group_position = [0,0,0]
		self.group_track_count = [0,0,0]
		datasource = self.get_datasource()
		if datasource:
			# plugin loader, pattern data
			self.plugin, self.pattern = datasource
			plugin = self.get_plugin()			
			if plugin in self.plugin_info:
				self.row, self.group, self.track, self.index, self.subindex = self.plugin_info[plugin].pattern_position
			self.input_connection_count = self.get_plugin().get_input_connection_count()
			# track count
			track_count = self.get_plugin().get_track_count()
			# global track not considered a track
			#~ if self.plugin.get_parameter_count(1) > 0:
				#~ self.track_count -= 1
			self.group_track_count = [self.input_connection_count, 1, track_count] # track count by group
			 # parameter counts
			self.parameter_count = [self.plugin.get_parameter_count(g) for g in range(3)]
			# parameter widths in columns
			self.parameter_width = [[get_length_from_param(self.plugin.get_parameter(g,i)) \
				for i in range(self.parameter_count[g])] for g in range(3)]
			# parameter type in columns
			self.parameter_type = [[self.plugin.get_parameter(g,i).get_type() \
				for i in range(self.parameter_count[g])] for g in range(3)]
			# track width in columns			
			self.track_width = [sum(self.parameter_width[g]) + self.parameter_count[g] for g in range(3)]
			# group positions
			self.group_position = [
				0, 
				(self.track_width[0]*self.group_track_count[0]), 
				(self.track_width[0]*self.group_track_count[0])+(self.track_width[1]*self.group_track_count[1])
			]
			# parameter positions, relative to track start
			self.parameter_position = []
			for g in range(3):
				pp = []
				x = 0
				for i in range(self.parameter_count[g]):
					pp.append(x)
					x += get_length_from_param(self.plugin.get_parameter(g,i)) + 1
				self.parameter_position.append(pp)
			# sub index counts
			self.subindex_count = [[get_subindexcount_from_param(self.plugin.get_parameter(g,i)) \
				for i in range(self.parameter_count[g])] for g in range(3)]
			# sub index offsets
			self.subindex_offset = [[get_subindexoffsets_from_param(self.plugin.get_parameter(g,i)) \
				for i in range(self.parameter_count[g])] for g in range(3)] 
			self.row_count = self.pattern.get_row_count() # row count
			self.prepare_textbuffer()
			self.adjust_scrollbars()
		# set the pattern position
		self.set_row(self.row)
		self.set_group(self.group)
		self.set_track(self.track)
		self.set_index(self.index)
		self.set_subindex(self.subindex)
		self.refresh_view()
		
	def adjust_scrollbars(self):
		w,h = self.GetClientSize()
		vw,vh = self.get_virtual_size()
		self.SetScrollbar(wx.HORIZONTAL, self.start_col, int((w - PATLEFTMARGIN) / float(self.column_width) + 0.5), vw)
		self.SetScrollbar(wx.VERTICAL, self.start_row, int((h - self.row_height) / float(self.row_height) + 0.5), vh)
		
	def set_octave(self, o):
		"""
		Sets the octave.
		
		@param o: Octave
		@type o: int
		"""
		self.octave = min(max(o,0), 9)
		
	def set_index(self, i):
		"""
		Sets the current index position.
		
		@param i: Index position.
		@type i: int
		"""
		self.index = min(max(i,0), self.parameter_count[self.group] - 1)
		
	def set_track(self, t):
		"""
		Sets the current track position.
		
		@param t: Track position.
		@type t: int
		"""
		self.track = min(max(t,0), self.group_track_count[self.group] - 1)
		
	def set_group(self, g):
		"""
		Sets the current group position.
		
		@param g: Group position.
		@type g: int
		"""
		g = min(max(g,0), 2)
		# skip empty groups
		if g < self.group:
			while g >= 0:
				if self.parameter_count[g] * self.group_track_count[g]:
					self.group = g
					return True
				g -= 1
		elif g >= self.group:
			while g <= 2:
				if self.parameter_count[g] * self.group_track_count[g]:
					self.group = g
					return True
				g += 1		
		return False

	def set_row(self, r):
		"""
		Sets the current row position.
		
		@param r: Row position.
		@type r: int
		"""
		self.row = min(max(r,0), self.row_count - 1)
		if self.row >= 0:
			w,h = self.GetClientSize()
			endrow = (((h - self.top_margin) / self.row_height) + self.start_row) - 1
			if (self.row < self.start_row):
				self.start_row = self.row
				self.ReDraw()
			elif (self.row >= endrow):
				self.start_row = self.row - (endrow - self.start_row)
				self.ReDraw()
		
	def set_subindex(self, si):
		"""
		Sets the current subindex position.
		
		@param si: Subindex position.
		@type si: int
		"""
		if not self.subindex_count:
			self.subindex = 0
			return
		self.subindex = min(max(si,0), self.subindex_count[self.group][self.index] - 1)
		
	def pattern_changed(self):
		"""
		Loads and redraws the pattern view after the pattern has been changed.
		"""		
		self.init_values()
		self.ReDraw()
		self.SetFocus()
		plugin = self.get_plugin()
		if plugin:
			self.plugin_info[plugin].patterngfx = {}

	def move_up(self, step = 1):
		"""
		Moves the cursor up.
		
		@param step: Amount the cursor is moved up.
		@type step: int
		"""	
		self.set_row(self.row - step)
		self.refresh_view()
		
	def move_down(self, step = 1):
		"""
		Moves the cursor down.
		
		@param step: Amount the cursor is moved down.
		@type step: int
		"""	
		self.set_row(self.row + step)
		self.refresh_view()
		
	def move_track_left(self):
		"""
		Moves the cursor one track position left.
		"""	
		if (self.track == 0):
			if (self.group == CONN) or not self.set_group(self.group - 1):
				return False
			self.set_track(self.group_track_count[self.group] - 1)
		else:
			self.set_track(self.track - 1)
		return True
		
	def move_index_left(self):
		"""
		Moves the cursor one index position left.
		"""	
		if self.index == 0:
			if not self.move_track_left():
				return False
			self.set_index(self.parameter_count[self.group] - 1)			
		else:
			self.set_index(self.index - 1)
		return True
		
	def move_subindex_left(self):
		"""
		Moves the cursor one subindex position left.
		"""	
		if self.subindex == 0:
			if not self.move_index_left():
				return False
			self.set_subindex(self.subindex_count[self.group][self.index] - 1)
		else:
			self.set_subindex(self.subindex - 1)
		return True
		
	def move_track_right(self):
		"""
		Moves the cursor one track position right.
		"""			
		if (self.track == self.group_track_count[self.group]-1):					
			if (self.group == TRACK) or not self.set_group(self.group + 1):
				return False
			self.set_track(0)
		else:
			self.set_track(self.track + 1)
		return True
		
	def move_index_right(self):
		"""
		Moves the cursor one index position right.
		"""	
		if self.index == (self.parameter_count[self.group] - 1):
			if not self.move_track_right():
				return False
			self.set_index(0)
		else:
			self.set_index(self.index + 1)
		return True
		
	def move_subindex_right(self):
		"""
		Moves the cursor one subindex position right.
		"""	
		if self.subindex == (self.subindex_count[self.group][self.index] - 1):
			if not self.move_index_right():
				return False
			self.set_subindex(0)
		else:
			self.set_subindex(self.subindex + 1)
		return True
		
	def show_cursor_left(self):
		"""
		Puts the cursor into visible frame after a jump to the left.
		"""
		w,h = self.get_charbounds()
		x,y = self.pattern_to_charpos(self.row, self.group, self.track, self.index, self.subindex)
		if x < self.start_col:
			self.start_col = max(x - (w / 3), 0)
			self.ReDraw()

	def show_cursor_right(self):
		"""
		Puts the cursor into visible frame after a jump to the right.
		"""
		w,h = self.get_charbounds()
		vw,vh = self.get_virtual_size()
		x,y = self.pattern_to_charpos(self.row, self.group, self.track, self.index, self.subindex)
		if x > w:
			self.start_col = min(self.start_col + x - w + (w / 3), vw - w + self.start_col)
			self.ReDraw()

	def move_left(self):
		"""
		Moves the cursor left.
		"""	
		self.move_subindex_left()
		self.show_cursor_left()
		self.refresh_view()
		
	def move_right(self):
		"""
		Moves the cursor right.
		"""	
		self.move_subindex_right()
		self.show_cursor_right()
		self.refresh_view()
		
	def adjust_selection(self):
		"""
		Adjusts the selection variables according to the selection mode.
		"""	
		if self.selection.mode > SEL_GROUP:
			self.selection.group = 0
		else:
			self.selection.group = self.group
		if self.selection.mode > SEL_TRACK:
			self.selection.track = 0
		else:
			self.selection.track = self.track
		if self.selection.mode > SEL_COLUMN:
			self.selection.index = 0
		else:
			self.selection.index = self.index
			
	def selection_range(self):
		"""
		Iterator that moves through the current selection.
		
		@return: Tuple pair of the next position (row, group, track, index)
		@rtype: (int, int, int, int)
		"""		
		if not self.selection:
			yield (self.row, self.group, self.track, self.index)
		elif self.selection.mode == SEL_COLUMN:
			for row in range(self.selection.begin, self.selection.end):
				yield (row, self.group, self.track, self.index)
		elif self.selection.mode == SEL_TRACK:
			for row in range(self.selection.begin, self.selection.end):
				for index in range(0, self.parameter_count[self.group]):
					yield (row, self.group, self.track, index)
		elif self.selection.mode == SEL_GROUP:
			for row in range(self.selection.begin, self.selection.end):
				for track in range(0, self.group_track_count[self.group]):
					for index in range(0, self.parameter_count[self.group]):
						yield (row, self.group, track, index)
		elif self.selection.mode == SEL_ALL:
			for row in range(self.selection.begin, self.selection.end):
				for group in range(3):
					tc = self.group_track_count[group]
					for track in range(0, tc):
						for index in range(0, self.parameter_count[group]):
							yield (row, group, track, index)
	
	def pattern_range(self):
		"""
		Iterator that moves through the entire pattern.
		
		@return: Tuple pair of the next position (row, group, track, index)
		@rtype: (int, int, int, int)
		"""
		for row in range(0, self.row_count):
			for group in range(3):
				tc = self.group_track_count[group]
				for track in range(0, tc):
					for index in range(0, self.parameter_count[group]):
						yield (row, group, track, index)
		
	def transpose_selection(self, offset):
		"""
		Transposes the current selection by an offset.
		
		@param offset: The amount that the values is incremented.
		@type offset: int
		"""
		for r,g,t,i in self.selection_range():
			p = self.plugin.get_parameter(g,i)
			v = self.pattern.get_value(r,g,t,i)
			if v != p.get_value_none():
				if (p.get_type() == 0):
					if v != zzub.zzub_note_value_off:
						v = max(min(mn2bn(bn2mn(v)+offset),p.get_value_max()),p.get_value_min())
				else:
					v = max(min(v+offset,p.get_value_max()),p.get_value_min())
				self.pattern.set_value(r,g,t,i,v)
		self.pattern_changed()
		
	def randomize_selection(self):
		"""
		Fills the current selection with random values.
		"""
		import random
		for r,g,t,i in self.selection_range():
			p = self.plugin.get_parameter(g,i)
			if (p.get_type() == 0):
				v = mn2bn(random.randrange(0,120))
			else:
				v = random.randrange(p.get_value_min(),p.get_value_max()+1)
			self.pattern.set_value(r,g,t,i,v)
		self.pattern_changed()

	def interpolate_selection(self):
		"""
		Fills the current selection with values interpolated from selection start to selection end.
		"""
		if not self.selection:
			return		
		for r,g,t,i in self.selection_range():
			p = self.plugin.get_parameter(g,i)
			v1 = self.pattern.get_value(self.selection.begin,g,t,i)
			v2 = self.pattern.get_value(self.selection.end-1,g,t,i)
			if (v1 != p.get_value_none()) and (v2 != p.get_value_none()):
				f = float(r - self.selection.begin) / float(self.selection.end - self.selection.begin)
				if (p.get_type() == 0):
					v1 = bn2mn(v1)
					v2 = bn2mn(v2)
					v = mn2bn(roundint((v2 - v1) * f + v1))
				else:
					v = roundint((v2 - v1) * f + v1)
				self.pattern.set_value(r,g,t,i,v)
		self.pattern_changed()

	def cut(self):
		"""
		Cuts the current selection into the clipboard
		"""
		if not self.selection:
			return
		self.copy()
		for r,g,t,i in self.selection_range():
			p = self.plugin.get_parameter(g,i)
			self.pattern.set_value(r,g,t,i,p.get_value_none())
		self.pattern_changed()

	def copy(self):
		"""
		Copies the current selection into the clipboard
		"""
		if not self.selection:
			return
		data = self.CLIPBOARD_MAGIC
		data += "%01x" % self.selection.mode		
		for r,g,t,i in self.selection_range():
			data += "%04x%01x%02x%02x%04x" % (r - self.selection.begin,g,t,i,self.pattern.get_value(r,g,t,i))
		clipboard = wx.TheClipboard
		if clipboard.Open():
			clipboard.SetData(wx.TextDataObject(data))
			clipboard.Close()
			
	def unpack_clipboard_data(self, d):
		"""
		Unpacks clipboard data
		
		@param d: Data that is to be unpacked.
		@type d: unicode
		"""
		magic,d = d[:len(self.CLIPBOARD_MAGIC)], d[len(self.CLIPBOARD_MAGIC):]
		assert magic == self.CLIPBOARD_MAGIC
		mode,d = int(d[:1],16),d[1:]
		yield mode
		while d:
			r,d = int(d[:4],16),d[4:]
			g,d = int(d[:1],16),d[1:]
			t,d = int(d[:2],16),d[2:]
			i,d = int(d[:2],16),d[2:]
			v,d = int(d[:4],16),d[4:]
			yield r,g,t,i,v
		
	def paste(self):
		"""
		Pastes the clipboard data into the pattern view.
		
		The pasting mechanism looks a bit weird but its effective.
		you can serialize pattern data in a pasteable form,
		and upon deserialization the app tries to make parameters
		as valid as possible.
		
		Buzz used to not paste at all if the format wasnt right
		we still try to make some sense out of what we get.
		"""
		
		clipboard = wx.TheClipboard
		data = ""
		if clipboard.Open():
			d = wx.TextDataObject()
			clipboard.GetData(d)
			data = d.GetText()
			clipboard.Close()
		try:
			gen = self.unpack_clipboard_data(data.strip())
			mode = gen.next()
			assert (mode >= 0) and (mode <= SEL_ALL)
			for r,g,t,i,v in gen:
				r = self.row + r
				assert (g >= 0) and (g <= 2)
				if (g < 0) or (g > 2):
					continue
				if (r < 0) or (r >= self.row_count):
					continue
				if mode == SEL_COLUMN: # am i in column paste mode?
					i = self.index # so paste at cursor column
				elif (i < 0) or (i >= self.parameter_count[g]): # if not, skip if out of bounds
					continue
				if mode in (SEL_TRACK, SEL_COLUMN): # am i pasting a track or a column?
					t = self.track # paste at cursor track
				elif (t < 0) or (t >= self.group_track_count[g]): # if not, skip if out of bounds
					continue
				p = self.plugin.get_parameter(g,i)
				ty = p.get_type()
				if v != p.get_value_none(): # if its not a none value
					if ty == 0: # is our target a note?
						v = fixbn(v) # fix it
						if v != zzub.zzub_note_value_off:
							v = min(max(v, p.get_value_min()),p.get_value_max()) # make sure it is properly clamped
					elif ty == 1: # switch
						v = v % 2 # even is zero, odd is one
						v = min(max(v, p.get_value_min()),p.get_value_max()) # make sure it is properly clamped
					elif ty == 2: # byte
						v = v & 0xFF # mask out first 8 bytes
						v = min(max(v, p.get_value_min()),p.get_value_max()) # make sure it is properly clamped
				self.pattern.set_value(r,g,t,i,v) # finally set it
			self.pattern_changed()
		except:
			import traceback
			traceback.print_exc()
			wx.MessageDialog(self, message="Couldn't paste.", caption = "Clipboard Error", style = wx.ICON_ERROR|wx.OK|wx.CENTER).ShowModal()
	
	def on_mousewheel(self, event):
		"""
		Callback that responds to mousewheeling in pattern view.
		
		@param event: Mouse event
		@type event: wx.MouseEvent
		"""		
		if event.GetWheelRotation() > 0:
			self.move_up()
		else:
			self.move_down()
	
	def on_left_down(self, event):		
		"""
		Callback that responds to left click in pattern view.
		
		@param event: Mouse event
		@type event: wx.MouseEvent
		"""
		self.SetFocus()
		row, group, track, index, subindex = self.pos_to_pattern(event.GetPosition())		
		self.set_row(row)
		self.set_group(group)
		self.set_track(track)
		self.set_index(index)
		self.set_subindex(subindex)
		self.refresh_view()
	
	def on_popup_remove_pattern(self, event=None):
		"""
		Callback that removes the current pattern.
		"""		
		dlg = wx.MessageDialog(self, message="sure?", caption = "Remove pattern", style = wx.ICON_EXCLAMATION|wx.YES_NO|wx.CENTER)
		result = dlg.ShowModal()
		if result == wx.ID_YES:
			m = self.get_plugin()			
			if self.pattern:
				m.remove_pattern(self.pattern)
			self.parent.toolbar.select_pattern(0)		
			
	def on_popup_create_pattern(self, event=None):
		"""
		Callback that creates a pattern.
		"""
		name = self.get_new_pattern_name()
		result = show_pattern_dialog(self,name,self.patternsize,DLGMODE_NEW)
		if not result:
			return
		name, self.patternsize = result
		m = self.get_plugin()
		p = m.create_pattern(self.patternsize)
		p.set_name(name)	
		for i in range(m.get_pattern_count()):
			if m.get_pattern(i) == p:
				self.parent.toolbar.select_pattern(i)
				break
	
	def on_popup_create_copy(self, event=None):
		"""
		Callback that creates a copy of the current pattern.
		"""
		name = self.get_new_pattern_name()
		result = show_pattern_dialog(self,name,self.row_count,DLGMODE_COPY)
		if not result:
			return
		name, self.patternsize = result
		m = self.get_plugin()
		p = m.create_pattern(self.row_count)
		p.set_name(name)
		for r,g,t,i in self.pattern_range():
			p.set_value(r,g,t,i,self.pattern.get_value(r,g,t,i))
		for i in range(m.get_pattern_count()):
			if m.get_pattern(i) == p:
				self.parent.toolbar.select_pattern(i)
				break
	
	def on_popup_solo(self, event=None):
		"""
		Callback that solos current plugin.
		"""		
		plugin = self.get_plugin()
		self.rootwindow.routeframe.view.solo(plugin)
		
	def on_popup_properties(self, event=None):
		"""
		Callback that shows the properties of the current pattern.
		"""
		result = show_pattern_dialog(self,self.pattern.get_name(),self.pattern.get_row_count(),DLGMODE_CHANGE)
		if not result:
			return
		name, rc = result
		self.patternsize = rc
		if self.pattern.get_name() != name:
			self.pattern.set_name(name)
		if self.pattern.get_row_count() != rc:
			self.pattern.set_row_count(rc)
		self.parent.toolbar.update_all()
		self.pattern_changed()
		
	def on_popup_add_track(self, event=None):
		"""
		Callback that adds a track.
		"""
		m = self.get_plugin()		
		m.set_track_count(m.get_track_count()+1)
		self.pattern_changed()

	
	def on_popup_delete_track(self, event=None):
		"""
		Callback that deletes last track.
		"""		
		dlg = wx.MessageDialog(self, message="sure?", caption = "Delete last track", style = wx.ICON_EXCLAMATION|wx.YES_NO|wx.CENTER)
		result = dlg.ShowModal()
		if result == wx.ID_YES:
			m = self.get_plugin()
			m.set_track_count(m.get_track_count()-1)
			self.pattern_changed()	
	
	def on_key_down(self, event):
		"""
		Callback that responds to key stroke in pattern view.
		
		@param event: Key event
		@type event: wx.KeyEvent
		"""		
		k = event.GetKeyCode()
		if event.ShiftDown() and event.ControlDown():
			if k == wx.WXK_RETURN:
				self.on_popup_create_copy()
			else:
				event.Skip()
		elif k == wx.WXK_TAB:
			if event.ShiftDown():
				# move to previous track
				if self.move_track_left():
					self.set_index(0)
					self.set_subindex(0)
					self.show_cursor_left()
					self.refresh_view()
			else:
				# move to next track
				if self.move_track_right():
					self.set_index(0)
					self.set_subindex(0)
					self.show_cursor_right()
					self.refresh_view()
		elif event.ShiftDown():			
			if k == wx.WXK_NUMPAD_ADD:
				self.transpose_selection(1)
			elif k == wx.WXK_NUMPAD_SUBTRACT:
				self.transpose_selection(-1)
			else:
				event.Skip()
		elif event.ControlDown():
			if k == wx.WXK_RETURN:
				self.on_popup_create_pattern()				
			elif k == wx.WXK_BACK:	
				self.on_popup_properties()
			elif k == wx.WXK_DELETE:
				self.on_popup_remove_pattern()
			elif k >= ord('1') and k <= ord('9'):
				self.row_step = k - ord('1') + 1
			elif k == ord('B'):
				if not self.selection:
					self.selection = self.Selection()
				else:
					if self.selection.begin == self.row:
						self.selection.mode = (self.selection.mode + 1) % 4
				self.selection.begin = self.row
				self.selection.end = max(self.row+1,self.selection.end)				
				self.adjust_selection()
				self.ReDraw()
			elif k == ord('E'):
				if not self.selection:
					self.selection = self.Selection()
				else:
					if self.selection.end == self.row+1:
						self.selection.mode = (self.selection.mode + 1) % 4
				self.selection.end = self.row+1
				self.selection.begin = max(min(self.selection.end-1,self.selection.begin),0)
				self.adjust_selection()
				self.ReDraw()
			elif k == ord('C'):
				self.copy()
			elif k == ord('V'):
				self.paste()
			elif k == ord('X'):
				self.cut()
			elif k == ord('R'):
				self.randomize_selection()
			elif k == ord('I'):
				self.interpolate_selection()
			elif k == ord('U'):
				self.selection = None
				self.ReDraw()
			elif k == ord('L'):
				self.on_popup_solo()
			elif k == wx.WXK_NUMPAD_ADD:
				self.on_popup_add_track()
			elif k == wx.WXK_NUMPAD_SUBTRACT:
				self.on_popup_delete_track()
			elif k == wx.WXK_UP:
				self.parent.toolbar.select_plugin(self.parent.toolbar.plugin-1)
			elif k == wx.WXK_DOWN:
				self.parent.toolbar.select_plugin(self.parent.toolbar.plugin+1)
			else:
				event.Skip()
		elif k == wx.WXK_LEFT or k == wx.WXK_NUMPAD_LEFT:
			self.move_left()
		elif k == wx.WXK_RIGHT or k == wx.WXK_NUMPAD_RIGHT:
			self.move_right()
		elif k == wx.WXK_UP or k == wx.WXK_NUMPAD_UP:
			self.move_up()
		elif k == wx.WXK_DOWN or k == wx.WXK_NUMPAD_DOWN:
			self.move_down()
		elif k == wx.WXK_PRIOR or k == wx.WXK_NUMPAD_PRIOR:
			self.move_up(16)
		elif k == wx.WXK_NEXT or k == wx.WXK_NUMPAD_NEXT:
			self.move_down(16)
		elif k == wx.WXK_HOME:
			# 1st: move to begin of track
			# 2nd: move to begin of group
			# 3rd: move to first group
			# 4th: move to first row
			if self.index != 0:
				self.set_index(0)
				self.set_subindex(0)
			elif self.track != 0:
				self.set_track(0)
				self.set_index(0)
				self.set_subindex(0)
			elif not self.set_group(1):
				self.set_row(0)
			self.show_cursor_left()
			self.refresh_view()
		elif k == wx.WXK_INSERT:
			self.pattern.insert_row(self.group, self.track, -1, self.row)
			self.pattern_changed()
		elif k == wx.WXK_DELETE:
			self.pattern.delete_row(self.group, self.track, -1, self.row)
			self.pattern_changed()
		elif k == wx.WXK_RETURN:
			mainwindow = self.parent.rootwindow
			mainwindow.select_page(mainwindow.PAGE_SEQUENCER)
		elif k == wx.WXK_NUMPAD_ADD:			
			self.parent.toolbar.next_pattern()
		elif k == wx.WXK_NUMPAD_SUBTRACT:
			self.parent.toolbar.prev_pattern()
		elif k == wx.WXK_NUMPAD_MULTIPLY:
			self.set_octave(self.octave+1)
			self.parent.toolbar.update_octaves()
		elif k == wx.WXK_NUMPAD_DIVIDE:
			self.set_octave(self.octave-1)
			self.parent.toolbar.update_octaves()
		elif self.plugin:		
			p = self.plugin.get_parameter(self.group,self.index)
			param_type = p.get_type()
			playtrack = False
			if param_type == 0 and k in range(256): # note
				# is there a wavetable param?
				wi = None
				wp = None
				wdata = None
				for i in range(self.parameter_count[self.group]):
					pwp = self.plugin.get_parameter(self.group,i)
					if pwp.get_flags() & zzub.zzub_parameter_flag_wavetable_index:
						wp = pwp
						wi = i
						break
				if self.subindex == 0:
					on = key_to_note(k)
					if on:
						o,n = on
						data = (min(self.octave+o,9)<<4) | (n+1)
						if wp != None:
							wdata = self.parent.toolbar.wave+1
						playtrack = True
					elif k == ord('.'):
						data = p.get_value_none()
						if wp != None:
							wdata = wp.get_value_none()
					elif k == ord('1'):
						data = zzub.zzub_note_value_off
						if wp != None:
							wdata = wp.get_value_none()
						playtrack = True
					else:
						event.Skip()
						return
					if wdata != None:
						self.pattern.set_value(self.row, self.group, self.track, wi, wdata)
				elif (self.subindex == 1) and (k >= ord('1')) and (k <= ord('9')):
						o = k-ord('1')+1
						data = (self.pattern.get_value(self.row, self.group, self.track, self.index) & 0xf) | (o << 4)
				else:
					event.Skip()
					return
			elif param_type == 1: # switch
				if k == ord('1') or k == ord('0'):
					data = {ord('1'):p.get_value_max(), ord('0'):p.get_value_min()}[k]
				elif k == ord('.'):
					data = p.get_value_none()
				else:
					event.Skip()
					return
			elif param_type in (2,3): # byte or word
				pw = self.parameter_width[self.group][self.index]
				if k >= ord('0') and k <= ord('9'):
					o = k-ord('0')
				elif k >= ord('A') and k <= ord('F'):
					o = 10 + k-ord('A')
				elif k == ord('.'):
					o = None
					data = p.get_value_none()
				else:
					event.Skip()
					return
				if o != None:
					bofs = (pw - self.subindex - 1)*4
					data = self.pattern.get_value(self.row, self.group, self.track, self.index)
					if data == p.get_value_none():
						data = 0
					data = (data ^ (data & (0xf << bofs))) | (o << bofs) # mask out old nibble, put in new nibble
					data = min(p.get_value_max(),max(p.get_value_min(), data))
					if p.get_flags() & zzub.zzub_parameter_flag_wavetable_index:
						self.parent.toolbar.wave = data - 1
						self.parent.toolbar.update_waveselect()
			else:
				event.Skip()
				return
			self.pattern.set_value(self.row, self.group, self.track, self.index, data)
			if playtrack and self.parent.toolbar.playnotes.GetValue():
				m = self.get_plugin()
				player.lock_tick()
				try:
					for index in range(self.parameter_count[self.group]):
						v = self.pattern.get_value(self.row, self.group, self.track, index)
						p = self.plugin.get_parameter(self.group, index)
						if v != p.get_value_none():
							m.set_parameter_value(self.group, self.track, index, v, 0)
					m.tick()
				except:
					import traceback
					traceback.print_exc()
				player.unlock_tick()
			self.update_line(self.row)
			self.ReDraw(self.row,self.row+1,False)
			self.move_down(self.row_step)
		else:
			event.Skip()
	
	def on_char(self, event):
		"""
		Callback that responds to key stroke in pattern view.
		
		@param event: Key event
		@type event: wx.KeyEvent
		"""		
		event.Skip()
		k = event.GetKeyCode()		
		if k == ord('<'):
			self.parent.toolbar.prev_wave()
		elif k == ord('>'):
			self.parent.toolbar.next_wave()

	def pattern_to_charpos(self, row, group, track=0, index=0, subindex=0):
		"""
		Converts a pattern position into a (x,y) character coordinate.
		
		@param row: Pattern row
		@param group: Specific pattern group (Connection, Global or Tracks)
		@param track: Track of the group
		@param index: Parameter index of the track
		@param subindex: Subindex of the index
		@type row, group, track, index, subindex: int
		@return: (x,y) character coordinate
		@rtype: (int, int)
		"""
		y = row
		x = self.group_position[group]
		if self.parameter_count[group]:
			x += self.parameter_position[group][index]
			x += self.subindex_offset[group][index][subindex]
		if group in (CONN, TRACK):
			x += self.track_width[group]*track
		return x,y

	def pattern_to_pos(self, row, group, track=0, index=0, subindex=0):
		"""
		Converts a pattern position into a (x,y) pixel coordinate.
		
		@param row: Pattern row
		@param group: Specific pattern group (Connection, Global or Tracks)
		@param track: Track of the group
		@param index: Parameter index of the track
		@param subindex: Subindex of the index
		@type row, group, track, index, subindex: int
		@return: (x,y) pixel coordinate
		@rtype: (int, int)
		"""
		x,y = self.pattern_to_charpos(row,group,track,index,subindex)
		return ((x - self.start_col)*self.column_width) + PATLEFTMARGIN + 4, self.top_margin + ((y - self.start_row) * self.row_height)
		
	def charpos_to_pattern(self, position):
		"""
		Converts a (x,y) character coordinate into a pattern position.
		
		@param position: Character coordinate.
		@type position: (int, int)
		@return: (row, group, track, index, subindex) representing a pattern position.
		@rtype: (int, int, int, int, int)
		"""
		x, y = position
		# find group
		if x < self.group_position[1]:
			group = 0
		elif x < self.group_position[2]:
			group = 1
		else:
			group = 2
		x -= self.group_position[group]
		# find track
		track = self.track
		out_of_bounds = False
		if self.track_width[group] != 0:
			track = x / self.track_width[group]
			x -= track*self.track_width[group]
			# bounds checking
			if track >= self.group_track_count[group] or track < 0:
				track = self.track
				out_of_bounds = True			
		# find index, subindex
		index = self.index
		subindex = self.subindex
		if not out_of_bounds:
			if self.parameter_count[group]:
				for i, pos in enumerate(self.parameter_position[group]):
					if x < pos:
						index = i-1
						break
				else:
					# last index
					index = i			
				x -= self.parameter_position[group][index]
				# subindex is that what remains
				subindex = x
		# find row
		row = y
		return (row, group, track, index, subindex)
		
	def pos_to_pattern(self, position):		
		"""
		Converts a (x,y) pixel coordinate into a pattern position.
		
		@param position: Pixel coordinate.
		@type position: (int, int)
		@return: (row, group, track, index, subindex) representing a pattern position.
		@rtype: (int, int, int, int, int)
		"""
		x, y = position
		return self.charpos_to_pattern(((x - PATLEFTMARGIN - 4) / self.column_width + self.start_col, (y - self.top_margin) / self.row_height + self.start_row))
		
	def get_charbounds(self):
		"""
		Returns the outermost coordinates in characters.
		"""
		w,h = self.GetClientSize()
		w -= PATLEFTMARGIN + 4
		h -= self.top_margin
		return self.start_col + (w / self.column_width) - 1, self.start_row + (h / self.row_height) - 1
		
	def get_virtual_size(self):
		"""
		Returns the size in characters of the virtual view area.
		"""
		h = self.pattern.get_row_count()
		w = 0
		for g in CONN,GLOBAL,TRACK:
			w += self.track_width[g] * self.group_track_count[g]
		return w,h
		
	def on_scroll_window(self, event):
		"""
		Handles window scrolling.
		"""
		self.start_col, self.start_row = self.GetScrollPos(wx.HORIZONTAL), self.GetScrollPos(wx.VERTICAL)
		self.ReDraw()
	
	def refresh_view(self):
		if not self.plugin:
			return
		# store current position
		plugin = self.get_plugin()
		#~ self.plugin_info = self.rootwindow.routeframe.view.plugin_info
		self.plugin_info[plugin].pattern_position = (self.row, self.group, self.track, self.index, self.subindex)		
		if self.plugin:
			if self.parameter_count[self.group] and self.group_track_count[self.group]:
				# change status bar
				self.parent.statusbar.SetStatusText('Row %s, Track %s' % (self.row,self.track), 0)
				parameter_list = self.plugin.get_parameter_list(self.group)
				self.parent.statusbar.SetStatusText(prepstr(parameter_list[self.index].get_description() or ""), 2)
				p = self.plugin.get_parameter(self.group,self.index)
				v = self.pattern.get_value(self.row, self.group, self.track, self.index)
				if v != p.get_value_none():
					text = prepstr(self.get_plugin().describe_value(self.group,self.index,v))
					s = get_str_from_param(p,self.pattern.get_value(self.row, self.group, self.track, self.index))
					self.parent.statusbar.SetStatusText("%s (%i) %s" % (s,v,text), 1)
				else:
					self.parent.statusbar.SetStatusText("", 1)
		self.Refresh()
		self.adjust_scrollbars()
	
	def onPostPaint(self, dc):
		"""
		Overriding a L{Canvas} method that is called after painting is completed. 
		Draws an XOR play cursor over the pattern view.
		
		@param dc: wx device context.
		@type dc: wx.PaintDC
		"""
		if not self.pattern:
			return
		w,h = self.GetClientSize()
		bbrush = wx.Brush('#ffffff',wx.SOLID)		
		dc.SetBrush(bbrush)
		dc.SetPen(wx.TRANSPARENT_PEN)
		dc.SetLogicalFunction(wx.XOR)
		dc.SetFont(self.font)
		PATROWHEIGHT = self.row_height
		PATTOPMARGIN = self.top_margin
		PATCOLWIDTH = self.column_width	
		cx,cy = self.pattern_to_pos(self.row, self.group, self.track, self.index, self.subindex)
		if (cx >= (PATLEFTMARGIN+4)) and (cy >= self.top_margin):
			dc.DrawRectangle(cx,cy,self.column_width,self.row_height)
		# draw play cursor
		current_position = player.get_position()
		seq = player.get_current_sequencer()
		for i in range(seq.get_track_count()):
			track = seq.get_track(i)
			track_plugin = track.get_plugin()
			plugin = self.get_plugin()			
			if plugin == track_plugin and self.pattern:			
				row_count = self.pattern.get_row_count()		
				events = track.get_event_list()
				for i, pair in enumerate(events):
					pos, value = pair
					# make sure event is a pattern
					if value >= 0x10:						
						pattern = plugin.get_pattern(value-0x10)
					else:
						continue
					# handle overlapping of patterns
					if i < len(events)-1 and events[i+1][0] <= current_position:						
						continue
					if self.pattern == pattern and pos < current_position \
					and current_position < pos + row_count:						
						y = self.top_margin + (current_position - pos - self.start_row)*self.row_height						 						
						w,h = self.GetSize()						
						dc.DrawRectangle(0, y, w, 2)
						return						
	def get_plugin(self):
		"""
		Returns the plugin of the pattern in the pattern view.
		
		@return: zzub plugin plugin.
		@rtype: zzub.Plugin
		"""	
		toolbar = self.parent.toolbar
		mi = min(toolbar.plugin, player.get_plugin_count()-1)
		if mi == -1:
			return
		return player.get_plugin(mi)
		
	def get_datasource(self):
		"""
		Returns the plugin and the current pattern in the pattern view
		
		@return: A tuple holding the plugin and the current pattern
		@rtype: (zzub.Plugin, zzub.Pattern)
		"""	
		tb = self.parent.toolbar
		plugin = self.get_plugin()
		pl = plugin.get_pluginloader()
		if not pl._handle:
			return
		pi = min(tb.pattern, plugin.get_pattern_count()-1)
		if pi == -1:
			return
		pattern = plugin.get_pattern(pi)		
		return pl, pattern
		
	def update_line(self, row):
		"""
		Updates a line of the pattern.
		
		@param row: Line that will be updated.
		@type row: int
		"""	
		for g in range(3):
			tc = self.group_track_count[g]
			for t in range(tc):
				s = ''
				for i in range(self.parameter_count[g]):
					p = self.plugin.get_parameter(g,i)
					s += get_str_from_param(p,self.pattern.get_value(row, g, t, i))
					if i != self.parameter_count[g]-1:
						s += ' '
				self.lines[row][g][t] = s
		
	def prepare_textbuffer(self):
		"""
		Initializes a buffer to handle the current pattern data.
		"""	
		self.lines = [None]*self.row_count
		for row in range(self.row_count):
			s = ''
			self.lines[row] = [None]*3
			for g in range(3):
				tc = self.group_track_count[g]
				self.lines[row][g] = [None]*tc
			self.update_line(row)

	def DrawBuffer(self,row=None,rows=None,fulldraw=True):	
		"""
		Overriding a L{Canvas} method that paints onto an offscreen buffer.
		Draws the pattern view graphics.
		"""	
		st = time.time()

		if row == None:
			row = self.start_row
		cfg = config.get_config()
		dc = self.buffer		
		w,h = self.GetSize()
		dc.SetFont(self.font)
		PATROWHEIGHT = self.row_height
		PATTOPMARGIN = self.top_margin
		PATCOLWIDTH = self.column_width
		bgbrush = cfg.get_brush('PE BG')
		fbrush1 = cfg.get_brush('PE BG Very Dark')
		fbrush2 = cfg.get_brush('PE BG Dark')
		selbrush = cfg.get_brush('PE Sel BG')
		
		pen = cfg.get_pen('PE Text')
		dc.SetBackgroundMode(wx.SOLID)
		dc.SetBackground(bgbrush)
		dc.SetBrush(bgbrush)
		dc.SetTextBackground(cfg.get_color('PE BG'))
		dc.SetTextForeground(cfg.get_color('PE Text'))
		# clear the view if no current pattern
		if not self.pattern:
			dc.Clear()
			return
			
		if not rows:
			rows = self.row_count		
		clipy1 = PATROWHEIGHT + ((row - self.start_row) * self.row_height)
		clipy2 = PATROWHEIGHT + ((rows - self.start_row) * self.row_height)
		
		start_row, start_group, start_track, start_index, start_subindex = self.charpos_to_pattern((self.start_col, self.start_row))
		
		# full draw clears everything and draws all the line numbers and lines
		if fulldraw:
			dc.Clear()
			# 14
			x, y = PATLEFTMARGIN, PATROWHEIGHT
			i = row
			y = clipy1
			dc.SetPen(wx.TRANSPARENT_PEN)
			#dc.SetTextBackground('#dad6c9')
			while (i < rows) and (y < h):
				dc.DrawLabel("%s" % i, wx.Rect(0, y, x-5, PATROWHEIGHT), wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL)
				y += PATROWHEIGHT
				i += 1
			dc.SetPen(pen)
			dc.DrawLine(x, 0, x, h)
			y = PATROWHEIGHT-1
			dc.DrawLine(0, y, w, y)
		startx = PATLEFTMARGIN + 4
		i = row
		y = clipy1
		dc.SetClippingRegion(startx, 0, w - startx, h)
		if self.lines:
			tc = self.group_track_count
			def draw_parameters(row, dc, group, track=0):
				"""Draw the parameter values"""
				xs,y = self.pattern_to_pos(row, group, track, 0)
				x = xs
				s = self.lines[row][group][track]
				w = PATCOLWIDTH * len(s)
				dc.DrawLabel(s, wx.Rect(x,y,w,PATROWHEIGHT), wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL)
			# draw track background
			dc.SetBackgroundMode(wx.SOLID)
			dc.SetPen(wx.TRANSPARENT_PEN)
			while (i < rows) and (y < h):
				# darker colour lines each 4 and 16 lines
				if (i % 16) == 0:
					dc.SetBrush(fbrush1)
				elif (i % 4) == 0:
					dc.SetBrush(fbrush2)
				else:
					dc.SetBrush(bgbrush)
				for g in CONN,GLOBAL,TRACK:
					if self.track_width[g]:
						for t in range(self.group_track_count[g]):
							if ((g == start_group) and (t >= start_track)) or (g > start_group):
								xs,fy = self.pattern_to_pos(row, g, t, 0)
								dc.DrawRectangle(xs,y,(self.track_width[g]-1)*self.column_width,self.row_height)
				i += 1
				y += PATROWHEIGHT
			# draw selection
			if self.selection:
				dc.SetBrush(selbrush)
				x,y1 = self.pattern_to_pos(self.selection.begin, 
					self.selection.group, self.selection.track, self.selection.index)
				x,y2 = self.pattern_to_pos(self.selection.end, 
					self.selection.group, self.selection.track, self.selection.index)
				y1 = max(clipy1, y1)
				y2 = min(clipy2, y2)
				if y2 > y1:
					if self.selection.mode == SEL_COLUMN:
						x2 = self.parameter_width[self.selection.group][self.selection.index]*self.column_width
						dc.DrawRectangle(x,y1,x2,y2-y1)
					elif self.selection.mode == SEL_TRACK:
						x2 = (self.track_width[self.selection.group]-1)*self.column_width
						dc.DrawRectangle(x,y1,x2,y2-y1)
					elif self.selection.mode == SEL_GROUP:
						for t in range(tc[self.selection.group]):
							x2 = (self.track_width[self.selection.group]-1)*self.column_width
							dc.DrawRectangle(x,y1,x2,y2-y1)
							x += self.track_width[self.selection.group] * self.column_width
					elif self.selection.mode == SEL_ALL:
						for g in range(3):
							if self.track_width[g]:
								for t in range(tc[g]):
									dc.DrawRectangle(x,y1,(self.track_width[g]-1)*self.column_width,y2-y1)
									x += self.track_width[g] * self.column_width
			# draw the parameter values
			i = row
			y = clipy1
			dc.SetBackgroundMode(wx.TRANSPARENT)
			for track in range(self.group_track_count[TRACK]):
				x, y = self.pattern_to_pos(row, TRACK, track, 0)								
				s = str(track)
				w = self.track_width[TRACK]*self.column_width
				dc.DrawLabel(s, wx.Rect(x,0,w,PATROWHEIGHT), wx.ALIGN_CENTER | wx.ALIGN_CENTER_VERTICAL)
			while (i < rows) and (y < h):
				x = PATLEFTMARGIN + 5
				for t in range(self.group_track_count[CONN]):
					draw_parameters(i, dc, CONN, t)
				draw_parameters(i, dc, GLOBAL, 0)
				for t in range(self.group_track_count[TRACK]):
					draw_parameters(i, dc, TRACK, t)
				i += 1
				y += PATROWHEIGHT
		#~ print "%ims" % ((time.time() - st)*1000)
		dc.DestroyClippingRegion()

__all__ = [
'PatternDialog',
'show_pattern_dialog',
'PatternToolBar',
'PatternPanel',
'get_str_from_param',
'get_length_from_param',
'get_subindexcount_from_param',
'get_subindexoffsets_from_param',
'PatternView',
]

if __name__ == '__main__':
	import sys, utils
	from main import run
	sys.argv.append('/home/paniq/stuff.ccm')
	run(sys.argv)
