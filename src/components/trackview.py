#encoding: latin-1

# Neil
# Modular Sequencer
# Copyright (C) 2006,2007,2008 The Neil Development Team
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

if __name__ == '__main__':
	import os
	os.system('../../bin/neil-combrowser neil.core.trackviewpanel')
	raise SystemExit
	
import gtk
import pango
import gobject
from neil.utils import PLUGIN_FLAGS_MASK, ROOT_PLUGIN_FLAGS, \
	GENERATOR_PLUGIN_FLAGS, EFFECT_PLUGIN_FLAGS, CONTROLLER_PLUGIN_FLAGS
from neil.utils import prepstr, from_hsb, to_hsb, get_item_count, \
	get_clipboard_text, set_clipboard_text, add_scrollbars
from neil.utils import is_effect,is_generator,is_controller,is_root, \
	get_new_pattern_name, filepath, synchronize_list
import random
import config
import neil.common as common
MARGIN = common.MARGIN
MARGIN2 = common.MARGIN2
MARGIN3 = common.MARGIN3
MARGIN0 = common.MARGIN0
import neil.com as com

SEQROWSIZE = 24

class Track(gtk.HBox):
	"""
	Track header. Displays controls to mute or solo the track.
	"""
	__neil__ = dict(
		id = 'neil.core.track',
		categories = [
		]
	)	
	
	def __init__(self, track, hadjustment=None):
		gtk.HBox.__init__(self)
		self.track = track
		self.hadjustment = hadjustment
		self.header = gtk.VBox()
		self.label = gtk.Label(prepstr(self.track.get_plugin().get_name()))
		self.label.set_alignment(0.0, 0.5)
		hbox = gtk.HBox()
		hbox.pack_start(self.label, True, True, 5)
		self.header.pack_start(hbox, True, True)
		self.header.pack_end(gtk.HSeparator(), False, False)
		self.view = com.get('neil.core.trackview', track, hadjustment)
		self.pack_start(self.header, False, False)
		self.pack_end(self.view, True, True)

class View(gtk.DrawingArea):
	"""
	base class for track-like views.
	"""
	def __init__(self, hadjustment=None):
		gtk.DrawingArea.__init__(self)
		self.step = 64
		self.patterngfx = {}
		self.hadjustment = hadjustment
		self.add_events(gtk.gdk.ALL_EVENTS_MASK)
		self.connect("expose_event", self.expose)
		if hadjustment:
			self.hadjustment.connect('value-changed', self.on_adjustment_value_changed)
			self.hadjustment.connect('changed', self.on_adjustment_changed)
		self.connect('size-allocate', self.on_size_allocate)
		
	def get_ticks_per_pixel(self):
		w,h = self.get_client_size()
		# size of view
		pagesize = int(self.hadjustment.page_size+0.5)
		# pixels per tick
		return pagesize / float(w)

	def on_size_allocate(self, *args):
		self.patterngfx = {}
		self.redraw()
		
	def on_adjustment_value_changed(self, adjustment):
		self.redraw()
		
	def on_adjustment_changed(self, adjustment):
		self.patterngfx = {}
		self.redraw()

	def redraw(self):
		if self.window:
			rect = self.get_allocation()
			self.window.invalidate_rect((0,0,rect.width,rect.height), False)

	def get_client_size(self):
		rect = self.get_allocation()
		return rect.width, rect.height

