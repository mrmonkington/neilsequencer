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

"""
Provides dialogs and controls to render the plugin view/router and its associated components.
"""

import gtk
import gobject
import cairo
import pangocairo
from aldrin.utils import PLUGIN_FLAGS_MASK, ROOT_PLUGIN_FLAGS, GENERATOR_PLUGIN_FLAGS, EFFECT_PLUGIN_FLAGS, CONTROLLER_PLUGIN_FLAGS
from aldrin.utils import is_effect,is_generator,is_controller,is_root
from aldrin.utils import prepstr, filepath, db2linear, linear2db, is_debug, filenameify, \
	get_item_count, question, error, new_listview, add_scrollbars, get_clipboard_text, set_clipboard_text, \
	gettext, new_stock_image_button
import config
import zzub
import sys,os
import indexer
import fnmatch
import ctypes
import time
import random
import Queue
from aldrin.preset import PresetCollection, Preset
import aldrin.common as common
from aldrin.common import MARGIN, MARGIN2, MARGIN3
from rack import ParameterView
from aldrin.presetbrowser import PresetView
from patterns import key_to_note

import aldrin.com as com

PLUGINWIDTH = 100
PLUGINHEIGHT = 25
LEDWIDTH,LEDHEIGHT = 8,PLUGINHEIGHT-6 # size of LED
LEDOFSX,LEDOFSY = 3,3 # offset of LED
CPUWIDTH,CPUHEIGHT = 8,PLUGINHEIGHT-6 # size of LED
CPUOFSX,CPUOFSY = PLUGINWIDTH-CPUWIDTH-3,3 # offset of LED

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
	__aldrin__ = dict(
		id = 'aldrin.core.parameterdialog',
		singleton = False,
		categories = [
		]
	)
	
	def __init__(self, manager, plugin, parent):
		gtk.Dialog.__init__(self, parent=parent.get_toplevel())
		self.plugin = plugin
		self.manager = manager
		self.manager.plugin_dialogs[plugin] = self
		self.paramview = ParameterView(plugin)
		self.set_title(self.paramview.get_title())
		self.vbox.add(self.paramview)
		self.connect('destroy', self.on_destroy)
		self.connect('realize', self.on_realize)
		eventbus = com.get('aldrin.core.eventbus')
		eventbus.zzub_delete_plugin += self.on_zzub_delete_plugin
		
	def on_realize(self, widget):
		self.set_default_size(*self.paramview.get_best_size())
		
	def on_zzub_delete_plugin(self, plugin):
		if plugin == self.plugin:
			self.destroy()
		
	def on_destroy(self, event):
		"""
		Handles destroy events.
		"""
		del self.manager.plugin_dialogs[self.plugin]

