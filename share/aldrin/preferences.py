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
Contains panels and dialogs related to application preferences.
"""

import sys, os
from wximport import wx
import zzub
import extman
import wx.html
import webbrowser

from utils import prepstr, buffersize_to_latency, filepath
import utils
import config

samplerates = [96000,48000,44100]
buffersizes = [32768,16384,8192,4096,2048,1024,512,256,128,64,32,16]

class CancelException(Exception):
	"""
	Is being thrown when the user hits cancel in a sequence of
	modal UI dialogs.
	"""

class DriverPanel(wx.Panel):
	"""
	Panel which allows to see and change audio driver settings.
	"""
	def __init__(self, *args, **kwds):
		"""
		Initializing.
		"""
		wx.Panel.__init__(self, *args, **kwds)
		#~ self.cbinput = wx.Choice(self, -1)
		self.cboutput = wx.Choice(self, -1)
		cbw,cbh = self.cboutput.GetMinSize()
		cbw = max(cbw,200)
		self.cboutput.SetMinSize((cbw,cbh))
		self.cbsamplerate = wx.Choice(self, -1)
		self.cbsamplerate.SetMinSize((cbw,cbh))
		self.cblatency = wx.Choice(self, -1)
		self.cblatency.SetMinSize((cbw,cbh))
		grid = wx.FlexGridSizer(3,2,5,5)
		sizer1 = wx.StaticBoxSizer(wx.StaticBox(self, -1, "Audio Output"), wx.VERTICAL)
		grid.Add(wx.StaticText(self, -1, "Driver:"),0, wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL | wx.LEFT, 5)
		grid.Add(self.cboutput, 1 , wx.EXPAND | wx.ALIGN_CENTER_VERTICAL | wx.RIGHT, 5)
		grid.Add(wx.StaticText(self, -1, "Samplerate:"),0, wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL | wx.LEFT, 5)
		grid.Add(self.cbsamplerate, 1 , wx.EXPAND | wx.ALIGN_CENTER_VERTICAL | wx.RIGHT, 5)
		grid.Add(wx.StaticText(self, -1, "Latency:"),0, wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL | wx.LEFT, 5)
		grid.Add(self.cblatency, 1 , wx.EXPAND | wx.ALIGN_CENTER_VERTICAL | wx.RIGHT, 5)
		sizer1.Add(grid, 0, wx.EXPAND|wx.ALL, 5)
		inputname, outputname, samplerate, buffersize = config.get_config().get_audiodriver_config()
		if not outputname:
			outputname = player.audiodriver_get_name(-1)
		for i in range(player.audiodriver_get_count()):
			name = prepstr(player.audiodriver_get_name(i))
			self.cboutput.Append(name)
			if player.audiodriver_get_name(i) == outputname:
				self.cboutput.SetSelection(i)
		for sr in samplerates:
			self.cbsamplerate.Append("%iHz" % sr)
		self.cbsamplerate.SetSelection(samplerates.index(samplerate))
		for bs in buffersizes:
			self.cblatency.Append("%.1fms" % buffersize_to_latency(bs, 44100))
		self.cblatency.SetSelection(buffersizes.index(buffersize))
		vsizer = wx.BoxSizer(wx.VERTICAL)
		vsizer.Add(sizer1, 0, wx.EXPAND|wx.ALL, 5)
		self.SetAutoLayout(True)
		self.SetSizerAndFit(vsizer)
		self.Layout()
		
	def apply(self):
		"""
		Validates user input and reinitializes the driver with current
		settings. If the reinitialization fails, the user is being
		informed and asked to change the settings.
		"""
		sr = self.cbsamplerate.GetSelection()
		if sr == -1:
			wx.MessageDialog(self, message="You did not select a valid sample rate.", caption = "Aldrin", style = wx.ICON_ERROR|wx.OK|wx.CENTER).ShowModal()
			raise CancelException
		sr = samplerates[sr]
		bs = self.cblatency.GetSelection()
		if bs == -1:
			wx.MessageDialog(self, message="You did not select a valid buffer size.", caption = "Aldrin", style = wx.ICON_ERROR|wx.OK|wx.CENTER).ShowModal()
			raise CancelException
		bs = buffersizes[bs]
		#~ i = self.cboutput.GetSelection()
		#~ if i == -1:
			#~ wx.MessageDialog(self, message="You did not select a valid output device.", caption = "Aldrin", style = wx.ICON_ERROR|wx.OK|wx.CENTER).ShowModal()
			#~ raise CancelException
		#~ iname = player.audiodriver_get_name(i)
		o = self.cboutput.GetSelection()
		if o == -1:
			wx.MessageDialog(self, message="You did not select a valid input device.", caption = "Aldrin", style = wx.ICON_ERROR|wx.OK|wx.CENTER).ShowModal()
			raise CancelException
		iname = ""
		oname = player.audiodriver_get_name(o)
		inputname, outputname, samplerate, buffersize = config.get_config().get_audiodriver_config()
		if (oname != outputname) or (samplerate != sr) or (bs != buffersize):
			config.get_config().set_audiodriver_config(iname, oname, sr, bs) # write back
			import driver
			try:
				driver.get_audiodriver().init()
			except driver.AudioInitException:
				import traceback
				traceback.print_exc()
				wx.MessageDialog(self, message="There was an error initializing the audio driver. Change settings and try again.", caption = "Aldrin", style = wx.ICON_ERROR|wx.OK|wx.CENTER).ShowModal()
				raise CancelException

class WavetablePanel(wx.Panel):
	"""
	Panel which allows to see and change paths to sample libraries.
	"""
	def __init__(self, *args, **kwds):
		wx.Panel.__init__(self, *args, **kwds)
		sizer = wx.BoxSizer(wx.HORIZONTAL)
		vsizer = wx.StaticBoxSizer(wx.StaticBox(self, -1, "Sound Folders"), wx.VERTICAL)
		#vsizer.Add(wx.StaticText(self, -1, "Sound Folders:"), 0, wx.ALIGN_LEFT|wx.ALL, 5)
		self.pathlist = wx.ListBox(self, -1, style=wx.LB_SINGLE|wx.LB_HSCROLL)
		vsizer.Add(self.pathlist, 1, wx.EXPAND|wx.LEFT|wx.BOTTOM|wx.RIGHT, 5)
		self.btnadd = wx.Button(self, wx.ID_ADD, "A&dd..")
		self.btnremove = wx.Button(self, wx.ID_REMOVE, "&Remove")
		hsizer = wx.BoxSizer(wx.HORIZONTAL)
		hsizer.Add(self.btnadd, 0, wx.ALIGN_LEFT, 0)
		hsizer.Add(self.btnremove, 0, wx.ALIGN_LEFT|wx.LEFT, 5)
		vsizer.Add(hsizer, 0, wx.LEFT|wx.BOTTOM|wx.RIGHT, 5)
		fssizer = wx.StaticBoxSizer(wx.StaticBox(self, -1, "freesound"), wx.VERTICAL)
		uname, passwd = config.get_config().get_credentials("Freesound")
		self.edusername = wx.TextCtrl(self, -1, uname)
		self.edpassword = wx.TextCtrl(self, -1, passwd, style=wx.TE_PASSWORD)
		logo = wx.StaticBitmap(self, -1, wx.Bitmap(filepath("res/fsbanner.png"), wx.BITMAP_TYPE_ANY))
		logo.SetBackgroundColour('#ffffff')
		fssizer.Add(logo, 0, wx.EXPAND)
		fsintro = wx.StaticText(self, -1, "The Freesound Project is a public library of Creative Commons licensed sounds and samples.")
		fsintro.Wrap(200)
		fssizer.Add(fsintro, 0, wx.EXPAND|wx.TOP|wx.LEFT|wx.RIGHT, 5)
		hsizer = wx.FlexGridSizer(2, 2)
		hsizer.AddGrowableCol(1)
		hsizer.Add(wx.StaticText(self, -1, "Username:"), 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL|wx.ALIGN_RIGHT, 5)
		hsizer.Add(self.edusername, 1, wx.EXPAND|wx.ALIGN_CENTER_VERTICAL|wx.TOP|wx.RIGHT|wx.BOTTOM, 5)
		hsizer.Add(wx.StaticText(self, -1, "Password:"), 0, wx.ALIGN_CENTER_VERTICAL|wx.ALL|wx.ALIGN_RIGHT, 5)
		hsizer.Add(self.edpassword, 1, wx.EXPAND|wx.ALIGN_CENTER_VERTICAL|wx.TOP|wx.RIGHT|wx.BOTTOM, 5)
		fssizer.Add(hsizer, 0, wx.EXPAND)
		fspwddesc = wx.StaticText(self, -1, "You will need an username and a password in order to search and download freesound samples.")
		fspwddesc.Wrap(200)
		fssizer.Add(fspwddesc, 0, wx.EXPAND|wx.BOTTOM|wx.LEFT|wx.RIGHT, 5)
		fsvisit = wx.Button(self, -1, "Visit website")
		fssizer.Add(fsvisit, 0, wx.EXPAND|wx.BOTTOM|wx.LEFT|wx.RIGHT, 5)
		sizer.Add(vsizer, 1, wx.EXPAND|wx.RIGHT|wx.TOP|wx.BOTTOM|wx.LEFT, 5)
		sizer.Add(fssizer, 1, wx.EXPAND|wx.TOP|wx.RIGHT|wx.BOTTOM, 5)
		self.SetAutoLayout(True)
		self.SetSizerAndFit(sizer)
		self.Layout()
		wx.EVT_BUTTON(self, self.btnadd.GetId(), self.on_add_path)
		wx.EVT_BUTTON(self, fsvisit.GetId(), lambda event: webbrowser.open("http://freesound.iua.upf.edu/"))
		wx.EVT_BUTTON(self, self.btnremove.GetId(), self.on_remove_path)
		for path in config.get_config().get_wavetable_paths():
			self.pathlist.Append(path)

	def apply(self):
		"""
		Stores list of paths back to config.
		"""
		olduname,oldpasswd = config.get_config().get_credentials("Freesound")
		uname = self.edusername.GetValue()
		passwd = self.edpassword.GetValue()
		if (uname != olduname) or (oldpasswd != passwd):
			import freesound
			fs = freesound.Freesound()
			try:
				if uname and passwd:
					fs.login(uname,passwd)
				config.get_config().set_credentials("Freesound",uname,passwd)
			except:
				import traceback
				traceback.print_exc()
				utils.error(self, "There was an error logging into freesound. Please make sure username and password are correct.")
				raise CancelException
		pathlist = []
		for i in range(self.pathlist.GetCount()):
			pathlist.append(self.pathlist.GetString(i))
		config.get_config().set_wavetable_paths(pathlist)
		
	def on_add_path(self, event):
		"""
		Handles 'Add' button click. Opens a directory selection dialog.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		dlg = wx.DirDialog(self, message="Select a folder that you want to access in wavetable window. Subfolders will be included automatically.")
		if dlg.ShowModal() == wx.ID_OK:
			if self.pathlist.FindString(dlg.GetPath()) == wx.NOT_FOUND:
				self.pathlist.Append(dlg.GetPath())
		dlg.Destroy()
		
	def on_remove_path(self, event):
		"""
		Handles 'Remove' button click. Removes the selected path from list.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		sel = self.pathlist.GetSelection()
		if sel != wx.NOT_FOUND:
			self.pathlist.Delete(sel)

class SelectControllerDialog(wx.Dialog):
	"""
	Dialog that records a controller from keyboard input.
	"""
	def __init__(self, rootwindow, *args, **kwds):
		self.rootwindow = rootwindow
		wx.Dialog.__init__(self, *args, **kwds)
		vsizer = wx.BoxSizer(wx.VERTICAL)
		hsizer = wx.BoxSizer(wx.HORIZONTAL)
		lsizer = wx.BoxSizer(wx.VERTICAL)
		lsizer.Add(wx.StaticText(self, -1, "Move a controller to select it"), 0, wx.ALIGN_LEFT|wx.BOTTOM, 10)
		self.controllerlabel = wx.StaticText(self, -1, "Controller:    ")
		self.channellabel = wx.StaticText(self, -1, "Channel:    ")
		self.valuelabel = wx.StaticText(self, -1, "Value:    ")
		lsizer.Add(self.controllerlabel, 0, wx.ALIGN_LEFT|wx.BOTTOM, 5)
		lsizer.Add(self.channellabel, 0, wx.ALIGN_LEFT|wx.BOTTOM, 5)
		lsizer.Add(self.valuelabel, 0, wx.ALIGN_LEFT, 0)
		rsizer = wx.BoxSizer(wx.VERTICAL)
		self.btnok = wx.Button(self, wx.ID_OK, "OK")
		self.btncancel = wx.Button(self, wx.ID_CANCEL, "Cancel")
		rsizer.Add(self.btnok, 0, wx.ALIGN_LEFT|wx.BOTTOM, 5)
		rsizer.Add(self.btncancel, 0, wx.ALIGN_LEFT, 0)
		hsizer.Add(lsizer, 0, wx.LEFT|wx.RIGHT, 5)
		hsizer.Add(rsizer, 0, wx.RIGHT, 5)
		vsizer.Add(hsizer, 0, wx.TOP|wx.BOTTOM, 5)
		hsizer = wx.BoxSizer(wx.HORIZONTAL)
		self.namelabel = wx.StaticText(self, -1, "Name:")
		self.editname = wx.TextCtrl(self, -1)
		hsizer.Add(self.namelabel, 0, wx.ALIGN_CENTER_VERTICAL|wx.LEFT|wx.RIGHT, 5)
		hsizer.Add(self.editname, 1, wx.EXPAND|wx.RIGHT, 5)
		vsizer.Add(hsizer, 1, wx.EXPAND|wx.BOTTOM, 5)
		self.target = None
		self.name = ''
		self.SetAutoLayout(True)
		self.SetSizerAndFit(vsizer)
		self.Layout()
		wx.EVT_CLOSE(self, self.on_close)
		wx.EVT_TEXT(self, self.editname.GetId(), self.on_editname_text)
		self.rootwindow.event_handlers.append(self.on_player_callback)
		self.update()
		
	def on_editname_text(self, event):
		"""
		Handler for name edit field input.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		self.name = event.GetString()
		self.update()
		
	def update(self):
		"""
		Decides whether the user can click OK or not. A controller value must
		be recorded and a name must have been entered.
		"""
		if self.target and self.name:
			self.btnok.Enable(True)
		else:
			self.btnok.Enable(False)
		
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
		if data.type == zzub.zzub_event_type_midi_control:
			ctrl = getattr(data,'').midi_message
			cmd = ctrl.status >> 4
			if cmd == 0xb:
				channel = ctrl.status & 0xf
				controller = ctrl.data1
				value = ctrl.data2
				self.controllerlabel.SetLabel(prepstr("Controller: %i" % controller))
				self.channellabel.SetLabel(prepstr("Channel: %i" % channel))
				self.valuelabel.SetLabel(prepstr("Value: %i" % value))
				self.target = channel,controller
				self.update()
		
	def on_close(self, event):
		"""
		Called when the dialog is closed.
		
		@param event: Close Event.
		@type event: wx.CloseEvent
		"""
		self.rootwindow.event_handlers.remove(self.on_player_callback)
		event.Skip()

