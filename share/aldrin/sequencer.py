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
Contains all classes and functions needed to render the sequence
editor and its associated components.
"""

from wximport import wx
from canvas import Canvas, BitmapBuffer
from utils import prepstr, from_hsb, to_hsb
import random
import ctypes
import zzub
import config
import time

SEQKEYS = '0123456789abcdefghijklmnopqrstuvwxyz'
SEQKEYMAP = dict(zip(SEQKEYS,range(0x10,len(SEQKEYS)+0x10)))
SEQKEYMAP[chr(45)] = 0x00
SEQKEYMAP[chr(44)] = 0x01

class PatternNotFoundException(Exception):
	"""	
	Exception thrown when pattern is not found.
	"""
	pass


class SequencerToolBar(wx.Panel):
	"""
	Sequencer Toolbar
	
	Allows to set the step size for the sequencer view.
	"""
	def __init__(self, *args, **kwds):
		"""
		Initialization.
		"""
		# begin wxGlade: SequencerFrame.__init__
		# kwds['style'] = wx.TB_HORIZONTAL|wx.TB_FLAT
		self.parent = args[0]
		wx.Panel.__init__(self, *args, **kwds)
		self.steplabel = wx.StaticText(self, -1, label="&Step")
		self.stepselect = wx.Choice(self, -1)
		self.steps = [1,2,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60,64,96,128,192,256,512,1024]
		wx.EVT_CHOICE(self, self.stepselect.GetId(), self.on_stepselect)

		sizer = wx.BoxSizer(wx.HORIZONTAL)
		sizer.Add(self.steplabel, 0, wx.ALIGN_CENTER_VERTICAL|wx.LEFT|wx.TOP|wx.BOTTOM, 5)
		sizer.Add(self.stepselect, 0, wx.ALIGN_CENTER_VERTICAL|wx.LEFT|wx.TOP|wx.BOTTOM, 5)
		self.SetAutoLayout(True)
		self.SetSizer(sizer)
		self.Layout()
		self.Fit()
		
	def increase_step(self):
		if self.parent.view.step < 1024:
			self.parent.view.step *= 2
		self.update_stepselect()
		self.parent.update_all()
		
	def decrease_step(self):
		if self.parent.view.step > 1:
			self.parent.view.step /= 2
		self.update_stepselect()
		self.parent.update_all()
		
	def update_all(self):
		"""
		Updates the toolbar to reflect sequencer changes.
		"""
		self.update_stepselect()
	
	def update_stepselect(self):
		"""
		Updates the step selection choice box.
		"""
		self.stepselect.Clear()
		for i in self.steps:
			self.stepselect.Append("%i" % i)
		try:
			self.stepselect.SetSelection(self.steps.index(self.parent.view.step))
		except:
			self.stepselect.SetStringSelection("%i" % self.parent.view.step)
		config.get_config().set_default_int('SequencerStep', self.parent.view.step)
		
	def on_stepselect(self, event):
		"""
		Handles events sent from the choice box when a step size is being selected.
		"""
		self.parent.view.step = self.steps[self.stepselect.GetSelection()]
		self.parent.update_all()

class SequencerPanel(wx.Panel):
	"""
	Sequencer pattern panel.
	
	Displays all the patterns available for the current track.
	"""
	def __init__(self, rootwindow, *args, **kwds):
		"""
		Initialization.
		"""
		# begin wxGlade: SequencerFrame.__init__
		#kwds["style"] = wx.DEFAULT_PANEL_STYLE
		self.rootwindow = rootwindow
		wx.Panel.__init__(self, *args, **kwds)
		self.splitter = wx.SplitterWindow(self, -1, style=wx.SP_LIVE_UPDATE|wx.SP_NOBORDER)
		self.seqpatternlist = wx.ListBox(self.splitter, -1, style=wx.LB_SINGLE|wx.SUNKEN_BORDER)		
		self.seqview = SequencerView(rootwindow, self.splitter, -1)		
		self.view = self.seqview
		self.toolbar = SequencerToolBar(self, -1)
		self.statusbar = wx.StatusBar(self, style=0)
		self.__set_properties()
		self.__do_layout()
		# end wxGlade
		self.update_list()
		self.toolbar.update_all()
		wx.EVT_SPLITTER_SASH_POS_CHANGED(self, self.splitter.GetId(), self.on_sash_pos_changed)
		
	def update_all(self):
		"""
		Updates everything to reflect changes in the sequencer.
		"""
		self.update_list()
		self.toolbar.update_all()
		for k,v in self.view.plugin_info.iteritems():
			v.patterngfx = {}
		self.view.update()
		
	def update_list(self):
		"""
		Updates the panel to reflect a sequence view change.
		"""
		self.seqpatternlist.Clear()
		self.seqpatternlist.Append('- Mute (X)')
		self.seqpatternlist.Append(', Break (<)')
		seq = player.get_current_sequencer()
		if (self.seqview.track != -1) and (self.seqview.track < seq.get_track_count()):
			track = seq.get_track(self.seqview.track)
			for pattern, key in zip(track.get_plugin().get_pattern_list(), SEQKEYS):
				name = "%s. %s" % (key, pattern.get_name())
				self.seqpatternlist.Append(name)
				
	def on_sash_pos_changed(self, event):
		"""
		Sent when the sash position changes.
		
		@param event: Event.
		@type event: wx.Event
		"""
		event.Skip()
		config.get_config().save_window_pos("SequencerSplitter", self.splitter)
		
	def __set_properties(self):
		"""
		Sets properties during initialization.
		"""
		# begin wxGlade: SequencerFrame.__set_properties
		self.statusbar.SetFieldsCount(2)
		self.statusbar.SetStatusWidths([-1, -10])
		self.statusbar.SetSize((100, 20))
		self.splitter.SetMinimumPaneSize(10)
		# end wxGlade

	def __do_layout(self):
		"""
		Arranges children components during initialization.
		"""
		# begin wxGlade: SequencerFrame.__do_layout
		self.splitter.SplitVertically(self.seqview, self.seqpatternlist, -100)
		#self.splitter.SetSashPosition(-100)
		self.splitter.SetSashGravity(0.0)
		config.get_config().load_window_pos("SequencerSplitter", self.splitter)
		sizer = wx.BoxSizer(wx.VERTICAL)
		sizer.Add(self.toolbar, 0, wx.EXPAND, 0)
		sizer.Add(self.splitter, 1, wx.EXPAND, 0)
		sizer.Add(self.statusbar, 0, wx.EXPAND, 0)
		self.SetAutoLayout(True)
		self.SetSizer(sizer)
		self.Layout()
		# end wxGlade

# end of class SequencerFrame

SEQTRACKSIZE = 16
SEQSTEP = 16
SEQLEFTMARGIN = 64
SEQTOPMARGIN = SEQTRACKSIZE
SEQROWSIZE = 24

class SequencerView(Canvas):
	"""
	Sequence viewer class.
	"""	
	CLIPBOARD_SEQUENCER = "SEQUENCERDATA"
	
	def __init__(self, rootwindow, *args, **kwds):
		"""
		Initialization.
		"""
		kwds['style'] = wx.SUNKEN_BORDER | wx.WANTS_CHARS
		self.rootwindow = rootwindow
		self.plugin_info = self.rootwindow.routeframe.view.plugin_info
		self.playpos = player.get_position()
		self.row = 0
		self.track = 0
		self.startseqtime = 0
		self.starttrack = 0
		self.step = config.get_config().get_default_int('SequencerStep', SEQSTEP)
		player.set_loop_end(self.step)
		self.parent = args[0].GetParent()
		self.selection_start = None
		self.selection_end = None
		self.dragging = False
		Canvas.__init__(self, *args, **kwds)
		self.timer = wx.Timer(self, -1)
		self.timer.Start(100)
		wx.EVT_TIMER(self, self.timer.GetId(), self.update_position)
		wx.EVT_MOUSEWHEEL(self, self.on_mousewheel)
		wx.EVT_LEFT_DOWN(self, self.on_left_down)
		wx.EVT_MOTION(self, self.on_motion)
		wx.EVT_LEFT_UP(self, self.on_left_up)
		wx.EVT_KEY_DOWN(self, self.on_key_down)
		wx.EVT_CONTEXT_MENU(self, self.on_context_menu)
		wx.EVT_SET_FOCUS(self, self.on_focus)
		
	def on_focus(self, event):		
		seq = player.get_current_sequencer()
		if seq.get_track_count() > 0:
			self.set_cursor_pos(self.track, self.row)
		self.parent.update_list()
		self.ReDraw()
		
	def track_row_to_pos(self, (track,row)):
		"""
		Converts track and row to a pixel coordinate.
		
		@param track: Track index.
		@type track: int
		@param row: Row index.
		@type row: int
		@return: Pixel coordinate.
		@rtype: (int, int)
		"""
		if row == -1:
			x = 0
		else:
			x = int((((float(row) - self.startseqtime)/self.step) * SEQROWSIZE) + SEQLEFTMARGIN + 0.5)
		if track == -1:
			y = 0
		else:
			y = ((track - self.starttrack) * SEQTRACKSIZE) + SEQTOPMARGIN
		return x,y
		
	def pos_to_track_row(self, (x,y)):
		"""
		Converts pixel coordinate to a track and row.
		
		@param x: Pixel coordinate.
		@type x: int
		@param y: Pixel coordinate.
		@type y: int
		@return: Tuple containing track and row index.
		@rtype: (int, int)
		"""
		if x < SEQLEFTMARGIN:
			row = -1
		else:
			row = (((x - SEQLEFTMARGIN) / SEQROWSIZE) * self.step) + self.startseqtime
		if y < SEQTOPMARGIN:
			track = -1
		else:
			track = ((y - SEQTOPMARGIN) / SEQTRACKSIZE) + self.starttrack
		return track,row
		
	def get_endtrack(self):
		"""
		Get the last visible track.
		"""
		w,h = self.GetSize()
		return self.pos_to_track_row((0,h))[0]
		
	def get_endrow(self):
		"""
		Get the last visible row.
		"""
		w,h = self.GetSize()
		return self.pos_to_track_row((w,0))[1]
		
	def set_cursor_pos(self, track, row):
		"""
		Updates the cursor position to a track and row.
		
		@param track: Pattern index.
		@type track: int
		@param row: Row index.
		@type row: int
		"""
		seq = player.get_current_sequencer()
		track = max(min(track, seq.get_track_count()-1),0)
		row = max(row,0)
		if (track,row) == (self.track,self.row):
			return
		if self.track != track:
			self.track = track
			endtrack = self.get_endtrack()
			if self.track >= endtrack:
				while self.track >= endtrack:
					self.starttrack += 1
					endtrack = self.get_endtrack()
				self.ReDraw()
			elif self.track < self.starttrack:
				while self.track < self.starttrack:
					self.starttrack -= 1
				self.ReDraw()
			self.parent.update_list()
		if self.row != row:
			self.row = row
			endrow = self.get_endrow()
			if self.row >= endrow:
				while self.row >= endrow:
					self.startseqtime += self.step
					endrow = self.get_endrow()
				self.ReDraw()
			elif self.row < self.startseqtime:
				while self.row < self.startseqtime:
					self.startseqtime -= self.step
				self.ReDraw()
		t = seq.get_track(self.track)		
		plugin = t.get_plugin()
		self.parent.statusbar.SetStatusText(prepstr('%s' % (self.row)), 0)
		self.parent.statusbar.SetStatusText(prepstr('%s' % (plugin.get_name())), 1)		
		self.Refresh()

	def insert_at_cursor(self, index = -1):
		"""
		Inserts a space at cursor.
		"""
		seq = player.get_current_sequencer()
		t = seq.get_track(self.track)
		if index != -1:
			pcount = t.get_plugin().get_pattern_count()
			t.set_event(self.row, min(index, 0x10 + pcount-1))
		else:
			t.move_events(self.row, self.step)
		self.ReDraw()
		
	def delete_at_cursor(self):
		"""
		Deletes pattern at cursor.
		"""		
		seq = player.get_current_sequencer()
		t = seq.get_track(self.track)
		t.remove_event_at(self.row)
		t.move_events(self.row, -self.step)
		self.ReDraw()
	
	def selection_range(self):
		seq = player.get_current_sequencer()		
		start = (min(self.selection_start[0], self.selection_end[0]), 
					min(self.selection_start[1], self.selection_end[1]))
		end = (max(self.selection_start[0], self.selection_end[0]), 
					max(self.selection_start[1], self.selection_end[1]))
		for track in range(start[0], end[0]+1):
			t = seq.get_track(track)
			events = dict(t.get_event_list())
			for row in range(start[1], end[1]+1):
				if row in events:
					yield track, row, events[row]
				else:
					yield track, row, -1			
				
	def unpack_clipboard_data(self, d):
		"""
		Unpacks clipboard data
		
		@param d: Data that is to be unpacked.
		@type d: unicode
		"""
		magic,d = d[:len(self.CLIPBOARD_SEQUENCER)], d[len(self.CLIPBOARD_SEQUENCER):]
		assert magic == self.CLIPBOARD_SEQUENCER
		while d:
			track,d = int(d[:4],16),d[4:]
			row,d = int(d[:8],16),d[8:]			
			value,d = int(d[:4],16),d[4:]			
			yield track, row, value
	
	def on_popup_copy(self, event=None):
		"""
		Copies the current selection into the clipboard
		"""
		if self.selection_start == None:
			return		
		data = self.CLIPBOARD_SEQUENCER		
		startrow = min(self.selection_start[1], self.selection_end[1])
		for track,row,value in self.selection_range():
			data += "%04x%08x%04x" % (track, row - startrow, value)
		clipboard = wx.TheClipboard
		if clipboard.Open():
			clipboard.SetData(wx.TextDataObject(data))
			clipboard.Close()
		
	def on_popup_cut(self, event=None):
		self.on_popup_copy(event)
		self.on_popup_delete(event)
		
	def on_popup_paste(self, event=None):	
		seq = player.get_current_sequencer()
		clipboard = wx.TheClipboard
		data = ""
		if clipboard.Open():
			d = wx.TextDataObject()
			clipboard.GetData(d)
			data = d.GetText()
			clipboard.Close()	
		for track,row,value in self.unpack_clipboard_data(data.strip()):
			t = seq.get_track(track)
			if value == -1:
				t.remove_event_at(self.row + row)
			else:
				t.set_event(self.row + row, value)
			
		self.ReDraw()
		
	def on_popup_delete(self, event):
		seq = player.get_current_sequencer()
		start = (min(self.selection_start[0], self.selection_end[0]), 
			min(self.selection_start[1], self.selection_end[1]))
		end = (max(self.selection_start[0], self.selection_end[0]), 
					max(self.selection_start[1], self.selection_end[1]))
		for track in range(start[0], end[0]+1):
			t = seq.get_track(track)
			for row in range(start[1], end[1]+1):
				t.remove_event_at(row)
		self.ReDraw()
		
	def on_popup_delete_track(self, event):
		"""
		Callback that handles track deletion via the popup menu
		
		@param event: Menu event.
		@type event: wx.CommandEvent
		"""
		seq = player.get_current_sequencer()
		seq.remove_track(self.track)
		track_count = seq.get_track_count()
		# moves cursor if beyond existing tracks
		if self.track > track_count-1:			
			self.set_cursor_pos(track_count-1, self.row)
		self.ReDraw()
		
	def on_popup_add_track(self, event):
		"""
		Callback that handles track addition via the popup menu
		
		@param event: Menu event.
		@type event: wx.CommandEvent
		"""
		seq = player.get_current_sequencer()
		seq.create_track(self.popup_plugins[event.GetId()])
		self.ReDraw()		

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
			self.popup_cut = wx.NewId()
			self.popup_copy = wx.NewId()
			self.popup_paste = wx.NewId()
			self.popup_delete = wx.NewId()

		# refresh the add plugin listing
		self.popup_plugins = {}
		for plugin in player.get_plugin_list():			
			self.popup_plugins[wx.NewId()] = plugin		
		
		self.Bind(wx.EVT_MENU, self.on_popup_delete_track, id=self.popup_delete_track)
		for plugin_id in self.popup_plugins.keys():
			self.Bind(wx.EVT_MENU, self.on_popup_add_track, id=plugin_id)
			
		menu = wx.Menu()		
		sm = wx.Menu()
		for id, plugin in self.popup_plugins.iteritems():
			sm.Append(id, prepstr(plugin.get_name()))		
		menu.AppendMenu(self.popup_add_track, "Add Track\tCtrl+Enter", sm)
		menu.Append(self.popup_delete_track, "Delete track\tCtrl+Del")
		menu.AppendSeparator()
		menu.Append(self.popup_cut, "Cut")
		menu.Append(self.popup_copy, "Copy")
		menu.Append(self.popup_paste, "Paste")
		menu.Append(self.popup_delete, "Delete")
		self.Bind(wx.EVT_MENU, self.on_popup_copy, id=self.popup_copy)
		self.Bind(wx.EVT_MENU, self.on_popup_paste, id=self.popup_paste)
		self.Bind(wx.EVT_MENU, self.on_popup_cut, id=self.popup_cut)
		self.Bind(wx.EVT_MENU, self.on_popup_delete, id=self.popup_delete)
		# Popup the menu.  If an item is selected then its handler
		# will be called before PopupMenu returns.
		self.PopupMenu(menu)
		menu.Destroy()
		
	def show_plugin_dialog(self):		
		choices = []
		for plugin in player.get_plugin_list():			
			choices.append(prepstr(plugin.get_name()))
		dlg = wx.SingleChoiceDialog(
			self, 
			'', 
			'Select Plugin', 
			choices,
			style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER | wx.CENTRE)
		#~ for child in dlg.GetChildren():
			#~ if isinstance(child,wx.ListBox):
				#~ child.SetFocus()
				#~ break
		if dlg.ShowModal() == wx.ID_OK:			
			seq = player.get_current_sequencer()
			seq.create_track(player.get_plugin(dlg.GetSelection()))
			self.ReDraw()
		dlg.Destroy()
		
	def on_key_down(self, event):
		"""
		Callback that responds to key stroke in sequence view.
		
		@param event: Key event
		@type event: wx.KeyEvent
		"""
		seq = player.get_current_sequencer()
		k = event.GetKeyCode()
		arrow_down = k in [wx.WXK_LEFT, wx.WXK_RIGHT, wx.WXK_UP, wx.WXK_DOWN,
			wx.WXK_NUMPAD_LEFT, wx.WXK_NUMPAD_RIGHT, wx.WXK_NUMPAD_UP, wx.WXK_NUMPAD_DOWN]	
		is_selecting = arrow_down and event.ShiftDown()
		if is_selecting:
			# starts the selection if nothing selected
			if self.selection_start == None:
				self.selection_start = (self.track, self.row)				
		elif arrow_down:
			self.deselect()
		if event.ShiftDown():
			if k == wx.WXK_NUMPAD_ADD or k == wx.WXK_LEFT or k == wx.WXK_NUMPAD_LEFT:
				self.parent.toolbar.increase_step()
				self.set_cursor_pos(self.track, self.row)
			elif k == wx.WXK_NUMPAD_SUBTRACT or k == wx.WXK_RIGHT or k == wx.WXK_NUMPAD_RIGHT:
				self.parent.toolbar.decrease_step()
				self.set_cursor_pos(self.track, self.row)
		elif event.ControlDown():
			if k == wx.WXK_RETURN:
				self.show_plugin_dialog()
			elif k == wx.WXK_DELETE:
				self.on_popup_delete_track(event)
			elif k == ord('B'):				
				player.set_loop_start(self.row)
				if player.get_loop_end() <= self.row:
					player.set_loop_end(self.row + self.step)
				self.ReDraw()
			elif k == ord('E'):
				pos = self.row# + self.step
				if player.get_loop_end() != pos:
					player.set_loop_end(pos)
					if pos > player.get_song_end():
						player.set_song_end(pos)
					if player.get_loop_start() >= pos:
						player.set_loop_start(0)
				else:
					player.set_song_end(pos)
				self.ReDraw()
			elif k == ord('L'):			
				mp = seq.get_track(self.track).get_plugin()
				self.rootwindow.routeframe.view.solo(mp)
				self.ReDraw()
			elif k == ord('I'):
				for track in seq.get_track_list():
					track.move_events(self.row, self.step)
				self.ReDraw()	
			elif k == ord('C'):	
				self.on_popup_copy()
			elif k == ord('X'):	
				self.on_popup_cut()
			elif k == ord('V'):	
				self.on_popup_paste()
			elif k == wx.WXK_UP or k == wx.WXK_NUMPAD_UP:
				if self.track > 0:
					seq.move_track(self.track, self.track-1)
					self.track -= 1
					self.ReDraw()	
			elif k == wx.WXK_DOWN or k == wx.WXK_NUMPAD_DOWN:
				if self.track < (seq.get_track_count()-1):
					seq.move_track(self.track, self.track+1)
					self.track += 1
					self.ReDraw()
			elif k == wx.WXK_LEFT or k == wx.WXK_NUMPAD_LEFT:
				self.set_cursor_pos(self.track, self.row - (self.step*16))
			elif k == wx.WXK_RIGHT or k == wx.WXK_NUMPAD_RIGHT:
				self.set_cursor_pos(self.track, self.row + (self.step*16))
			else:
				event.Skip()
		elif k == wx.WXK_LEFT or k == wx.WXK_NUMPAD_LEFT:
			self.set_cursor_pos(self.track, self.row - self.step)
		elif k == wx.WXK_RIGHT or k == wx.WXK_NUMPAD_RIGHT:
			self.set_cursor_pos(self.track, self.row + self.step)
		elif k == wx.WXK_UP or k == wx.WXK_NUMPAD_UP:
			self.set_cursor_pos(self.track-1, self.row)
		elif k == wx.WXK_DOWN or k == wx.WXK_NUMPAD_DOWN:
			self.set_cursor_pos(self.track+1, self.row)
		elif (k < 256) and (chr(k).lower() in SEQKEYMAP):
			idx = SEQKEYMAP[chr(k).lower()]
			mp = seq.get_track(self.track).get_plugin()
			if (idx < 0x10) or ((idx-0x10) < mp.get_pattern_count()):
				if (idx >= 0x10):
					newrow = self.row + mp.get_pattern(idx-0x10).get_row_count()
					newrow = newrow - (newrow % self.step)
				else:
					newrow = self.row + self.step
				self.insert_at_cursor(idx)
				self.set_cursor_pos(self.track, newrow)
		elif k == 32: # space
			spl = self.parent.seqpatternlist
			sel = min(max(spl.GetSelection(),0),spl.GetCount()-1)
			if sel >= 2:
				sel = sel - 2 + 0x10
			self.insert_at_cursor(sel)
			self.set_cursor_pos(self.track, self.row + self.step)
		elif k == wx.WXK_DELETE:
			self.delete_at_cursor()
		elif k == wx.WXK_INSERT:
			self.insert_at_cursor()
		elif k == 46: # dot
			m,pat,bp = self.get_pattern_at(self.track, self.row, includespecial=True)
			if pat != None:
				if pat >= 0x10:
					pat = m.get_pattern(pat - 0x10)
					length = pat.get_row_count()
				else:
					length = self.step
			else:
				length = 0
			if (self.row < (bp + length)):
				newrow = bp + length
				t = seq.get_track(self.track)
				t.remove_event_at(bp)
				self.set_cursor_pos(self.track, newrow - (newrow % self.step))
				self.ReDraw()
		elif k == wx.WXK_PRIOR or k == wx.WXK_NUMPAD_PRIOR:
			spl = self.parent.seqpatternlist
			sel = spl.GetSelection()
			spl.SetSelection(min(max(sel-1,0),spl.GetCount()-1))
		elif k == wx.WXK_NEXT or k == wx.WXK_NUMPAD_NEXT:
			spl = self.parent.seqpatternlist
			sel = spl.GetSelection()
			spl.SetSelection(min(max(sel+1,0),spl.GetCount()-1))
		elif k == wx.WXK_RETURN:
			m, index, bp = self.get_pattern_at(self.track, self.row)
			if index == None:
				track = seq.get_track(self.track)
				self.jump_to_pattern(track.get_plugin())
				return
			self.jump_to_pattern(m, index)
		else:
			event.Skip()
		# update selection after cursor movement
		if is_selecting:
			self.selection_end = (self.track, self.row)	
			self.ReDraw()
		
			
	def jump_to_pattern(self, plugin, index=0):
		"""
		Views a pattern in the pattern view.
		
		@param plugin: Plugin.
		@type plugin: zzub.Plugin
		@param index: Pattern index.
		@type index: int
		"""
		mainwindow = self.parent.rootwindow
		pf = mainwindow.patternframe
		tb = pf.toolbar
		tb.plugin = 0
		tb.pattern = 0
		for i in range(player.get_plugin_count()):
			if player.get_plugin(i) == plugin:
				tb.plugin = i
				break
		tb.pattern = index
		pf.update_all()
		mainwindow.select_page(mainwindow.PAGE_PATTERN)
		
	def get_pattern_at(self, track, row, includespecial=False):
		"""
		Gets the pattern and plugin given a sequencer track and row.
		
		@param track: Track index.
		@type track: int
		@param row: Row index.
		@type row: int
		@return: Tuple containing plugin and pattern index.
		@rtype: (zzub.Plugin, int)		
		"""		
		seq = player.get_current_sequencer()
		track = seq.get_track(track)
		plugin = track.get_plugin()
		bestmatch = None
		bestpos = row
		for pos, value in track.get_event_list():
			if pos > row:
				break
			elif includespecial:
				bestpos = pos
				bestmatch = value
			elif (value >= 0x10):
				bestpos = pos
				bestmatch = value - 0x10
		return plugin, bestmatch, bestpos
	
	def deselect(self):
		"""
		Deselects the current selection.
		"""		
		if self.selection_end != None:
			self.dragging = False
			self.selection_end = None
			self.selection_start = None
			self.ReDraw()
	
	def on_mousewheel(self, event):
		"""
		Callback that responds to mousewheeling in sequencer.
		
		@param event: Mouse event
		@type event: wx.MouseEvent
		"""		
		if event.GetWheelRotation() > 0:
			self.set_cursor_pos(self.track, self.row - self.step)
		else:
			self.set_cursor_pos(self.track, self.row + self.step)
	
	def on_left_down(self, event):
		"""
		Callback that responds to left click down in sequence view.
		
		@param event: Mouse event
		@type event: wx.MouseEvent
		"""		
		self.SetFocus()
		seq = player.get_current_sequencer()
		track_count = seq.get_track_count()
		track, row = self.pos_to_track_row(event.GetPosition())		
		if track < track_count:
			if track == -1:
				player.set_position(max(row,0))
			elif row == -1:
				mp = seq.get_track(track).get_plugin()				
				self.rootwindow.routeframe.view.toggle_mute(mp)
				self.ReDraw()
			else:
				self.set_cursor_pos(track,row)
				self.deselect()
				self.dragging = True
				self.CaptureMouse()
	
	def on_motion(self, event):
		"""
		Callback that responds to mouse motion in sequence view.
		
		@param event: Mouse event
		@type event: wx.MouseEvent
		"""		
		if self.dragging:
			select_track, select_row = self.pos_to_track_row(event.GetPosition())			
			# start selection if nothing selected			
			if self.selection_start == None:				
				self.selection_start = (self.track, self.row)
			if self.selection_start:
				seq = player.get_current_sequencer()
				select_track = min(seq.get_track_count()-1, max(select_track, 0))
				select_row = max(select_row, 0)
				self.selection_end = (select_track, select_row)
				self.ReDraw()
			
	def on_left_up(self, event):
		"""
		Callback that responds to left click up in sequence view.
		
		@param event: Mouse event
		@type event: wx.MouseEvent
		"""
		if self.dragging:
			self.dragging = False
			self.ReleaseMouse()

	def update_position(self, event):
		"""
		Updates the position.
		"""
		if not self.parent.IsShown(): return 	# Only needed by OSX
		playpos = player.get_position()
		if self.playpos != playpos:
			self.playpos = playpos
			self.Refresh()

	def onPostPaint(self, dc):
		"""
		Overriding a L{Canvas} method that is called after painting is completed. 
		Draws an XOR play cursor over the pattern view.
		
		@param dc: wx device context.
		@type dc: wx.PaintDC
		"""
		w,h = self.GetSize()
		bbrush = wx.Brush('#ffffff',wx.SOLID)		
		dc.SetBrush(bbrush)
		dc.SetPen(wx.TRANSPARENT_PEN)
		dc.SetLogicalFunction(wx.XOR)
		seq = player.get_current_sequencer()
		track_count = seq.get_track_count()
		# draw cursor
		if track_count > 0:
			x,y = self.track_row_to_pos((self.track,self.row))
			dc.DrawRectangle(x,y+1,SEQROWSIZE-1,SEQTRACKSIZE-1)
		if self.playpos >= self.startseqtime:
			# draw play cursor
			x = SEQLEFTMARGIN + int((float(self.playpos - self.startseqtime) / self.step) * SEQROWSIZE)
			dc.DrawRectangle(x, 0, 2, h)
			
	def update(self):
		"""
		Updates the view after a lot of data has changed. This will also
		reset selection.
		"""
		self.startseqtime = self.startseqtime - (self.startseqtime % self.step)
		self.selection_start = None
		if self.row != -1:
			self.row = self.row - (self.row % self.step)
		self.ReDraw()
		
	def DrawBuffer(self):
		"""
		Overriding a L{Canvas} method that paints onto an offscreen buffer.
		Draws the pattern view graphics.
		"""	
		st = time.time()
		w,h = self.GetSize()
		dc = self.buffer
		seq = player.get_current_sequencer()
		cfg = config.get_config()
		bghsb = to_hsb(*cfg.get_float_color('SE BG'))
		bgb = max(bghsb[2],0.1)
		font = wx.Font(7.5, wx.FONTFAMILY_DEFAULT, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_NORMAL)
		type2brush = {
			zzub.zzub_plugin_type_effect : cfg.get_brush('MV Effect'),
			zzub.zzub_plugin_type_generator : cfg.get_brush('MV Generator'),
			zzub.zzub_plugin_type_master : cfg.get_brush('MV Master'),
		}
		bgbrush = cfg.get_brush('SE BG')
		fbrush1 = cfg.get_brush('SE BG Very Dark')
		fbrush2 = cfg.get_brush('SE BG Dark')
		sbrushes = [cfg.get_brush('SE Mute'), cfg.get_brush('SE Break')]
		select_brush = cfg.get_brush('SE Sel BG')
		vlinepen = cfg.get_pen('SE BG Dark')
		pen1 = cfg.get_pen('SE BG Very Dark')
		pen2 = cfg.get_pen('SE BG Dark')
		pen = cfg.get_pen('SE Line')
		loop_pen = wx.Pen(cfg.get_color('SE Loop Line'), 1, wx.SHORT_DASH)
		invbrush = wx.Brush('#ffffff',wx.SOLID)		
		dc.SetFont(font)
		dc.SetBackground(bgbrush)
		dc.SetBrush(bgbrush)
		dc.SetTextForeground(cfg.get_color('SE Text'))
		dc.Clear()
		# 14
		x, y = SEQLEFTMARGIN, SEQTOPMARGIN
		i = self.startseqtime
		while x < w:
			dc.SetPen(wx.TRANSPARENT_PEN)
			if (i % (16*self.step)) == 0:
				dc.SetPen(pen1)
				dc.DrawLine(x-1, 0, x-1, h)
				dc.DrawLabel("%s" % i, wx.Rect(x, 0, SEQROWSIZE, SEQTRACKSIZE), wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL)
			elif (i % (4*self.step)) == 0:
				dc.SetPen(pen2)
				dc.DrawLine(x-1, 0, x-1, h)
				dc.DrawLabel("%s" % i, wx.Rect(x, 0, SEQROWSIZE, SEQTRACKSIZE), wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL)
			x += SEQROWSIZE
			i += self.step
		dc.SetPen(pen)
		dc.DrawLine(0, y, w, y)
		endrow = self.startseqtime + (w / SEQROWSIZE) * self.step
		tracklist = seq.get_track_list()
		solo_plugin = self.rootwindow.routeframe.view.solo_plugin
		sel = False
		if self.selection_start != None and self.selection_end != None:
			sel = True
			selstart = (min(self.selection_start[0], self.selection_end[0]), 
				min(self.selection_start[1], self.selection_end[1]))
			selend = (max(self.selection_start[0], self.selection_end[0]), 
				max(self.selection_start[1], self.selection_end[1]))
		for track_index in range(self.starttrack, len(tracklist)):
			track = tracklist[track_index]
			m = track.get_plugin()
			pi = self.plugin_info[m]
			pgfx = pi.patterngfx
			mname = m.get_name()
			title = prepstr(mname)
			if solo_plugin and solo_plugin != m and m.get_type() == zzub.zzub_plugin_type_generator:
				title = "[%s]" % title
			elif self.plugin_info[m].muted:
				title = "(%s)" % title
			dc.SetPen(pen)
			dc.SetBrush(type2brush[m.get_type()])
			dc.DrawRectangle(0, y, SEQLEFTMARGIN, SEQTRACKSIZE+1)
			dc.DrawLabel(title, wx.Rect(0, y, SEQLEFTMARGIN-4, SEQTRACKSIZE), wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL)
			intrack = sel and ((track_index >= selstart[0]) and (track_index <= selend[0]))
			# draw selected block if it falls within selection
			if intrack:
				x = SEQLEFTMARGIN
				i = self.startseqtime
				while x < w:
					if (i >= selstart[1]) and (i <= selend[1]):
						dc.SetBrush(select_brush) 
						dc.SetPen(wx.TRANSPARENT_PEN) 
						dc.DrawRectangle(x,y+1,SEQROWSIZE-1,SEQTRACKSIZE-1)					
					x += SEQROWSIZE
					i += self.step
			for pos, value in track.get_event_list():
				bb = pgfx.get(value, None)
				if not bb:
					if value >= 0x10:
						pat = m.get_pattern(value-0x10)
						name,length = prepstr(pat.get_name()), pat.get_row_count()
					elif value == 0x00:
						name,length = "X", 1
						special = True
					elif value == 0x01:
						name,length = "<", 1
						special = True
					else:
						print "unknown value:",value
						name,length = "???",0
					psize = max(int(((SEQROWSIZE * length) / self.step) + 0.5),2)
					bb = BitmapBuffer(psize-1, SEQTRACKSIZE-1)
					pgfx[value] = bb
					bb.SetFont(font)
					if value < 0x10:
						bb.SetPen(wx.TRANSPARENT_PEN)
						bb.SetBrush(sbrushes[value])
					else:
						random.seed(mname+name)
						hue = random.random()
						cb = 1.0
						r,g,b = from_hsb(hue, 0.2, cb*bgb)
						bb.SetBrush(wx.Brush(wx.Colour(int(r*255),int(g*255),int(b*255)),wx.SOLID))
						r,g,b = from_hsb(hue, 1.0, cb*bgb*0.7)
						bb.SetPen(wx.Pen(wx.Colour(int(r*255),int(g*255),int(b*255)), 1, wx.SOLID))
					ofs = 0
					bb.DrawRectangle(0, 0, psize-1, SEQTRACKSIZE-1)
					bb.DrawLabel(name, wx.Rect(2, 0, psize-3, SEQTRACKSIZE), wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL)
				bbw,bbh = bb.GetSize()
				x = SEQLEFTMARGIN + (((pos - self.startseqtime) * SEQROWSIZE) / self.step)
				if ((x+bbw) >= SEQLEFTMARGIN) and (x < w):
					ofs = max(SEQLEFTMARGIN - x,0)
					dc.Blit(x+ofs, y+1, bbw-ofs, bbh, bb, ofs, 0)
					if intrack and (pos >= selstart[1]) and (pos <= selend[1]):
						dc.SetBrush(invbrush)
						dc.SetPen(wx.TRANSPARENT_PEN)
						dc.SetLogicalFunction(wx.XOR)
						dc.DrawRectangle(x+ofs, y+1, bbw-ofs, bbh)
						#~ dc.SetLogicalFunction(wx.COPY)
			dc.SetLogicalFunction(wx.COPY)
			y += SEQTRACKSIZE			
			dc.SetPen(vlinepen)
			dc.DrawLine(SEQLEFTMARGIN, y, w, y)
		dc.SetPen(pen)
		x = SEQLEFTMARGIN-1
		dc.DrawLine(x, 0, x, h)
		se = player.get_song_end()
		x,y = self.track_row_to_pos((0,se))
		if (x >= SEQLEFTMARGIN):
			dc.SetPen(pen)
			dc.DrawLine(x-1, 0, x-1, h)
		dc.SetPen(loop_pen)
		lb,le = player.get_song_loop()
		x,y = self.track_row_to_pos((0,lb))
		if (x >= SEQLEFTMARGIN):
			dc.DrawLine(x-1, 0, x-1, h)
		x,y = self.track_row_to_pos((0,le))
		if (x >= SEQLEFTMARGIN):
			dc.DrawLine(x-1, 0, x-1, h)
		#print "%.2fms" % ((time.time() - st)*1000)

__all__ = [
'PatternNotFoundException',
'PluginDialog',
'SequencerPanel',
'SequencerView',
]

if __name__ == '__main__':
	import sys, utils
	from main import run
	sys.argv.append(utils.filepath('demosongs/paniq-knark.ccm'))
	run(sys.argv)