class TimelineView(View):
	"""
	timeline view. shows a horizontal sequencer timeline.
	"""
	__neil__ = dict(
		id = 'neil.core.timelineview',
		categories = [
		]
	)
	
	def __init__(self, hadjustment=None):
		View.__init__(self, hadjustment)
		self.set_size_request(-1, 16)
		
	def expose(self, widget, *args):
		player = com.get('neil.core.player')
		w,h = self.get_client_size()
		gc = self.window.new_gc()
		cm = gc.get_colormap()
		drawable = self.window
		cfg = config.get_config()
		bgbrush = cm.alloc_color(cfg.get_color('SE BG'))
		pen1 = cm.alloc_color(cfg.get_color('SE BG Very Dark'))
		pen2 = cm.alloc_color(cfg.get_color('SE BG Dark'))
		textcolor = cm.alloc_color(cfg.get_color('SE Text'))

		gc.set_foreground(bgbrush)
		gc.set_background(bgbrush)
		drawable.draw_rectangle(gc, True, 0, 0, w, h)
		
		layout = pango.Layout(self.get_pango_context())
		desc = pango.FontDescription('Sans 7.5')
		layout.set_font_description(desc)
		layout.set_width(-1)
		
		# first visible tick
		start = int(self.hadjustment.get_value()+0.5)
		# size of view
		pagesize = int(self.hadjustment.page_size+0.5)
		# last visible tick 
		end = int(self.hadjustment.get_value() + self.hadjustment.page_size + 0.5)
		
		# pixels per tick
		tpp = self.get_ticks_per_pixel()

		# distance of indices to print
		stepsize = int(4*self.step + 0.5)
		
		# first index to print
		startindex = (start / stepsize) * stepsize
		
		i = startindex
		while i < end:
			x = int((i - start)/tpp + 0.5)
			if i == (i - i%(stepsize*4)):
				gc.set_foreground(pen1)
			else:
				gc.set_foreground(pen2)
			drawable.draw_line(gc, x-1, 0, x-1, h)
			gc.set_foreground(textcolor)
			layout.set_text("%i" % i)
			px,py = layout.get_pixel_size()
			drawable.draw_layout(gc, x, h/2 - py/2, layout)
			i += stepsize

