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
from gtkimport import gtk
import gobject
import pango
from utils import prepstr, filepath, get_item_count, get_clipboard_text, set_clipboard_text, question, error, get_new_pattern_name
import pickle
import zzub
import time
import random
import common
player = common.get_player()
from common import MARGIN, MARGIN2, MARGIN3, MARGIN0
from guievents import global_events

from utils import NOTES, roundint
PATLEFTMARGIN = 48
CONN = 0
GLOBAL = 1
TRACK = 2

patternsizes = [
1,4,8,12,16,24,32,48,64,96,128,192,256,512
]

class PatternDialog(gtk.Dialog):
	"""
	Pattern Dialog Box.
	
	This dialog is used to create a new pattern or a copy of a pattern, and to modify existent patterns.
	"""
	def __init__(self, parent):
		"""
		Initialization.
		"""
		gtk.Dialog.__init__(self,
			"Pattern Properties",
			parent.get_toplevel(),
			gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
			None
		)		
		vbox = gtk.VBox(False, MARGIN)
		vbox.set_border_width(MARGIN)
		self.btnok = self.add_button(gtk.STOCK_OK, gtk.RESPONSE_OK)
		self.btncancel = self.add_button(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL)
		self.namelabel = gtk.Label("Name")
		self.edtname = gtk.Entry()
		self.lengthlabel = gtk.Label("Length")
		self.lengthbox = gtk.combo_box_entry_new_text()
		self.chkswitch = gtk.CheckButton('Switch to new pattern')
		for s in patternsizes:
			self.lengthbox.append_text(str(s))
		self.rowslabel = gtk.Label("Rows")
		sgroup1 = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
		sgroup2 = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
		def add_row(c1, c2):
			row = gtk.HBox(False, MARGIN)
			c1.set_alignment(1, 0.5)
			row.pack_start(c1, expand=False)
			row.pack_start(c2)
			sgroup1.add_widget(c1)
			sgroup2.add_widget(c2)
			vbox.pack_start(row, expand=False)
		add_row(self.namelabel, self.edtname)
		add_row(self.rowslabel, self.lengthbox)
		vbox.pack_start(self.chkswitch, expand=False)
		self.edtname.connect('activate', self.on_enter)
		self.lengthbox.child.connect('activate', self.on_enter)
		self.vbox.add(vbox)
		self.show_all()
		
	def on_enter(self, widget):
		self.response(gtk.RESPONSE_OK)
		
# pattern dialog modes
DLGMODE_NEW = 0
DLGMODE_COPY = 1
DLGMODE_CHANGE = 2
def show_pattern_dialog(parent, name, length, dlgmode):
	"""
	Shows the pattern creation/modification dialog.
	
	@param parent: Parent container
	@type parent: gtk.Widget
	@param name: Pattern name
	@type name: string
	@param length: Pattern name
	@type length: int
	@param dlgmode: Dialog mode (DLGMODE_NEW: create new pattern, 
	DLGMODE_COPY: create copy of pattern, DLGMODE_CHANGE: modify pattern)
	@type dlgmode: int
	@return: Tuple containing pattern name, length, and whether to switch to new pattern or not
	@rtype: (string, int, int)
	"""
	dlg = PatternDialog(parent)
	dlg.dlgmode = dlgmode
	if dlgmode == DLGMODE_NEW:
		dlg.set_title("New Pattern")
	elif dlgmode == DLGMODE_COPY:
		dlg.set_title("Create copy of pattern")
		dlg.lengthbox.set_sensitive(False)
	elif dlgmode == DLGMODE_CHANGE:
		dlg.set_title("Pattern Properties")
		dlg.chkswitch.set_sensitive(False)
	dlg.edtname.set_text(name)
	dlg.chkswitch.set_active(config.get_config().get_default_int('SwitchToNewPattern', 1))
	dlg.lengthbox.child.set_text(str(length))
	dlg.edtname.select_region(0, -1)
	dlg.edtname.grab_focus()
	response = dlg.run()
	dlg.hide_all()
	result = None
	if response == gtk.RESPONSE_OK:
		switch = int(dlg.chkswitch.get_active())
		config.get_config().set_default_int('SwitchToNewPattern', switch)
		result = str(dlg.edtname.get_text()), int(dlg.lengthbox.child.get_text()), switch
	dlg.destroy()
	return result
		