class ControllerPanel(wx.Panel):
	"""
	Panel which allows to set up midi controller mappings.
	"""
	def __init__(self, rootwindow, *args, **kwds):
		self.rootwindow = rootwindow
		self.sort_column = 0
		wx.Panel.__init__(self, *args, **kwds)
		vsizer = wx.BoxSizer(wx.VERTICAL)
		sizer1 = wx.StaticBoxSizer(wx.StaticBox(self, -1, "Controllers"), wx.VERTICAL)
		self.controllers = wx.ListCtrl(self, -1, style=wx.SUNKEN_BORDER | wx.LC_REPORT)
		sizer1.Add(self.controllers, 1, wx.EXPAND|wx.ALL, 5)
		self.btnadd = wx.Button(self, wx.ID_ADD, "A&dd..")
		self.btnremove = wx.Button(self, wx.ID_REMOVE, "&Remove")
		hsizer = wx.BoxSizer(wx.HORIZONTAL)
		hsizer.Add(self.btnadd, 0, wx.ALIGN_LEFT, 0)
		hsizer.Add(self.btnremove, 0, wx.ALIGN_LEFT|wx.LEFT, 5)
		sizer1.Add(hsizer, 0, wx.LEFT|wx.BOTTOM|wx.RIGHT, 5)
		vsizer.Add(sizer1, 1, wx.EXPAND|wx.ALL, 5)
		self.SetAutoLayout(True)
		self.SetSizerAndFit(vsizer)
		self.Layout()
		wx.EVT_BUTTON(self, self.btnadd.GetId(), self.on_add_controller)
		wx.EVT_BUTTON(self, self.btnremove.GetId(), self.on_remove_controller)
		wx.EVT_LIST_COL_CLICK(self, self.controllers.GetId(), self.on_controllers_column_click)
		self.controllers.InsertColumn(0, "Name")
		self.controllers.InsertColumn(1, "Channel")
		self.controllers.InsertColumn(2, "Controller")
		self.update_controllers()		
		
	def make_key(self, data):
		"""
		Returns a new numeric key for a python object to be used as item data for list controls.
		"""
		self.keys.append(data)
		return len(self.keys)-1
		
	def on_controllers_column_click(self, event):
		"""
		Handles clicks on the controller list column. Sorts the list.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		self.sort_column = event.m_col
		self.sort_list()
		
	def sort_list(self):
		self.controllers.SortItems(self.sort_cmp_func)
	
	def sort_cmp_func(self, a, b):
		a = self.keys[a][self.sort_column]
		b = self.keys[b][self.sort_column]
		if type(a) in (str,unicode):
			return cmp(a.lower(),b.lower())
		return cmp(a,b)
		
	def update_controllers(self):
		"""
		Updates the controller list.
		"""
		self.controllers.DeleteAllItems()
		self.keys = []
		i = 0
		for name,channel,ctrlid in config.get_config().get_midi_controllers():			
			self.controllers.InsertStringItem(i, name)
			self.controllers.SetStringItem(i, 1, "%i" % channel)
			self.controllers.SetStringItem(i, 2, "%i" % ctrlid)
			self.controllers.SetItemData(i, self.make_key((name,channel,ctrlid)))
			i += 1
		self.sort_list()
		
	def on_add_controller(self, event):
		"""
		Handles 'Add' button click. Opens a popup that records controller events.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		dlg = SelectControllerDialog(self.rootwindow, self, -1)
		if dlg.ShowModal() == wx.ID_OK:
			i = self.controllers.GetItemCount()
			self.controllers.InsertStringItem(i, dlg.name)
			channel,ctrlid = dlg.target
			self.controllers.SetStringItem(i, 1, "%i" % channel)
			self.controllers.SetStringItem(i, 2, "%i" % ctrlid)
			self.controllers.SetItemData(i, self.make_key((dlg.name,channel,ctrlid)))
		dlg.Close()
		dlg.Destroy()
		self.sort_list()
		
	def on_remove_controller(self, event):
		"""
		Handles 'Remove' button click. Removes the selected controller from list.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		item = -1
		while True:
			item = self.controllers.GetNextItem(item, wx.LIST_NEXT_ALL, wx.LIST_STATE_SELECTED)
			if item == -1:
				break
			self.controllers.DeleteItem(item)
		
	def apply(self):
		"""
		Validates user input and reinitializes the driver with current
		settings. If the reinitialization fails, the user is being
		informed and asked to change the settings.
		"""
		ctrllist = []
		item = -1
		while True:
			item = self.controllers.GetNextItem(item, wx.LIST_NEXT_ALL, wx.LIST_STATE_DONTCARE)
			if item == -1:
				break
			name = self.controllers.GetItem(item,0).GetText()
			channel = int(self.controllers.GetItem(item,1).GetText())
			ctrlid = int(self.controllers.GetItem(item,2).GetText())
			ctrllist.append((name,channel,ctrlid))
		config.get_config().set_midi_controllers(ctrllist)

class MidiPanel(wx.Panel):
	"""
	Panel which allows to see and change a list of used MIDI output devices.
	"""
	def __init__(self, *args, **kwds):
		wx.Panel.__init__(self, *args, **kwds)
		vsizer = wx.BoxSizer(wx.VERTICAL)
		vsizer.Add(wx.StaticText(self, -1, "Please tick the devices which you want to use."), 0, wx.ALL, 5)
		sizer1 = wx.StaticBoxSizer(wx.StaticBox(self, -1, "MIDI Input"), wx.VERTICAL)
		self.idevicelist = wx.CheckListBox(self, -1)
		inputlist = config.get_config().get_mididriver_inputs()
		for i in range(player.mididriver_get_count()):
			if player.mididriver_is_input(i):
				name = prepstr(player.mididriver_get_name(i))
				idx = self.idevicelist.Append(name)
				if name in inputlist:
					self.idevicelist.Check(idx, True)
		sizer1.Add(self.idevicelist, 1, wx.EXPAND|wx.ALL, 5)
		sizer2 = wx.StaticBoxSizer(wx.StaticBox(self, -1, "MIDI Output"), wx.VERTICAL)
		self.odevicelist = wx.CheckListBox(self, -1)
		outputlist = config.get_config().get_mididriver_outputs()
		for i in range(player.mididriver_get_count()):
			if player.mididriver_is_output(i):
				name = prepstr(player.mididriver_get_name(i))
				idx = self.odevicelist.Append(name)
				if name in outputlist:
					self.odevicelist.Check(idx, True)
		sizer2.Add(self.odevicelist, 1, wx.EXPAND|wx.ALL, 5)
		vsizer.Add(sizer1, 1, wx.EXPAND|wx.LEFT|wx.BOTTOM|wx.RIGHT, 5)
		vsizer.Add(sizer2, 1, wx.EXPAND|wx.LEFT|wx.BOTTOM|wx.RIGHT, 5)
		self.SetAutoLayout(True)
		self.SetSizerAndFit(vsizer)
		self.Layout()

	def apply(self):
		"""
		Adds the currently selected drivers to the list.
		"""
		inputlist = []
		for i in range(self.idevicelist.GetCount()):
			if self.idevicelist.IsChecked(i):
				inputlist.append(self.idevicelist.GetString(i))
		config.get_config().set_mididriver_inputs(inputlist)
		outputlist = []
		for i in range(self.odevicelist.GetCount()):
			if self.odevicelist.IsChecked(i):
				outputlist.append(self.odevicelist.GetString(i))
		config.get_config().set_mididriver_outputs(outputlist)

class KeyboardPanel(wx.Panel):
	"""
	Panel which allows to see and change the current keyboard configuration.
	"""
	
	KEYMAPS = [
		('en', 'English (QWERTY)'),
		('de', 'Deutsch (QWERTZ)'),
		('dv', 'Dvorak (\',.PYF)')
	]
	
	def __init__(self, *args, **kwds):
		wx.Panel.__init__(self, *args, **kwds)
		vsizer = wx.BoxSizer(wx.VERTICAL)
		hsizer = wx.BoxSizer(wx.HORIZONTAL)
		hsizer.Add(wx.StaticText(self, -1, "Keyboard Map:"), 0, wx.ALIGN_CENTER_VERTICAL | wx.RIGHT, 5)
		self.cblanguage = wx.Choice(self, -1)
		sel = 0
		lang = config.get_config().get_keymap_language()
		index = 0
		for kmid, name in self.KEYMAPS:
			self.cblanguage.Append(name)
			if lang == kmid:
				sel = index
			index += 1
		hsizer.Add(self.cblanguage, 1, wx.EXPAND)
		vsizer.Add(hsizer, 0, wx.EXPAND|wx.ALL, 5)
		self.SetAutoLayout(True)
		self.SetSizerAndFit(vsizer)
		self.Layout()
		self.cblanguage.SetSelection(sel)

	def apply(self):
		"""
		applies the keymap.
		"""
		config.get_config().set_keymap_language(self.KEYMAPS[self.cblanguage.GetSelection()][0])

class ExtensionsPanel(wx.Panel):
	"""
	Panel which allows to enable and disable extensions.
	"""
	
	def __init__(self, *args, **kwds):
		wx.Panel.__init__(self, *args, **kwds)
		vsizer = wx.BoxSizer(wx.VERTICAL)
		#~ self.extlist = ExtensionListBox(self, -1, style=wx.SUNKEN_BORDER)
		self.extman = extman.get_extension_manager()
		self.cfg = config.get_config()
		self.extlist = wx.CheckListBox(self, -1)
		exts = config.get_config().get_enabled_extensions()
		for ext in self.extman.extensions:
			name = prepstr(ext.name)
			idx = self.extlist.Append(name)
			if ext.uri in exts:
				self.extlist.Check(idx, True)
		self.htmldesc = wx.html.HtmlWindow(self, -1)
		hsizer = wx.BoxSizer(wx.HORIZONTAL)		
		hsizer.Add(self.extlist, 1, wx.EXPAND|wx.RIGHT, 5)
		hsizer.Add(self.htmldesc, 1, wx.EXPAND)
		vsizer.Add(hsizer, 1, wx.EXPAND|wx.ALL, 5)
		vsizer.Add(wx.StaticText(self, -1, "Click OK and restart Aldrin to apply changes."), 0, wx.LEFT|wx.BOTTOM|wx.RIGHT, 5)
		self.SetAutoLayout(True)
		self.SetSizerAndFit(vsizer)
		self.Layout()
		wx.EVT_LISTBOX(self, self.extlist.GetId(), self.on_extlist_select)
		
	def on_extlist_select(self, event):
		"""
		Called when an extension is selected. Updates the html description.
		
		@param event: Event.
		@type event: wx.Event
		"""
		ext = self.extman.extensions[event.GetSelection()]
		out = """<html><head></head>
		<style type="text/css">
		body {
			font-size: 8pt;
		}
		</style>
		<body><font size="-4">"""
		out += '<b>%s</b><br><br>' % ext.name
		out += '%s<br><br>' % ext.description
		out += 'Author: %s' % ext.author
		out += "</font></body></html>"
		self.htmldesc.SetPage(out)
		
	def apply(self):
		"""
		Updates the config object with the currently selected extensions.
		"""
		exts = []
		for i in range(len(self.extman.extensions)):
			ext = self.extman.extensions[i]
			if self.extlist.IsChecked(i):
				exts.append(ext.uri)
		config.get_config().set_enabled_extensions(exts)
		

class PreferencesDialog(wx.Dialog):
	"""
	This Dialog aggregates the different panels and allows
	the user to switch between them using a tab control.
	"""
	def __init__(self, rootwindow, *args, **kwds):
		kwds['style'] = wx.RESIZE_BORDER | wx.DEFAULT_DIALOG_STYLE
		wx.Dialog.__init__(self, *args, **kwds)
		self.SetTitle("Preferences")
		self.SetMinSize((500,400))
		nb = wx.Notebook(self, -1, style = wx.NB_TOP)
		self.driverpanel = DriverPanel(nb,-1)
		self.wavetablepanel = WavetablePanel(nb,-1)
		self.midipanel = MidiPanel(nb,-1)
		self.controllerpanel = ControllerPanel(rootwindow,nb,-1)
		self.keyboardpanel = KeyboardPanel(nb,-1)
		self.extensionspanel = ExtensionsPanel(nb,-1)
		nb.InsertPage(0, self.driverpanel, "Audio")
		nb.InsertPage(1, self.midipanel, "MIDI")
		nb.InsertPage(2, self.controllerpanel, "Controllers")
		nb.InsertPage(3, self.keyboardpanel, "Keyboard")
		nb.InsertPage(4, self.wavetablepanel, "Sound Library")
		nb.InsertPage(5, self.extensionspanel, "Extensions")
		btnok = wx.Button(self, wx.ID_OK, "OK")
		btncancel = wx.Button(self, wx.ID_CANCEL, "Cancel")
		btnapply = wx.Button(self, wx.ID_APPLY, "Apply")
		buttonsizer = wx.BoxSizer(wx.HORIZONTAL)
		buttonsizer.Add(btnok, 0, wx.EXPAND)
		buttonsizer.Add(btncancel, 0, wx.EXPAND|wx.LEFT, 5)
		buttonsizer.Add(btnapply, 0, wx.EXPAND|wx.LEFT, 5)		
		sizer = wx.BoxSizer(wx.VERTICAL)
		sizer.Add(nb, 1, wx.EXPAND|wx.ALL, 5)
		sizer.Add(buttonsizer, 0, wx.ALIGN_RIGHT|wx.LEFT|wx.RIGHT|wx.BOTTOM, 5)
		self.SetAutoLayout(True)
		self.SetSizer(sizer)
		self.Layout()
		self.Centre()
		self.Fit()
		wx.EVT_BUTTON(self, wx.ID_OK, self.on_ok)
		wx.EVT_BUTTON(self, wx.ID_APPLY, self.on_apply)
		
	def apply(self):
		"""
		Apply changes in settings without closing the dialog.
		"""
		self.wavetablepanel.apply()
		self.extensionspanel.apply()
		self.keyboardpanel.apply()
		self.driverpanel.apply()
		self.controllerpanel.apply()
		self.midipanel.apply()
		
	def on_apply(self, event):
		"""
		Event handler for apply button.
		
		@param event: Event forwarded by wxPython.
		@type event: wx.Event
		"""
		try:
			self.apply()		
		except CancelException:
			pass

	def on_ok(self, event):
		"""
		Event handler for OK button. Calls apply
		and then closes the dialog.
		
		@param event: Event forwarded by wxPython.
		@type event: wx.Event
		"""
		try:
			self.apply()		
			event.Skip()
		except CancelException:
			pass

def show_preferences(rootwindow, parent):
	"""
	Shows the {PreferencesDialog}.
	
	@param rootwindow: The root window which receives zzub callbacks.
	@type rootwindow: wx.Frame
	@param parent: Parent window.
	@type parent: wx.Window
	"""
	dlg = PreferencesDialog(rootwindow, parent, -1)
	dlg.ShowModal()
	dlg.Destroy()

__all__ = [
'CancelException',
'DriverPanel',
'WavetablePanel',
'MidiInPanel',
'MidiOutPanel',
'KeyboardPanel',
'ExtensionsPanel',
'PreferencesDialog',
'show_preferences',
]

if __name__ == '__main__':
	import sys, utils
	from main import run
	#sys.argv.append(utils.filepath('demosongs/test.bmx'))
	run(sys.argv)