class TrackView(View):
	"""
	Track view. Displays the content of one track.
	"""
	__neil__ = dict(
		id = 'neil.core.trackview',
		categories = [
		]
	)
		
	def __init__(self, track, hadjustment=None):
		self.track = track
		View.__init__(self, hadjustment)
		self.set_size_request(-1, 22)
		
	def get_pattern_pixmap(self, gc, layout, pos, value):
		bb = None #self.patterngfx.get(value, None)
		w,h = self.get_client_size()
		if not bb:
			tpp = self.get_ticks_per_pixel()
		
			cfg = config.get_config()
			bghsb = to_hsb(*cfg.get_float_color('SE BG'))
			bgb = max(bghsb[2],0.1)
			cm = gc.get_colormap()
			textcolor = cm.alloc_color(cfg.get_color('SE Text'))
			m = self.track.get_plugin()
			mname = m.get_name()
			title = prepstr(mname)
			if value >= 0x10:
				pat = m.get_pattern(value-0x10)
				name,length = prepstr(pat.get_name()), pat.get_row_count()
			elif value == 0x00:
				name,length = "X", 1
			elif value == 0x01:
				name,length = "<", 1
			else:
				print "unknown value:",value
				name,length = "???",0
			# first visible tick
			offset = int(self.hadjustment.get_value()+0.5)			
			start = pos
			end = pos + length
			ps1 = int(((start-offset) / tpp) + 0.5)
			ps2 = int(((end-offset) / tpp) + 0.5)
			psize = max(ps2-ps1,2) # max(int(((SEQROWSIZE * length) / self.step) + 0.5),2)
			bbh = h-2
			bb = gtk.gdk.Pixmap(self.window, psize-1, bbh-1, -1)
			self.patterngfx[value] = bb					
			if value < 0x10:
				gc.set_foreground(sbrushes[value])
				bb.draw_rectangle(gc, True, 0, 0, psize-1, bbh-1)
			else:
				random.seed(mname+name)
				hue = random.random()
				cb = 1.0
				r,g,b = from_hsb(hue, 0.2, cb*bgb)
				gc.set_foreground(cm.alloc_color('#%02X%02X%02X' % (int(r*255),int(g*255),int(b*255))))
				bb.draw_rectangle(gc, True, 0,0, psize-2, bbh-2)
				r,g,b = from_hsb(hue, 0.5, cb*bgb*0.5)
				gc.set_foreground(cm.alloc_color('#%02X%02X%02X' % (int(r*255),int(g*255),int(b*255))))
				pat = m.get_pattern(value-0x10)
				bh = bbh-2-4
				bw = max(psize-2-2, 1)
				# 0.3: DEAD - no get_bandwidth_digest
				#~ digest = pat.get_bandwidth_digest(bw)
				#~ for evx,evh in enumerate(digest):
					#~ if evh:
						#~ evh = max(int(bh * (0.5 + evh*0.5) + 0.5), 1)
						#~ bb.draw_rectangle(gc, True, 1+evx, 2+bh-evh, 1, evh )
				r,g,b = from_hsb(hue, 1.0, cb*bgb*0.7)
				gc.set_foreground(cm.alloc_color('#%02X%02X%02X' % (int(r*255),int(g*255),int(b*255))))
				bb.draw_rectangle(gc, False, 0, 0, psize-2, bbh-2)
				ofs = 0
				layout.set_text(name)
				px,py = layout.get_pixel_size()
				gc.set_foreground(textcolor)
				bb.draw_layout(gc, 2, 0 + bbh/2 - py/2, layout)
		return bb
	
	def expose(self, widget, *args):
		player = com.get('neil.core.player')
		w,h = self.get_client_size()
		gc = self.window.new_gc()
		cm = gc.get_colormap()
		drawable = self.window
		cfg = config.get_config()
		bghsb = to_hsb(*cfg.get_float_color('SE BG'))
		bgb = max(bghsb[2],0.1)
		bgbrush = cm.alloc_color(cfg.get_color('SE BG'))
		sbrushes = [cm.alloc_color(cfg.get_color('SE Mute')), cm.alloc_color(cfg.get_color('SE Break'))]
		select_brush = cm.alloc_color(cfg.get_color('SE Sel BG'))
		vlinepen = cm.alloc_color(cfg.get_color('SE BG Dark'))
		pen1 = cm.alloc_color(cfg.get_color('SE BG Very Dark'))
		pen2 = cm.alloc_color(cfg.get_color('SE BG Dark'))
		pen = cm.alloc_color(cfg.get_color('SE Line'))
		loop_pen = cm.alloc_color(cfg.get_color('SE Loop Line'))
		invbrush = cm.alloc_color('#ffffff')
		textcolor = cm.alloc_color(cfg.get_color('SE Text'))

		gc.set_foreground(bgbrush)
		gc.set_background(bgbrush)
		drawable.draw_rectangle(gc, True, 0, 0, w, h)
		
		layout = pango.Layout(self.get_pango_context())
		#~ layout.set_font_description(self.fontdesc)
		layout.set_width(-1)
		
		# first visible tick
		start = int(self.hadjustment.get_value()+0.5)
		# size of view
		pagesize = int(self.hadjustment.page_size+0.5)
		# last visible tick 
		end = int(self.hadjustment.get_value() + self.hadjustment.page_size + 0.5)
		
		# pixels per tick
		tpp = self.get_ticks_per_pixel()

		# distance of indices to print
		stepsize = int(4*self.step + 0.5)
		
		# first index to print
		startindex = (start / stepsize) * stepsize
		
		# timeline
		i = startindex
		while i < end:
			x = int((i - start)/tpp + 0.5)
			if i == (i - i%(stepsize*4)):
				gc.set_foreground(pen1)
			else:
				gc.set_foreground(pen2)
			drawable.draw_line(gc, x-1, 0, x-1, h)
			i += stepsize
			
		sel = False
		for pos, value in self.track.get_event_list():
			bb = self.get_pattern_pixmap(gc, layout, pos, value)
			bbw,bbh = bb.get_size()
			x = int((pos - start)/tpp + 0.5)
			if ((x+bbw) >= 0) and (x < w):
				self.window.draw_drawable(gc, bb, 0, 0, x, 1, bbw, bbh)

#				if intrack and (pos >= selstart[1]) and (pos <= selend[1]):
#					gc.set_foreground(invbrush)
#					gc.set_function(gtk.gdk.XOR)
#					drawable.draw_rectangle(gc, True, x+ofs, y+1, bbw-ofs, bbh)
#					gc.set_function(gtk.gdk.COPY)
		#gc.set_foreground(vlinepen)
		#drawable.draw_line(gc, 0, y, w, y)
		
