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
Provides dialogs and controls to render the plugin view/router and its associated components.
"""

from wximport import wx
from canvas import Canvas, BitmapBuffer
from utils import prepstr, filepath, db2linear, linear2db, is_debug, filenameify
import config
import zzub
import sys,os
import indexer
import fnmatch
import ctypes
import time
import Queue
from preset import PresetCollection, Preset

PLUGINWIDTH = 100
PLUGINHEIGHT = 50
LEDWIDTH,LEDHEIGHT = 8,PLUGINHEIGHT-4 # size of LED
LEDOFSX,LEDOFSY = 2,2 # offset of LED

VOLBARWIDTH = 32
VOLBARHEIGHT = 128
VOLKNOBHEIGHT = 16

AREA_ANY = 0
AREA_PANNING = 1
AREA_LED = 2

class OscillatorView(Canvas):
	"""
	Oscillator viewer.
	
	Visualizes a pcm wave float buffer.
	"""
	
	def __init__(self, plugin, *args, **kwds):
		"""
		Initialization.
		"""
		self.plugin = plugin
		self.x = 0
		self.reset_osc()
		self.lastpos = 0
		self.w = 0
		self.sb = Queue.Queue()
		Canvas.__init__(self, *args, **kwds)
		self.pp = self.plugin.add_post_process(self.mix_callback, 0)
		
	def reset_osc(self):
		pass
		#~ self.sr = 0
		#~ self.l1 = 9999
		#~ self.l2 = -9999
		#~ self.r1 = 9999
		#~ self.r2 = -9999
		
	def mix_callback(self, left, right, size, tag):
		"""
		called by the plugin
		"""
		i = 0
		while (i < size) and (self.w > 0):
			self.sb.put(left[i])
			self.sb.put(right[i])
			i += 1
			self.w -= 1
				
	def destroy(self):
		"""
		shut down and remove from post process queue.
		"""
		self.plugin.remove_post_process(self.pp)
		
	def DrawBuffer(self):
		"""
		Overriding a L{Canvas} method that paints onto an offscreen buffer.
		Draws the oscillator graphics.
		"""
		w,h = self.GetClientSize()
		if self.w == 0:
			self.w = w
		if (self.sb.qsize() < w):
			return
		dc = self.buffer
		cfg = config.get_config()
		bgbrush = cfg.get_brush("SA Freq BG")
		pen = cfg.get_pen("SA Freq Line")
		dc.SetBackground(bgbrush)
		dc.Clear()
		dc.SetPen(pen)
		h2 = h/2
		h4 = h/4
		h34 = h4*3
		x = 0
		while x < w:
			#~ if self.sb.empty():
				#~ break
			l = self.sb.get()
			r = self.sb.get()
			x1 = x
			yl1 = -l * h4 + h4
			yr1 = -r * h4 + h34
			dc.DrawLine(x1, yl1, x1, yl1+1)
			dc.DrawLine(x1, yr1, x1, yr1+1)
			x += 1

class SignalAnalysisDialog(wx.Dialog):
	"""
	Displays a visualization of plugin traffic.
	"""
	def __init__(self, plugin, *args, **kwds):
		"""
		Initializer.
		
		@param plugin: Plugin object.
		@type plugin: wx.Plugin
		"""
		self.plugin = plugin
		kwds['style'] = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER
		wx.Dialog.__init__(self, *args, **kwds)
		self.SetMinSize((300,200))
		vsizer = wx.BoxSizer(wx.VERTICAL)
		self.oscview = OscillatorView(self.plugin, self, -1)
		vsizer.Add(self.oscview, 1, wx.EXPAND)
		self.SetAutoLayout(True)
		self.SetSizerAndFit(vsizer)
		self.Layout()
		self.Center()
		self.timer = wx.Timer(self, -1)
		self.timer.Start(1000/50)
		wx.EVT_TIMER(self, self.timer.GetId(), self.on_timer)
		wx.EVT_CLOSE(self, self.on_close)

	def on_close(self, event):
		self.oscview.destroy()
		event.Skip()

	def on_timer(self, event):
		"""
		Event handler for the timer.
		"""
		self.oscview.ReDraw()
		
class AttributesDialog(wx.Dialog):
	"""
	Displays plugin atttributes and allows to edit them.
	"""
	
	def __init__(self, plugin, *args, **kwds):
		"""
		Initializer.
		
		@param plugin: Plugin object.
		@type plugin: wx.Plugin
		"""
		wx.Dialog.__init__(self, *args, **kwds)
		self.plugin = plugin
		self.pluginloader = plugin.get_pluginloader()
		self.SetMinSize((300,200))
		vsizer = wx.BoxSizer(wx.VERTICAL)
		self.attriblist = wx.ListCtrl(self, -1, style=wx.SUNKEN_BORDER | wx.LC_REPORT | wx.LC_SINGLE_SEL)
		vsizer.Add(self.attriblist, 1, wx.EXPAND|wx.ALL, 5)
		hsizer = wx.BoxSizer(wx.HORIZONTAL)
		self.edvalue = wx.TextCtrl(self, -1)
		self.btnset = wx.Button(self, -1, "&Set")
		self.btnok = wx.Button(self, wx.ID_OK)
		self.btncancel = wx.Button(self, wx.ID_CANCEL)
		hsizer.Add(self.edvalue, 0, wx.ALIGN_CENTER_VERTICAL | wx.RIGHT, 5)
		hsizer.Add(self.btnset, 0, wx.ALIGN_CENTER_VERTICAL)
		hsizer.Add((0,0), 1)
		hsizer.Add(self.btnok, 0, wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL | wx.RIGHT, 5)
		hsizer.Add(self.btncancel, 0, wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL)
		vsizer.Add(hsizer, 0, wx.EXPAND|wx.LEFT|wx.RIGHT|wx.BOTTOM, 5)
		self.attriblist.InsertColumn(0, "Attribute", wx.LIST_AUTOSIZE)
		self.attriblist.InsertColumn(1, "Value", wx.LIST_AUTOSIZE)
		self.attriblist.InsertColumn(2, "Min", wx.LIST_AUTOSIZE)
		self.attriblist.InsertColumn(3, "Max", wx.LIST_AUTOSIZE)
		self.attriblist.InsertColumn(4, "Default", wx.LIST_AUTOSIZE)
		self.attribs = []
		for i in range(self.pluginloader.get_attribute_count()):
			attrib = self.pluginloader.get_attribute(i)
			self.attribs.append(self.plugin.get_attribute_value(i))
			self.attriblist.InsertStringItem(i, prepstr(attrib.get_name()))
			self.attriblist.SetStringItem(i, 1, "%i" % self.plugin.get_attribute_value(i), -1)
			self.attriblist.SetStringItem(i, 2, "%i" % attrib.get_value_min(), -1)
			self.attriblist.SetStringItem(i, 3, "%i" % attrib.get_value_max(), -1)
			self.attriblist.SetStringItem(i, 4, "%i" % attrib.get_value_default(), -1)
		wx.EVT_SIZE(self, self.on_size)
		self.SetSizerAndFit(vsizer)
		self.Layout()
		self.Center()
		wx.EVT_BUTTON(self, self.btnset.GetId(), self.on_set)
		wx.EVT_BUTTON(self, wx.ID_OK, self.on_ok)
		wx.EVT_LIST_ITEM_FOCUSED(self, self.attriblist.GetId(), self.on_attrib_item_focused)
		if self.attribs:
			self.attriblist.SetItemState(0, wx.LIST_STATE_FOCUSED|wx.LIST_STATE_SELECTED, wx.LIST_STATE_FOCUSED|wx.LIST_STATE_SELECTED)
			self.attriblist.SetFocus()
			self.on_attrib_item_focused(None)
			
	def get_focused_item(self):
		"""
		Returns the currently focused attribute index.
		
		@return: Index of the attribute currently selected.
		@rtype: int
		"""
		return self.attriblist.GetNextItem(-1, wx.LIST_NEXT_ALL, wx.LIST_STATE_FOCUSED)
		
	def on_attrib_item_focused(self, event):
		"""
		Called when an attribute item is being focused.
		
		@param event: Event.
		@type event: wx.Event
		"""
		v = self.attribs[self.get_focused_item()]
		self.edvalue.SetValue("%i" % v)
		
	def on_set(self, event):
		"""
		Called when the "set" button is being pressed.
		
		@param event: Event.
		@type event: wx.Event
		"""
		i = self.get_focused_item()
		attrib = self.pluginloader.get_attribute(i)
		try:
			v = int(self.edvalue.GetValue())
			assert v >= attrib.get_value_min()
			assert v <= attrib.get_value_max()
		except:
			return
		self.attribs[i] = v
		self.attriblist.SetStringItem(i, 1, "%i" % v, -1)
			
	def on_ok(self, event):
		"""
		Called when the "ok" button is being pressed.
		
		@param event: Event.
		@type event: wx.Event
		"""
		for i in range(len(self.attribs)):
			self.plugin.set_attribute_value(i, self.attribs[i])
		event.Skip()		
		
	def on_size(self, event):
		"""
		Called when the dialog is being resized.
		
		@param event: Event.
		@type event: wx.Event
		"""
		self.Layout()
		x,y,w,h = self.attriblist.GetClientRect()
		w -= 16
		self.attriblist.SetColumnWidth(0, w/3)
		self.attriblist.SetColumnWidth(1, w/6)
		self.attriblist.SetColumnWidth(2, w/6)
		self.attriblist.SetColumnWidth(3, w/6)
		self.attriblist.SetColumnWidth(4, w/6)


class PresetDialog(wx.Dialog):
	"""
	Displays presets and allows to edit them.
	"""
	
	def __init__(self, plugin, *args, **kwds):
		"""
		Initializer.
		
		@param pluginloader: Pluginloader object.
		@type pluginloader: wx.Pluginloader
		"""
		self.parent = args[0]
		wx.Dialog.__init__(self, *args, **kwds)
		self.plugin = plugin
		self.pluginloader = plugin.get_pluginloader()
		sizer = wx.FlexGridSizer(2,2,5,5)
		self.presetlist = wx.ListBox(self, -1, style=wx.SUNKEN_BORDER)
		self.presetlist.SetMinSize((100,150))
		self.commentbox = wx.TextCtrl(self, -1, style=wx.SUNKEN_BORDER | wx.TE_MULTILINE | wx.TE_RICH2 | wx.TE_BESTWRAP)
		self.edname = wx.TextCtrl(self, -1)
		self.btnadd = wx.Button(self, -1, "&Add")
		self.btndelete = wx.Button(self, -1, "&Delete")
		self.btnimport = wx.Button(self, -1, "&Import...")
		hsizer = wx.BoxSizer(wx.HORIZONTAL)
		hsizer.Add(self.btnadd, 0, wx.RIGHT, 5)
		hsizer.Add(self.btndelete, 0, wx.RIGHT, 5)
		hsizer.Add(self.btnimport, 0)
		sizer.Add(self.presetlist, 0, wx.LEFT|wx.TOP, 5)
		sizer.Add(self.commentbox, 0, wx.EXPAND|wx.TOP|wx.RIGHT, 5)
		sizer.Add(self.edname, 0, wx.EXPAND|wx.ALIGN_CENTER_VERTICAL|wx.LEFT|wx.BOTTOM, 5)
		sizer.Add(hsizer, 0, wx.BOTTOM|wx.RIGHT, 5)
		self.SetAutoLayout(True)
		self.SetSizerAndFit(sizer)
		self.Layout()
		self.Center()
		wx.EVT_LISTBOX(self, self.presetlist.GetId(), self.on_presetlist_select)
		wx.EVT_LISTBOX_DCLICK(self, self.presetlist.GetId(), self.on_presetlist_dclick)
		wx.EVT_BUTTON(self, self.btnadd.GetId(), self.on_button_add)
		wx.EVT_BUTTON(self, self.btndelete.GetId(), self.on_button_delete)
		wx.EVT_BUTTON(self, self.btnimport.GetId(), self.on_button_import)		
		wx.EVT_TEXT(self, self.edname.GetId(), self.on_edit_name)
		self.presets = config.get_config().get_plugin_presets(self.pluginloader)
		self.update_presetlist()
		self.update_addbutton()
		
	def update_presetlist(self):
		"""
		Updates the preset list.
		"""
		self.presetlist.Clear()
		for preset in self.presets.presets:
			self.presetlist.Append(prepstr(preset.name))
		sel = self.presetlist.FindString(self.edname.GetValue())
		if sel != wx.NOT_FOUND:
			self.presetlist.SetSelection(sel)
			self.presetlist.SetFirstItem(sel)
		else:
			self.edname.SetValue('')
			self.update_addbutton()
			
	def update_addbutton(self):
		"""
		Updates the add button caption depending on whether
		the editbox name is in the preset list or not.
		"""
		if not self.edname.GetValue():
			self.btnadd.Enable(False)
		else:
			self.btnadd.Enable(True)
		if self.presetlist.FindString(self.edname.GetValue()) == wx.NOT_FOUND:
			self.btnadd.SetLabel("&Add")
		else:
			self.btnadd.SetLabel("&Update")
			
	def on_edit_name(self, event):
		"""
		Handler for changes in the edit box.
		
		@param event: Event
		@type event: wx.Event
		"""
		self.update_addbutton()
			
	def on_presetlist_select(self, event):
		"""
		Handler for preset list selection changes.
		
		@param event: Event.
		@type event: wx.Event
		"""
		sel = self.presetlist.GetSelection()
		preset = self.presets.presets[sel]
		self.edname.SetValue(prepstr(preset.name))
		self.commentbox.SetValue(prepstr(preset.comment))
		self.update_addbutton()

	def on_presetlist_dclick(self, event):
		"""
		Handler for preset list double clicks.
		
		@param event: Event.
		@type event: wx.Event
		"""
		sel = self.presetlist.GetSelection()
		preset = self.presets.presets[sel]
		self.parent.apply_preset(preset)

	def on_button_add(self, event):
		"""
		Handler for clicks on the 'Add' button. Either
		adds or updates a preset from current plugin settings.
		
		@param event: Button event.
		@type event: wx.Event
		"""
		if self.presetlist.FindString(self.edname.GetValue()) == wx.NOT_FOUND:
			preset = Preset()
			self.presets.presets.append(preset)
		else:
			found = False
			for preset in self.presets.presets:
				if prepstr(preset.name) == self.edname.GetValue():
					found = True
					break
			assert found
		preset.name = self.edname.GetValue()
		preset.comment = self.commentbox.GetValue()
		preset.pickup(self.plugin)
		self.presets.sort()
		config.get_config().set_plugin_presets(self.pluginloader, self.presets)
		self.update_presetlist()
		self.update_addbutton()

	def on_button_delete(self, event):
		"""
		Handler for clicks on the 'Delete' button. Deletes
		the currently selected plugin from the list.
		
		@param event: Button event.
		@type event: wx.Event
		"""
		sel = self.presetlist.GetSelection()
		if sel == -1:
			return
		del self.presets.presets[sel]
		config.get_config().set_plugin_presets(self.pluginloader, self.presets)
		self.update_presetlist()
		self.update_addbutton()

	def on_button_import(self, event):
		"""
		Handler for clicks on the 'Import' button. Imports
		a list of presets.
		
		@param event: Button event.
		@type event: wx.Event
		"""

class ParameterDialog(wx.Frame):
	"""
	Displays parameter sliders for a plugin in a new Dialog.
	"""
	
	UNBIND_ALL = wx.NewId()
	CONTROLLER = wx.NewId()
	for i in range(256):
		wx.RegisterId(CONTROLLER+i)
	
	def __init__(self, rootwindow, plugin, *args, **kwds):
		"""
		Initializer.
		
		@param plugin: The plugin object for which to display parameters.
		@type plugin: zzub.Plugin
		"""
		self.rootwindow = rootwindow
		self.parent = args[0]
		kwds['style'] = wx.DEFAULT_FRAME_STYLE | wx.RESIZE_BORDER | wx.FRAME_TOOL_WINDOW
		wx.Frame.__init__(self, *args, **kwds)
		self.plugin = plugin
		self.parent.plugin_dialogs[self.plugin] = self
		name = prepstr(self.plugin.get_name())
		pl = self.plugin.get_pluginloader()
		classname = prepstr(pl.get_name())
		title = "%s - %s" % (name,classname)
		oc = self.plugin.get_output_channels()
		if oc  == 2:
			title += " (Stereo Output)"
		elif oc == 1:
			title += " (Mono Output)"
		self.SetTitle(title)
		self.presetbox = wx.Choice(self, -1)

		pw,ph = self.presetbox.GetBestSize()
		self.btnedit = wx.Button(self, -1, "&Edit...")
		w,h = self.btnedit.GetBestSize()
		btnsize = 60,ph
		self.btnedit.SetSize(btnsize)
		self.btncopy = wx.Button(self, -1, "&Copy")
		self.btncopy.SetSize(btnsize)
		self.btnrandom = wx.Button(self, -1, "&Random")
		self.btnrandom.SetSize(btnsize)
		self.btnhelp = wx.Button(self, -1, "&Help")
		self.btnhelp.SetSize(btnsize)
		menugroup = wx.BoxSizer(wx.HORIZONTAL)
		menugroup.Add(self.presetbox, 1, wx.EXPAND|wx.LEFT|wx.TOP|wx.BOTTOM, 5)
		menugroup.Add(self.btnedit, 0, wx.ALIGN_CENTER_VERTICAL | wx.FIXED_MINSIZE|wx.LEFT|wx.TOP|wx.BOTTOM, 5)
		menugroup.Add(self.btncopy, 0, wx.ALIGN_CENTER_VERTICAL | wx.FIXED_MINSIZE|wx.LEFT|wx.TOP|wx.BOTTOM, 5)
		menugroup.Add(self.btnrandom, 0, wx.ALIGN_CENTER_VERTICAL | wx.FIXED_MINSIZE|wx.LEFT|wx.TOP|wx.BOTTOM, 5)
		menugroup.Add(self.btnhelp, 0, wx.ALIGN_CENTER_VERTICAL | wx.FIXED_MINSIZE|wx.ALL, 5)
		toplevelgroup = wx.BoxSizer(wx.VERTICAL)
		toplevelgroup.Add(menugroup, 0, wx.EXPAND|wx.ALIGN_TOP, 0)
		
		scrollwindow = wx.ScrolledWindow(self, -1, style=wx.SUNKEN_BORDER|wx.VSCROLL|wx.HSCROLL)

		self.pluginloader = pl

		self.update_presets()
		self.presetbox.SetSelection(0)
		wx.EVT_CHOICE(self, self.presetbox.GetId(), self.on_select_preset)

		rowgroup = wx.BoxSizer(wx.VERTICAL)
		
		self.id2pid = {}
		self.pid2ctrls = {}
		
		def add_slider(g,t,i):
			p = pl.get_parameter(g,i)
			if not (p.get_flags() & zzub.zzub_parameter_flag_state):
				return
			if g == 1:
				name = prepstr(p.get_name())
			else:
				name = "%i-%s" % (t,prepstr(p.get_name()))
			sliderid = wx.NewId()
			namelabel = wx.StaticText(scrollwindow, -1, name)
			w,h = namelabel.GetBestSize()
			namelabel.SetSize((100,h))
			slider = wx.Slider(scrollwindow, sliderid)
			#slider.SetMinSize((50,22))			
			slider.SetRange(p.get_value_min(),p.get_value_max())
			v = plugin.get_parameter_value(g,t,i)
			slider.SetValue(v)
			valuelabel = wx.StaticText(scrollwindow, -1, "")
			w,h = valuelabel.GetBestSize()
			valuelabel.SetSize((100,h))
			slidergroup = wx.BoxSizer(wx.HORIZONTAL)
			slidergroup.Add(namelabel, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_LEFT | wx.FIXED_MINSIZE, 0)	
			slidergroup.Add(slider, 1, wx.ALIGN_CENTER_VERTICAL | wx.EXPAND|wx.LEFT|wx.RIGHT, 5)	
			slidergroup.Add(valuelabel, 0, wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_RIGHT | wx.FIXED_MINSIZE, 0)	
			rowgroup.Add(slidergroup, 1, wx.EXPAND|wx.ALIGN_TOP|wx.LEFT|wx.RIGHT, 5)
			self.id2pid[sliderid] = (g,t,i)
			self.id2pid[namelabel.GetId()] = (g,t,i)
			self.id2pid[valuelabel.GetId()] = (g,t,i)
			self.pid2ctrls[(g,t,i)] = [namelabel,slider,valuelabel]
			wx.EVT_CONTEXT_MENU(slider, self.on_context_menu)
			wx.EVT_CONTEXT_MENU(namelabel, self.on_context_menu)
			wx.EVT_CONTEXT_MENU(valuelabel, self.on_context_menu)
			wx.EVT_MOUSEWHEEL(slider, self.on_mousewheel)
			wx.EVT_SCROLL(slider, self.on_scroll_changed)
			wx.EVT_KEY_DOWN(slider, self.on_key_down)
			self.update_valuelabel(g,t,i)
			
		for i in range(pl.get_parameter_count(1)): # globals
			add_slider(1,0,i)
		# tracks
		for t in range(plugin.get_track_count()):
			for i in range(pl.get_parameter_count(2)):
				add_slider(2,t,i)
				
		wx.EVT_BUTTON(self, self.btnedit.GetId(), self.on_button_edit)
		wx.EVT_BUTTON(self, self.btncopy.GetId(), self.on_button_copy)
		wx.EVT_BUTTON(self, self.btnrandom.GetId(), self.on_button_random)
		wx.EVT_BUTTON(self, self.btnhelp.GetId(), self.on_button_help)
		wx.EVT_MENU_RANGE(self, self.CONTROLLER, self.CONTROLLER+255, self.on_bind_controller)
		wx.EVT_MENU(self, self.UNBIND_ALL, self.on_unbind_all)
		wx.EVT_WINDOW_DESTROY(self, self.on_destroy)
		wx.EVT_CLOSE(self, self.on_close)	
		
		scrollwindow.SetAutoLayout(True)
		scrollwindow.SetSizer(rowgroup)
		#scrollwindow.SetVirtualSizeHints(16,16)
		scrollwindow.SetScrollRate(1,1)
		
		toplevelgroup.Add(scrollwindow, 1, wx.EXPAND|wx.ALIGN_TOP, 0)

		self.SetAutoLayout(True)
		self.SetSizerAndFit(toplevelgroup)
		self.Layout()

		cdx,cdy,cdw,cdh = wx.GetClientDisplayRect()

		scrollwindow.Layout()
		svx,svy = scrollwindow.GetVirtualSize()
		swx,swy = scrollwindow.GetClientSize()
		dwx,dwy = self.GetSize()
		dwy = min(dwy - swy + svy, int(cdh * 0.9))
		self.SetSize(wx.Size(dwx, dwy))
		
		self.rootwindow.event_handlers.append(self.on_callback)
		self.Center()
		
	def Destroy(self):
		self.rootwindow.event_handlers.remove(self.on_callback)
		del self.parent.plugin_dialogs[self.plugin]
		wx.Frame.Destroy(self)
		
	def on_unbind_all(self, event):
		"""
		Unbinds all controllers from the selected parameter.
		
		@param event: Event.
		@type event: wx.Event
		"""
		g,t,i = self.current_param
		player.remove_midimapping(self.plugin, g, t, i)

	def on_bind_controller(self, event):
		"""
		Handles clicks on controller names in the context menu. Associates
		a controller with a plugin parameter.
		
		@param event: Event.
		@type event: wx.Event
		"""
		g,t,i = self.current_param
		name,channel,ctrlid = config.get_config().get_midi_controllers()[event.GetId() - self.CONTROLLER]
		print player.add_midimapping(self.plugin, g, t, i, channel, ctrlid)
		
	def on_context_menu(self, event):
		"""
		Event handler for requests to show the context menu.
		
		@param event: event.
		@type event: wx.Event
		"""
		g,t,i = self.id2pid[event.GetId()]
		nl,s,vl = self.pid2ctrls[(g,t,i)]
		mx,my = self.ScreenToClientXY(*event.GetPosition())
		menu = wx.Menu()
		submenu = wx.Menu()
		index = 0
		def cmp_nocase(a,b):
			return cmp(a[0].lower(),b[0].lower())
		for name,channel,ctrlid in sorted(config.get_config().get_midi_controllers(), cmp_nocase):
			submenu.Append(self.CONTROLLER+index, prepstr(name), "", wx.ITEM_NORMAL)
			index += 1
		menu.AppendMenu(-1, "&Bind to MIDI Controller", submenu, "")
		menu.Append(self.UNBIND_ALL, "&Unbind All", "", wx.ITEM_NORMAL)
		self.current_param = (g,t,i)
		self.PopupMenuXY(menu, mx, my)
		
	def on_callback(self, player, plugin, data):
		"""
		parameter window callback for ui events sent by zzub.
		
		@param player: player instance.
		@type player: zzub.Player
		@param plugin: plugin instance
		@type plugin: zzub.Plugin
		@param data: event data.
		@type data: zzub_event_data_t
		"""
		if plugin == self.plugin:
			if data.type == zzub.zzub_event_type_parameter_changed:
				data = getattr(data,'').change_parameter
				g,t,i,v = data.group, data.track, data.param, data.value
				p = self.pluginloader.get_parameter(g,i)
				if p.get_flags() & zzub.zzub_parameter_flag_state:
					nl,s,vl = self.pid2ctrls[(g,t,i)]
					v = self.plugin.get_parameter_value(g,t,i)
					s.SetValue(v)
					self.update_valuelabel(g,t,i)
					
	def update_presets(self):
		"""
		Updates the preset box.
		"""
		self.presets = config.get_config().get_plugin_presets(self.pluginloader)
		s = self.presetbox.GetStringSelection()
		self.presetbox.Clear()
		self.presetbox.Append('<default>')
		for preset in self.presets.presets:
			self.presetbox.Append(prepstr(preset.name))
		self.presetbox.SetStringSelection(s)
					
	def apply_preset(self, preset=None):
		if not preset:
			for g in range(1,3):
				for t in range(self.plugin.get_group_track_count(g)):
						for i in range(self.pluginloader.get_parameter_count(g)):
							p = self.pluginloader.get_parameter(g,i)
							if p.get_flags() & zzub.zzub_parameter_flag_state:
								self.plugin.set_parameter_value(g,t,i,p.get_value_default(),0)
		else:			
			preset.apply(self.plugin)
			self.presetbox.SetStringSelection(prepstr(preset.name))
		self.update_all_sliders()
		
		
	def on_select_preset(self, event):
		"""
		Handler for changes of the choice box. Changes the current parameters
		according to preset.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		sel = max(self.presetbox.GetSelection() - 1,-1)
		if sel == -1:
			self.apply_preset(None)
		else:
			self.apply_preset(self.presets.presets[sel])
			
	def update_all_sliders(self):
		"""
		Updates all sliders. Should only be called when most sliders
		have been changed at once, e.g. after a preset change.
		"""
		for g in range(1,3):
			for t in range(self.plugin.get_group_track_count(g)):
				for i in range(self.pluginloader.get_parameter_count(g)):
					p = self.pluginloader.get_parameter(g,i)
					if p.get_flags() & zzub.zzub_parameter_flag_state:
						nl,s,vl = self.pid2ctrls[(g,t,i)]
						v = self.plugin.get_parameter_value(g,t,i)						
						s.SetValue(v)
						self.update_valuelabel(g,t,i)
		
	def on_button_edit(self, event):
		"""
		Handler for clicks on the 'Edit' button. Opens the
		preset dialog.
		"""
		dlg = PresetDialog(self.plugin, self, -1)
		dlg.ShowModal()
		dlg.Destroy()
		self.update_presets()

	def on_button_copy(self, event):
		"""
		Handler for clicks on the 'Copy' button. Constructs a paste buffer
		which can be used for pasting in the pattern editor.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		import patterns
		CLIPBOARD_MAGIC = patterns.PatternView.CLIPBOARD_MAGIC
		data = CLIPBOARD_MAGIC
		data += "%01x" % patterns.SEL_ALL
		for g in range(1,3):
			for t in range(self.plugin.get_group_track_count(g)):
				for i in range(self.pluginloader.get_parameter_count(g)):
					p = self.pluginloader.get_parameter(g,i)
					if p.get_flags() & zzub.zzub_parameter_flag_state:
						v = self.plugin.get_parameter_value(g,t,i)
						data += "%04x%01x%02x%02x%04x" % (0,g,t,i,v)
		clipboard = wx.TheClipboard
		if clipboard.Open():
			clipboard.SetData(wx.TextDataObject(data))
			clipboard.Close()
		
	def on_button_random(self, event):
		"""
		Handler for clicks on the 'Random' button.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		import random
		for g in range(1,3):
			for t in range(self.plugin.get_group_track_count(g)):
				for i in range(self.pluginloader.get_parameter_count(g)):
					p = self.pluginloader.get_parameter(g,i)
					if p.get_flags() & zzub.zzub_parameter_flag_state:
						nl,s,vl = self.pid2ctrls[(g,t,i)]
						v = random.randint(p.get_value_min(), p.get_value_max())
						self.plugin.set_parameter_value(g,t,i,v,0)
						s.SetValue(v)
						self.update_valuelabel(g,t,i)
	
	def on_button_help(self, event):
		"""
		Handler for clicks on the 'Help' button.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		uri = filenameify(self.pluginloader.get_uri())
		name = filenameify(self.pluginloader.get_name())		
		helpfilepaths = [
			filepath('../doc/zzub/plugins/' + uri + '/index.html'),
			filepath('../doc/zzub/plugins/' + name + '/index.html'),
		]
		for path in helpfilepaths:
			print "searching for '%s'..." % path
			if os.path.isfile(path):
				import webbrowser
				webbrowser.open_new(path)
				return
		wx.MessageDialog(self, message="Sorry, there's no help for this plugin yet.", caption = "Help", style = wx.ICON_WARNING|wx.OK|wx.CENTER).ShowModal()
		
	def on_key_down(self, event):
		"""
		Callback that responds to key stroke.
		
		@param event: Key event
		@type event: wx.KeyEvent
		"""		
		key = event.GetKeyCode()		
		if (key >= ord('0')) and (key <= ord('9')):
			g,t,i = self.id2pid[event.GetId()]
			p = self.pluginloader.get_parameter(g,i)
			minv = p.get_value_min()
			maxv = p.get_value_max()
			data_entry = DataEntry((minv,maxv,chr(key)), self)
			position = self.GetPosition()
			offset = event.GetEventObject().GetPosition()
			data_entry.SetPosition((position[0] + offset[0], position[1] + offset[1]))
			if data_entry.ShowModal() == wx.ID_OK:
				try:
					value = int(data_entry.edit.GetValue())
					nl,s,vl = self.pid2ctrls[(g,t,i)]
					s.SetValue(value)
					self.plugin.set_parameter_value(g,t,i,s.GetValue(),0)					
					self.update_valuelabel(g,t,i)
				except:
					import traceback
					traceback.print_exc()
			data_entry.Destroy()
		event.Skip()
	
	def on_close(self, event):
		"""
		Handles close events.
		"""
		self.Destroy()
		
	def on_destroy(self, event):
		"""
		Handles destroy events.
		"""
		
	def on_mousewheel(self, event):
		"""
		Sent when the mousewheel is used on a slider.

		@param event: A mouse event.
		@type event: wx.MouseEvent
		"""
		g,t,i = self.id2pid[event.GetId()]
		nl,s,vl = self.pid2ctrls[(g,t,i)]
		v = self.plugin.get_parameter_value(g,t,i)
		if event.m_wheelRotation > 0:
			v += 1
		else:
			v -= 1
		self.plugin.set_parameter_value(g,t,i,v,1)
		v = self.plugin.get_parameter_value(g,t,i)
		s.SetValue(v)
		self.update_valuelabel(g,t,i)
		
	def update_valuelabel(self, g, t, i):
		"""
		Updates the right label for a parameter slider.
		
		@param g: The group this parameter belongs to.
		@type g: int
		@param t: The track of the group this parameter belongs to.
		@type t: int
		@param i: The parameter index within the track.
		@type i: int
		"""
		nl,s,vl = self.pid2ctrls[(g,t,i)]
		v = self.plugin.get_parameter_value(g,t,i)
		text = prepstr(self.plugin.describe_value(g,i,v))
		if not text:
			text = "%i" % v
		vl.SetLabel(text)
		
	def on_scroll_changed(self, event):
		"""
		Event handler for changes in slider movements.
		
		@param event: A scroll event.
		@type event: wx.ScrollEvent
		"""
		g,t,i = self.id2pid[event.GetId()]
		nl,s,vl = self.pid2ctrls[(g,t,i)]
		s.SetValue(s.GetValue()) # quantize slider position
		self.plugin.set_parameter_value(g,t,i,s.GetValue(),1)
		self.update_valuelabel(g,t,i)

class PluginBrowserDialog(wx.Dialog):
	"""
	Displays all available plugins and some meta information.
	"""
	def __init__(self, *args, **kwds):
		"""
		Initializer.
		"""
		wx.Dialog.__init__(self, *args, **kwds)
		self.SetTitle("New Plugin")
		self.treeview = wx.TreeCtrl(self, -1, style=wx.SUNKEN_BORDER | wx.TR_HIDE_ROOT | wx.TR_HAS_BUTTONS)
		self.treeview.SetMinSize((400,300))
		self.namelabel = wx.StaticText(self, -1, "Name:")
		self.namevalue = wx.StaticText(self, -1)
		self.authorlabel = wx.StaticText(self, -1, "Author:")
		self.authorvalue = wx.StaticText(self, -1)
		self.urilabel = wx.StaticText(self, -1, "URI:")
		self.urivalue = wx.StaticText(self, -1)
		rootnode = self.treeview.AddRoot("Plugins")
		generatornode = self.treeview.AppendItem(rootnode, "Generators")
		effectnode = self.treeview.AppendItem(rootnode, "Effects")
		name2pl = {}
		for pl in player.get_pluginloader_list():
			name = pl.get_name()
			name2pl[name] = pl
		for name in sorted(name2pl, lambda a,b : cmp(a.lower(),b.lower())):
			pl = name2pl[name]
			if pl.get_type() == zzub.zzub_plugin_type_generator:
				node = generatornode
			elif pl.get_type() == zzub.zzub_plugin_type_effect:
				node = effectnode
			else:
				continue
			n = self.treeview.AppendItem(node, prepstr(name))
			self.treeview.SetPyData(n, pl)
			def add_parameter_group(n, params):
				for param in params:
					paramnode = self.treeview.AppendItem(n, prepstr(param.get_name()))
					type2name = {
						zzub.zzub_parameter_type_note : 'note',
						zzub.zzub_parameter_type_switch : 'switch',
						zzub.zzub_parameter_type_byte : 'byte',
						zzub.zzub_parameter_type_word : 'word',
					}
					self.treeview.AppendItem(paramnode, prepstr('type = "%s"' % type2name[param.get_type()]))
					self.treeview.AppendItem(paramnode, prepstr('description = "%s"' % param.get_description()))
					self.treeview.AppendItem(paramnode, prepstr('minvalue = "%s"' % param.get_value_min()))
					self.treeview.AppendItem(paramnode, prepstr('maxvalue = "%s"' % param.get_value_max()))
					self.treeview.AppendItem(paramnode, prepstr('defvalue = "%s"' % param.get_value_default()))
					self.treeview.AppendItem(paramnode, prepstr('novalue = "%s"' % param.get_value_none()))
					flags = param.get_flags()
					if flags & zzub.zzub_parameter_flag_wavetable_index:
						self.treeview.AppendItem(paramnode, prepstr('waveindex = "true"'))
					if flags & zzub.zzub_parameter_flag_state:
						self.treeview.AppendItem(paramnode, prepstr('state = "true"'))
					if flags & zzub.zzub_parameter_flag_event_on_edit:
						self.treeview.AppendItem(paramnode, prepstr('editevent = "true"'))
			if pl.get_parameter_count(1):
				globalnode = self.treeview.AppendItem(n, "Global Parameters")
				add_parameter_group(globalnode, pl.get_parameter_list(1))
			if pl.get_parameter_count(2):
				tracknode = self.treeview.AppendItem(n, "Track Parameters")
				add_parameter_group(tracknode, pl.get_parameter_list(2))
			if pl.get_attribute_count():
				attribsnode = self.treeview.AppendItem(n, "Attributes")
				for attrib in pl.get_attribute_list():
					attribnode = self.treeview.AppendItem(attribsnode, prepstr(attrib.get_name()))
					self.treeview.AppendItem(attribnode, prepstr('minvalue = "%s"' % attrib.get_value_min()))
					self.treeview.AppendItem(attribnode, prepstr('maxvalue = "%s"' % attrib.get_value_max()))
					self.treeview.AppendItem(attribnode, prepstr('defvalue = "%s"' % attrib.get_value_default()))
			
		sizer = wx.BoxSizer(wx.VERTICAL)
		sizer.Add(self.treeview, 0, wx.EXPAND | wx.ALL, border=5)
		gridsizer = wx.FlexGridSizer(3,2,5,5)
		gridsizer.Add(self.namelabel, 0, wx.ALIGN_LEFT)
		gridsizer.Add(self.namevalue, 0, wx.EXPAND)
		gridsizer.Add(self.authorlabel, 0, wx.ALIGN_LEFT)
		gridsizer.Add(self.authorvalue, 0, wx.EXPAND)
		gridsizer.Add(self.urilabel, 0, wx.ALIGN_LEFT)
		gridsizer.Add(self.urivalue, 0, wx.EXPAND)
		sizer.Add(gridsizer, 0, wx.RIGHT | wx.LEFT | wx.BOTTOM, border=5)
		self.SetAutoLayout(True)
		self.SetSizer(sizer)
		self.Layout()
		self.Fit()
		self.Centre()
		wx.EVT_LEFT_DCLICK(self.treeview, self.on_treeview_dclick)
		wx.EVT_TREE_SEL_CHANGED(self, self.treeview.GetId(), self.on_treeview_sel_changed)
		self.action = None

	def on_treeview_dclick(self, event):
		"""
		Callback that responds to double click in the tree. Same as "new plugin" event.
		
		@param event: MouseEvent event
		@type event: wx.MouseEvent
		"""
		pl = self.treeview.GetPyData(self.treeview.GetSelection())
		if pl:
			self.pl = pl
			self.EndModal(wx.ID_OK)
		else:
			event.Skip()

	def on_treeview_sel_changed(self, event):
		"""
		Handles changes in the treeview. Updates meta information.
		
		@param event: Tree event.
		@type event: wx.TreeEvent
		"""
		pl = self.treeview.GetPyData(event.GetItem())
		if pl:
			self.namevalue.SetLabel(prepstr(pl.get_short_name()))
			self.authorvalue.SetLabel(prepstr(pl.get_author()))
			self.urivalue.SetLabel(prepstr(pl.get_uri()))
		else:
			self.namevalue.SetLabel("")
			self.authorvalue.SetLabel("")
			self.urivalue.SetLabel("")

def show_plugin_browser_dialog(parent):
	"""
	Shows the plugin browser dialog.
	
	@param parent: Parent window.
	@type parent: wx.Window
	"""
	dlg = PluginBrowserDialog(parent)
	if dlg.ShowModal() == wx.ID_OK:
		return dlg.pl
	dlg.Destroy()
		
class RoutePanel(wx.Panel):
	"""
	Contains the view panel and manages parameter dialogs.
	"""
	def __init__(self, rootwindow, *args, **kwds):
		"""
		Initializer.
		
		@param rootwindow: Main window.
		@type rootwindow: wx.Frame
		"""
		self.rootwindow = rootwindow
		wx.Panel.__init__(self, *args, **kwds)
		self.view = RouteView(rootwindow, self, -1)
		sizer_2 = wx.BoxSizer(wx.HORIZONTAL)
		sizer_2.Add(self.view, 1, wx.EXPAND, 0)
		self.SetAutoLayout(True)
		self.SetSizer(sizer_2)
		self.Layout()
		
	def Show(self, show):
		wx.Panel.Show(self)
		self.view.Show(show)
		self.Layout()
		
	def reset(self):
		"""
		Resets the router view. Used when
		a new song is being loaded.
		"""
		self.view.reset()
		
	def update_all(self):		
		self.view.update_info()
		
		
class DataEntry(wx.Dialog):
	"""
	A data entry control meant for numerical input of slider values.
	"""
	def __init__(self, (minval,maxval,v), *args, **kwds):
		"""
		Initializer.
		"""
		kwds['style'] = wx.SIMPLE_BORDER
		kwds['size'] = (200,50)
		wx.Dialog.__init__(self, *args, **kwds)		
		self.label = wx.StaticText(self, -1, label ="Enter Value:")
		self.edit = wx.TextCtrl(self, -1, v, style=wx.TE_PROCESS_ENTER)		
		label = wx.StaticText(self)
		vsizer = wx.BoxSizer(wx.VERTICAL)
		s = wx.BoxSizer(wx.HORIZONTAL)
		s.Add(self.label, 0,  wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
		s.Add(self.edit, 1, wx.ALIGN_CENTER_VERTICAL| wx.ALL, 5)
		vsizer.Add(s, 0, wx.EXPAND)
		vsizer.Add(wx.StaticText(self, -1, label = prepstr("%s - %s" % (minval,maxval))), wx.ALL, 5)
		self.SetSizerAndFit(vsizer)
		self.SetAutoLayout(True)
		self.Layout()
		wx.EVT_TEXT_ENTER(self, self.edit.GetId(), self.on_text_enter)
		wx.EVT_KEY_DOWN(self.edit, self.on_key_down)		
		wx.EVT_SET_FOCUS(self.edit, self.on_focus)		
	
	def on_focus(self, event):	
		pos = len(self.edit.GetValue())
		self.edit.SetSelection(pos,pos)

	def on_text_enter(self, event):
		self.EndModal(wx.ID_OK)
	
	def on_key_down(self, event):
		
		if event.KeyCode() == wx.WXK_ESCAPE:
			self.EndModal(wx.ID_CANCEL)
		event.Skip()		

class VolumeSlider(wx.Panel):
	"""
	A temporary popup volume control for the router. Can
	only be summoned parametrically and will vanish when the
	left mouse button is being released.
	"""
	def __init__(self, *args, **kwds):
		"""
		Initializer.
		"""
		kwds['size'] = (VOLBARWIDTH,VOLBARHEIGHT)
		self.conn = None
		wx.Panel.__init__(self, *args, **kwds)
		self.Show(False)
		wx.EVT_MOTION(self, self.on_motion)
		wx.EVT_LEFT_UP(self, self.on_left_up)
		wx.EVT_PAINT(self, self.on_paint)
		
	def on_motion(self, event):
		"""
		Event handler for mouse movements.
		
		@param event: Mouse event.
		@type event: wx.MouseEvent
		"""
		newpos = event.GetPosition()[1]
		delta = newpos - self.y
		if delta == 0:
			return
		self.y = newpos
		self.amp = max(min(self.amp + (float(delta) / VOLBARHEIGHT), 1.0), 0.0)
		amp = min(max(int(db2linear(self.amp * -48.0, -48.0) * 16384.0), 0), 16384)
		self.conn.set_amplitude(amp)
		self.Refresh()
		
	def on_paint(self, event):
		"""
		Event handler for paint requests.
		
		@param event: Paint event.
		@type event: wx.PaintEvent
		"""
		dc = wx.PaintDC(self)
		dc.BeginDrawing()
		w,h = self.GetClientSize()
		cfg = config.get_config()		
		whitebrush = cfg.get_brush('MV Amp BG')
		blackbrush = cfg.get_brush('MV Amp Handle')
		outlinepen = cfg.get_pen('MV Amp Border')
		dc.SetPen(outlinepen)
		dc.SetBrush(whitebrush)
		dc.DrawRectangle(0, 0, w, h)
		dc.SetPen(wx.TRANSPARENT_PEN)
		dc.SetBrush(blackbrush)
		if self.conn:
			pos = int(self.amp * (VOLBARHEIGHT - VOLKNOBHEIGHT))
			dc.DrawRectangle(1, pos+1, VOLBARWIDTH-2, VOLKNOBHEIGHT-2)
		dc.EndDrawing()
		
	def display(self, (mx,my), conn):
		"""
		Called by the router view to show the control.
		
		@param mx: X coordinate of the control center in pixels.
		@type mx: int
		@param my: Y coordinate of the control center in pixels.
		@type my: int
		@param conn: Connection to control.
		@type conn: zzub.Connection
		"""
		self.y = VOLBARHEIGHT / 2
		self.conn = conn
		self.amp = (linear2db((self.conn.get_amplitude() / 16384.0), -48.0) / -48.0)
		self.MoveXY(int(mx - VOLBARWIDTH*0.5), int(my - VOLBARHEIGHT*0.5))
		self.Show(True)
		self.CaptureMouse()
		
	def on_left_up(self, event):
		"""
		Event handler for left mouse button releases. Will
		hide control.
		
		@param event: Mouse event.
		@type event: wx.MouseEvent
		"""
		self.Show(False)
		self.ReleaseMouse()

class PluginInfo(object):
	"""
	Encapsulates data associated with a plugin.
	"""
	def __init__(self, plugin):
		self.plugin = plugin
		self.muted = False
		self.pattern_position = (0, 0, 0, 0, 0)
		self.plugingfx = None
		self.patterngfx = {}
		self.amp = -9999.0
		
	def reset_plugingfx(self):
		self.plugingfx = None
		self.amp = -9999.0
		
class RouteView(Canvas):
	"""
	Allows to monitor and control plugins and their connections.
	"""	
	current_plugin = None
	context_plugin = None
	context_conn = None
	connecting = False
	dragging = False
	dragoffset = 0,0
	contextmenupos = 0,0
	
	MUTE = wx.NewId()
	SOLO = wx.NewId()
	CENTERPAN = wx.NewId()
	PARAMS = wx.NewId()
	ATTRIBS = wx.NewId()
	RENAME = wx.NewId()
	DELETE = wx.NewId()
	ABOUT = wx.NewId()
	SIGNALANALYSIS = wx.NewId()
	IMPORTSONG = wx.NewId()
	UNMUTEALL = wx.NewId()
	DISCONNECT = wx.NewId()
	CMDBASEID = wx.NewId()
	for i in range(256):
		wx.RegisterId(CMDBASEID+i)
	MAX_PLUGINS = 8192
	NEWPLUGIN = wx.NewId()
	for i in range(MAX_PLUGINS):
		wx.RegisterId(NEWPLUGIN+i)
		
	def yield_newplugin_indices(self):
		for i in range(self.MAX_PLUGINS):
			yield self.NEWPLUGIN+i
	
	def __init__(self, rootwindow, *args, **kwds):
		"""
		Initializer.
		
		@param rootwindow: Main window.
		@type rootwindow: wx.Frame
		"""
		kwds['style'] = wx.SUNKEN_BORDER		
		self.plugin_dialogs = {}
		self.parent = args[0]
		self.rootwindow = rootwindow
		self.rootwindow.event_handlers.append(self.on_player_callback)
		self.solo_plugin = None
		# storage of additional plugin data
		self.plugin_info = {}
		self.update_info()
		self.update_colors()
		Canvas.__init__(self, *args, **kwds)
		self.volume_slider = VolumeSlider(self, -1)		
		self.plugin_tree = indexer.parse_index(player, config.get_config().get_index_path())
		
		for i in self.yield_newplugin_indices():
			wx.EVT_MENU(self, i, self.on_popup_new_plugin)
		wx.EVT_MIDDLE_DOWN(self, self.on_left_down)
		wx.EVT_MIDDLE_UP(self, self.on_left_up)
		wx.EVT_LEFT_DOWN(self, self.on_left_down)
		wx.EVT_LEFT_UP(self, self.on_left_up)
		wx.EVT_LEFT_DCLICK(self, self.on_left_dclick)
		wx.EVT_CONTEXT_MENU(self, self.on_context_menu)
		wx.EVT_MOTION(self, self.on_motion)
		wx.EVT_MENU(self, self.PARAMS, self.on_popup_show_params)
		wx.EVT_MENU(self, self.ATTRIBS, self.on_popup_show_attribs)
		wx.EVT_MENU(self, self.DISCONNECT, self.on_popup_disconnect)
		wx.EVT_MENU(self, self.DELETE, self.on_popup_delete)
		wx.EVT_MENU(self, self.MUTE, self.on_popup_mute)
		wx.EVT_MENU(self, self.SOLO, self.on_popup_solo)
		wx.EVT_MENU(self, self.RENAME, self.on_popup_rename)
		wx.EVT_MENU(self, self.SIGNALANALYSIS, self.on_popup_show_signalanalysis)
		for i in range(256):
			wx.EVT_MENU(self, self.CMDBASEID+i, self.on_popup_command)
		wx.EVT_SET_FOCUS(self, self.on_focus)
		self.draw_led_timer = wx.Timer(self, -1)
		self.draw_led_timer.Start(100)
		wx.EVT_TIMER(self, self.draw_led_timer.GetId(), self.on_draw_led_timer)
		
	def update_colors(self):
		"""
		Updates the routers color scheme.
		"""
		cfg = config.get_config()
		self.type2brush = [
			{
				zzub.zzub_plugin_type_master : cfg.get_brush("MV Master"),
				zzub.zzub_plugin_type_generator : cfg.get_brush("MV Generator"),
				zzub.zzub_plugin_type_effect : cfg.get_brush("MV Effect"),
			},
			{
				zzub.zzub_plugin_type_master : cfg.get_brush("MV Master"),
				zzub.zzub_plugin_type_generator : cfg.get_brush("MV Generator Mute"),
				zzub.zzub_plugin_type_effect : cfg.get_brush("MV Effect Mute"),
			}
		]
		self.ledtype2brush = [
			{
				zzub.zzub_plugin_type_master : cfg.get_brush("MV Master LED Off"),
				zzub.zzub_plugin_type_generator : cfg.get_brush("MV Generator LED Off"),
				zzub.zzub_plugin_type_effect : cfg.get_brush("MV Effect LED Off"),
			},
			{
				zzub.zzub_plugin_type_master : cfg.get_brush("MV Master LED On"),
				zzub.zzub_plugin_type_generator : cfg.get_brush("MV Generator LED On"),
				zzub.zzub_plugin_type_effect : cfg.get_brush("MV Effect LED On"),
			},
			{
				zzub.zzub_plugin_type_master : cfg.get_pen("MV Master LED Border"),
				zzub.zzub_plugin_type_generator : cfg.get_pen("MV Generator LED Border"),
				zzub.zzub_plugin_type_effect : cfg.get_pen("MV Effect LED Border"),
			},
			{
				zzub.zzub_plugin_type_master : cfg.get_brush("MV Machine LED Warning"),
				zzub.zzub_plugin_type_generator : cfg.get_brush("MV Machine LED Warning"),
				zzub.zzub_plugin_type_effect : cfg.get_brush("MV Machine LED Warning"),
			},			
		]
		for k,v in self.plugin_info.iteritems():
			v.reset_plugingfx()
		
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
			plugin = zzub.Plugin(getattr(data,'').delete_plugin.plugin)
			dlg = self.plugin_dialogs.get(plugin,None)
			if dlg:
				dlg.Destroy()

	def reset(self):
		"""
		Destroys all parameter dialogs. Used when
		a new song is being loaded.
		"""
		for dlg in self.plugin_dialogs.values():
			dlg.Destroy()
		self.plugin_dialogs = {}

	def update_info(self):
		previous = dict(self.plugin_info)
		self.plugin_info.clear()
		for mp in player.get_plugin_list():
			if mp in previous:
				self.plugin_info[mp] = previous[mp]
			else:
				self.plugin_info[mp] = PluginInfo(mp)
			
	def on_focus(self, event):
		self.ReDraw()
	
	def on_popup_rename(self, event):
		dialog = wx.TextEntryDialog(self, "Rename plugin", caption = "Rename plugin",
			defaultValue=self.context_plugin.get_name(), style = wx.OK | wx.CANCEL)
		if dialog.ShowModal() == wx.ID_OK:
			self.context_plugin.set_name(dialog.GetValue())
			self.plugin_info[self.context_plugin].reset_plugingfx()
			self.ReDraw()
		dialog.Destroy()
	
	def solo(self, plugin):		
		if not plugin or plugin == self.solo_plugin:
			# soloing deactived so apply muted states
			self.solo_plugin = None			
			for plugin, info in self.plugin_info.iteritems():
				plugin.set_mute(info.muted)
				info.reset_plugingfx()
		elif plugin.get_type() == zzub.zzub_plugin_type_generator:
			# mute all plugins except solo plugin
			self.solo_plugin = plugin			
			for plugin, info in self.plugin_info.iteritems():				
				if plugin != self.solo_plugin and plugin.get_type() == zzub.zzub_plugin_type_generator:					
					plugin.set_mute(True)
					info.reset_plugingfx()
				elif plugin == self.solo_plugin:
					plugin.set_mute(info.muted)
					info.reset_plugingfx()
	
	def on_popup_solo(self, event):
		"""
		Event handler for the "Mute" context menu option.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		"""		
		if self.solo_plugin != self.context_plugin:
			self.solo(self.context_plugin)
		else:
			self.solo(None)		
		self.ReDraw()
	
	def toggle_mute(self, plugin):
		self.plugin_info[plugin].muted = not self.plugin_info[plugin].muted		
		if not self.solo_plugin or self.context_plugin == self.solo_plugin:
			plugin.set_mute(self.plugin_info[plugin].muted)
		self.plugin_info[plugin].reset_plugingfx()
	
	def on_popup_mute(self, event):
		"""
		Event handler for the "Mute" context menu option.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		"""
		self.toggle_mute(self.context_plugin)		
		self.ReDraw()
		
	def on_popup_delete(self, event):
		"""
		Event handler for the "Delete" context menu option.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		"""
		dlg = wx.MessageDialog(self, message="No undo!", caption = "Remove plugin?", style = wx.ICON_EXCLAMATION|wx.YES_NO|wx.CENTER)
		res = dlg.ShowModal()
		if res == wx.ID_YES:
			inplugs = []
			outplugs = []
			# record all connections
			while True:
				conns = self.context_plugin.get_input_connection_list()
				if not conns:
					break
				conn = conns.pop()
				input = conn.get_input()
				amp = conn.get_amplitude()
				pan = conn.get_panning()
				inplugs.append((input,amp,pan))
				self.context_plugin.delete_input(input)
			while True:
				conns = self.context_plugin.get_output_connection_list()
				if not conns:
					break
				conn = conns.pop()
				output = conn.get_output()
				amp = conn.get_amplitude()
				pan = conn.get_panning()
				outplugs.append((output,amp,pan))
				output.delete_input(self.context_plugin)
			# and now restore them
			for inplug,iamp,ipan in inplugs:
				for outplug,oamp,opan in outplugs:
					newamp = (iamp*oamp)/16384
					newpan = ipan
					outplug.add_input(inplug, newamp, newpan)
			del self.plugin_info[self.context_plugin]
			self.context_plugin.destroy()
			self.ReDraw()
		dlg.Destroy()
		
	def on_popup_disconnect(self, event):
		"""
		Event handler for the "Disconnect" context menu option.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		"""
		self.context_conn.get_output().delete_input(self.context_conn.get_input())
		self.ReDraw()
		
	def show_parameter_dialog(self, plugin):
		"""
		Shows a parameter dialog for a plugin.
		
		@param plugin: Plugin instance.
		@type plugin: Plugin
		"""
		dlg = self.plugin_dialogs.get(plugin,None)
		if not dlg:
			dlg = ParameterDialog(self.rootwindow, plugin, self)
		dlg.Show()

	def on_popup_show_signalanalysis(self, event):
		"""
		Event handler for the "Signal Analysis" context menu option.
		"""
		dlg = SignalAnalysisDialog(self.context_conn.get_input(), self, -1)
		dlg.ShowModal()
		dlg.Destroy()
		
	def on_popup_show_attribs(self, event):
		"""
		Event handler for the "Attributes..." context menu option.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		"""
		dlg = AttributesDialog(self.context_plugin, self, -1)
		dlg.ShowModal()
		dlg.Destroy()
		
	def on_popup_show_params(self, event):
		"""
		Event handler for the "Parameters..." context menu option.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		"""
		self.show_parameter_dialog(self.context_plugin)
		
	def on_popup_new_plugin(self, event=None, pl = None):
		"""
		Event handler for "new plugin" context menu options. if pl is omitted,
		the pluginloader is deduced from event.GetId().
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		@param pl: Pluginloader
		@type event: zzub.Pluginloader
		"""
		if not pl:
			pl = self.id2plugin[event.GetId() - self.NEWPLUGIN]
		basename = pl.get_short_name()
		name = pl.get_short_name()
		basenumber = 2
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
		mp = player.create_plugin(None, 0, name, pl)
		assert mp._handle
		if mp.get_type() == zzub.zzub_plugin_type_generator and \
			(pl.get_parameter_count(1) or pl.get_parameter_count(2)):
			pattern = mp.create_pattern(16)
			pattern.set_name('00')
			seq = player.get_current_sequencer()
			seq.create_track(mp)
		mp.set_position(*self.pixel_to_float(self.contextmenupos))
		# if we have a context plugin, prepend connections
		if self.context_plugin:
			inplugs = []
			# record all connections
			while True:
				conns = self.context_plugin.get_input_connection_list()
				if not conns:
					break
				conn = conns.pop()
				input = conn.get_input()
				amp = conn.get_amplitude()
				pan = conn.get_panning()
				inplugs.append((input,amp,pan))
				self.context_plugin.delete_input(input)
			# restore
			for inplug,amp,pan in inplugs:
				mp.add_input(inplug, amp, pan)
			self.context_plugin.add_input(mp, 16384, 16384)
		# if we have a context connection, replace that one
		elif self.context_conn:
			amp = self.context_conn.get_amplitude()
			pan = self.context_conn.get_panning()
			minput = self.context_conn.get_input()
			moutput = self.context_conn.get_output()
			moutput.delete_input(minput)
			mp.add_input(minput, amp, pan)
			moutput.add_input(mp, 16384, 16384)
		self.rootwindow.document_changed()
		# selects the plugin in the pattern view
		plugintoolbar = self.rootwindow.patternframe.toolbar
		plugintoolbar.select_plugin(plugintoolbar.pluginselect.GetCount()-1)
		# add plugin information
		self.plugin_info[mp] = PluginInfo(mp)
		# open parameter view if its an effect
		if mp.get_type() == zzub.zzub_plugin_type_effect:
			self.show_parameter_dialog(mp)
		
	def on_popup_command(self, event):
		"""
		Event handler for requests to show the context menu.
		
		@param event: event.
		@type event: wx.Event
		"""
		submenuindex,index = self.cmdmap[event.GetId()]
		print (submenuindex<<8) | index
		self.context_plugin.command((submenuindex<<8) | index)
		
	def get_plugin_menu(self, include_generators = True, include_effects = True):
		"""
		Generates and returns a new plugin menu.
		
		@return: A menu containing commands to instantiate new plugins.
		@rtype: wx.Menu
		"""
		def fill_menu(menu,node,indexgen):
			add_separator = False
			for child in node.children:
				if isinstance(child, indexer.Directory) and not child.is_empty():
					if add_separator:
						add_separator = False
						if menu.GetMenuItemCount():
							menu.AppendSeparator()
					submenu = wx.Menu()
					menu.AppendMenu(-1, prepstr(child.name), submenu, "")
					fill_menu(submenu, child, indexgen)
				elif isinstance(child, indexer.Reference):
					if child.pluginloader:
						if not include_generators and (child.pluginloader.get_type() == zzub.zzub_plugin_type_generator):
							continue
						if not include_effects and (child.pluginloader.get_type() == zzub.zzub_plugin_type_effect):
							continue
					if add_separator:
						add_separator = False
						if menu.GetMenuItemCount():
							menu.AppendSeparator()
					menuid = indexgen.next()
					self.id2plugin[menuid - self.NEWPLUGIN] = child.pluginloader
					menu.Append(menuid, prepstr(child.name), "", wx.ITEM_NORMAL)
					if not child.pluginloader:
						menu.Enable(menuid, False)
				elif isinstance(child, indexer.Separator):
					add_separator = True					
		plugin_menu = wx.Menu()
		self.id2plugin = [None]*self.MAX_PLUGINS
		fill_menu(plugin_menu, self.plugin_tree, self.yield_newplugin_indices())
		return plugin_menu
		
	def on_context_menu(self, event):
		"""
		Event handler for requests to show the context menu.
		
		@param event: event.
		@type event: wx.Event
		"""
		mx,my = self.ScreenToClientXY(*event.GetPosition())
		self.contextmenupos = mx,my
		menu = wx.Menu()
		res = self.get_plugin_at((mx,my))
		if res:
			mp,(x,y),area = res
			menu.Append(self.MUTE, "&Mute", "Toggle bypass", wx.ITEM_CHECK)
			item = menu.FindItemById(self.MUTE)
			if self.plugin_info[mp].muted:
				item.Check()
			if mp.get_type() == zzub.zzub_plugin_type_generator:
				menu.Append(self.SOLO, "&Solo", "Toggle solo", wx.ITEM_CHECK)
				item = menu.FindItemById(self.SOLO)			
				if self.solo_plugin == mp:
					item.Check()
			#~ menu.Append(self.CENTERPAN, "Cen&ter Pan", "", wx.ITEM_NORMAL)
			menu.AppendSeparator()
			menu.Append(self.PARAMS, "&Parameters...", "View parameters", wx.ITEM_NORMAL)
			menu.Append(self.ATTRIBS, "&Attributes...", "Show Attributes", wx.ITEM_NORMAL)
			menu.AppendSeparator()
			menu.Append(self.RENAME, "&Rename...", "", wx.ITEM_NORMAL)
			if mp.get_type() != zzub.zzub_plugin_type_master:
				menu.Append(self.DELETE, "&Delete", "Delete plugin", wx.ITEM_NORMAL)
			if mp.get_type() in (zzub.zzub_plugin_type_effect,zzub.zzub_plugin_type_master):
				menu.AppendSeparator()
				menu.AppendMenu(-1, "&Prepend Effect", self.get_plugin_menu(include_generators=False), "Prepend a new effect plugin and reroute connections.")
			self.cmdmap = {}
			commands = mp.get_commands()
			if commands:
				menu.AppendSeparator()
				baseid = self.CMDBASEID
				submenuindex = 0
				for index in range(len(commands)):
					cmd = commands[index]
					if cmd.startswith('/'):
						submenu = wx.Menu()
						subcommands = mp.get_sub_commands(index)
						submenuindex += 1
						for subindex in range(len(subcommands)):
							subcmd = subcommands[subindex]
							cmdid = baseid
							baseid += 1
							self.cmdmap[cmdid] = submenuindex,subindex
							submenu.Append(cmdid, prepstr(subcmd), "", wx.ITEM_NORMAL)
						menu.AppendMenu(-1, prepstr(cmd), submenu, "")
					else:
						cmdid = baseid
						baseid += 1
						self.cmdmap[cmdid] = 0,index
						menu.Append(cmdid, prepstr(cmd), "", wx.ITEM_NORMAL)
			self.context_plugin = mp
		else:
			self.context_plugin = None
			conn = self.get_connection_at((mx,my))
			if conn:
				self.context_conn = conn
				menu.Append(self.DISCONNECT, "&Disconnect plugins", "Disconnect plugins", wx.ITEM_NORMAL)
				menu.AppendSeparator()
				menu.AppendMenu(-1, "&Insert Effect", self.get_plugin_menu(include_generators=False), "Insert a new plugin and reroute connections.")
				menu.AppendSeparator()
				menu.Append(self.SIGNALANALYSIS, "&Signal Analysis", "Signal Analysis", wx.ITEM_NORMAL)
			else:
				self.context_conn = None
				menu = self.get_plugin_menu()
		self.PopupMenuXY(menu, mx, my)
		self.context_conn = None
		self.context_plugin = None
		
	def float_to_pixel(self, (x, y)):
		"""
		Converts a router coordinate to an on-screen pixel coordinate.
		
		@param x: X coordinate.
		@type x: float
		@param y: Y coordinate.
		@type y: float
		@return: A tuple returning the pixel coordinate.
		@rtype: (int,int)
		"""
		w,h = self.GetSize()
		cx,cy = w*0.5, h * 0.5
		return cx * (1 + x), cy * (1 + y)
		
	def pixel_to_float(self, (x, y)):
		"""
		Converts an on-screen pixel coordinate to a router coordinate.
		
		@param x: X coordinate.
		@type x: int
		@param y: Y coordinate.
		@type y: int
		@return: A tuple returning the router coordinate.
		@rtype: (float, float)
		"""
		w,h = self.GetSize()
		cx,cy = w*0.5, h * 0.5		
		return (x / cx) - 1, (y / cy) - 1
		
	def get_connection_at(self, (mx,my)):
		"""
		Finds the connection arrow at a specific position.
		
		@param mx: X coordinate in pixels.
		@type mx: int
		@param my: Y coordinate in pixels.
		@type my: int
		@return: A connection item or None.
		@rtype: zzub.Connection or None
		"""
		w,h = self.GetSize()
		cx,cy = w*0.5, h * 0.5
		def get_pixelpos(x,y):
			return cx * (1+x), cy * (1+y)
		for mp in player.get_plugin_list():
			rx,ry = get_pixelpos(*mp.get_position())
			for conn in mp.get_input_connection_list():
				crx, cry = get_pixelpos(*conn.get_input().get_position())
				cpx,cpy = (crx + rx) * 0.5, (cry + ry) * 0.5
				dx,dy = cpx - mx, cpy - my
				length = (dx*dx + dy*dy) ** 0.5
				if length <= 14:
					return conn
		
	def get_plugin_at(self, (x,y)):
		"""
		Finds a plugin at a specific position.
		
		@param x: X coordinate in pixels.
		@type x: int
		@param y: Y coordinate in pixels.
		@type y: int
		@return: A connection item, exact pixel position and area (AREA_ANY, AREA_PANNING, AREA_LED) or None.
		@rtype: (zzub.Plugin,(int,int),int) or None
		"""
		w,h = self.GetSize()
		cx,cy = w*0.5, h * 0.5
		mx, my = x,y
		PW, PH = PLUGINWIDTH / 2, PLUGINHEIGHT / 2
		area = AREA_ANY
		for mp in reversed(player.get_plugin_list()):
			x,y = mp.get_position()
			x,y = int(cx * (1+x)), int(cy * (1+y))
			if (mx >= (x - PW)) and (mx <= (x + PW)) and (my >= (y - PH)) and (my <= (y + PH)):
				if wx.Rect(x-PW+LEDOFSX,y-PH+LEDOFSY,LEDWIDTH,LEDHEIGHT).Inside((mx,my)):
					area = AREA_LED
				return mp,(x,y),area
				
	def on_left_dclick(self, event):
		"""
		Event handler for left doubleclicks. If the doubleclick
		hits a plugin, the parameter window is being shown.
		
		@param event: Mouse event.
		@type event: wx.MouseEvent
		"""
		mx,my = event.GetPosition()
		res = self.get_plugin_at((mx,my))
		if not res:
			pl = show_plugin_browser_dialog(self)
			if pl:
				self.contextmenupos = mx,my
				self.on_popup_new_plugin(pl=pl)
			return
		self.context_plugin,(x,y),area = res
		if area == AREA_ANY:
			data = zzub.zzub_event_data_t()
			data.type = zzub.zzub_event_type_double_click;
			if self.context_plugin.invoke_event(data, 1) != 0:
				self.on_popup_show_params(event)
		
	def on_left_down(self, event):
		"""
		Event handler for left mouse button presses. Initiates
		plugin dragging or connection volume adjustments.
		
		@param event: Mouse event.
		@type event: wx.MouseEvent
		"""
		mx,my = event.GetPosition()
		res = self.get_plugin_at((mx,my))
		if res:
			self.current_plugin,(x,y),area = res
			self.dragoffset = mx-x, my-y
			if area == AREA_LED:
				self.toggle_mute(self.current_plugin)		
				self.ReDraw()
			elif event.ShiftDown() or event.MiddleDown():
				self.connecting = True
			elif not self.connecting:
				self.dragging = True
				self.CaptureMouse()
		else:
			conn = self.get_connection_at((mx,my))
			if conn:
				self.volume_slider.display((mx,my), conn)
		
	def on_motion(self, event):
		"""
		Event handler for mouse movements.
		
		@param event: Mouse event.
		@type event: wx.MouseEvent
		"""
		if self.dragging:
			ox,oy = self.dragoffset
			mx,my = event.GetPosition()
			size = self.GetSize()
			x,y = max(0, min(mx - ox, size.width)), max(0, min(my - oy, size.height))
			self.current_plugin.set_position(*self.pixel_to_float((x,y)))
			self.ReDraw()
		elif self.connecting:
			self.connectpos = event.GetPosition()
			self.ReDraw()
		
	def on_left_up(self, event):
		"""
		Event handler for left mouse button releases.
		
		@param event: Mouse event.
		@type event: wx.MouseEvent
		"""
		if self.connecting:
			res = self.get_plugin_at(event.GetPosition())
			if res:
				mp,(x,y),area = res
				mp.add_input(self.current_plugin, 16384, 16384)
		self.current_plugin = None
		if self.dragging:
			self.ReleaseMouse()
		self.dragging = False		
		self.connecting = False
		self.ReDraw()
		
	def update_all(self):
		self.plugin_info = {}
		for mp in player.get_plugin_list():
			self.plugin_info = PluginInfo(mp)
		self.update_colors()
		
	def on_draw_led_timer(self, event):
		"""
		Timer event that only updates the plugin leds.
		
		@param event: Paint event.
		@type event: wx.PaintEvent
		"""
		# need to paint to front buffer directly.
		# if you get problems on your platform (OSX?)
		# with this statement, add an exception
		# for your platform. don't change this
		# or you will break the other platforms.
		if sys.platform == 'darwin':
			# don't do anything at all
			pass
		else:
			self.ReDraw(leds_only = True)
		
	def draw_leds(self):
		"""
		Draws only the leds into the offscreen buffer.
		"""
		cfg = config.get_config()
		w,h = self.GetSize()
		cx,cy = w*0.5,h*0.5
		def get_pixelpos(x,y):
			return cx * (1+x), cy * (1+y)
		dc = self.buffer
		textcolor = cfg.get_color("MV Machine Text")
		pluginpen = cfg.get_pen("MV Machine Border")
		font = wx.Font(8, wx.FONTFAMILY_DEFAULT, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_BOLD)
		PW, PH = PLUGINWIDTH / 2, PLUGINHEIGHT / 2
		mplist = [(mp,get_pixelpos(*mp.get_position())) for mp in player.get_plugin_list()]
		region = wx.Region(0,0,w,h)
		PW, PH = PLUGINWIDTH / 2, PLUGINHEIGHT / 2
		for mp,(rx,ry) in mplist:
			rx,ry = rx - PW, ry - PH
			pi = self.plugin_info.get(mp,None)
			if not pi:
				continue
			if not pi.plugingfx:
				pi.plugingfx = BitmapBuffer(PLUGINWIDTH, PLUGINHEIGHT)
				pidc = pi.plugingfx
				pidc.SetPen(pluginpen)
				pidc.SetTextForeground(textcolor)
				pidc.SetFont(font)
				# adjust colour for muted plugins
				if pi.muted:
					pidc.SetBrush(self.type2brush[1][mp.get_type()])
				else:
					pidc.SetBrush(self.type2brush[0][mp.get_type()])			
				pidc.DrawRectangle(0,0,PLUGINWIDTH,PLUGINHEIGHT)
				if self.solo_plugin and self.solo_plugin != mp and mp.get_type() == zzub.zzub_plugin_type_generator:
					title = prepstr('[' + mp.get_name() + ']')
				elif self.plugin_info[mp].muted:
					title = prepstr('(' + mp.get_name() + ')')
				else:
					title = prepstr(mp.get_name())
				pidc.DrawLabel(title, wx.Rect(0, 0, PLUGINWIDTH, PLUGINHEIGHT), wx.ALIGN_CENTER)
				
			maxl, maxr = mp.get_last_peak()
			amp = min(max(maxl,maxr),1.0)
			if amp != pi.amp:
				pidc = pi.plugingfx
				pidc.SetPen(self.ledtype2brush[2][mp.get_type()])
				if amp >= 1:
					pidc.SetBrush(self.ledtype2brush[3][mp.get_type()])
					pidc.DrawRectangle(LEDOFSX, LEDOFSY, LEDWIDTH, LEDHEIGHT)
				else:
					pidc.SetBrush(self.ledtype2brush[0][mp.get_type()])
					pidc.DrawRectangle(LEDOFSX, LEDOFSY, LEDWIDTH, LEDHEIGHT)
					amp = 1.0 - (linear2db(amp,-76.0)/-76.0)
					height = int(LEDHEIGHT*amp)-2
					if (height > 0):
						pidc.SetPen(wx.TRANSPARENT_PEN)
						pidc.SetBrush(self.ledtype2brush[1][mp.get_type()])
						pidc.DrawRectangle(LEDOFSX+1, LEDOFSY+LEDHEIGHT-height, LEDWIDTH-2, height-1)
				pi.amp = amp

			dc.Blit(rx, ry, PLUGINWIDTH, PLUGINHEIGHT, pi.plugingfx, 0, 0)
		
	def DrawBuffer(self, leds_only = False):
		"""
		Draws plugins, connections and arrows to an offscreen buffer.
		"""
		if not self.IsShown():
			return
		if leds_only:
			self.draw_leds()
			return
		cfg = config.get_config()
		dc = self.buffer
		w,h = self.GetSize()
		arrowbrush = cfg.get_brush("MV Arrow")
		bgbrush = cfg.get_brush("MV Background")
		linepen = cfg.get_pen("MV Line")
		dc.SetBackground(bgbrush)
		dc.SetBrush(bgbrush)
		dc.Clear()
		cx,cy = w*0.5,h*0.5
		def get_pixelpos(x,y):
			return cx * (1+x), cy * (1+y)
		dc.SetBrush(arrowbrush)
		dc.SetPen(linepen)
		mplist = [(mp,get_pixelpos(*mp.get_position())) for mp in player.get_plugin_list()]
		
		def draw_line(crx,cry,rx,ry):
			vx, vy = (rx-crx), (ry-cry)
			length = (vx*vx+vy*vy)**0.5
			vx, vy = vx/length, vy/length
			dc.DrawLine(crx,cry,rx,ry)
		def draw_line_arrow(crx,cry,rx,ry):
			vx, vy = (rx-crx), (ry-cry)
			length = (vx*vx+vy*vy)**0.5
			vx, vy = vx/length, vy/length
			dc.DrawLine(crx,cry,rx,ry)
			cpx,cpy = crx + vx * (length * 0.5), cry + vy * (length * 0.5)
			t1 = wx.Point(cpx - vx * 10 + vy * 10, cpy - vy * 10 - vx * 10)
			t2 = wx.Point(cpx + vx * 10, cpy + vy * 10)
			t3 = wx.Point(cpx - vx * 10 - vy * 10, cpy - vy * 10 + vx * 10)
			dc.DrawPolygon([t1,t2,t3])
		
		for mp,(rx,ry) in mplist:
			for conn in mp.get_input_connection_list():
				crx, cry = get_pixelpos(*conn.get_input().get_position())
				draw_line_arrow(crx,cry,rx,ry)
		self.draw_leds()
		dc.SetPen(linepen)
		if self.connecting:
			crx, cry = get_pixelpos(*self.current_plugin.get_position())
			rx,ry= self.connectpos
			draw_line(crx,cry,rx,ry)

__all__ = [
'ParameterDialog',
'AttributesDialog',
'PluginBrowserDialog',
'show_plugin_browser_dialog',
'RoutePanel',
'VolumeSlider',
'RouteView',
]

if __name__ == '__main__':
	import sys, utils
	from main import run
	sys.argv.append(utils.filepath('demosongs/paniq-knark.ccm'))
	run(sys.argv)
