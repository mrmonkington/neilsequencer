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

from gtkimport import gtk
import gobject
import cairo
import pangocairo
from utils import PLUGIN_FLAGS_MASK, ROOT_PLUGIN_FLAGS, GENERATOR_PLUGIN_FLAGS, EFFECT_PLUGIN_FLAGS, CONTROLLER_PLUGIN_FLAGS
from utils import is_effect,is_generator,is_controller,is_root
from utils import prepstr, filepath, db2linear, linear2db, is_debug, filenameify, \
	get_item_count, question, error, new_listview, add_scrollbars, get_clipboard_text, set_clipboard_text, \
	gettext, new_stock_image_button
import config
import zzub
import sys,os
import indexer
import fnmatch
import ctypes
import time
import extman, interface
import Queue
from preset import PresetCollection, Preset
import common
player = common.get_player()
from common import MARGIN, MARGIN2, MARGIN3
from rack import ParameterView
from presetbrowser import PresetView
from patterns import key_to_note

PLUGINWIDTH = 100
PLUGINHEIGHT = 25
LEDWIDTH,LEDHEIGHT = 8,PLUGINHEIGHT-4 # size of LED
LEDOFSX,LEDOFSY = 2,2 # offset of LED
CPUWIDTH,CPUHEIGHT = 8,PLUGINHEIGHT-4 # size of LED
CPUOFSX,CPUOFSY = PLUGINWIDTH-CPUWIDTH-2,2 # offset of LED

ARROWRADIUS = 8

QUANTIZEX = PLUGINWIDTH+ARROWRADIUS*2
QUANTIZEY = PLUGINHEIGHT+ARROWRADIUS*2

VOLBARWIDTH = 32
VOLBARHEIGHT = 128
VOLKNOBHEIGHT = 16

AREA_ANY = 0
AREA_PANNING = 1
AREA_LED = 2

class OscillatorView(gtk.DrawingArea):
	"""
	Oscillator viewer.
	
	Visualizes a pcm wave float buffer.
	"""
	
	def __init__(self, plugin):
		"""
		Initialization.
		"""
		gtk.DrawingArea.__init__(self)
		self.plugin = plugin
		self.x = 0
		self.lastpos = 0
		self.w = 0
		self.sb = Queue.Queue()
		self.drawingarea = self
		gobject.timeout_add(100, self.DrawBuffer)
		gobject.timeout_add(10, self.get_peaks)
		self.left=[]
		self.right=[]
		
	def get_peaks(self):
		self.left.append(self.plugin.get_last_peak()[0])
		self.right.append(self.plugin.get_last_peak()[1])
		return True
		
	def DrawBuffer(self):
		"""
		Overriding a L{Canvas} method that paints onto an offscreen buffer.
		Draws the oscillator graphics.
		"""
		alloc=self.get_allocation()
		w,h = alloc.width, alloc.height
		try:
			gc = self.drawingarea.window.new_gc()
		except:
			return False
		cm = gc.get_colormap()
		drawable = self.drawingarea.window
		drawable.clear()
		h2 = h/2
		h4 = h/4
		h34 = h4*3
		x = 0
		a=0
		while x < w-4:
			try:
				if a+1>(len(self.left)):
					a=0
				l=self.left[a]
				r=self.right[a]
			except:
				l=0
				r=0
			x1 = x
			yl1 = int(-l * h4 + h4)
			yr1 = int(-r * h4 + h34)
			drawable.draw_line(gc, x1, yl1, x1+12, yl1+1)
			drawable.draw_line( gc,x1, yr1, x1+12, yr1+1)
			x += 12
			a +=1
		self.left=[]
		self.right=[]
		return True

class SignalAnalysisDialog(gtk.Dialog):
	"""
	Displays a visualization of plugin traffic.
	"""
	def __init__(self, rootwindow, plugin, parent):
		"""
		Displays Signal Analysis view
		"""
		gtk.Dialog.__init__(self, parent=parent.get_toplevel())
		self.view = parent
		self.plugin = plugin
		self.oscview = OscillatorView(plugin)
		self.vbox.add(self.oscview)		
		self.pluginloader = plugin.get_pluginloader()
		self.resize(300,200)
		self.show_all()

		
class AttributesDialog(gtk.Dialog):
	"""
	Displays plugin atttributes and allows to edit them.
	"""
	
	def __init__(self, plugin, parent):
		"""
		Initializer.
		
		@param plugin: Plugin object.
		@type plugin: wx.Plugin
		"""
		gtk.Dialog.__init__(self,
			"Attributes",
			parent.get_toplevel(),
			gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
			None
		)		
		vbox = gtk.VBox(False, MARGIN)
		vbox.set_border_width(MARGIN)
		self.plugin = plugin
		self.pluginloader = plugin.get_pluginloader()
		self.resize(300,200)
		self.attriblist, self.attribstore, columns = new_listview([
			('Attribute', str),
			('Value', str),
			('Min', str),
			('Max', str),
			('Default', str),
		])
		vbox.add(add_scrollbars(self.attriblist))
		hsizer = gtk.HButtonBox()
		hsizer.set_spacing(MARGIN)
		hsizer.set_layout(gtk.BUTTONBOX_START)
		self.edvalue = gtk.Entry()
		self.edvalue.set_size_request(50,-1)
		self.btnset = gtk.Button("_Set")
		self.btnok = self.add_button(gtk.STOCK_OK, gtk.RESPONSE_OK)
		self.btncancel = self.add_button(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL)
		hsizer.pack_start(self.edvalue, expand=False)
		hsizer.pack_start(self.btnset, expand=False)
		vbox.pack_start(hsizer, expand=False)
		self.attribs = []
		for i in range(self.pluginloader.get_attribute_count()):
			attrib = self.pluginloader.get_attribute(i)
			self.attribs.append(self.plugin.get_attribute_value(i))
			self.attribstore.append([
				prepstr(attrib.get_name()), 
				"%i" % self.plugin.get_attribute_value(i),
				"%i" % attrib.get_value_min(),
				"%i" % attrib.get_value_max(),
				"%i" % attrib.get_value_default(),
			])
		self.btnset.connect('clicked', self.on_set)
		self.connect('response', self.on_ok)
		self.attriblist.get_selection().connect('changed', self.on_attrib_item_focused)
		if self.attribs:
			self.attriblist.grab_focus()
			self.attriblist.get_selection().select_path((0,))
		self.vbox.add(vbox)
		self.show_all()
			
	def get_focused_item(self):
		"""
		Returns the currently focused attribute index.
		
		@return: Index of the attribute currently selected.
		@rtype: int
		"""
		store, rows = self.attriblist.get_selection().get_selected_rows()
		return rows[0][0]
		
	def on_attrib_item_focused(self, selection):
		"""
		Called when an attribute item is being focused.
		
		@param event: Event.
		@type event: wx.Event
		"""
		v = self.attribs[self.get_focused_item()]
		self.edvalue.set_text("%i" % v)
		
	def on_set(self, widget):
		"""
		Called when the "set" button is being pressed.
		"""
		i = self.get_focused_item()
		attrib = self.pluginloader.get_attribute(i)
		try:
			v = int(self.edvalue.get_text())
			assert v >= attrib.get_value_min()
			assert v <= attrib.get_value_max()
		except:
			error(self, "<b><big>The number you entered is invalid.</big></b>\n\nThe number must be in the proper range.")
			return
		self.attribs[i] = v
		iter = self.attribstore.get_iter((i,))
		self.attribstore.set_value(iter, 1, "%i" % v)
			
	def on_ok(self, widget, response):
		"""
		Called when the "ok" or "cancel" button is being pressed.
		"""
		if response == gtk.RESPONSE_OK:
			for i in range(len(self.attribs)):
				self.plugin.set_attribute_value(i, self.attribs[i])