#		gc.set_foreground(pen)
#		x = SEQLEFTMARGIN-1
#		drawable.draw_line(gc, x, 0, x, h)
#		se = player.get_song_end()
#		x,y = self.track_row_to_pos((0,se))
#		if (x >= SEQLEFTMARGIN):
#			gc.set_foreground(pen)
#			drawable.draw_line(gc, x-1, 0, x-1, h)
#		gc.set_foreground(loop_pen)
#		gc.line_style = gtk.gdk.LINE_ON_OFF_DASH
#		gc.set_dashes(0, (1,1))
#		lb,le = player.get_loop()
#		x,y = self.track_row_to_pos((0,lb))
#		if (x >= SEQLEFTMARGIN):
#			drawable.draw_line(gc, x-1, 0, x-1, h)
#		x,y = self.track_row_to_pos((0,le))
#		if (x >= SEQLEFTMARGIN):
#			drawable.draw_line(gc, x-1, 0, x-1, h)
#		self.draw_xor()
		
		return False

class TrackViewPanel(gtk.VBox):
	"""
	Sequencer pattern panel.
	
	Displays all the patterns available for the current track.
	"""
	__neil__ = dict(
		id = 'neil.core.trackviewpanel',
		singleton = True,
		categories = [
			'neil.viewpanel',
			'view',
		]
	)	
	
	__view__ = dict(
			label = "Tracks",
			stockid = "neil_sequencer",
			shortcut = '<Shift>F4',
			order = 4,
	)
	
	def __init__(self):
		"""
		Initialization.
		"""
		gtk.VBox.__init__(self)
		self.sizegroup = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
		self.hscroll = gtk.HScrollbar()
		hadjustment = self.hscroll.get_adjustment()
		hadjustment.set_all(0, 0, 16384, 1, 1024, 2300)
		self.timeline = com.get('neil.core.timelineview', hadjustment)
		self.trackviews = gtk.VBox()
		
		vbox = gtk.VBox()
	
		hbox = gtk.HBox()
		timeline_padding = gtk.HBox()
		self.sizegroup.add_widget(timeline_padding)
		hbox.pack_start(timeline_padding, False, False)
		hbox.pack_start(self.timeline)
		
		vbox.pack_start(hbox, False, False)
		vbox.pack_start(self.trackviews)
		
		hbox = gtk.HBox()
		scrollbar_padding = gtk.HBox()
		self.sizegroup.add_widget(scrollbar_padding)
		hbox.pack_start(scrollbar_padding, False, False)
		hbox.pack_start(self.hscroll)

		vbox.pack_end(hbox, False, False)

		self.pack_start(vbox)
		eventbus = com.get('neil.core.eventbus')
		eventbus.zzub_sequencer_changed += self.update_tracks
		eventbus.zzub_set_sequence_tracks += self.update_tracks
		eventbus.zzub_sequencer_remove_track += self.update_tracks
		self.update_tracks()
		self.show_all()
		self.trackviews.connect('size-allocate', self.on_size_allocate)
		
	def on_size_allocate(self, *args):
		rect = self.get_allocation()
		w,h = rect.width, rect.height
		hadjustment = self.hscroll.get_adjustment()
		hadjustment.page_size = int(((64.0 * w) / 24.0) + 0.5)
	
	def update_tracks(self, *args):
		player = com.get('neil.core.player')
		tracklist = list(player.get_sequence_list())

		def insert_track(i,track):
			print "insert",i,track
			trackview = com.get('neil.core.track', track, self.hscroll.get_adjustment())
			self.trackviews.pack_start(trackview, False, False)
			self.trackviews.reorder_child(trackview, i)
			self.sizegroup.add_widget(trackview.header)
			trackview.show_all()

		def del_track(i):
			print "del",i
			trackview = self.trackviews.get_children()[i]
			trackview.track = None
			self.sizegroup.remove_widget(trackview.header)
			self.trackviews.remove(trackview)
			trackview.destroy()
			
		def swap_track(i,j):
			print "swap",i,j
			pass

		tracks = [trackview.track for trackview in self.trackviews]
		synchronize_list(tracks, tracklist, insert_track, del_track, swap_track)

__neil__ = dict(
	classes = [
		TrackViewPanel,
		Track,
		TrackView,
		TimelineView,
	],
)