class ParameterDialogManager:
	"""
	Manages the different parameter dialogs.
	"""
	__aldrin__ = dict(
		id = 'aldrin.core.parameterdialog.manager',
		singleton = True,
		categories = [
		]
	)
		
	def __init__(self):
		self.plugin_dialogs = {}
	
	def show(self, plugin, parent):
		"""
		Shows a parameter dialog for a plugin.
		
		@param plugin: Plugin instance.
		@type plugin: Plugin
		"""
		dlg = self.plugin_dialogs.get(plugin,None)
		if not dlg:
			dlg = ParameterDialog(self, plugin, parent)
		dlg.show_all()
		
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
	
	__aldrin__ = dict(
		id = 'aldrin.core.pluginbrowser',
		categories = [
		]
	)
	
	PBD_PTYPE = 0
	PBD_PARAMS = 1
	PBD_ATTRS = 2
	PBD_NAME = 3
	PBD_AUTHOR = 4
	PBD_PLUGIN = 5
	PBD_NATIVE = 6
	def __init__(self, parent=None, *args, **kwds):
		"""
		Initializer.
		"""
		self.routeview = parent
		gtk.Dialog.__init__(self,
				title="Add Plugins",
				parent=parent.get_toplevel(),
				# If it's modal, the effect plugin racks that pop up can't be closed
				flags=gtk.DIALOG_DESTROY_WITH_PARENT,
		)
		self.set_size_request(800, 500)
		

		self.search_entry = gtk.Entry()
		self.search_entry.set_text("(Search)")
		self.action_area.pack_start(self.search_entry, True, True, 0)
		self.show_nonnative_button = gtk.CheckButton(label="Show Non-Native")
		self.action_area.pack_start(self.show_nonnative_button, True, True, 0)
		self.show_nonnative_button.set_active(False)
		self.close_button = gtk.Button(stock=gtk.STOCK_CLOSE)
		self.action_area.pack_start(self.close_button, True, True, 0)

		col_names = ["Type", "Parameters", "Attributes", "Name", "Author"]
		# The undisplayed columns are the pluginloader ref and a bool for whether it's native
		self.store = gtk.ListStore(str, int, int, str, str, object, bool)
		# create the TreeViewColumns to display the data
		self.columns = [gtk.TreeViewColumn(name) for name in col_names]

		# filter is derived from the ListStore: it holds a changeable subset of the rows.
		self.filter = self.store.filter_new()
		self.filter.set_visible_func(self.filter_fn, data=None)

		# view is a widget displaying the rows in the filter.
		self.view = gtk.TreeView(self.filter)
		# We have custom search.
		self.view.set_enable_search(False)
		self.view.set_rules_hint(True)
		# This is supposed to speed up display, but causes an error?
		# self.view.set_fixed_height_mode(True)
		self.view.set_hadjustment(gtk.Adjustment())
		self.view.set_vadjustment(gtk.Adjustment())

		for i, col in enumerate(self.columns):
			# add column to view
			self.view.append_column(col)
			# create a CellRendererText to render the data
			cell = gtk.CellRendererText()
			# add the cell to the column. The bool indicates whether it's allowed to expand
			col.pack_start(cell, [False, False, False, True, True][i])
			# retrieve text from column i in self.store
			col.add_attribute(cell, 'text', i)
			col.set_sizing(gtk.TREE_VIEW_COLUMN_GROW_ONLY)

		self.populate(com.get('aldrin.core.plugintree'))

		self.scrollwindow = gtk.ScrolledWindow()
		self.scrollwindow.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		self.scrollwindow.add(self.view)
		self.vbox.add(self.scrollwindow)

		self.view.connect("row-activated", self.row_activated_cb)
		self.search_entry.connect("changed", lambda x: self.filter.refilter())
		self.show_nonnative_button.connect("toggled", lambda x: self.filter.refilter())
		self.close_button.connect("clicked", lambda x: self.destroy())
		
		self.show_all()

	def filter_fn(self, model, iter, data):
		ptype = model.get(iter, PluginBrowserDialog.PBD_PTYPE)[0]
		name = model.get(iter, PluginBrowserDialog.PBD_NAME)[0]
		author = model.get(iter, PluginBrowserDialog.PBD_AUTHOR)[0]
		native = model.get(iter, PluginBrowserDialog.PBD_NATIVE)[0]
		if not (ptype and name and author):
			return True
		if not self.show_nonnative_button.get_active() and not native:
			return False
		search_txt = self.search_entry.get_text()
		if "(Search)" in search_txt:
			return True
		name = name.lower()
		author = author.lower()
		ptype = ptype.lower()
		for token in search_txt.lower().strip().split(" "):
			if not (token in name or token in author or token in ptype):
				return False
		return True

	def row_activated_cb(self, treeview, path, view_column):
		iter = self.filter.get_iter(path)
		pl = self.filter.get_value(iter, PluginBrowserDialog.PBD_PLUGIN)
		if pl:
			# User might add multiple plugins from this dialog, so can't use any fixed
			# location. Generate a random location.
			rect = self.routeview.get_allocation()
			w,h = rect.width, rect.height
			self.routeview.contextmenupos = (random.uniform(10, w-10), random.uniform(10, h-10))
			self.routeview.on_popup_new_plugin(None, pl)

	def populate(self, node):
		def get_type_text(pl):
			if is_generator(pl):
				return "Generator"
			elif is_effect(pl):
				return "Effect"
			elif is_controller(pl):
				return "Controller"
			elif is_root(pl):
				return "Root"
			else:
				return "Other"
		for child in node.children:
			if isinstance(child, indexer.Directory) and not child.is_empty():
				self.populate(child)
			elif isinstance(child, indexer.Reference):
				if child.pluginloader:
					pl = child.pluginloader
					if "ladspa" in pl.get_loader_name().lower() or "dssi" in pl.get_loader_name().lower():
						native = False
					else:
						native = True
					self.store.append([get_type_text(pl),
							   sum([pl.get_parameter_count(g) for g in range(1, 3)]),
							   pl.get_attribute_count(),
							   prepstr(pl.get_name()),
							   prepstr(pl.get_author()),
							   pl,
							   native])
			elif isinstance(child, indexer.Separator):
				pass