class PatternToolBar(gtk.HBox):
	"""
	Pattern Toolbar
	
	Contains lists of the plugins, patterns, waves and octaves available.
	"""
	def __init__(self, view):
		"""
		Initialization.
		"""
		# begin wxGlade: SequencerFrame.__init__
		self.view = view
		gtk.HBox.__init__(self, False, MARGIN)
		self.set_border_width(MARGIN)
		self.pluginlabel = gtk.Label()
		self.pluginlabel.set_text_with_mnemonic("_Plugin")
		self.pluginselect = gtk.combo_box_new_text()
		self.pluginselect.connect('changed', self.on_pluginselect)
		self.pluginlabel.set_mnemonic_widget(self.pluginselect)
		self.patternlabel = gtk.Label()
		self.patternlabel.set_text_with_mnemonic("_Pattern")
		self.patternselect = gtk.combo_box_new_text()
		self.patternselect.connect('changed', self.on_patternselect)
		self.patternlabel.set_mnemonic_widget(self.patternselect)
		self.wavelabel = gtk.Label()
		self.wavelabel.set_text_with_mnemonic("_Instrument")
		self.waveselect = gtk.combo_box_new_text()
		self.waveselect.connect('changed', self.on_waveselect)
		self.waveselect.connect('notify::popup-shown', self.on_waveselect_popup)
		self.wavelabel.set_mnemonic_widget(self.waveselect)
		self.octavelabel = gtk.Label()
		self.octavelabel.set_text_with_mnemonic("_Base octave")
		self.octaveselect = gtk.combo_box_new_text()
		self.octaveselect.connect('changed', self.on_octaveselect)
		self.octavelabel.set_mnemonic_widget(self.octaveselect)
		self.playnotes = gtk.CheckButton(label="Play _notes")

		self.playnotes.set_active(True)
		
		self.plugin_index = 0
		self.index_to_plugin = {}
		self.plugin_to_index = {}
		self.pattern = 0
		self.wave = 0
		self.cb2w = {} # combobox index to wave index
		self.waveselect_tokens = []
		
		self.pack_start(self.pluginlabel, expand=False)
		self.pack_start(self.pluginselect, expand=False)
		self.pack_start(self.patternlabel, expand=False)
		self.pack_start(self.patternselect, expand=False)
		self.pack_start(self.wavelabel, expand=False)
		self.pack_start(self.waveselect, expand=False)
		self.pack_start(self.octavelabel, expand=False)
		self.pack_start(self.octaveselect, expand=False)
		self.pack_start(self.playnotes, expand=False)
		
		global_events.connect('pattern-removed', self.update_patternselect)
		global_events.connect('pattern-created', self.update_patternselect)
		global_events.connect('pattern-name-changed', self.update_patternselect)
		global_events.connect('song-opened', self.update_all)
		
	def reset(self):
		self.plugin_index = 0
		self.pattern = 0
		self.wave = 0
		
	def on_pluginselect(self, widget):		
		"""
		Callback to handle selection of the pluginselect list.
		"""
		if widget.get_active() == -1:
			return
		if self.plugin_index == widget.get_active():
			return
		self.select_plugin(widget.get_active())
		
	def on_patternselect(self, widget):
		"""
		Callback to handle selection of the patternselect list.
		"""
		print "on_patternselect"
		if widget.get_active() == -1:
			return
		if self.pattern == widget.get_active():
			return
		self.pattern = widget.get_active()
		self.view.pattern_changed()
		
	def on_waveselect_popup(self, widget, *args):
		if widget.get_property('popup-shown'):
			for win in gtk.window_list_toplevels():
				if (len(win.get_children()) == 1) and (isinstance(win.get_children()[0],gtk.Menu)):
					menu = win.get_children()[0]
					if menu.get_attach_widget() == widget:
						for index,item in enumerate(menu.get_children()):
							self.waveselect_tokens.append((item,item.connect('select', self.on_waveselect_menu_activate,widget,index)))
		else:
			for item,token in self.waveselect_tokens:
				item.disconnect(token)
			self.waveselect_tokens = []
			player.stop_wave()
		
	def on_waveselect_menu_activate(self, item, widget, index):
		wave = self.cb2w[index]
		w = player.get_wave(wave)
		if w.get_level_count() >= 1:
			from utils import db2linear
			vol = min(max(config.get_config().get_sample_preview_volume(),-76.0),0.0)
			amp = db2linear(vol,limit=-76.0)
			player.set_wave_amp(amp)
			player.play_wave(w, 0, (4 << 4) + 1)
		
	def on_waveselect(self, widget):
		"""
		Callback to handle selection of the waveselect list.
		"""
		if widget.get_active() == -1:
			return
		if self.wave == self.cb2w[widget.get_active()]:
			return
		self.wave = self.cb2w[widget.get_active()]
		self.view.grab_focus()
		
	def on_octaveselect(self, widget):
		"""
		Callback to handle selection of different octaves.
		"""
		if widget.get_active() == -1:
			return
		if self.view.octave == widget.get_active():
			return
		self.view.octave = widget.get_active()
		self.view.grab_focus()
		
	def update_pluginselect(self):
		"""
		Updates the plugin selection box.
		"""
		self.plugin_index = min(max(self.plugin_index, 0), player.get_plugin_count()-1)
		self.pluginselect.get_model().clear()
		for i, plugin in enumerate(sorted(player.get_plugin_list(), lambda a,b:cmp(a.get_name().lower(),b.get_name().lower()))):
			self.index_to_plugin[i] = plugin
			self.pluginselect.append_text(prepstr(plugin.get_name()))
		self.plugin_to_index = dict([(v,k) for (k,v) in self.index_to_plugin.iteritems()])
		if self.plugin_index != -1:
			self.pluginselect.set_active(self.plugin_index)
		
	def next_wave(self):
		"""
		Selects the next wave.
		"""
		self.waveselect.set_active(min(self.waveselect.get_active()+1,get_item_count(self.waveselect.get_model())-1))
		self.wave = self.cb2w[self.waveselect.get_active()]

	def prev_wave(self):
		"""
		Selects the previous wave.
		"""
		self.waveselect.set_active(max(self.waveselect.get_active()-1,0))
		self.wave = self.cb2w[self.waveselect.get_active()]
		
	def select_plugin(self, i):
		"""
		Selects a plugin
		
		@param i: Plugin index.
		@type i: int
		"""
		self.plugin_index = min(max(i, 0), player.get_plugin_count()-1)
		self.view.selection = None
		self.view.start_col = 0
		self.update_pluginselect()
		self.update_patternselect()
		self.view.pattern_changed()
		try: 
			self.view.show_cursor_right()
		except AttributeError:
			pass

	def get_combo_plugin(self, index):
		"""
		Retrieves plugin associated with selected combo box index
		"""
		return self.index_to_plugin[index]

	
	def select_pattern(self, i):
		"""
		Selects a pattern.
		
		@param i: Pattern index.
		@type i: int
		"""
		if self.plugin_index != -1:
			plugin = self.get_combo_plugin(self.plugin_index)
			self.pattern = min(max(i, 0),plugin.get_pattern_count()-1)
		else:
			self.pattern = -1
		self.update_patternselect()
		self.view.pattern_changed()
		
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
			
	def update_patternselect(self, *args):
		"""
		Rebuilds and updates the patternselect list.
		"""
		self.patternselect.get_model().clear()
		if self.plugin_index != -1:
			mp = self.get_combo_plugin(self.plugin_index)
			for p in mp.get_pattern_list():
				self.patternselect.append_text(prepstr(p.get_name()))
			self.pattern = min(max(self.pattern, 0),mp.get_pattern_count()-1)
			if self.pattern != -1:
				self.patternselect.set_active(self.pattern)
		
	def update_waveselect(self):
		"""
		Rebuilds and updates the waveselect list.
		"""
		self.cb2w = {}
		self.waveselect.get_model().clear()
		count = 0
		wi = 0
		for i in range(player.get_wave_count()):
			w = player.get_wave(i)
			if w.get_level_count() >= 1:
				self.waveselect.append_text("%02X. %s" % (i+1, prepstr(w.get_name())))
				if i == self.wave:
					wi = count
				self.cb2w[count] = i
				count += 1
		if not wi:
			self.wave = 0
		self.waveselect.set_active(wi)
		
	def update_octaves(self):
		"""
		Rebuilds and updates the octaveselect list.
		"""
		self.octaveselect.get_model().clear()
		for i in range(10):
			self.octaveselect.append_text("%i" % i)
		self.octaveselect.set_active(self.view.octave)
		
	def update_all(self):
		"""
		Updates the toolbar to reflect a pattern change.
		"""
		self.update_pluginselect()
		self.update_patternselect()
		self.update_waveselect()
		self.update_octaves()

class PatternPanel(gtk.VBox):
	"""
	Panel containing the pattern toolbar and pattern view.
	"""
	def __init__(self, rootwindow):
		"""
		Initialization.
		
		@param rootwindow: Window that contains the component.
		@type rootwindow: main.AldrinFrame
		"""
		gtk.VBox.__init__(self)
		self.rootwindow = rootwindow
		self.rootwindow.event_handlers.append(self.on_player_callback)
		self.statusbar = gtk.HBox(False, MARGIN)
		self.statusbar.set_border_width(MARGIN0)
		vscroll = gtk.VScrollbar()
		hscroll = gtk.HScrollbar()
		self.view = PatternView(rootwindow, hscroll, vscroll)
		self.toolbar = PatternToolBar(self.view)
		self.view.toolbar = self.toolbar
		self.view.statusbar = self.statusbar
		self.view.pattern_changed()
		self.pack_start(self.toolbar, expand=False)
		scrollwin = gtk.Table(2,2)
		scrollwin.attach(self.view, 0, 1, 0, 1, gtk.FILL|gtk.EXPAND, gtk.FILL|gtk.EXPAND)
		scrollwin.attach(vscroll, 1, 2, 0, 1, 0, gtk.FILL)
		scrollwin.attach(hscroll, 0, 1, 1, 2, gtk.FILL, 0)
		self.pack_start(scrollwin)
		self.pack_end(self.statusbar, expand=False)
		self.toolbar.update_all()
		self.statuslabels = []
		for i in range(3):
			label = gtk.Label()
			self.statuslabels.append(label)
			self.statusbar.pack_start(label, expand=False)
			self.statusbar.pack_start(gtk.VSeparator(), expand=False)
		self.view.statuslabels = self.statuslabels
		
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
		#elif data.type == zzub.zzub_event_type_parameter_changed and player.get_automation():
		#	self.view.update_line(player.get_position()) 
		#	self.view.redraw() 

	def reset(self):
		"""
		Resets the toolbar
		"""
		self.toolbar.reset()
		
	def update_all(self, *args):
		"""
		Updates the toolbar and the pattern view to reflect a pattern change.
		"""
		self.toolbar.update_all()
		self.view.pattern_changed()

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
	if k<128:
		k = chr(k).lower().upper()
	else:
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

