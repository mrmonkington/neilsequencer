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
Contains all classes and functions needed to render the sequence
editor and its associated components.
"""

if __name__ == '__main__':
	import os
	os.system('../../bin/aldrin-combrowser aldrin.core.trackviewpanel')
	raise SystemExit
	
import gtk
import pango
import gobject
from aldrin.utils import PLUGIN_FLAGS_MASK, ROOT_PLUGIN_FLAGS, \
	GENERATOR_PLUGIN_FLAGS, EFFECT_PLUGIN_FLAGS, CONTROLLER_PLUGIN_FLAGS
from aldrin.utils import prepstr, from_hsb, to_hsb, get_item_count, \
	get_clipboard_text, set_clipboard_text, add_scrollbars
from aldrin.utils import is_effect,is_generator,is_controller,is_root, \
	get_new_pattern_name, filepath, synchronize_list
import random
import config
import aldrin.common as common
MARGIN = common.MARGIN
MARGIN2 = common.MARGIN2
MARGIN3 = common.MARGIN3
MARGIN0 = common.MARGIN0
import aldrin.com as com

class Track(gtk.HBox):
	"""
	Track header. Displays controls to mute or solo the track.
	"""
	__aldrin__ = dict(
		id = 'aldrin.core.track',
		categories = [
		]
	)	
	
	def __init__(self, track):
		gtk.HBox.__init__(self)
		self.track = track
		self.header = gtk.Label(self.track.get_plugin().get_name())
		self.view = com.get('aldrin.core.trackview', track)
		self.pack_start(self.header, False, False)
		self.pack_end(self.view, True, True)

class TimelineView(gtk.DrawingArea):
	"""
	timeline view. shows a horizontal sequencer timeline.
	"""
	__aldrin__ = dict(
		id = 'aldrin.core.timelineview',
		categories = [
		]
	)
	
	def __init__(self):
		gtk.DrawingArea.__init__(self)
		self.set_size_request(-1, 16)
		self.step = 64
		self.startseqtime = 0
		self.add_events(gtk.gdk.ALL_EVENTS_MASK)
		self.connect("expose_event", self.expose)

	def get_client_size(self):
		rect = self.get_allocation()
		return rect.width, rect.height

	def expose(self, widget, *args):
		player = com.get('aldrin.core.player')
		w,h = self.get_client_size()
		gc = self.window.new_gc()
		cm = gc.get_colormap()
		drawable = self.window
		cfg = config.get_config()
		bghsb = to_hsb(*cfg.get_float_color('SE BG'))
		bgb = max(bghsb[2],0.1)
		type2brush = {
			EFFECT_PLUGIN_FLAGS : cm.alloc_color(cfg.get_color('MV Effect')),
			GENERATOR_PLUGIN_FLAGS : cm.alloc_color(cfg.get_color('MV Generator')),
			ROOT_PLUGIN_FLAGS : cm.alloc_color(cfg.get_color('MV Master')),
			CONTROLLER_PLUGIN_FLAGS : cm.alloc_color(cfg.get_color('MV Controller')),
		}
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
		desc = pango.FontDescription('Sans 7.5')
		layout.set_font_description(desc)
		layout.set_width(-1)
		
		SEQROWSIZE = 24
		
		# 14
		x, y = 0, 0
		i = self.startseqtime
		while x < w:
			if (i % (16*self.step)) == 0:
				gc.set_foreground(pen1)
				drawable.draw_line(gc, x-1, 0, x-1, h)
				gc.set_foreground(textcolor)
				layout.set_text(str(i))
				px,py = layout.get_pixel_size()
				drawable.draw_layout(gc, x, h/2 - py/2, layout)
			elif (i % (4*self.step)) == 0:
				gc.set_foreground(pen2)
				drawable.draw_line(gc, x-1, 0, x-1, h)
				gc.set_foreground(textcolor)
				layout.set_text(str(i))
				px,py = layout.get_pixel_size()
				drawable.draw_layout(gc, x, h/2 - py/2, layout)
			x += SEQROWSIZE
			i += self.step

class TrackView(gtk.DrawingArea):
	"""
	Track view. Displays the content of one track.
	"""
	__aldrin__ = dict(
		id = 'aldrin.core.trackview',
		categories = [
		]
	)
		
	def __init__(self, track):
		gtk.DrawingArea.__init__(self)
		self.set_size_request(-1, 22)
		self.track = track
		self.patterngfx = {}
		self.step = 64
		self.startseqtime = 0
		self.add_events(gtk.gdk.ALL_EVENTS_MASK)
		self.set_property('can-focus', True)
		self.connect("expose_event", self.expose)
		
	def get_client_size(self):
		rect = self.get_allocation()
		return rect.width, rect.height
		
	def draw_pattern(self, gc, layout, pos, value):
		SEQROWSIZE = 24
		bb = self.patterngfx.get(value, None)
		w,h = self.get_client_size()
		if not bb:
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
			psize = max(int(((SEQROWSIZE * length) / self.step) + 0.5),2)
			bb = gtk.gdk.Pixmap(self.window, psize-1, h-1, -1)
			self.patterngfx[value] = bb					
			if value < 0x10:
				gc.set_foreground(sbrushes[value])
				bb.draw_rectangle(gc, True, 0, 0, psize-1, h-1)
			else:
				random.seed(mname+name)
				hue = random.random()
				cb = 1.0
				r,g,b = from_hsb(hue, 0.2, cb*bgb)
				gc.set_foreground(cm.alloc_color('#%02X%02X%02X' % (int(r*255),int(g*255),int(b*255))))
				bb.draw_rectangle(gc, True, 0,0, psize-2, h-2)
				r,g,b = from_hsb(hue, 0.5, cb*bgb*0.5)
				gc.set_foreground(cm.alloc_color('#%02X%02X%02X' % (int(r*255),int(g*255),int(b*255))))
				pat = m.get_pattern(value-0x10)
				bh = h-2-4
				bw = max(psize-2-2, 1)
				# 0.3: DEAD - no get_bandwidth_digest
				#~ digest = pat.get_bandwidth_digest(bw)
				#~ for evx,evh in enumerate(digest):
					#~ if evh:
						#~ evh = max(int(bh * (0.5 + evh*0.5) + 0.5), 1)
						#~ bb.draw_rectangle(gc, True, 1+evx, 2+bh-evh, 1, evh )
				r,g,b = from_hsb(hue, 1.0, cb*bgb*0.7)
				gc.set_foreground(cm.alloc_color('#%02X%02X%02X' % (int(r*255),int(g*255),int(b*255))))
				bb.draw_rectangle(gc, False, 0, 0, psize-2, h-2)
				ofs = 0
				layout.set_text(name)
				px,py = layout.get_pixel_size()
				gc.set_foreground(textcolor)
				bb.draw_layout(gc, 2, 0 + h/2 - py/2, layout)
		bbw,bbh = bb.get_size()
		x = (((pos - self.startseqtime) * SEQROWSIZE) / self.step)
		if ((x+bbw) >= 0) and (x < w):
			ofs = max(-x,0)
			self.window.draw_drawable(gc, bb, ofs, 0, x+ofs, 1, bbw-ofs, bbh)
	
	def expose(self, widget, *args):
		player = com.get('aldrin.core.player')
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
		
		SEQROWSIZE = 24
		
		# 14
		x, y = 0, 0
		i = self.startseqtime
		while x < w:
			if (i % (16*self.step)) == 0:
				gc.set_foreground(pen1)
				drawable.draw_line(gc, x-1, 0, x-1, h)
			elif (i % (4*self.step)) == 0:
				gc.set_foreground(pen2)
				drawable.draw_line(gc, x-1, 0, x-1, h)
			x += SEQROWSIZE
			i += self.step
		sel = False
		for pos, value in self.track.get_event_list():
			self.draw_pattern(gc, layout, pos, value)
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
	__aldrin__ = dict(
		id = 'aldrin.core.trackviewpanel',
		singleton = True,
		categories = [
			'aldrin.viewpanel',
			'view',
		]
	)	
	
	__view__ = dict(
			label = "Tracks",
			stockid = "aldrin_sequencer",
			shortcut = '<Shift>F4',
			order = 4,
	)
	
	def __init__(self):
		"""
		Initialization.
		"""
		gtk.VBox.__init__(self)
		self.sizegroup = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
		self.timeline = com.get('aldrin.core.timelineview')
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
		self.hscroll = gtk.HScrollbar()
		hbox.pack_start(scrollbar_padding, False, False)
		hbox.pack_start(self.hscroll)

		vbox.pack_end(hbox, False, False)

		self.pack_start(vbox)
		eventbus = com.get('aldrin.core.eventbus')
		eventbus.zzub_sequencer_changed += self.update_tracks
		eventbus.zzub_set_sequence_tracks += self.update_tracks
		eventbus.zzub_sequencer_remove_track += self.update_tracks
		self.update_tracks()
		self.show_all()
	
	def update_tracks(self, *args):
		player = com.get('aldrin.core.player')
		tracklist = list(player.get_sequence_list())

		def insert_track(i,track):
			print "insert",i,track
			trackview = com.get('aldrin.core.track', track)
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

__aldrin__ = dict(
	classes = [
		TrackViewPanel,
		Track,
		TrackView,
		TimelineView,
	],
)