class RoutePanel(gtk.VBox):
	"""
	Contains the view panel and manages parameter dialogs.
	"""
	__aldrin__ = dict(
		id = 'aldrin.core.routerpanel',
		singleton = True,
		categories = [
			'aldrin.viewpanel',
			'view',
		]
	)
	
	__view__ = dict(
			label = "Router",
			stockid = "aldrin_router",
			shortcut = 'F3',
			default = True,
			order = 3,
	)
	
	def __init__(self):
		"""
		Initializer.
		"""
		gtk.VBox.__init__(self)
		self.view = com.get('aldrin.core.router.view', self)
		sizer_2 = gtk.HBox()
		sizer_2.add(self.view)
		self.add(sizer_2)
		
	def handle_focus(self):
		self.view.grab_focus()
		
	def reset(self):
		"""
		Resets the router view. Used when
		a new song is being loaded.
		"""
		self.view.reset()
		
	def update_all(self):		
		self.view.update_colors()
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
		self.conn.set_amplitude(amp)
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
		self.amp = (linear2db((self.conn.get_amplitude()/ 16384.0), -48.0) / -48.0)
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
	__aldrin__ = dict(
		id = 'aldrin.core.router.view',
		singleton = True,
		categories = [
		]
	)
	
	current_plugin = None
	connecting = False
	dragging = False
	dragoffset = 0,0
	contextmenupos = 0,0
	
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
	COLOR_BORDER_IN = 10
	COLOR_BORDER_OUT = 11
	COLOR_BORDER_SELECT = 12
	COLOR_TEXT = 13
	
	def __init__(self, parent):
		"""
		Initializer.
		
		@param rootwindow: Main window.
		@type rootwindow: AldrinFrame
		"""
		self.panel = parent
		self.routebitmap = None
		eventbus = com.get('aldrin.core.eventbus')
		eventbus.zzub_connect += self.on_zzub_redraw_event
		eventbus.zzub_disconnect += self.on_zzub_redraw_event
		eventbus.zzub_plugin_changed += self.on_zzub_redraw_event
		self.solo_plugin = None
		self.selected_plugin = None
		self.autoconnect_target=None
		self.chordnotes=[]
		self.update_colors()
		gtk.DrawingArea.__init__(self)
		self.volume_slider = VolumeSlider()		
		self.add_events(gtk.gdk.ALL_EVENTS_MASK)
		self.set_property('can-focus', True)
		self.connect('button-press-event', self.on_left_down)
		self.connect('button-release-event', self.on_left_up)
		self.connect('motion-notify-event', self.on_motion)
		self.connect("expose_event", self.expose)
		self.connect('key-press-event', self.on_key_jazz, None)	
		self.connect('key-release-event', self.on_key_jazz_release, None)
		self.connect('size-allocate', self.on_size_allocate)
		if config.get_config().get_led_draw() == True:
			gobject.timeout_add(100, self.on_draw_led_timer)
		#~ wx.EVT_SET_FOCUS(self, self.on_focus)
		
	def on_size_allocate(self, widget, requisition):
		self.routebitmap = None
		
	def update_colors(self):
		"""
		Updates the routers color scheme.
		"""
		cfg = config.get_config()
		names = [
			'MV ${PLUGIN}', 
			'MV ${PLUGIN} Mute', 
			'MV ${PLUGIN} LED Off', 
			'MV ${PLUGIN} LED On', 
			'MV ${PLUGIN} LED Border', 
			'MV ${PLUGIN} LED Warning', 
			'MV CPU LED Off', 
			'MV CPU LED On', 
			'MV CPU LED Border', 
			'MV CPU LED Warning',
			'MV ${PLUGIN} Border In',
			'MV ${PLUGIN} Border Out',
			'MV ${PLUGIN} Border Selected',
			'MV ${PLUGIN} Text',
		]
		flagids = [
			(ROOT_PLUGIN_FLAGS, 'Master'),
			(GENERATOR_PLUGIN_FLAGS, 'Generator'),
			(EFFECT_PLUGIN_FLAGS, 'Effect'),
			(CONTROLLER_PLUGIN_FLAGS, 'Controller'),
		]
		self.flags2brushes = {}
		for flags,name in flagids:
			brushes = []
			for name in [x.replace('${PLUGIN}',name) for x in names]:
				brushes.append(cfg.get_color(name))
			self.flags2brushes[flags] = brushes
		common.get_plugin_infos().reset_plugingfx()
		
	def on_zzub_redraw_event(self, *args):
		self.redraw()
			
	def on_focus(self, event):
		self.redraw()

	def on_context_menu(self, widget, event):
		"""
		Event handler for requests to show the context menu.
		
		@param event: event.
		@type event: wx.Event
		"""
		mx,my = int(event.x), int(event.y)
		player = com.get('aldrin.core.player')
		player.plugin_origin = self.pixel_to_float((mx,my))
		res = self.get_plugin_at((mx,my))
		if res:
			mp,(x,y),area = res
			menu = com.get('aldrin.core.contextmenu', 'plugin', mp)
		else:
			res = self.get_connection_at((mx,my))
			if res:
				mp, index = res
				menu = com.get('aldrin.core.contextmenu', 'connection', (mp, index))
			else:
				menu = com.get('aldrin.core.contextmenu', 'router', None)
		menu.popup(self, event)
		
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
		player = com.get('aldrin.core.player')
		rect = self.get_allocation()
		w,h = rect.width, rect.height
		cx,cy = w*0.5, h * 0.5
		def get_pixelpos(x,y):
			return cx * (1+x), cy * (1+y)
		for mp in player.get_plugin_list():
			rx,ry = get_pixelpos(*mp.get_position())
			for index in xrange(mp.get_input_connection_count()):
				crx, cry = get_pixelpos(*mp.get_input_connection_plugin(index).get_position())
				cpx,cpy = (crx + rx) * 0.5, (cry + ry) * 0.5
				dx,dy = cpx - mx, cpy - my
				length = (dx*dx + dy*dy) ** 0.5
				if length <= 14: # why exactly 14?
					return mp, index
		
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
		player = com.get('aldrin.core.player')
		for mp in reversed(list(player.get_plugin_list())):
			pi = common.get_plugin_infos().get(mp)
			if not pi.songplugin:
				continue
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
		player = com.get('aldrin.core.player')
		mx,my = int(event.x), int(event.y)
		res = self.get_plugin_at((mx,my))
		if not res:
			# User adds plugins while browser stays open. Nothing to do when it's closed.
			com.get('aldrin.core.pluginbrowser', self)
			return
		mp,(x,y),area = res
		if area == AREA_ANY:
			data = zzub.zzub_event_data_t()
			data.type = zzub.zzub_event_type_double_click;
			if mp.invoke_event(data, 1) != 0:
				com.get('aldrin.core.parameterdialog.manager').show(mp, self)
		
	def on_left_down(self, widget, event):
		"""
		Event handler for left mouse button presses. Initiates
		plugin dragging or connection volume adjustments.
		
		@param event: Mouse event.
		@type event: wx.MouseEvent
		"""
		player = com.get('aldrin.core.player')
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
			self.current_plugin.dragpos = self.current_plugin.get_position()
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
				com.get('aldrin.core.eventbus').show_plugin(self.selected_plugin)
				common.get_plugin_infos().get(self.selected_plugin).reset_plugingfx()									
			if last:
				common.get_plugin_infos().get(last).reset_plugingfx()
		else:
			res = self.get_connection_at((mx,my))
			if res:
				mp, index = res
				ox, oy = self.window.get_origin()
				connectiontype = mp.get_input_connection_type(index)
				if connectiontype == zzub.zzub_connection_type_audio:
					self.volume_slider.display((ox+mx,oy+my), conn, index)
				elif connectiontype == zzub.zzub_connection_type_event:
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
			self.current_plugin.dragpos = self.pixel_to_float((x,y))
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
		mx,my = int(event.x), int(event.y)
		player = com.get('aldrin.core.player')
		if self.dragging:
			self.dragging = False
			self.grab_remove()
			ox,oy = self.dragoffset
			size = self.get_allocation()
			x,y = max(0, min(mx - ox, size.width)), max(0, min(my - oy, size.height))
			if (event.state & gtk.gdk.CONTROL_MASK):
				# quantize position
				x = int(float(x)/QUANTIZEX + 0.5) * QUANTIZEX
				y = int(float(y)/QUANTIZEY + 0.5) * QUANTIZEY
			self.current_plugin.set_position(*self.pixel_to_float((x,y)))
			player.history_commit("move plugin")
		if self.connecting:
			res = self.get_plugin_at((mx,my))
			if res:
				mp,(x,y),area = res
				if self.current_plugin == mp:
					pass
				elif is_controller(self.current_plugin):
					#mp.add_event_input(self.current_plugin)
					pass
				else:
					mp.add_input(self.current_plugin, zzub.zzub_connection_type_audio)
					player.history_commit("new nonnection")
		self.current_plugin = None
		self.connecting = False
		self.redraw()
		
	def update_all(self):
		self.update_colors()
	
	def on_draw_led_timer(self):
		"""
		Timer event that only updates the plugin leds.
		"""
		# TODO: find some other way to find out whether we are really visible
		#if self.rootwindow.get_current_panel() != self.panel:
		#	return True
		if self.window:
			player = com.get('aldrin.core.player')
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
			self.routebitmap = None
			rect = self.get_allocation()
			self.window.invalidate_rect((0,0,rect.width,rect.height), False)
		
	def draw_leds(self):
		"""
		Draws only the leds into the offscreen buffer.
		"""
		player = com.get('aldrin.core.player')
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
		#font = wx.Font(8, wx.FONTFAMILY_DEFAULT, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_BOLD)
		PW, PH = PLUGINWIDTH / 2, PLUGINHEIGHT / 2

		pluginlist = list(player.get_plugin_list())
		
		audiodriver = com.get('aldrin.core.driver.audio')
		cpu = min(audiodriver.get_cpu_load(), 100.0)
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
			pi = common.get_plugin_infos().get(mp)
			if not pi.songplugin:
				continue
			if self.dragging and mp == self.current_plugin:
				rx,ry = get_pixelpos(*self.current_plugin.dragpos)
			rx,ry = rx - PW, ry - PH
			pi = common.get_plugin_infos().get(mp)
			if not pi:
				continue			
			brushes = self.flags2brushes.get(mp.get_flags() & PLUGIN_FLAGS_MASK, self.flags2brushes[GENERATOR_PLUGIN_FLAGS])
			if not pi.plugingfx:
				pi.plugingfx = gtk.gdk.Pixmap(self.window, PLUGINWIDTH, PLUGINHEIGHT, -1)
				# adjust colour for muted plugins
				if pi.muted:
					gc.set_foreground(cm.alloc_color(brushes[self.COLOR_MUTED]))
				else:
					gc.set_foreground(cm.alloc_color(brushes[self.COLOR_DEFAULT]))
				pi.plugingfx.draw_rectangle(gc, True, -1,-1,PLUGINWIDTH+1,PLUGINHEIGHT+1)
				gc.set_foreground(cm.alloc_color(brushes[self.COLOR_BORDER_OUT]))
				pi.plugingfx.draw_rectangle(gc, False, 0, 0, PLUGINWIDTH-1,PLUGINHEIGHT-1)
				gc.set_foreground(cm.alloc_color(brushes[self.COLOR_BORDER_IN]))
				pi.plugingfx.draw_rectangle(gc, False, 1, 1, PLUGINWIDTH-3,PLUGINHEIGHT-3)
				if self.solo_plugin and self.solo_plugin != mp and is_generator(mp):
					title = prepstr('[' + mp.get_name() + ']')
				elif pi.muted:
					title = prepstr('(' + mp.get_name() + ')')
				else:
					title = prepstr(mp.get_name())
				layout.set_text(title)
				lw,lh = layout.get_pixel_size()
				if mp == self.selected_plugin:
					gc.set_foreground(cm.alloc_color(brushes[self.COLOR_BORDER_SELECT]))
					pi.plugingfx.draw_rectangle(gc, False, PLUGINWIDTH/2 - lw/2 - 3, PLUGINHEIGHT/2 - lh/2, lw + 6, lh)
				gc.set_foreground(cm.alloc_color(brushes[self.COLOR_TEXT]))
				pi.plugingfx.draw_layout(gc, PLUGINWIDTH/2 - lw/2, PLUGINHEIGHT/2 - lh/2, layout)
			
			if config.get_config().get_led_draw() == True:		
				maxl, maxr = mp.get_last_peak()
				amp = min(max(maxl,maxr),1.0)
				if amp != pi.amp:
					if amp >= 1:
						gc.set_foreground(cm.alloc_color(brushes[self.COLOR_LED_WARNING]))
						pi.plugingfx.draw_rectangle(gc, True, LEDOFSX, LEDOFSY, LEDWIDTH-1, LEDHEIGHT-1)
					else:
						gc.set_foreground(cm.alloc_color(brushes[self.COLOR_LED_OFF]))
						pi.plugingfx.draw_rectangle(gc, True, LEDOFSX, LEDOFSY, LEDWIDTH-1, LEDHEIGHT-1)
						amp = 1.0 - (linear2db(amp,-76.0)/-76.0)
						height = int((LEDHEIGHT-4)*amp + 0.5)
						if (height > 0):
							gc.set_foreground(cm.alloc_color(brushes[self.COLOR_LED_ON]))
							pi.plugingfx.draw_rectangle(gc, True, LEDOFSX+2, LEDOFSY+LEDHEIGHT-height-2, LEDWIDTH-4, height)
					gc.set_foreground(cm.alloc_color(brushes[self.COLOR_LED_BORDER]))
					pi.plugingfx.draw_rectangle(gc, False, LEDOFSX, LEDOFSY, LEDWIDTH-1, LEDHEIGHT-1)
					pi.amp = amp

				try:
					relperc = max(((cpu_loads[mp]*cpu) / (biggestload*100))*0.1 + pi.cpu*0.9, 0.0)
				except ZeroDivisionError:
					relperc = max(pi.cpu, 0.0)
				if relperc != pi.cpu:
					pi.cpu = relperc
					gc.set_foreground(cm.alloc_color(brushes[self.COLOR_CPU_OFF]))
					pi.plugingfx.draw_rectangle(gc, True, CPUOFSX, CPUOFSY, CPUWIDTH-1, CPUHEIGHT-1)
					height = int((CPUHEIGHT-4)*relperc + 0.5)
					if (height > 0):
						if relperc >= cputreshold:
							gc.set_foreground(cm.alloc_color(brushes[self.COLOR_CPU_WARNING]))
						else:
							gc.set_foreground(cm.alloc_color(brushes[self.COLOR_CPU_ON]))
						pi.plugingfx.draw_rectangle(gc, True, CPUOFSX+2, CPUOFSY+CPUHEIGHT-height-2, CPUWIDTH-4, height)
					gc.set_foreground(cm.alloc_color(brushes[self.COLOR_LED_BORDER]))
					pi.plugingfx.draw_rectangle(gc, False, CPUOFSX, CPUOFSY, CPUWIDTH-1, CPUHEIGHT-1)
				
			self.window.draw_drawable(gc, pi.plugingfx, 0, 0, int(rx), int(ry), -1, -1)
		
	def draw(self, ctx):
		"""
		Draws plugins, connections and arrows to an offscreen buffer.
		"""
		player = com.get('aldrin.core.player')
		cfg = config.get_config()
		rect = self.get_allocation()
		w,h = rect.width,rect.height
		arrowcolors = {
			zzub.zzub_connection_type_audio : [
				cfg.get_float_color("MV Arrow"),
				cfg.get_float_color("MV Arrow Border In"),
				cfg.get_float_color("MV Arrow Border Out"),
			],
			zzub.zzub_connection_type_event : [
				cfg.get_float_color("MV Controller Arrow"),
				cfg.get_float_color("MV Controller Arrow Border In"),
				cfg.get_float_color("MV Controller Arrow Border Out"),
			],
		}
		bgbrush = cfg.get_float_color("MV Background")
		linepen = cfg.get_float_color("MV Line")
		
		cx,cy = w*0.5,h*0.5
		def get_pixelpos(x,y):
			return cx * (1+x), cy * (1+y)
		
		def draw_line(bmpctx,crx,cry,rx,ry):
			vx, vy = (rx-crx), (ry-cry)
			length = (vx*vx+vy*vy)**0.5
			vx, vy = vx/length, vy/length
			bmpctx.move_to(crx,cry)
			bmpctx.line_to(rx,ry)
			bmpctx.set_source_rgb(*linepen)
			bmpctx.stroke()
		def draw_line_arrow(bmpctx,clr,crx,cry,rx,ry):
			vx, vy = (rx-crx), (ry-cry)
			length = (vx*vx+vy*vy)**0.5
			if not length:
				return
			vx, vy = vx/length, vy/length
			bmpctx.move_to(crx,cry)
			bmpctx.line_to(rx,ry)
			bmpctx.set_source_rgb(*linepen)
			bmpctx.stroke()
			cpx,cpy = crx + vx * (length * 0.5), cry + vy * (length * 0.5)
			def make_triangle(radius):
				t1 = (int(cpx - vx * radius + vy * radius), int(cpy - vy * radius - vx * radius))
				t2 = (int(cpx + vx * radius), int(cpy + vy * radius))
				t3 = (int(cpx - vx * radius - vy * radius), int(cpy - vy * radius + vx * radius))
				return t1,t2,t3
			def draw_triangle(t1,t2,t3):
				bmpctx.move_to(*t1)
				bmpctx.line_to(*t2)
				bmpctx.line_to(*t3)
				bmpctx.close_path()
			tri1 = make_triangle(ARROWRADIUS)
			tri2 = make_triangle(ARROWRADIUS-1)
			draw_triangle(*tri1)
			bmpctx.set_source_rgb(*clr[0])
			bmpctx.fill()
			draw_triangle(*tri2)
			bmpctx.set_source_rgb(*clr[1])
			bmpctx.stroke()
			draw_triangle(*tri1)
			bmpctx.set_source_rgb(*clr[2])
			bmpctx.stroke()

		if not self.routebitmap:
			self.routebitmap = gtk.gdk.Pixmap(self.window, w, h, -1)
			bmpctx = self.routebitmap.cairo_create()
			bmpctx.translate(0.5,0.5)
			bmpctx.set_source_rgb(*bgbrush)
			bmpctx.rectangle(0,0,w,h)
			bmpctx.fill()
			bmpctx.set_line_width(1)
			mplist = [(mp,get_pixelpos(*mp.get_position())) for mp in player.get_plugin_list()]
			
			for mp,(rx,ry) in mplist:
				if self.dragging and mp == self.current_plugin:
					rx,ry = get_pixelpos(*self.current_plugin.dragpos)
				for index in xrange(mp.get_input_connection_count()):
					#~ if not (conn.get_input().get_pluginloader().get_flags() & zzub.plugin_flag_no_output):
					targetmp = mp.get_input_connection_plugin(index)
					pi = common.get_plugin_infos().get(targetmp)
					if not pi.songplugin:
						continue
					tmppos = targetmp.get_position()
					if self.dragging and targetmp == self.current_plugin:
						tmppos = self.current_plugin.dragpos
					crx, cry = get_pixelpos(*tmppos)
					draw_line_arrow(bmpctx,arrowcolors[mp.get_input_connection_type(index)],int(crx),int(cry),int(rx),int(ry))
		gc = self.window.new_gc()
		self.window.draw_drawable(gc, self.routebitmap, 0, 0, 0, 0, -1, -1)
		if self.connecting:
			ctx.set_line_width(1)
			crx, cry = get_pixelpos(*self.current_plugin.get_position())
			rx,ry= self.connectpos
			draw_line(ctx,int(crx),int(cry),int(rx),int(ry))
		self.draw_leds()
	
	# This method is not *just* for key-jazz, it handles all key-events in router. Rename?
	def on_key_jazz(self, widget, event, plugin):
		mask = event.state
		kv = event.keyval
		k = gtk.gdk.keyval_name(kv)
		if (mask & gtk.gdk.CONTROL_MASK):
			if k == 'Return':
				com.get('aldrin.core.pluginbrowser', self)
				return
		if not plugin:			
			if self.selected_plugin:
				plugin = self.selected_plugin
			else:
				return
		#if self.rootwindow.on_key_down(widget, event):
		#	return
		note = None
		player = com.get('aldrin.core.player')
		octave = player.octave
		if  k == 'KP_Multiply':			
			octave = min(max(octave+1,0), 9)
		elif k ==  'KP_Divide':
			octave = min(max(octave-1,0), 9)
		elif kv < 256:
			note = key_to_note(kv)
		player.octave = octave
		if note:
			if note not in self.chordnotes:
				self.chordnotes.append(note)
				n=((note[0]+octave)<<4|note[1]+1)
				plugin.play_midi_note(n, 0, 127)

	def on_key_jazz_release(self, widget, event, plugin):
		if not plugin:			
			if self.selected_plugin:
				plugin = self.selected_plugin
			else:
				return
		kv = event.keyval
		if kv<256:
			octave = com.get('aldrin.core.patternpanel').view.octave
			note = key_to_note(kv)
			if note in self.chordnotes:
				self.chordnotes.remove(note)
				n=((note[0]+octave)<<4|note[1]+1)
				plugin.play_midi_note(zzub.zzub_note_value_off, n, 0)
				
__all__ = [
'ParameterDialog',
'ParameterDialogManager',
'AttributesDialog',
'PluginBrowserDialog',
'RoutePanel',
'VolumeSlider',
'RouteView',
]

__aldrin__ = dict(
	classes = [
		ParameterDialog,
		ParameterDialogManager,
		AttributesDialog,
		PluginBrowserDialog,
		RoutePanel,
		VolumeSlider,
		RouteView,
	],
)

if __name__ == '__main__':
	import testplayer, utils
	player = testplayer.get_player()
	player.load_ccm(utils.filepath('demosongs/paniq-knark.ccm'))
	window = testplayer.TestWindow()
	window.add(RoutePanel(window))
	window.PAGE_ROUTE = 1
	window.index = 1
	window.show_all()
	gtk.main()