class PatternView(gtk.DrawingArea):
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
	
	def __init__(self, rootwindow, hscroll, vscroll):
		"""
		Initialization.
		
		@param rootwindow: Window that contains the component.
		@type rootwindow: main.AldrinFrame
		"""
		self.rootwindow = rootwindow
		self.hscroll = hscroll
		self.vscroll = vscroll
		self.toolbar = None
		self.statusbar = None
		self.patternsize = 16
		self.index = None
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
		self.keystartselect = None
		self.keyendselect = None
		self.selection_start = None
		self.dragging = False
		self.shiftselect = None
		self.clickpos = None
		self.track_width=[0,0,0]
		self.plugin_info = common.get_plugin_infos()
		gtk.DrawingArea.__init__(self)
		#"Bitstream Vera Sans Mono"
		self.update_font()
		# implements horizontal scrolling
		self.start_col = 0
		self.add_events(gtk.gdk.ALL_EVENTS_MASK)
		self.set_property('can-focus', True)
		self.connect("expose_event", self.expose)
		self.connect('key-press-event', self.on_key_down)
		self.connect('key-release-event', self.on_key_up)
		self.connect('button-press-event', self.on_button_down)
		self.connect('button-release-event', self.on_button_up)
		self.connect('motion-notify-event', self.on_motion)
		self.connect('scroll-event', self.on_mousewheel)
		gobject.timeout_add(100, self.update_position)
		self.hscroll.connect('change-value', self.on_hscroll_window)
		self.vscroll.connect('change-value', self.on_vscroll_window)
	
	def update_font(self):
		pctx = self.get_pango_context()
		desc = pango.FontDescription(config.get_config().get_pattern_font()) #.get_font_description()
		#~ desc.set_style(pango.STYLE_NORMAL)
		#~ desc.set_family('Monospace')
		#~ desc.set_weight(pango.WEIGHT_BOLD)
		pctx.set_font_description(desc)
		self.fontdesc = desc
		self.font = pctx.load_font(desc)
		metrics = self.font.get_metrics(None)
		fh = (metrics.get_ascent() + metrics.get_descent()) / pango.SCALE
		fw = metrics.get_approximate_digit_width() / pango.SCALE
		self.row_height = fh # row height
		self.top_margin = fh # top margin
		self.column_width = fw # column width
	
	def on_copy(self, widget):
		"""
		Sent when the copy function is selected from the menu.
		"""
		self.copy()

	def on_cut(self, widget):
		"""
		Sent when the cut function is selected from the menu.
		"""
		self.cut()

	def on_paste(self, widget):
		"""
		Sent when the paste function is selected from the menu.
		"""
		self.paste()

	def on_context_menu(self, event):
		"""
		Callback that constructs and displays the popup menu
		"""
		def make_submenu_item(submenu, name):
			item = gtk.MenuItem(label=name)
			item.set_submenu(submenu)
			return item
		def make_menu_item(label, desc, func, *args):
			item = gtk.MenuItem(label=label)
			if func:
				item.connect('activate', func, *args)
			return item
		def make_check_item(toggled, label, desc, func, *args):
			item = gtk.CheckMenuItem(label=label)
			item.set_active(toggled)
			if func:
				item.connect('toggled', func, *args)
			return item
			
		menu = gtk.Menu()
		mx,my = int(event.x), int(event.y)
		menu.append(make_menu_item("Add track", "", self.on_popup_add_track))
		menu.append(make_menu_item("Delete last track", "", self.on_popup_delete_track))
		menu.append(gtk.SeparatorMenuItem())
		menu.append(make_menu_item("New pattern...", "", self.on_popup_create_pattern))
		menu.append(make_menu_item("Pattern properties...", "", self.on_popup_properties))
		menu.append(make_menu_item("Remove pattern...","", self.on_popup_remove_pattern))
		menu.append(make_menu_item("Create copy...", "", self.on_popup_create_copy))
		menu.append(gtk.SeparatorMenuItem())
		menu.append(make_menu_item("Double", "", self.on_popup_double))
		menu.append(make_menu_item("Halve", "", self.on_popup_halve))
		menu.append(gtk.SeparatorMenuItem())
		menu.append(make_menu_item("Transpose selection up", "", self.transpose_selection,1))
		menu.append(make_menu_item("Transpose selection down", "", self.transpose_selection,-1))
		menu.append(make_menu_item("Randomize selection", "", self.randomize_selection, None))
		menu.append(make_menu_item("Constrained randomize", "", self.randomize_selection, "constrain"))
		menu.append(make_menu_item("Interpolate selection", "", self.interpolate_selection))
		menu.append(gtk.SeparatorMenuItem())
		issolo = self.rootwindow.routeframe.view.solo_plugin == self.get_plugin()
		menu.append(make_check_item(issolo, "Solo Plugin", "Toggle solo", self.on_popup_solo))
		menu.append(gtk.SeparatorMenuItem())
		menu.append(make_menu_item("Cut", "", self.on_popup_cut))
		menu.append(make_menu_item("Copy", "", self.on_popup_copy))
		menu.append(make_menu_item("Paste", "", self.on_popup_paste))
		menu.append(make_menu_item("Delete", "", self.on_popup_delete))

		menu.show_all()
		menu.attach_to_widget(self, None)
		menu.popup(None, None, None, event.button, event.time)
		
	def update_position(self):
		"""
		Updates the position.
		"""
		if self.rootwindow.index != self.rootwindow.PAGE_PATTERN:
			return True
		playpos = player.get_position()
		if self.playpos != playpos:
			self.draw_playpos_xor()
			self.playpos = playpos
			self.draw_playpos_xor()
		return True
			
	def get_new_pattern_name(self, m=None):
		"""
		Finds an unused pattern name.
		"""
		if not m:
			m = self.get_plugin()
		return get_new_pattern_name(m)
		
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
			self.row, self.group, self.track, self.index, self.subindex = self.plugin_info.get(plugin).pattern_position
			self.selection = self.plugin_info.get(plugin).selection
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
		
	def get_client_size(self):
		rect = self.get_allocation()
		return rect.width, rect.height
		
	def adjust_scrollbars(self):
		w,h = self.get_client_size()
		vw,vh = self.get_virtual_size()
		pw, ph = int((w - PATLEFTMARGIN) / float(self.column_width) + 0.5), int((h - self.row_height) / float(self.row_height) + 0.5)
		hrange = vw - pw
		vrange = vh - ph
		if hrange <= 0:
			self.hscroll.hide()
		else:
			self.hscroll.show()
		if vrange <= 0:
			self.vscroll.hide()
		else:
			self.vscroll.show()
		adj = self.hscroll.get_adjustment()
		adj.set_all(self.start_col, 0, vw, 1, 1, pw)
		adj = self.vscroll.get_adjustment()
		adj.set_all(self.start_row, 0, vh, 1, 1, ph)
		
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
		
	def show_row(self, r):
		"""
		Makes a row in the editor visible.
		
		@param r: Row position.
		@type r: int
		"""
		row = min(max(r,0), self.row_count - 1)
		if row >= 0:
			w,h = self.get_client_size()
			endrow = (((h - self.top_margin) / self.row_height) + self.start_row) - 1
			if (row < self.start_row):
				self.start_row = row
				self.redraw()
			elif (row >= endrow):
				self.start_row = row - (endrow - self.start_row)
				self.redraw()

	def set_row(self, r):
		"""
		Sets the current row position.
		
		@param r: Row position.
		@type r: int
		"""
		self.row = min(max(r,0), self.row_count - 1)
		self.show_row(self.row)
		
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
		
	def expose(self, widget, event):
		self.context = widget.window.cairo_create()
		self.draw(self.context)
		return False
		
	def redraw(self,row=None,rows=None,fulldraw=True):
		if self.window:
			w,h = self.get_client_size()
			self.window.invalidate_rect((0,0,w,h), False)
		
	def pattern_changed(self):
		"""
		Loads and redraws the pattern view after the pattern has been changed.
		"""
		self.init_values()
		#self.redraw() ## do i want this?
		self.grab_focus()
		plugin = self.get_plugin()
		if plugin:
			self.plugin_info.get(plugin).reset_patterngfx()

	def move_up(self, step = 1):
		"""
		Moves the cursor up.
		
		@param step: Amount the cursor is moved up.
		@type step: int
		"""	
		self.draw_cursor_xor()
		self.set_row(self.row - step)
		self.draw_cursor_xor()
		self.update_statusbar()
		#~ self.refresh_view()
		
	def move_down(self, step = 1):
		"""
		Moves the cursor down.
		
		@param step: Amount the cursor is moved down.
		@type step: int
		"""	
		self.draw_cursor_xor()
		#for i in range(step+2):
		#	self.update_line(self.row+i+1)
		self.set_row(self.row + step)
		self.draw_cursor_xor()
		self.update_statusbar()
		#~ self.refresh_view()
		
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
			self.redraw()

	def show_cursor_right(self):
		"""
		Puts the cursor into visible frame after a jump to the right.
		"""
		w,h = self.get_charbounds()
		vw,vh = self.get_virtual_size()
		x,y = self.pattern_to_charpos(self.row, self.group, self.track, self.index, self.subindex)
		if x > w:
			self.start_col = min(self.start_col + x - w + (w / 3), vw - w + self.start_col)
			self.redraw()
		
	def move_left(self):
		"""
		Moves the cursor left.
		"""	
		if not self.pattern:
			return
		self.draw_xor()
		self.move_subindex_left()
		self.show_cursor_left()
		self.draw_xor()
		self.update_statusbar()
		#~ self.refresh_view()		
		
	def move_right(self):
		"""
		Moves the cursor right.
		"""	
		if not self.pattern:
			return
		self.draw_xor()
		self.move_subindex_right()
		self.show_cursor_right()
		self.draw_xor()
		self.update_statusbar()
		#~ self.refresh_view()
		
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
		
	def transpose_selection(self, widget, offset):
		"""
		Transposes the current selection by an offset.
		
		@param offset: The amount that the values is incremented.
		@type offset: int
		"""
		for r,g,t,i in self.selection_range():
			if r>self.pattern.get_row_count()-1:
				break
			if r<0:
				continue
			p = self.plugin.get_parameter(g,i)
			v = self.pattern.get_value(r,g,t,i)
			if v != p.get_value_none():
				if (p.get_type() == 0):
					if v != zzub.zzub_note_value_off:
						v = max(min(mn2bn(bn2mn(v)+offset),p.get_value_max()),p.get_value_min())
				else:
					v = max(min(v+offset,p.get_value_max()),p.get_value_min())
				self.pattern.set_value(r,g,t,i,v)
		tmp_sel = self.selection
		self.pattern_changed()
		self.selection = tmp_sel

	def randomize_selection(self, widget=None, mode=None):
		"""
		Fills the current selection with random values. If the
		selection contains some values, only generate values
		which themselves already appear ("shuffle"). This is
		handy for notes, where we might want to randomise over
		(eg) a chord or a scale, rather than chromatically.
		Can be used to fill a column with a single value:
		enter it at the top, Ctrl-B, go to the bottom, Ctrl-E,
		then run this.

		If mode="constrain" (ctrl-shift-r) only generate
		values *between* min and max of those which already
		appear. 

		Generated values will be spaced in the pattern
		according to row_step, so hit Ctrl-[1-9] to adjust it
		first.

		A nice use-case is to run this repeatedly, waiting for
		a nice selection to appear. Repeat ctrl-X ctrl-R to
		get unconstrained randomisation; repeat ctrl-R to keep
		"shuffling"; repeat ctrl-shift-R to keep doing
		constrained randomisation. Repeating either of the
		latter two will eventually lead to a narrowing of the
		range.
		"""
		if not self.selection:
			return
		if self.row_step == 0:
			step = 1
		else:
			step = self.row_step
		# Unlike the interpolate_selection method, we can't do
		# this without looking at the whole selection first.
		# So first, save the min and max and all vals present
		# in each column of the selection.
		minv = dict()
		maxv = dict()
		allv = dict()
		for r, g, t, i in self.selection_range():
			if r > self.pattern.get_row_count() - 1:
				break
			if r < 0:
				continue
			val = self.pattern.get_value(r, g, t, i)
			p = self.plugin.get_parameter(g, i)
			if val == p.get_value_none():
				continue
			# We're not guaranteed that (g, t, i) index a
			# well-formed matrix (eg g=2, t=2 might exist
			# even though g=0, t=2 does not), so can't use
			# list-of-list-of-list: use dict.
			key = (g, t, i)
			if not allv.has_key(key):
				# We don't need all the values, just
				# the unique ones. Use set.
				allv[key] = set([val])
			else:
				if not val in allv[key]:
					allv[key].add(val)
			if p.get_type() == 0 and val == zzub.zzub_note_value_off:
				# Note type: many values (above midi
				# note 120, but "below" note-off) are
				# invalid! Therefore, can't blindly
				# randomise between min and max, when
				# max == note-off: so don't add
				# note-off to minv and maxv.
				continue
			if not minv.has_key(key) or val < minv[key]:
				minv[key] = val
			if not maxv.has_key(key) or val > maxv[key]:
				maxv[key] = val
			
		
		# Go through the selection randomising each cell.
		for r, g, t, i in self.selection_range():
			if r > self.pattern.get_row_count() - 1:
				break
			if r < 0:
				continue
			p = self.plugin.get_parameter(g, i)
			key = (g, t, i)
			# If row_step > 1 clear some rows
			if (r - self.selection.begin) % step != 0:
				v = p.get_value_none()

			elif mode == "constrain":
				if allv.has_key(key):
					v1, v2 = minv[key], maxv[key]
					if p.get_type() == 0:
						# Note type
						v1 = bn2mn(v1)
						v2 = bn2mn(v2)
						if zzub.zzub_note_value_off in allv[key]:
							# Hack to allow note-offs.
							ir = random.randint(v1, v2 + 1)
							if ir == v2 + 1:
								v = zzub.zzub_note_value_off
							else:
								v = mn2bn(ir)
						else:
							ir = random.randint(v1, v2)
							v = mn2bn(ir)
							
					else:
						# Any other type
						v = random.randint(v1, v2)
				else:
					v = p.get_value_none()

			else:
				if allv.has_key(key):
					# Subset of existing values
					v = random.choice(list(allv[key]))
				else:
					# No values exist: traditional randomize
					if (p.get_type() == 0):
						v = mn2bn(random.randrange(0, 120))
					else:
						v = random.randrange(p.get_value_min(),p.get_value_max()+1)
						
			self.pattern.set_value(r, g, t, i, v)
		tmp_sel = self.selection
		self.pattern_changed()
		self.selection = tmp_sel

	def interpolate_selection(self, widget=None):
		"""
		Fills the current selection with values interpolated
		from selection start to selection end. Generated
		values will be spaced in the pattern according to
		row_step, so hit Ctrl-[1-9] to adjust it first.
		"""
		if not self.selection:
			return		
		if self.row_step == 0:
			step = 1
		else:
			step = self.row_step
		for r,g,t,i in self.selection_range():
			if r>self.pattern.get_row_count()-1:
				break
			if r<0:
				continue
			p = self.plugin.get_parameter(g,i)
			v1 = self.pattern.get_value(self.selection.begin,g,t,i)
			v2 = self.pattern.get_value(self.selection.end-1,g,t,i)
			if (v1 != p.get_value_none()) and (v2 != p.get_value_none()):
				if (p.get_type() == 0 and (v1 == zzub.zzub_note_value_off or v2 == zzub.zzub_note_value_off)):
					continue
				# If row_step > 1, clear some rows.
				# Sometimes this might prevent the
				# interpolation from ever achieving
				# the final value (v2), but the fix
				# would be to add the final value to
				# the last row, and this would disrupt
				# the row_step-induced rhythm.
				if (r - self.selection.begin) % step != 0:
					v = p.get_value_none()
				else:
					f = float(r - self.selection.begin) / float(self.selection.end - self.selection.begin - 1)
					if (p.get_type() == 0):
						v1 = bn2mn(v1)
						v2 = bn2mn(v2)
						v = mn2bn(roundint((v2 - v1) * f + v1))
					else:
						v = roundint((v2 - v1) * f + v1)
				self.pattern.set_value(r,g,t,i,v)
		tmp_sel = self.selection
		self.pattern_changed()
		self.selection = tmp_sel

	def cut(self):
		"""
		Cuts the current selection into the clipboard
		"""
		if not self.selection:
			return
		self.copy()
		for r,g,t,i in self.selection_range():
			if r>self.pattern.get_row_count()-1:
				break
			if r<0:
				continue
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
			if r>self.pattern.get_row_count()-1:
				break
			if r<0:
				continue
			data += "%04x%01x%02x%02x%04x" % (r - self.selection.begin,g,t,i,self.pattern.get_value(r,g,t,i))
		set_clipboard_text(data)
		
	def delete(self):
		"""
		Deletes the current selection
		"""
		for r,g,t,i in self.selection_range():
			if r>self.pattern.get_row_count()-1:
				break
			if r<0:
				continue
			p = self.plugin.get_parameter(g,i)
			self.pattern.set_value(r,g,t,i,p.get_value_none())
		self.pattern_changed()
			
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
		
		data = get_clipboard_text()
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
			#Non Buzz-like behaviour (naughty naughty!) ;)  :
			#self.set_row(r+1)
			self.update_statusbar()	
			self.pattern_changed()
		except:
			import traceback
			traceback.print_exc()
			error(self, "Couldn't paste.")
	
	def on_mousewheel(self, widget, event):
		"""
		Callback that responds to mousewheeling in pattern view.
		"""		
		if event.direction == gtk.gdk.SCROLL_UP:
			self.move_up()
			self.adjust_scrollbars()
		elif event.direction == gtk.gdk.SCROLL_DOWN:
			self.move_down()
			self.adjust_scrollbars()
	
	def on_button_down(self, widget, event):		
		"""
		Callback that responds to mouse click in pattern view.
		"""
		if not self.selection:
			self.selection = self.Selection()
		self.grab_focus()
		if event.button == 3:
			self.on_context_menu(event)
		if not self.pattern:
			return
		if event.button == 1:
			self.draw_xor()
			x,y = int(event.x), int(event.y)
			row, group, track, index, subindex = self.pos_to_pattern((x,y))
			self.set_row(row)
			self.set_group(group)
			self.set_track(track)
			self.set_index(index)
			self.set_subindex(subindex)
			self.draw_xor()
			self.update_statusbar()
			self.dragging = True
			self.selection.begin = row
			self.selection.end = row
			self.clickpos = self.pos_to_pattern((x,y))
			self.adjust_selection()
			self.redraw()
	
	def on_motion(self, widget, event):
		"""
		Callback that responds to mouse motion in sequence view.
		
		@param event: Mouse event
		@type event: wx.MouseEvent
		"""
		x,y,state = self.window.get_pointer()
		x, y = int(x),int(y)
		row, group, track, index, subindex = self.pos_to_pattern((x,y))
		if self.dragging:
			row, group, track, index, subindex = self.pos_to_pattern((x,y))
			if group != self.clickpos[1]:
				self.selection.mode=SEL_ALL
			elif track != self.clickpos[2]:
				self.selection.mode=SEL_GROUP
			elif index != self.clickpos[3]:
				self.selection.mode=SEL_TRACK
			else:
				self.selection.mode=SEL_COLUMN
			self.show_row(row)
			if row<self.clickpos[0]:
				self.selection.end=self.clickpos[0]+1
				self.selection.begin=row
			else:
				self.selection.begin=self.clickpos[0]
				self.selection.end = row+1
			self.adjust_selection()
			self.redraw()
				
	def on_button_up(self, widget, event):
		"""
		Callback that responds to mouse button release event in pattern view.
		"""
		
		if event.button == 1:
			self.dragging = False

	def on_popup_remove_pattern(self, event=None):
		"""
		Callback that removes the current pattern.
		"""		
		response = question(self, "<b><big>Do you really want to remove this pattern?</big></b>\n\nYou cannot reverse this action.", False)
		if response == gtk.RESPONSE_YES:
			m = self.get_plugin()			
			if self.pattern:
				m.remove_pattern(self.pattern)
			self.toolbar.select_pattern(0)		
			
	def on_popup_create_pattern(self, event=None, m=None):
		"""
		Callback that creates a pattern.
		"""
		name = self.get_new_pattern_name(m)
		result = show_pattern_dialog(self,name,self.patternsize,DLGMODE_NEW)
		if not result:
			return
		name, self.patternsize, switch = result
		if not m:
			m = self.get_plugin()
		p = m.create_pattern(self.patternsize)
		p.set_name(name)
		if switch:
			for i in range(m.get_pattern_count()):
				if m.get_pattern(i) == p:
					self.toolbar.select_pattern(i)
					break
	
	def on_popup_double(self, event=None):
		"""
		Callback that doubles the length of the current pattern while
		keeping notes intact
		"""
		pattern_index=[]
		pattern_contents=[]
		for r,g,t,i in self.pattern_range():
			pattern_index.append((r,g,t,i))
			pattern_contents.append(self.pattern.get_value(r,g,t,i))
			param = self.plugin.get_parameter(g,i)
			self.pattern.set_value(r,g,t,i,param.get_value_none())
		item=0
		self.pattern.set_row_count(self.pattern.get_row_count()*2)
		for r,g,t,i in pattern_index:
			self.pattern.set_value(r*2,g,t,i,pattern_contents[item])
			item+=1
		self.pattern_changed()
		
	def on_popup_halve(self, event=None):
		"""
		Callback that halves the length of the current pattern while
		keeping notes intact
		"""
		if self.pattern.get_row_count()==1:
			return
		for r,g,t,i in self.pattern_range():
			if r%2:
				continue
			self.pattern.set_value(r/2,g,t,i,self.pattern.get_value(r,g,t,i))
		self.pattern.set_row_count(self.pattern.get_row_count()/2)
		self.pattern_changed()
		
	def on_popup_create_copy(self, event=None):
		"""
		Callback that creates a copy of the current pattern.
		"""
		name = self.get_new_pattern_name()
		result = show_pattern_dialog(self,name,self.row_count,DLGMODE_COPY)
		if not result:
			return
		name, self.patternsize, switch = result
		m = self.get_plugin()
		p = m.create_pattern(self.row_count)
		p.set_name(name)
		for r,g,t,i in self.pattern_range():
			p.set_value(r,g,t,i,self.pattern.get_value(r,g,t,i))
		if switch:
			for i in range(m.get_pattern_count()):
				if m.get_pattern(i) == p:
					self.toolbar.select_pattern(i)
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
		name, rc, switch = result
		self.patternsize = rc
		if self.pattern.get_name() != name:
			self.pattern.set_name(name)
		if self.pattern.get_row_count() != rc:
			self.pattern.set_row_count(rc)
		self.toolbar.update_all()
		self.pattern_changed()
		
	def on_popup_add_track(self, event=None):
		"""
		Callback that adds a track.
		"""
		m = self.get_plugin()		
		m.set_track_count(m.get_track_count()+1)
		self.pattern_changed()
		# recreate sliders in parameter view
		dlg = self.rootwindow.routeframe.view.plugin_dialogs.get(m,None)
		if dlg:			
			pv = dlg.paramview
			for child in pv.rowgroup.get_children():
				pv.rowgroup.remove(child)
			pv.create_sliders(pv.rowgroup)
			dlg.show_all()

	
	def on_popup_delete_track(self, event=None):
		"""
		Callback that deletes last track.
		"""		
		if question(self, "<b><big>Really delete last track?</big></b>\n\nThis action can not be undone.", False) == gtk.RESPONSE_YES:
			m = self.get_plugin()
			m.set_track_count(m.get_track_count()-1)
			self.pattern_changed()
			# recreate sliders in parameter view
			dlg = self.rootwindow.routeframe.view.plugin_dialogs.get(m,None)
			if dlg:			
				pv = dlg.paramview
				for child in pv.rowgroup.get_children():
					pv.rowgroup.remove(child)
				pv.create_sliders(pv.rowgroup)
				dlg.show_all()
	
	def on_popup_cut(self, event=None):
		"""
		Callback that cuts selection
		"""
		self.cut()
	
	def on_popup_copy(self, event=None):
		"""
		Callback that copies selection
		"""
		self.copy()
		
	def on_popup_paste(self, event=None):
		"""
		Callback that pastes selection
		"""
		self.paste()
	
	def on_popup_delete(self, event=None):
		"""
		Callback that deletes selection
		"""
		self.delete()
	
	def on_key_down(self, widget, event):
		"""
		Callback that responds to key stroke in pattern view.
		
		@param event: Key event
		@type event: wx.KeyEvent
		"""
		mask = event.state
		kv = event.keyval
		# convert keypad numbers	
		if gtk.gdk.keyval_from_name('KP_0') <= kv <= gtk.gdk.keyval_from_name('KP_9'):
			kv = kv - gtk.gdk.keyval_from_name('KP_0')  + gtk.gdk.keyval_from_name('0') 
		k = gtk.gdk.keyval_name(kv)
		#~ print mask,k,kv
		if k == 'less':
			self.toolbar.prev_wave()
		elif k == 'greater':
			self.toolbar.next_wave()
		elif (mask & gtk.gdk.CONTROL_MASK) and (mask & gtk.gdk.SHIFT_MASK):
			if k == 'Return':
				self.on_popup_create_copy()
			elif k == 'R':
				# R, not r
				self.randomize_selection(widget=None, mode="constrain")
			else:
				return False
		elif k in ('Tab', 'ISO_Left_Tab'):
			if (mask & gtk.gdk.SHIFT_MASK):
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
		elif mask & gtk.gdk.SHIFT_MASK and (k in ('KP_Add','asterisk','plus')):
			self.transpose_selection(None, 1)
		elif mask & gtk.gdk.SHIFT_MASK and (k in ('KP_Subtract','minus','underscore')):
			self.transpose_selection(None, -1)
		elif mask & gtk.gdk.SHIFT_MASK and k == 'Down':
			if not self.selection:
				self.selection = self.Selection()
			if self.shiftselect==None:
				self.shiftselect=self.row
			self.move_down()
			if self.row<self.shiftselect:
				self.selection.end=self.shiftselect+1
				self.selection.begin=self.row
			else:
				self.selection.begin=self.shiftselect
				self.selection.end = self.row+1
			self.adjust_selection()
			self.redraw()
		elif mask & gtk.gdk.SHIFT_MASK and k == 'Up':
			if not self.selection:
				self.selection = self.Selection()
			if self.shiftselect==None:
				self.shiftselect=self.row
			self.move_up()
			if self.row<self.shiftselect:
				self.selection.end=self.shiftselect+1
				self.selection.begin=self.row
			else:
				self.selection.begin=self.shiftselect
				self.selection.end = self.row+1
			self.adjust_selection()
			self.redraw()
		elif mask & gtk.gdk.SHIFT_MASK and (k == 'Right' or k == 'Left'):
			if not self.selection:
				self.selection = self.Selection()
			if self.shiftselect==None:
				self.shiftselect=self.row
				self.selection.begin=self.shiftselect
				self.selection.end = self.row+1
			self.selection.mode = (self.selection.mode + 1) % 4
			self.adjust_selection()
			self.redraw()
		elif (mask & gtk.gdk.CONTROL_MASK):
			if k == 'Return':
				self.on_popup_create_pattern()				
			elif k == 'BackSpace':	
				self.on_popup_properties()
			elif k == 'Delete':
				self.on_popup_remove_pattern()
			elif kv >= ord('0') and kv <= ord('9'):
				self.row_step = kv - ord('0')
			elif k == 'b':
				if not self.selection:
					self.selection = self.Selection()
				if self.keystartselect:
					self.selection.begin=self.keystartselect
				if self.keyendselect:
					self.selection.end=self.keyendselect
				if self.selection.begin == self.row:
						self.selection.mode = (self.selection.mode + 1) % 4
				self.selection.begin = self.row
				self.keystartselect = self.row
				self.selection.end = max(self.row+1,self.selection.end)				
				self.adjust_selection()
				self.update_plugin_info()				
				self.redraw()
			elif k == 'e':
				if not self.selection:
					self.selection = self.Selection()
				if self.keystartselect:
					self.selection.begin=self.keystartselect
				if self.keyendselect:
					self.selection.end=self.keyendselect
				if self.selection.end == self.row+1:
					self.selection.mode = (self.selection.mode + 1) % 4
				self.selection.end = self.row+1
				self.keyendselect=self.row+1
				self.selection.begin = max(min(self.selection.end-1,self.selection.begin),0)
				self.adjust_selection()
				self.redraw()
			elif k == 'c':
				self.copy()
			elif k == 'v':
				self.paste()
			elif k == 'x':
				self.cut()
			elif k == 'r':
				self.randomize_selection(widget=None, mode=None)
			elif k == 'i':
				self.interpolate_selection()
			elif k == 'u':
				self.selection = None
				self.update_plugin_info()
				self.redraw()
			elif k == 'l':
				self.on_popup_solo()
			elif k in ('KP_Add','plus'):
				self.on_popup_add_track()
			elif k in ('KP_Subtract','minus'):
				self.on_popup_delete_track()
			elif k == 'Up':
				self.toolbar.select_plugin(self.toolbar.plugin_index-1)
			elif k == 'Down':
				self.toolbar.select_plugin(self.toolbar.plugin_index+1)
			else:
				return False
		elif k == 'Left' or k == 'KP_Left':
			self.move_left()
			self.adjust_scrollbars()
		elif k == 'Right' or k == 'KP_Right':
			self.move_right()
			self.adjust_scrollbars()
		elif k == 'Up' or k == 'KP_Up':
			self.move_up()
			self.adjust_scrollbars()
		elif k == 'Down' or k == 'KP_Down':
			self.move_down()
			self.adjust_scrollbars()
		elif k == 'Page_Up' or k == 'KP_Page_Up':
			self.move_up(16)
			self.adjust_scrollbars()
		elif k == 'Page_Down' or k == 'KP_Page_Down':
			self.move_down(16)
			self.adjust_scrollbars()
		elif k == 'Home':
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
			self.adjust_scrollbars()
			self.refresh_view()
		elif k == 'Insert':
			self.pattern.insert_row(self.group, self.track, -1, self.row)
			del self.lines[self.group][self.track][-1]
			self.lines[self.group][self.track].insert(self.row, "")
			self.update_line(self.row)
			self.redraw()
			plugin = self.get_plugin()
			if plugin:
				self.plugin_info.get(plugin).reset_patterngfx()
		elif k == 'Delete':
			#if self.selection!=None:
			#	if self.row>=self.selection.begin and self.row<self.selection.end:
			#		self.delete()
			#	else:
			#		self.pattern.delete_row(self.group, self.track, -1, self.row)
			#else:
			self.pattern.delete_row(self.group, self.track, -1, self.row)
			del self.lines[self.group][self.track][self.row]
			self.lines[self.group][self.track].append('')
			self.update_line(self.row_count-1)
			self.redraw()
			plugin = self.get_plugin()
			if plugin:
				self.plugin_info.get(plugin).reset_patterngfx()
		elif k == 'Return':
			mainwindow = self.rootwindow
			mainwindow.select_page(mainwindow.PAGE_SEQUENCER)
		elif k in ('KP_Add','plus'):			
			self.toolbar.next_pattern()
		elif k in ('KP_Subtract','minus'):
			self.toolbar.prev_pattern()
		elif k in ('KP_Multiply', 'dead_acute'):
			self.set_octave(self.octave+1)
			self.toolbar.update_octaves()
		elif k in ('KP_Divide', 'ssharp'):
			self.set_octave(self.octave-1)
			self.toolbar.update_octaves()
		elif k == 'Escape':
			self.selection = None
			self.shiftselect = None
			self.update_plugin_info()
			self.redraw()
		elif self.plugin and (kv < 256):
			p = self.plugin.get_parameter(self.group,self.index)
			param_type = p.get_type()
			playtrack = False
			if (param_type == 0): # note
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
					on = key_to_note(kv)
					if on:
						o,n = on
						data = (min(self.octave+o,9)<<4) | (n+1)
						if wp != None:
							wdata = self.toolbar.wave+1
						playtrack = True
					elif k == 'period':
						data = p.get_value_none()
						if wp != None:
							wdata = wp.get_value_none()
					elif k == '1':
						data = zzub.zzub_note_value_off
						if wp != None:
							wdata = wp.get_value_none()
						playtrack = True
					else:
						return False
					if wdata != None:
						self.pattern.set_value(self.row, self.group, self.track, wi, wdata)
				elif (self.subindex == 1) and (k >= '1') and (k <= '9'):
						o = ord(k)-ord('1')+1
						data = (self.pattern.get_value(self.row, self.group, self.track, self.index) & 0xf) | (o << 4)
				else:
					return False
			elif param_type == 1: # switch
				if k == '1' or k == '0':
					data = {'1':p.get_value_max(), '0':p.get_value_min()}[k]
				elif k == 'period':
					data = p.get_value_none()
				else:
					return False
			elif param_type in (2,3): # byte or word
				pw = self.parameter_width[self.group][self.index]
				if k >= '0' and k <= '9':
					o = ord(k)-ord('0')
				elif kv >= ord('a') and kv <= ord('f'):
					o = 10 + kv-ord('a')
				elif k == 'period':
					o = None
					data = p.get_value_none()
				else:
					return False
				if o != None:
					bofs = (pw - self.subindex - 1)*4
					data = self.pattern.get_value(self.row, self.group, self.track, self.index)
					if data == p.get_value_none():
						data = 0
					data = (data ^ (data & (0xf << bofs))) | (o << bofs) # mask out old nibble, put in new nibble
					data = min(p.get_value_max(),max(p.get_value_min(), data))
					if p.get_flags() & zzub.zzub_parameter_flag_wavetable_index:
						self.toolbar.wave = data - 1
						self.toolbar.update_waveselect()
			else:
				return False
			self.pattern.set_value(self.row, self.group, self.track, self.index, data)
			self.play_note(playtrack)
		else:
			return False
		return True
		
	def play_note(self, playtrack):
		"""
		Plays entered note
		"""
		if playtrack and self.toolbar.playnotes.get_active():
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
		self.redraw()
		self.move_down(self.row_step)
	
	def on_key_up(self, widget, event):
		"""
		Callback that responds to key release
		"""
		if config.get_config().get_pattern_noteoff():
			kv = event.keyval
			k = gtk.gdk.keyval_name(kv)
			if (k == 'Shift_L' or k=='Shift_R'):
				self.shiftselect = None
			if self.plugin:
				parameter_list = self.plugin.get_parameter_list(self.group)
				if parameter_list[self.index].get_description() == "Note" and kv<256:
					on = key_to_note(kv)
					if on:
						m = self.get_plugin()
						m.set_parameter_value(self.group, self.track, self.index, zzub.zzub_note_value_off, 0)
	
	def on_char(self, event):
		"""
		Callback that responds to key stroke in pattern view.
		
		@param event: Key event
		@type event: wx.KeyEvent
		"""		
		event.Skip()
		k = event.GetKeyCode()		
		if k == ord('<'):
			self.toolbar.prev_wave()
		elif k == ord('>'):
			self.toolbar.next_wave()

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
		w,h = self.get_client_size()
		w -= PATLEFTMARGIN + 4
		h -= self.top_margin
		return self.start_col + (w / self.column_width) - 1, self.start_row + (h / self.row_height) - 1
		
	def get_virtual_size(self):
		"""
		Returns the size in characters of the virtual view area.
		"""
		try: h = self.pattern.get_row_count()
		except AttributeError:
			return 0,0
		w = 0
		for g in CONN,GLOBAL,TRACK:
			w += self.track_width[g] * self.group_track_count[g]
		return w,h

	def on_vscroll_window(self, widget, scroll, value):
		"""
		Handles vertical window scrolling.
		"""
		adj = widget.get_adjustment()
		minv = adj.get_property('lower')
		maxv = adj.get_property('upper')
		pagesize = adj.get_property('page-size')
		value = int(max(min(value, maxv - pagesize), minv) + 0.5)
		widget.set_value(value)
		if self.start_row != value:
			self.start_row = value
			#w,h=self.get_client_size()
			#for row in range(h/self.row_height):
			#	self.update_line(self.start_row+row)
			self.redraw()
		return True

	def on_hscroll_window(self, widget, scroll, value):
		"""
		Handles horizontal window scrolling.
		"""
		adj = widget.get_adjustment()
		minv = adj.get_property('lower')
		maxv = adj.get_property('upper')
		pagesize = adj.get_property('page-size')
		value = int(max(min(value, maxv - pagesize), minv) + 0.5)
		widget.set_value(value)
		if self.start_col != value:
			self.start_col = value
			self.redraw()
			
	def update_plugin_info(self):				
		plugin = self.get_plugin()
		pi = self.plugin_info.get(plugin)
		# store current position
		pi.pattern_position = (self.row, self.group, self.track, self.index, self.subindex)
		pi.selection = self.selection
		
	def update_statusbar(self):
		# update plugin info
		self.update_plugin_info();		
		if self.plugin:
			if self.parameter_count[self.group] and self.group_track_count[self.group]:
				# change status bar
				self.statuslabels[0].set_label('Row %s, Track %s' % (self.row,self.track))
				parameter_list = self.plugin.get_parameter_list(self.group)
				self.statuslabels[2].set_label(prepstr(parameter_list[self.index].get_description() or ""))
				p = self.plugin.get_parameter(self.group,self.index)
				v = self.pattern.get_value(self.row, self.group, self.track, self.index)
				if v != p.get_value_none():
					text = prepstr(self.get_plugin().describe_value(self.group,self.index,v))
					s = get_str_from_param(p,self.pattern.get_value(self.row, self.group, self.track, self.index))
					self.statuslabels[1].set_label("%s (%i) %s" % (s,v,text))
				else:
					self.statuslabels[1].set_label("")		
					
	def refresh_view(self):
		if not self.plugin:
			return
		self.update_statusbar()
		self.redraw()
		self.adjust_scrollbars()
		self.update_font()

	def create_xor_gc(self):
		if not self.pattern:
			return
		if not self.window:
			return
		gc = self.window.new_gc()
		cm = gc.get_colormap()
		w,h = self.get_client_size()
		bbrush = cm.alloc_color('#ffffff')
		gc.set_function(gtk.gdk.XOR)
		gc.set_foreground(bbrush)
		gc.set_background(bbrush)
		self.xor_gc = gc

	def draw_xor(self):
		self.draw_cursor_xor()
		self.draw_playpos_xor()

	def draw_cursor_xor(self):
		if not self.pattern:
			return
		if not self.window:
			return
		drawable = self.window
		if not hasattr(self, "xor_gc"):
			self.create_xor_gc()
		gc = self.xor_gc
		PATROWHEIGHT = self.row_height
		PATTOPMARGIN = self.top_margin
		PATCOLWIDTH = self.column_width	
		cx,cy = self.pattern_to_pos(self.row, self.group, self.track, self.index, self.subindex)
		if (cx >= (PATLEFTMARGIN+4)) and (cy >= self.top_margin):
			drawable.draw_rectangle(gc, True,cx,cy,self.column_width,self.row_height)

	def draw_playpos_xor(self):
		if not self.pattern:
			return
		if not self.window:
			return	
		drawable = self.window
		if not hasattr(self, "xor_gc"):
			self.create_xor_gc()
		gc = self.xor_gc
		# draw play cursor
		current_position = self.playpos
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
						w,h = self.get_client_size()						
						drawable.draw_rectangle(gc, True,0, y, w, 2)
						return
	
	def get_plugin(self):
		"""
		Returns the plugin of the pattern in the pattern view.
		
		@return: zzub plugin plugin.
		@rtype: zzub.Plugin
		"""	
		if player.get_plugin_count() == 0:
			return
		toolbar = self.toolbar
		try:
			return toolbar.get_combo_plugin(toolbar.plugin_index)
		except KeyError:
			return player.get_plugin(0)
		
	def get_datasource(self):
		"""
		Returns the plugin and the current pattern in the pattern view
		
		@return: A tuple holding the plugin and the current pattern
		@rtype: (zzub.Plugin, zzub.Pattern)
		"""	
		tb = self.toolbar
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
			if self.lines[g]:
				tc = self.group_track_count[g]
				for t in range(tc):
					s = ' '.join([get_str_from_param(self.plugin.get_parameter(g, i),
									 self.pattern.get_value(row, g, t, i))
									for i in range(self.parameter_count[g])])
					try: 
						self.lines[g][t][row] = s
					except IndexError:
						pass

	# This does the same job as update_line, but if we need to
	# update a lot of data at once, it's faster to use update_col.
	def update_col(self, group, track):
		count = self.parameter_count[group]
		cols = [None] * count
		for i in range(count):
			param = self.plugin.get_parameter(group, i)
			get_value = self.pattern.get_value
			cols[i] = [get_str_from_param(param, get_value(row, group, track, i))
				   for row in range(self.row_count)]
		for row in range(self.row_count):
			try:
				self.lines[group][track][row] = \
				    ' '.join([cols[i][row] for i in range(count)])
			except IndexError:
				pass
		

	def prepare_textbuffer(self):
		"""
		Initializes a buffer to handle the current pattern data.
		"""
		#st = time.time()
		self.lines = [None]*3	
		for group in range(3):
			if self.parameter_count[group] > 0:
				tc = self.group_track_count[group]
				self.lines[group] = [None]*tc			
				for track in range(tc):
					self.lines[group][track] = [None]*self.row_count
					self.update_col(group, track)
			else:
				self.lines[group] = []
		#w,h=self.get_client_size()
		#self.row=0
		#for row in range(h/self.row_height):
		#print "end of prepare_textbuffer %.2f" % ((time.time() - st) * 1000.0)

	def get_line_pattern(self):
		master = player.get_plugin(0) 
		tpb = master.get_parameter_value(1, 0, 2)
		return {
			16: [64,32,16,8,4],
			12: [48,24,12,4],
			8: [32,16,8,4],
			6: [24,12,6],
			3: [12,4],
		}.get(tpb,[16,4])

	def draw(self,ctx):	
		"""
		Overriding a L{Canvas} method that paints onto an offscreen buffer.
		Draws the pattern view graphics.
		"""	
		st = time.time()
		row=None
		rows=None
		fulldraw=True
		if row == None:
			row = self.start_row
		cfg = config.get_config()
		w,h = self.get_client_size()
		PATROWHEIGHT = self.row_height
		PATTOPMARGIN = self.top_margin
		PATCOLWIDTH = self.column_width
		
		gc = self.window.new_gc()
		cm = gc.get_colormap()
		drawable = self.window

		bgbrush = cm.alloc_color(cfg.get_color('PE BG'))
	
		fbrush1 = cm.alloc_color(cfg.get_color('PE BG Very Dark'))
		fbrush2 = cm.alloc_color(cfg.get_color('PE BG Dark'))
		selbrush = cm.alloc_color(cfg.get_color('PE Sel BG'))
		pen = cm.alloc_color(cfg.get_color('PE Text'))
		
		gc.set_foreground(bgbrush)
		gc.set_background(bgbrush)
		gc.set_fill(gtk.gdk.SOLID)
		
		layout = pango.Layout(self.get_pango_context())
		layout.set_font_description(self.fontdesc)
		layout.set_width(-1)
	
		# clear the view if no current pattern
		if not self.pattern:
			drawable.draw_rectangle(gc, True, 0, 0, w, h)
			return

		if not rows:
			rows = self.row_count		
		clipy1 = PATROWHEIGHT + ((row - self.start_row) * self.row_height)
		clipy2 = PATROWHEIGHT + ((rows - self.start_row) * self.row_height)
		
		start_row, start_group, start_track, start_index, start_subindex = self.charpos_to_pattern((self.start_col, self.start_row))
		# full draw clears everything and draws all the line numbers and lines
		if fulldraw:
			drawable.draw_rectangle(gc, True, 0, 0, w, h)
			x, y = PATLEFTMARGIN, PATROWHEIGHT
			i = row
			y = clipy1
			gc.set_foreground(pen)
			num_rows = min(rows - row, (h - clipy1) / PATROWHEIGHT + 1)
			s = '\n'. join([str(i) for i in xrange(row, row+num_rows)])
			layout.set_text(s)
			px,py =layout.get_pixel_size()
			drawable.draw_layout(gc, x-5 - px, y, layout)
			drawable.draw_line(gc, x, 0, x, h)
			y = PATROWHEIGHT-1
			drawable.draw_line(gc, 0, y, w, y)
		startx = PATLEFTMARGIN + 4
		i = row
		y = clipy1
		gc.set_clip_rectangle(gtk.gdk.Rectangle(startx, 0, w - startx, h))
		if self.lines:
			linepattern = self.get_line_pattern()
			lpcount = len(linepattern)
			linecolors = []
			for lpindex, lp in enumerate(linepattern):
				lpf2 = lpindex / float(lpcount-1)
				lpf1 = 1.0 - lpf2
				red = int(fbrush1.red * lpf1 + fbrush2.red * lpf2)
				green = int(fbrush1.green * lpf1 + fbrush2.green * lpf2)
				blue = int(fbrush1.blue * lpf1 + fbrush2.blue * lpf2)
				linecolors.append(cm.alloc_color(red,green,blue))
			tc = self.group_track_count
			
			def draw_parameters_range(row, num_rows, group, track=0):
				"""Draw the parameter values for a range of rows"""
				x,y = self.pattern_to_pos(row, group, track, 0)
				s = '\n'.join([self.lines[group][track][i] for i in xrange(row, row+num_rows)])
				w = PATCOLWIDTH * len(self.lines[group][track][row])
				layout.set_text(s)
				px,py = layout.get_pixel_size()
				drawable.draw_layout(gc, x, y, layout)
				return x + px
			def draw_parameters(row, group, track=0):
				"""Draw the parameter values"""
				x,y = self.pattern_to_pos(row, group, track, 0)
				s = self.lines[group][track][row]
				w = PATCOLWIDTH * len(s)
				layout.set_text(s)
				px,py = layout.get_pixel_size()
				drawable.draw_layout(gc, x,y + PATROWHEIGHT/2 - (py/2), layout)
			
			# draw track background
			gc.set_foreground(bgbrush)
			gc.set_background(bgbrush)
			for g in CONN,GLOBAL,TRACK:
				if self.track_width[g]:
					for t in range(self.group_track_count[g]):
							if ((g == start_group) and (t >= start_track)) or (g > start_group):
								xs,fy = self.pattern_to_pos(row, g, t, 0)
								width = (self.track_width[g]-1)*self.column_width
								drawable.draw_rectangle(gc,True,xs,clipy1, width, clipy1+num_rows*PATROWHEIGHT)
								if xs + width > w:
									break
			while (i < rows) and (y < h):
				do_draw = False
				for lp,lc in zip(reversed(linepattern), reversed(linecolors)):
					if (i % lp) == 0:
						gc.set_foreground(lc)
						gc.set_background(lc)
						do_draw = True
				if do_draw:
					for g in CONN, GLOBAL, TRACK:
						if self.track_width[g]:
							for t in range(self.group_track_count[g]):
								if ((g == start_group) and (t >= start_track)) or (g > start_group):
									xs, fy = self.pattern_to_pos(row, g, t, 0)
									width = (self.track_width[g]-1)*self.column_width
									drawable.draw_rectangle(gc,True,xs,y, width, self.row_height)
									if xs + width > w:
										break
				i += 1
				y += PATROWHEIGHT
			# draw selection
			if self.selection:
				gc.set_foreground(selbrush)
				gc.set_background(selbrush)
				x,y1 = self.pattern_to_pos(self.selection.begin, 
					self.selection.group, self.selection.track, self.selection.index)
				x,y2 = self.pattern_to_pos(self.selection.end, 
					self.selection.group, self.selection.track, self.selection.index)
				y1 = max(clipy1, y1)
				y2 = min(clipy2, y2)
				if y2 > y1:
					if self.selection.mode == SEL_COLUMN:
						x2 = self.parameter_width[self.selection.group][self.selection.index]*self.column_width
						drawable.draw_rectangle(gc,True,x,y1,x2,y2-y1)
					elif self.selection.mode == SEL_TRACK:
						x2 = (self.track_width[self.selection.group]-1)*self.column_width
						drawable.draw_rectangle(gc,True,x,y1,x2,y2-y1)
					elif self.selection.mode == SEL_GROUP:
						for t in range(tc[self.selection.group]):
							x2 = (self.track_width[self.selection.group]-1)*self.column_width
							drawable.draw_rectangle(gc,True,x,y1,x2,y2-y1)
							x += self.track_width[self.selection.group] * self.column_width
					elif self.selection.mode == SEL_ALL:
						for g in range(3):
							if self.track_width[g]:
								for t in range(tc[g]):
									drawable.draw_rectangle(gc,True,x,y1,(self.track_width[g]-1)*self.column_width,y2-y1)
									x += self.track_width[g] * self.column_width
			# draw the parameter values
			i = row
			y = clipy1
			gc.set_foreground(pen)
			for track in range(self.group_track_count[TRACK]):
				x, y = self.pattern_to_pos(row, TRACK, track, 0)								
				s = str(track)
				width = self.track_width[TRACK]*self.column_width
				layout.set_text(s)
				px,py = layout.get_pixel_size()
				drawable.draw_layout(gc, x + width/2 - px/2, PATROWHEIGHT/2 - (py/2), layout)
			parameter_list = self.plugin.get_parameter_list(TRACK)
			num_rows = min(rows - row, (h - clipy1) / PATROWHEIGHT + 1)
			out_of_bounds = False
			for t in range(self.group_track_count[CONN]):
				extent = draw_parameters_range(row, num_rows, CONN, t)
				out_of_bounds = extent > w
			if not out_of_bounds:	
				if self.lines[GLOBAL]:
					extent = draw_parameters_range(row, num_rows, GLOBAL, 0)
					out_of_bounds = extent > w
				if not out_of_bounds:
					for t in range(self.group_track_count[TRACK]):
						extent = draw_parameters_range(row, num_rows, TRACK, t)
						if extent > w:
							break
						
		self.draw_xor()
		#print "%ims" % ((time.time() - st)*1000)

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
	import testplayer, utils
	player = testplayer.get_player()
	player.load_ccm(utils.filepath('demosongs/paniq-knark.ccm'))
	window = testplayer.TestWindow()
	# update_position() needs the index to be set:
	# (main.AldrinFrame.PAGE_PATTERN = 0)
	window.PAGE_PATTERN = 0
	window.index = 0 
	window.add(PatternPanel(window))
	window.show_all()
	gtk.main()