class ParameterDialog(gtk.Dialog):
	"""
	Displays parameter sliders for a plugin in a new Dialog.
	"""
	def __init__(self, rootwindow, plugin, parent):
		gtk.Dialog.__init__(self, parent=parent.get_toplevel())
		self.view = parent
		self.plugin = plugin
		self.view.plugin_dialogs[plugin] = self
		self.paramview = ParameterView(rootwindow, plugin)
		self.set_title(self.paramview.get_title())
		self.vbox.add(self.paramview)
		self.connect('destroy', self.on_destroy)
		self.connect('realize', self.on_realize)
		
	def on_realize(self, widget):
		self.set_default_size(*self.paramview.get_best_size())
		
	def on_destroy(self, event):
		"""
		Handles destroy events.
		"""
		del self.view.plugin_dialogs[self.plugin]
		
		
class PresetDialog(gtk.Dialog):
	"""
	Displays parameter sliders for a plugin in a new Dialog.
	"""
	def __init__(self, rootwindow, plugin, parent):
		gtk.Dialog.__init__(self, parent=parent.get_toplevel())
		self.view = parent
		self.plugin = plugin		
		self.presetview = PresetView(rootwindow, plugin, rootwindow)
		self.set_title(self.presetview.get_title())
		self.vbox.add(self.presetview)		
		self.connect('realize', self.on_realize)
		
	def on_realize(self, widget):
		self.set_default_size(300,500)
		
	
class PluginBrowserDialog(gtk.Dialog):
	"""
	Displays all available plugins and some meta information.
	"""
	def __init__(self, *args, **kwds):
		"""
		Initializer.
		"""
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
			if (pl.get_flags() & PLUGIN_FLAGS_MASK) == GENERATOR_PLUGIN_FLAGS:
				node = generatornode
			elif (pl.get_flags() & PLUGIN_FLAGS_MASK) == EFFECT_PLUGIN_FLAGS:
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
		
class RoutePanel(gtk.VBox):
	"""
	Contains the view panel and manages parameter dialogs.
	"""
	def __init__(self, rootwindow):
		"""
		Initializer.
		
		@param rootwindow: Main window.
		@type rootwindow: wx.Frame
		"""
		self.rootwindow = rootwindow
		gtk.VBox.__init__(self)
		self.view = RouteView(rootwindow, self)
		sizer_2 = gtk.HBox()
		sizer_2.add(self.view)
		self.add(sizer_2)
		
	def reset(self):
		"""
		Resets the router view. Used when
		a new song is being loaded.
		"""
		self.view.reset()
		
	def update_all(self):		
		self.view.redraw()
		
		
class VolumeSlider(gtk.Window):
	"""
	A temporary popup volume control for the router. Can
	only be summoned parametrically and will vanish when the
	left mouse button is being released.
	"""
	def __init__(self):
		"""
		Initializer.
		"""
		self.conn = None
		gtk.Window.__init__(self, gtk.WINDOW_POPUP)
		self.drawingarea = gtk.DrawingArea()
		self.add(self.drawingarea)
		self.drawingarea.add_events(gtk.gdk.ALL_EVENTS_MASK)
		self.drawingarea.set_property('can-focus', True)
		self.resize(VOLBARWIDTH,VOLBARHEIGHT)
		self.hide_all()
		self.drawingarea.connect('motion-notify-event', self.on_motion)
		self.drawingarea.connect('expose-event', self.expose)
		self.drawingarea.connect('button-release-event', self.on_left_up)
		
	def expose(self, widget, event):
		self.draw()
		return False
		
	def redraw(self):
		if self.window:
			rect = self.drawingarea.get_allocation()
			self.drawingarea.window.invalidate_rect((0,0,rect.width, rect.height), False)

	def on_motion(self, widget, event):
		"""
		Event handler for mouse movements.
		"""
		x,y,state = self.drawingarea.window.get_pointer()
		newpos = int(y)
		delta = newpos - self.y
		if delta == 0:
			return
		self.y = newpos
		self.amp = max(min(self.amp + (float(delta) / VOLBARHEIGHT), 1.0), 0.0)
		amp = min(max(int(db2linear(self.amp * -48.0, -48.0) * 16384.0), 0), 16384)
		self.conn.get_audio_connection().set_amplitude(amp)
		self.redraw()
		return True
		
	def draw(self):
		"""
		Event handler for paint requests.
		"""
		gc = self.drawingarea.window.new_gc()
		cm = gc.get_colormap()
		drawable = self.drawingarea.window

		rect = self.drawingarea.get_allocation()
		w,h = rect.width, rect.height

		cfg = config.get_config()
		whitebrush = cm.alloc_color(cfg.get_color('MV Amp BG'))
		blackbrush = cm.alloc_color(cfg.get_color('MV Amp Handle'))
		outlinepen = cm.alloc_color(cfg.get_color('MV Amp Border'))
		
		gc.set_foreground(whitebrush)
		drawable.draw_rectangle(gc, True, 0, 0, w, h)
		gc.set_foreground(outlinepen)
		drawable.draw_rectangle(gc, False, 0, 0, w-1, h-1)
		
		if self.conn:
			gc.set_foreground(blackbrush)
			pos = int(self.amp * (VOLBARHEIGHT - VOLKNOBHEIGHT))
			drawable.draw_rectangle(gc, True, 1, pos+1, VOLBARWIDTH-2, VOLKNOBHEIGHT-2)
		
	def display(self, (mx,my), conn, index):
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
		self.amp = (linear2db((self.conn.get_audio_connection().get_amplitude()/ 16384.0), -48.0) / -48.0)
		self.move(int(mx - VOLBARWIDTH*0.5), int(my - VOLBARHEIGHT*0.5))
		self.show_all()
		self.drawingarea.grab_add()
		
	def on_left_up(self, widget, event):
		"""
		Event handler for left mouse button releases. Will
		hide control.
		
		@param event: Mouse event.
		@type event: wx.MouseEvent
		"""
		self.hide_all()
		self.drawingarea.grab_remove()

class RouteView(gtk.DrawingArea):
	"""
	Allows to monitor and control plugins and their connections.
	"""	
	current_plugin = None
	connecting = False
	dragging = False
	dragoffset = 0,0
	contextmenupos = 0,0
	
	DRAW_ALL = 1
	DRAW_LEDS = 2
	
	drawrequest = 0
	
	# 0 = default
	# 1 = muted
	# 2 = led off
	# 3 = led on
	# 4 = led border
	# 5 = led warning
	COLOR_DEFAULT = 0
	COLOR_MUTED = 1
	COLOR_LED_OFF = 2
	COLOR_LED_ON = 3
	COLOR_LED_BORDER = 4
	COLOR_LED_WARNING = 5
	COLOR_CPU_OFF = 6
	COLOR_CPU_ON = 7
	COLOR_CPU_BORDER = 8
	COLOR_CPU_WARNING = 9
	
	def __init__(self, rootwindow, parent):
		"""
		Initializer.
		
		@param rootwindow: Main window.
		@type rootwindow: AldrinFrame
		"""
		self.plugin_dialogs = {}		
		self.panel = parent
		self.rootwindow = rootwindow
		self.rootwindow.event_handlers.append(self.on_player_callback)
		self.solo_plugin = None
		self.selected_plugin = None
		self.autoconnect_target=None
		self.chordnotes=[]
		self.update_colors()
		gtk.DrawingArea.__init__(self)
		self.volume_slider = VolumeSlider()		
		self.plugin_tree = indexer.parse_index(player, config.get_config().get_index_path())
		self.add_events(gtk.gdk.ALL_EVENTS_MASK)
		self.set_property('can-focus', True)
		self.connect('button-press-event', self.on_left_down)
		self.connect('button-release-event', self.on_left_up)
		self.connect('motion-notify-event', self.on_motion)
		self.connect("expose_event", self.expose)
		self.connect('key-press-event', self.on_key_jazz, None)	
		self.connect('key-release-event', self.on_key_jazz_release, None)		
		self.connect('visibility-notify-event', self.on_visibility_notify)
		gobject.timeout_add(100, self.on_draw_led_timer)
		#~ wx.EVT_SET_FOCUS(self, self.on_focus)
		
	def on_visibility_notify(self, widget, event):
		self.drawrequest = self.DRAW_ALL | self.DRAW_LEDS
		
	def update_colors(self):
		"""
		Updates the routers color scheme.
		"""
		cfg = config.get_config()
		flags2brushnames = {
			ROOT_PLUGIN_FLAGS: ('MV Master', 'MV Master', 'MV Master LED Off', 'MV Master LED On', 'MV Master LED Border', 'MV Machine LED Warning', 'MV CPU LED Off', 'MV CPU LED On', 'MV CPU LED Border', 'MV CPU LED Warning'),
			GENERATOR_PLUGIN_FLAGS: ('MV Generator', 'MV Generator Mute', 'MV Generator LED Off', 'MV Generator LED On', 'MV Generator LED Border', 'MV Machine LED Warning', 'MV CPU LED Off', 'MV CPU LED On', 'MV CPU LED Border', 'MV CPU LED Warning'),
			EFFECT_PLUGIN_FLAGS: ('MV Effect', 'MV Effect Mute', 'MV Effect LED Off', 'MV Effect LED On', 'MV Effect LED Border', 'MV Machine LED Warning', 'MV CPU LED Off', 'MV CPU LED On', 'MV CPU LED Border', 'MV CPU LED Warning'),
			CONTROLLER_PLUGIN_FLAGS: ('MV Controller', 'MV Controller Mute', 'MV Controller LED Off', 'MV Controller LED On', 'MV Controller LED Border', 'MV Machine LED Warning', 'MV CPU LED Off', 'MV CPU LED On', 'MV CPU LED Border', 'MV CPU LED Warning'),
		}
		self.flags2brushes = {}
		for flags,names in flags2brushnames.iteritems():
			brushes = []
			for name in names:
				brushes.append(cfg.get_color(name))
			self.flags2brushes[flags] = brushes
		common.get_plugin_infos().reset_plugingfx()
		
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
				dlg.destroy()
		elif data.type == zzub.zzub_event_type_disconnect:
			self.redraw()
		elif data.type == zzub.zzub_event_type_connect:
			self.redraw()

	def reset(self):
		"""
		Destroys all parameter dialogs. Used when
		a new song is being loaded.
		"""
		for dlg in self.plugin_dialogs.values():
			dlg.destroy()
		self.solo_plugin = None
		self.selected_plugin = None
		self.plugin_dialogs = {}
			
	def on_focus(self, event):
		self.redraw()
	
	def on_popup_rename(self, widget, mp):
		text = gettext(self, "Enter new plugin name:", prepstr(mp.get_name()))
		if text:
			mp.set_name(text)
			common.get_plugin_infos().get(mp).reset_plugingfx()
			self.redraw()
	
	def solo(self, plugin):		
		if not plugin or plugin == self.solo_plugin:
			# soloing deactived so apply muted states
			self.solo_plugin = None			
			for plugin, info in common.get_plugin_infos().iteritems():
				plugin.set_mute(info.muted)
				info.reset_plugingfx()
		elif is_generator(plugin):
			# mute all plugins except solo plugin
			self.solo_plugin = plugin			
			for plugin, info in common.get_plugin_infos().iteritems():				
				if plugin != self.solo_plugin and is_generator(plugin):					
					plugin.set_mute(True)
					info.reset_plugingfx()
				elif plugin == self.solo_plugin:
					plugin.set_mute(info.muted)
					info.reset_plugingfx()
	
	def on_popup_solo(self, widget, mp):
		"""
		Event handler for the "Mute" context menu option.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		"""		
		if self.solo_plugin != mp:
			self.solo(mp)
		else:
			self.solo(None)		
		self.redraw()
	
	def toggle_mute(self, plugin):
		pi = common.get_plugin_infos().get(plugin)
		pi.muted = not pi.muted
		# make sure a machine muted by solo is not unmuted manually
		if not self.solo_plugin or plugin == self.solo_plugin or is_effect(plugin):
			plugin.set_mute(pi.muted)
		pi.reset_plugingfx()
	
	def on_popup_mute(self, widget, mp):
		"""
		Event handler for the "Mute" context menu option.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		"""
		self.toggle_mute(mp)		
		self.redraw()
		
	def on_popup_delete(self, widget, mp):
		"""
		Event handler for the "Delete" context menu option.
		"""
		res = question(self, "<b><big>Remove plugin?</big></b>\n\nThis action can not be reversed.", allowcancel = False)
		if res == gtk.RESPONSE_YES:
			inplugs = []
			outplugs = []
			# record all connections
			while True:
				conns = mp.get_input_connection_list()
				if not conns:
					break
				conn = conns.pop()
				input = conn.get_input()
				for i in range(conn.get_output().get_input_connection_count()):
						if conn.get_output().get_input_connection(i)==conn:
							break
				try:
					aconn = conn.get_audio_connection()
					amp = aconn.get_amplitude()
					pan = aconn.get_panning()
					inplugs.append((input,amp,pan))
				except:
					import traceback
					print traceback.format_exc()
				mp.delete_input(input)
			while True:
				conns = mp.get_output_connection_list()
				if not conns:
					break
				conn = conns.pop()
				output = conn.get_output()
				for i in range(conn.get_output().get_input_connection_count()):
						if conn.get_output().get_input_connection(i)==conn:
							break
				try:
					aconn = conn.get_audio_connection()
					amp = aconn.get_amplitude()
					pan = aconn.get_panning()
					outplugs.append((output,amp,pan))
				except:
					import traceback
					print traceback.format_exc()
				output.delete_input(mp)
			# and now restore them
			for inplug,iamp,ipan in inplugs:
				for outplug,oamp,opan in outplugs:
					newamp = (iamp*oamp)/16384
					newpan = ipan
					outplug.add_audio_input(inplug, newamp, newpan)
			del common.get_plugin_infos()[mp]
			mp.destroy()
			self.redraw()
		
	def on_popup_disconnect(self, widget, conn):
		"""
		Event handler for the "Disconnect" context menu option.
		
		@param event: Menu event.
		@type event: wx.MenuEvent
		"""
		conn.get_output().delete_input(conn.get_input())
		self.redraw()
		
	def show_parameter_dialog(self, plugin):
		"""
		Shows a parameter dialog for a plugin.
		
		@param plugin: Plugin instance.
		@type plugin: Plugin
		"""
		dlg = self.plugin_dialogs.get(plugin,None)
		if not dlg:
			dlg = ParameterDialog(self.rootwindow, plugin, self)
		dlg.show_all()

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
		self.show_parameter_dialog(mp)
		
	def on_popup_new_plugin(self, widget, pluginloader, kargs={}):
		"""
		Event handler for "new plugin" context menu options.
		"""
		basename = pluginloader.get_short_name()
		name = pluginloader.get_short_name()
		basenumber = 2
		mask=gtk.get_current_event_state()
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
		mp = player.create_plugin(None, 0, name, pluginloader)
		assert mp._handle
		if ((mp.get_flags() & PLUGIN_FLAGS_MASK) == GENERATOR_PLUGIN_FLAGS) and \
			(pluginloader.get_parameter_count(1) or pluginloader.get_parameter_count(2)):
			pattern = mp.create_pattern(self.rootwindow.seqframe.view.step)
			pattern.set_name('00')
			seq = player.get_current_sequencer()
			t=seq.create_track(mp)
			t.set_event(0,16)
			if not(mask & gtk.gdk.SHIFT_MASK):
				if self.autoconnect_target==None:
					player.get_plugin_list()[0].add_audio_input(mp, 16384, 16384)
				else:
					self.autoconnect_target.add_audio_input(mp, 16384, 16384)
		mp.set_position(*self.pixel_to_float(self.contextmenupos))
		# if we have a context plugin, prepend connections
		if 'plugin' in kargs:
			plugin = kargs['plugin']
			inplugs = []
			# record all connections
			while True:
				conns = plugin.get_input_connection_list()
				if not conns:
					break
				conn = conns.pop()
				input = conn.get_input()
				for i in range(conn.get_output().get_input_connection_count()):
						if conn.get_output().get_input_connection(i)==conn:
							break
				try:
					aconn = conn.get_audio_connection()
					amp = aconn.get_amplitude()
					pan = aconn.get_panning()
					inplugs.append((input,amp,pan))
				except:
					import traceback
					print traceback.format_exc()
				plugin.delete_input(input)
			# restore
			for inplug,amp,pan in inplugs:
				mp.add_audio_input(inplug, amp, pan)
			plugin.add_audio_input(mp, 16384, 16384)
		# if we have a context connection, replace that one
		elif 'conn' in kargs:
			conn = kargs['conn']
			for i in range(conn.get_output().get_input_connection_count()):
					if conn.get_output().get_input_connection(i)==conn:
						break
			try:
				aconn = conn.get_audio_connection()
				amp = aconn.get_amplitude()
				pan = aconn.get_panning()
				minput = conn.get_input()
				moutput = conn.get_output()
				moutput.delete_input(minput)
				mp.add_audio_input(minput, amp, pan)
				moutput.add_audio_input(mp, 16384, 16384)
			except:
				import traceback
				print traceback.format_exc()
		self.rootwindow.document_changed()
		# selects the plugin in the pattern view
		plugintoolbar = self.rootwindow.patternframe.toolbar
		plugintoolbar.select_plugin(get_item_count(plugintoolbar.pluginselect.get_model())-1)
		# add plugin information
		common.get_plugin_infos().add_plugin(mp)
		# open parameter view if its an effect
		if is_effect(mp):
			self.show_parameter_dialog(mp)
		#tab to first note column on adding a new machine
		patternframe=self.rootwindow.patternframe.view
		if patternframe.move_track_right():
				patternframe.set_index(0)
				patternframe.set_subindex(0)
				patternframe.show_cursor_right()
				patternframe.refresh_view()
		
	def get_plugin_menu(self, include_generators = True, include_effects = True, include_controllers = True, **kargs):
		"""
		Generates and returns a new plugin menu.
		
		@return: A menu containing commands to instantiate new plugins.
		@rtype: wx.Menu
		"""
		cfg = config.get_config()
		def make_submenu_item(submenu, name):
			item = gtk.MenuItem(label=name)
			item.set_submenu(submenu)
			return item
		def make_menu_item(label, desc, func, *args):
			item = gtk.ImageMenuItem(stock_id=label)
			if func:
				item.connect('activate', func, *args)
			return item
		def fill_menu(menu,node):
			add_separator = False
			for child in node.children:
				if isinstance(child, indexer.Directory) and not child.is_empty():
					if add_separator:
						add_separator = False
						if menu.get_children():
							menu.append(gtk.SeparatorMenuItem())
					submenu = gtk.Menu()
					fill_menu(submenu, child)
					menu.append(make_submenu_item(submenu, prepstr(child.name)))
				elif isinstance(child, indexer.Reference):
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
							menu.append(gtk.SeparatorMenuItem())
					item = make_menu_item(prepstr(child.name), "", self.on_popup_new_plugin, child.pluginloader, kargs)
					if child.icon:
						iconpath = cfg.get_plugin_icon_path(child.icon)
						if iconpath:
							image = gtk.Image()
							image.set_from_pixbuf(gtk.gdk.pixbuf_new_from_file(iconpath))
							item.set_image(image)
					if not child.pluginloader:
						item.set_sensitive(False)
					menu.append(item)
				elif isinstance(child, indexer.Separator):
					add_separator = True
		plugin_menu = gtk.Menu()
		fill_menu(plugin_menu, self.plugin_tree)
		plugin_menu.append(gtk.SeparatorMenuItem())
		plugin_menu.append(make_menu_item("Unmute All", "", self.on_popup_unmute_all))
		return plugin_menu
	
	def on_popup_unmute_all(self, widget):
		"""
		Event handler for unmute all menu option
		"""
		for mp in reversed(player.get_plugin_list()):
			info = common.get_plugin_infos().get(mp)
			info.muted=False
			mp.set_mute(info.muted)
			info.reset_plugingfx()
		
	def on_popup_command(self, widget, plugin, subindex, index):
		"""
		Event handler for plugin commands
		"""
		#need to implement this!
		pass
	
	def on_popup_set_target(self, widget, plugin):
		"""
		Event handler for menu option to set machine as target for default connection
		"""
		self.autoconnect_target = plugin
	
	def on_context_menu(self, widget, event):
		"""
		Event handler for requests to show the context menu.
		
		@param event: event.
		@type event: wx.Event
		"""
		mx,my = int(event.x), int(event.y)
		self.contextmenupos = mx,my
		menu = gtk.Menu()
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
		res = self.get_plugin_at((mx,my))
		if res:
			mp,(x,y),area = res
			menu.append(make_check_item(common.get_plugin_infos().get(mp).muted, "_Mute", "Toggle Bypass", self.on_popup_mute, mp))
			if is_generator(mp):
				menu.append(make_check_item(self.solo_plugin == mp, "_Solo", "Toggle Solo", self.on_popup_solo, mp))
			menu.append(gtk.SeparatorMenuItem())
			menu.append(make_menu_item("_Parameters...", "View parameters", self.on_popup_show_params, mp))
			menu.append(make_menu_item("_Attributes...", "Show Attributes", self.on_popup_show_attribs, mp))
			menu.append(make_menu_item("P_resets...", "Manage presets", self.on_popup_show_presets, mp))
			menu.append(gtk.SeparatorMenuItem())
			menu.append(make_menu_item("_Rename...", "", self.on_popup_rename, mp))
			if not is_root(mp):
				menu.append(make_menu_item("_Delete", "Delete plugin", self.on_popup_delete, mp))
			if is_effect(mp) or is_root(mp):
				menu.append(gtk.SeparatorMenuItem())
				menu.append(make_check_item(self.autoconnect_target == mp, "Default Target","Connect new generators to this plugin",self.on_popup_set_target, mp))
				menu.append(make_submenu_item(self.get_plugin_menu(include_generators=False, include_controllers=False, plugin=mp), "_Prepend Effect"))
			commands = mp.get_commands()
			if commands:
				menu.append(gtk.SeparatorMenuItem())
				submenuindex = 0
				for index in range(len(commands)):
					cmd = commands[index]
					if cmd.startswith('/'):
						submenu = gtk.Menu()
						subcommands = mp.get_sub_commands(index)
						submenuindex += 1
						for subindex in range(len(subcommands)):
							subcmd = subcommands[subindex]
							submenu.append(make_menu_item(prepstr(subcmd), "", self.on_popup_command, mp, submenuindex, subindex))
						menu.append(make_submenu_item(submenu, prepstr(cmd[1:])))
					else:
						menu.append(make_menu_item(prepstr(cmd), "", self.on_popup_command, mp, 0, index))
			extman.get_extension_manager().extend_menu(interface.UIOBJECT_ROUTE_MENU_PLUGIN, menu, plugin=mp)
		else:
			try:
				conn, index = self.get_connection_at((mx,my))
			except TypeError:
				conn = None
			if conn:
				menu.append(make_menu_item("_Disconnect plugins", "Disconnect plugins", self.on_popup_disconnect, conn))
				if conn.get_type() == zzub.zzub_connection_type_audio:
					menu.append(gtk.SeparatorMenuItem())
					menu.append(make_submenu_item(self.get_plugin_menu(include_generators=False,include_controllers=False, conn=conn), "_Insert Effect"))
					menu.append(gtk.SeparatorMenuItem())
					menu.append(make_menu_item("_Signal Analysis", "Signal Analysis", self.on_popup_show_signalanalysis, conn))
				elif conn.get_type() == zzub.zzub_connection_type_event:
					menu.append(gtk.SeparatorMenuItem())
					mi = conn.get_input()
					for param in mi.get_pluginloader().get_parameter_list(3):
						print param
				extman.get_extension_manager().extend_menu(interface.UIOBJECT_ROUTE_MENU_CONNECTION, menu, connection=conn)
			else:
				menu = self.get_plugin_menu()
				extman.get_extension_manager().extend_menu(interface.UIOBJECT_ROUTE_MENU, menu)
		
		menu.show_all()
		menu.attach_to_widget(self, None)
		menu.popup(None, None, None, event.button, event.time)
		
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
		rect = self.get_allocation()
		w,h = rect.width, rect.height
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
		rect = self.get_allocation()
		w,h = rect.width, rect.height
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
		rect = self.get_allocation()
		w,h = rect.width, rect.height
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
					for i in range(conn.get_output().get_input_connection_count()):
						if conn.get_output().get_input_connection(i)==conn:
							break
					return conn, i
		
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
		rect = self.get_allocation()
		w,h = rect.width, rect.height
		cx,cy = w*0.5, h * 0.5
		mx, my = x,y
		PW, PH = PLUGINWIDTH / 2, PLUGINHEIGHT / 2
		area = AREA_ANY
		for mp in reversed(player.get_plugin_list()):
			x,y = mp.get_position()
			x,y = int(cx * (1+x)), int(cy * (1+y))
			if (mx >= (x - PW)) and (mx <= (x + PW)) and (my >= (y - PH)) and (my <= (y + PH)):
				if sum(tuple(gtk.gdk.Rectangle(x-PW+LEDOFSX,y-PH+LEDOFSY,LEDWIDTH,LEDHEIGHT).intersect((mx,my,1,1)))):
					area = AREA_LED
				return mp,(x,y),area
				
	def on_left_dclick(self, widget, event):
		"""
		Event handler for left doubleclicks. If the doubleclick
		hits a plugin, the parameter window is being shown.
		"""
		mx,my = int(event.x), int(event.y)
		res = self.get_plugin_at((mx,my))
		if not res:
		#	pl = show_plugin_browser_dialog(self)
		#	if pl:
		#		self.contextmenupos = mx,my
		#		self.on_popup_new_plugin(None, pl)
			return
		mp,(x,y),area = res
		if area == AREA_ANY:
			data = zzub.zzub_event_data_t()
			data.type = zzub.zzub_event_type_double_click;
			if mp.invoke_event(data, 1) != 0:
				self.show_parameter_dialog(mp)
		
	def on_left_down(self, widget, event):
		"""
		Event handler for left mouse button presses. Initiates
		plugin dragging or connection volume adjustments.
		
		@param event: Mouse event.
		@type event: wx.MouseEvent
		"""
		if (event.button == 3):
			return self.on_context_menu(widget, event)
		if not event.button in (1,2):
			return
		if (event.button == 1) and (event.type == gtk.gdk._2BUTTON_PRESS):
			return self.on_left_dclick(widget, event)
		mx,my = int(event.x), int(event.y)
		res = self.get_plugin_at((mx,my))
		if res:
			self.current_plugin,(x,y),area = res
			self.dragoffset = mx-x, my-y
			if area == AREA_LED:
				self.toggle_mute(self.current_plugin)		
				self.redraw()
			elif (event.state & gtk.gdk.SHIFT_MASK) or (event.button == 2):
				if is_controller(self.current_plugin):
					pass
				else:
					self.connecting = True
					self.connectpos = int(mx), int(my)
			elif not self.connecting:
				self.dragging = True
				self.grab_add()
			last = self.selected_plugin
			self.selected_plugin = self.current_plugin
			player.set_midi_plugin(self.current_plugin)
			if self.selected_plugin:
				i=0
				for mp in player.get_plugin_list():
					if self.selected_plugin==mp:
						break
					i+=1
				if hasattr(self.rootwindow,'patternframe'):
					self.rootwindow.patternframe.toolbar.pluginselect.set_active(i)
				common.get_plugin_infos().get(self.selected_plugin).reset_plugingfx()									
			if last:
				common.get_plugin_infos().get(last).reset_plugingfx()
			if hasattr(self.rootwindow, 'select_page'):
				self.rootwindow.select_page(self.rootwindow.PAGE_ROUTE)
		else:
			try:
				conn, index = self.get_connection_at((mx,my))
			except TypeError:
				conn=None
			if conn:
				ox, oy = self.window.get_origin()
				if conn.get_type() == zzub.zzub_connection_type_audio:
					self.volume_slider.display((ox+mx,oy+my), conn, index)
				elif conn.get_type() == zzub.zzub_connection_type_event:
					# no idea what to do when clicking on an event connection yet
					pass
			else:
				if self.selected_plugin:
					last = self.selected_plugin
					self.selected_plugin = None
					common.get_plugin_infos().get(last).reset_plugingfx()
		
	def on_motion(self, widget, event):
		"""
		Event handler for mouse movements.
		
		@param event: Mouse event.
		@type event: wx.MouseEvent
		"""
		x,y,state = self.window.get_pointer()
		if self.dragging:
			ox,oy = self.dragoffset
			mx,my = int(x), int(y)
			size = self.get_allocation()
			x,y = max(0, min(mx - ox, size.width)), max(0, min(my - oy, size.height))
			if (event.state & gtk.gdk.CONTROL_MASK):
				# quantize position
				x = int(float(x)/QUANTIZEX + 0.5) * QUANTIZEX
				y = int(float(y)/QUANTIZEY + 0.5) * QUANTIZEY
			self.current_plugin.set_position(*self.pixel_to_float((x,y)))
			self.redraw()
		elif self.connecting:
			self.connectpos = int(x), int(y)
			self.redraw()
		return True
		
	def on_left_up(self, widget, event):
		"""
		Event handler for left mouse button releases.
		
		@param event: Mouse event.
		@type event: wx.MouseEvent
		"""
		if self.dragging:
			self.dragging = False
			self.grab_remove()
		mx,my = int(event.x), int(event.y)
		if self.connecting:
			res = self.get_plugin_at((mx,my))
			if res:
				mp,(x,y),area = res
				if is_controller(self.current_plugin):
					#mp.add_event_input(self.current_plugin)
					pass
				else:
					mp.add_audio_input(self.current_plugin, 0x4000, 0x4000)
		self.current_plugin = None
		self.connecting = False
		self.redraw()
		
	def update_all(self):
		self.update_colors()
	
	def on_draw_led_timer(self):
		"""
		Timer event that only updates the plugin leds.
		"""
		if self.rootwindow.index != self.rootwindow.PAGE_ROUTE:
			return True
		if self.window:
			self.drawrequest |= self.DRAW_LEDS
			rect = self.get_allocation()
			w,h = rect.width, rect.height
			cx,cy = w*0.5,h*0.5
			def get_pixelpos(x,y):
				return cx * (1+x), cy * (1+y)
			PW, PH = PLUGINWIDTH / 2, PLUGINHEIGHT / 2
			for mp,(rx,ry) in ((mp,get_pixelpos(*mp.get_position())) for mp in player.get_plugin_list()):
				rx,ry = rx - PW, ry - PH
				self.window.invalidate_rect((int(rx),int(ry),PLUGINWIDTH,PLUGINHEIGHT), False)
		return True
			
	def expose(self, widget, event):
		self.context = widget.window.cairo_create()
		self.draw(self.context)
		return False
		
	def redraw(self):
		if self.window:
			self.drawrequest |= self.DRAW_ALL | self.DRAW_LEDS
			rect = self.get_allocation()
			self.window.invalidate_rect((0,0,rect.width,rect.height), False)
		
	def draw_leds(self):
		"""
		Draws only the leds into the offscreen buffer.
		"""
		gc = self.window.new_gc()
		cm = gc.get_colormap()
		cfg = config.get_config()
		rect = self.get_allocation()
		import pango
		layout = pango.Layout(self.get_pango_context())
		#~ layout.set_font_description(self.fontdesc)
		layout.set_width(-1)
		w,h = rect.width,rect.height
		cx,cy = w*0.5,h*0.5
		def get_pixelpos(x,y):
			return cx * (1+x), cy * (1+y)
		textcolor = cm.alloc_color(cfg.get_color("MV Machine Text"))
		pluginpen = cm.alloc_color(cfg.get_color("MV Machine Border"))
		pluginpenselected = cm.alloc_color(cfg.get_color("MV Machine Border Selected"))
		#font = wx.Font(8, wx.FONTFAMILY_DEFAULT, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_BOLD)
		PW, PH = PLUGINWIDTH / 2, PLUGINHEIGHT / 2

		pluginlist = player.get_plugin_list()

		cpu = min(player.audiodriver_get_cpu_load(), 100.0)
		cpu_loads = {}
		biggestload = 0
		try:
			for mp in pluginlist:
				cpuload = mp.get_last_worktime()
				biggestload = max(biggestload, cpuload)
				cpu_loads[mp] = cpuload
		except:
			import traceback
			traceback.print_exc()
		total_workload = max(sum(cpu_loads.values()),0.001)
		cputreshold = 0.75
	
		for mp,(rx,ry) in ((mp,get_pixelpos(*mp.get_position())) for mp in pluginlist):
			rx,ry = rx - PW, ry - PH
			pi = common.get_plugin_infos().get(mp)
			if not pi:
				continue			
			if not pi.plugingfx:
				pi.plugingfx = gtk.gdk.Pixmap(self.window, PLUGINWIDTH, PLUGINHEIGHT, -1)
				# adjust colour for muted plugins
				if pi.muted:
					gc.set_foreground(cm.alloc_color(self.flags2brushes.get(mp.get_flags() & PLUGIN_FLAGS_MASK, self.flags2brushes[GENERATOR_PLUGIN_FLAGS])[self.COLOR_MUTED]))
				else:
					gc.set_foreground(cm.alloc_color(self.flags2brushes.get(mp.get_flags() & PLUGIN_FLAGS_MASK, self.flags2brushes[GENERATOR_PLUGIN_FLAGS])[self.COLOR_DEFAULT]))
				pi.plugingfx.draw_rectangle(gc, True, -1,-1,PLUGINWIDTH+1,PLUGINHEIGHT+1)
				if mp == self.selected_plugin:
					gc.set_foreground(pluginpenselected)
				else:
					gc.set_foreground(pluginpen)
				pi.plugingfx.draw_rectangle(gc, False, 0, 0, PLUGINWIDTH-1,PLUGINHEIGHT-1)
				if self.solo_plugin and self.solo_plugin != mp and is_generator(mp):
					title = prepstr('[' + mp.get_name() + ']')
				elif pi.muted:
					title = prepstr('(' + mp.get_name() + ')')
				else:
					title = prepstr(mp.get_name())
				layout.set_text(title)
				lw,lh = layout.get_pixel_size()
				gc.set_foreground(textcolor)
				pi.plugingfx.draw_layout(gc, PLUGINWIDTH/2 - lw/2, PLUGINHEIGHT/2 - lh/2, layout)
				
			maxl, maxr = mp.get_last_peak()
			amp = min(max(maxl,maxr),1.0)
			if amp != pi.amp:
				if amp >= 1:
					gc.set_foreground(cm.alloc_color(self.flags2brushes.get(mp.get_flags() & PLUGIN_FLAGS_MASK, self.flags2brushes[GENERATOR_PLUGIN_FLAGS])[self.COLOR_LED_WARNING]))
					pi.plugingfx.draw_rectangle(gc, True, LEDOFSX, LEDOFSY, LEDWIDTH-1, LEDHEIGHT-1)
				else:
					gc.set_foreground(cm.alloc_color(self.flags2brushes.get(mp.get_flags() & PLUGIN_FLAGS_MASK, self.flags2brushes[GENERATOR_PLUGIN_FLAGS])[self.COLOR_LED_OFF]))
					pi.plugingfx.draw_rectangle(gc, True, LEDOFSX, LEDOFSY, LEDWIDTH-1, LEDHEIGHT-1)
					amp = 1.0 - (linear2db(amp,-76.0)/-76.0)
					height = int((LEDHEIGHT-4)*amp + 0.5)
					if (height > 0):
						gc.set_foreground(cm.alloc_color(self.flags2brushes.get(mp.get_flags() & PLUGIN_FLAGS_MASK, self.flags2brushes[GENERATOR_PLUGIN_FLAGS])[self.COLOR_LED_ON]))
						pi.plugingfx.draw_rectangle(gc, True, LEDOFSX+2, LEDOFSY+LEDHEIGHT-height-2, LEDWIDTH-4, height)
				gc.set_foreground(cm.alloc_color(self.flags2brushes.get(mp.get_flags() & PLUGIN_FLAGS_MASK, self.flags2brushes[GENERATOR_PLUGIN_FLAGS])[self.COLOR_LED_BORDER]))
				pi.plugingfx.draw_rectangle(gc, False, LEDOFSX, LEDOFSY, LEDWIDTH-1, LEDHEIGHT-1)
				pi.amp = amp

			try:
				relperc = max(((cpu_loads[mp]*cpu) / (biggestload*100))*0.1 + pi.cpu*0.9, 0.0)
			except ZeroDivisionError:
				relperc = max(pi.cpu, 0.0)
			if relperc != pi.cpu:
				pi.cpu = relperc
				gc.set_foreground(cm.alloc_color(self.flags2brushes.get(mp.get_flags() & PLUGIN_FLAGS_MASK, self.flags2brushes[GENERATOR_PLUGIN_FLAGS])[self.COLOR_CPU_OFF]))
				pi.plugingfx.draw_rectangle(gc, True, CPUOFSX, CPUOFSY, CPUWIDTH-1, CPUHEIGHT-1)
				height = int((CPUHEIGHT-4)*relperc + 0.5)
				if (height > 0):
					if relperc >= cputreshold:
						gc.set_foreground(cm.alloc_color(self.flags2brushes.get(mp.get_flags() & PLUGIN_FLAGS_MASK, self.flags2brushes[GENERATOR_PLUGIN_FLAGS])[self.COLOR_CPU_WARNING]))
					else:
						gc.set_foreground(cm.alloc_color(self.flags2brushes.get(mp.get_flags() & PLUGIN_FLAGS_MASK, self.flags2brushes[GENERATOR_PLUGIN_FLAGS])[self.COLOR_CPU_ON]))
					pi.plugingfx.draw_rectangle(gc, True, CPUOFSX+2, CPUOFSY+CPUHEIGHT-height-2, CPUWIDTH-4, height)
				gc.set_foreground(cm.alloc_color(self.flags2brushes.get(mp.get_flags() & PLUGIN_FLAGS_MASK, self.flags2brushes[GENERATOR_PLUGIN_FLAGS])[self.COLOR_CPU_BORDER]))
				pi.plugingfx.draw_rectangle(gc, False, CPUOFSX, CPUOFSY, CPUWIDTH-1, CPUHEIGHT-1)
			
			self.window.draw_drawable(gc, pi.plugingfx, 0, 0, int(rx), int(ry), -1, -1)
		
	def draw(self, ctx):
		"""
		Draws plugins, connections and arrows to an offscreen buffer.
		"""
		cfg = config.get_config()
		rect = self.get_allocation()
		w,h = rect.width,rect.height
		arrowcolors = {
			zzub.zzub_connection_type_audio : cfg.get_float_color("MV Arrow"),
			zzub.zzub_connection_type_event : cfg.get_float_color("MV Controller Arrow")
		}
		bgbrush = cfg.get_float_color("MV Background")
		linepen = cfg.get_float_color("MV Line")
		ctx.translate(0.5,0.5)
		ctx.set_source_rgb(*bgbrush)
		ctx.rectangle(0,0,w,h)
		ctx.fill()
		ctx.set_line_width(1)
		
		cx,cy = w*0.5,h*0.5
		def get_pixelpos(x,y):
			return cx * (1+x), cy * (1+y)
		mplist = [(mp,get_pixelpos(*mp.get_position())) for mp in player.get_plugin_list()]
		
		def draw_line(crx,cry,rx,ry):
			vx, vy = (rx-crx), (ry-cry)
			length = (vx*vx+vy*vy)**0.5
			vx, vy = vx/length, vy/length
			ctx.move_to(crx,cry)
			ctx.line_to(rx,ry)
			ctx.set_source_rgb(*linepen)
			ctx.stroke()
		def draw_line_arrow(clr,crx,cry,rx,ry):
			vx, vy = (rx-crx), (ry-cry)
			length = (vx*vx+vy*vy)**0.5
			if not length:
				return
			vx, vy = vx/length, vy/length
			ctx.move_to(crx,cry)
			ctx.line_to(rx,ry)
			ctx.set_source_rgb(*linepen)
			ctx.stroke()
			cpx,cpy = crx + vx * (length * 0.5), cry + vy * (length * 0.5)
			t1 = (int(cpx - vx * ARROWRADIUS + vy * ARROWRADIUS), int(cpy - vy * ARROWRADIUS - vx * ARROWRADIUS))
			t2 = (int(cpx + vx * ARROWRADIUS), int(cpy + vy * ARROWRADIUS))
			t3 = (int(cpx - vx * ARROWRADIUS - vy * ARROWRADIUS), int(cpy - vy * ARROWRADIUS + vx * ARROWRADIUS))
			ctx.move_to(*t1)
			ctx.line_to(*t2)
			ctx.line_to(*t3)
			ctx.close_path()
			ctx.set_source_rgb(*clr)
			ctx.fill_preserve()
			ctx.set_source_rgb(*linepen)
			ctx.stroke()
		
		if self.drawrequest & self.DRAW_ALL:
			self.drawrequest = self.drawrequest ^ (self.drawrequest & self.DRAW_ALL)
			for mp,(rx,ry) in mplist:
				for conn in mp.get_input_connection_list():
					#~ if not (conn.get_input().get_pluginloader().get_flags() & zzub.plugin_flag_no_output):
					crx, cry = get_pixelpos(*conn.get_input().get_position())
					draw_line_arrow(arrowcolors[conn.get_type()],int(crx),int(cry),int(rx),int(ry))
		if self.drawrequest & self.DRAW_LEDS:
			self.drawrequest = self.drawrequest ^ (self.drawrequest & self.DRAW_LEDS)
			self.draw_leds()
		if self.connecting:
			crx, cry = get_pixelpos(*self.current_plugin.get_position())
			rx,ry= self.connectpos
			draw_line(int(crx),int(cry),int(rx),int(ry))
	
	def on_key_jazz(self, widget, event, plugin):
		if not plugin:			
			if self.selected_plugin:
				plugin = self.selected_plugin
			else:
				return
		if self.rootwindow.on_key_down(widget, event):
			return
		kv = event.keyval
		k = gtk.gdk.keyval_name(kv)
		note = None
		octave = self.rootwindow.patternframe.view.octave
		if  k == 'KP_Multiply':			
			octave = min(max(octave+1,0), 9)
		elif k ==  'KP_Divide':
			octave = min(max(octave-1,0), 9)
		elif kv < 256:
			note = key_to_note(kv)
		self.rootwindow.patternframe.view.octave=octave
		self.rootwindow.patternframe.view.toolbar.update_octaves()
		if note:
			if note not in self.chordnotes:
				self.chordnotes.append(note)
				try:
					n=((note[0]+octave)<<4|note[1]+1)
					plugin.play_midi_note(n, 0, 127)
				except TypeError:
					pass
				except:
					import traceback
					traceback.print_exc()

	def on_key_jazz_release(self, widget, event, plugin):
		if not plugin:			
			if self.selected_plugin:
				plugin = self.selected_plugin
			else:
				return
		kv = event.keyval
		k = gtk.gdk.keyval_name(kv)
		info = common.get_plugin_infos().get(plugin)
		if kv<256:
			octave = self.rootwindow.patternframe.view.octave
			note = key_to_note(kv)
			self.chordnotes.remove(note)
			try:
				n=((note[0]+octave)<<4|note[1]+1)
				plugin.play_midi_note(zzub.zzub_note_value_off, n, 0)
			except TypeError:
				pass
			except:
				import traceback 
				traceback.print_exc()


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
	import testplayer, utils
	player = testplayer.get_player()
	player.load_ccm(utils.filepath('demosongs/paniq-knark.ccm'))
	window = testplayer.TestWindow()
	window.add(RoutePanel(window))
	# update_position() needs the index to be set:
	# (main.AldrinFrame.PAGE_ROUTE = 1)
	window.PAGE_ROUTE = 1
	window.index = 1
	window.show_all()
	gtk.main()
