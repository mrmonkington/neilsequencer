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
Provides dialogs, classes and controls to display/load/save envelopes
"""

from gtkimport import gtk
import os, sys
from utils import prepstr, db2linear, linear2db, note2str
from utils import read_int, write_int
import zzub
import config
import common
from common import MARGIN, MARGIN2, MARGIN3
player = common.get_player()

# size of border
BORDER = 5
# size of the envelope dots
DOTSIZE = 8
# matches existing points exactly or approximately
EXACT = 0
NEXT = 1

class WaveEditView(gtk.DrawingArea):
	"""
	Envelope viewer.
	
	A drawing surface where you can specify how the volume of a sample changes over time.
	"""
	
	def __init__(self, wavetable):
		"""
		Initialization.
		"""
		self.wavetable = wavetable
		self.wave = None
		self.level = 0
		gtk.DrawingArea.__init__(self)
		self.add_events(gtk.gdk.ALL_EVENTS_MASK)
		self.connect('button-press-event', self.on_left_down)
		self.connect('button-release-event', self.on_left_up)
		self.connect('motion-notify-event', self.on_motion)
		self.connect('enter-notify-event', self.on_enter)
		self.connect('leave-notify-event', self.on_leave)
		self.connect("expose_event", self.expose)
			
	def expose(self, widget, event):
		self.context = widget.window.cairo_create()
		self.draw(self.context)
		return False

	def get_client_size(self):
		rect = self.get_allocation()
		return rect.width, rect.height

	def redraw(self):
		if self.window:
			w,h = self.get_client_size()
			self.window.invalidate_rect((0,0,w,h), False)
			
	def on_enter(self, widget, event):
		"""
		Called when the mouse enters the wave editor.
		"""
		self.redraw()
		
	def on_leave(self, widget, event):
		"""
		Called when the mouse leaves the wave editor.
		"""
		self.redraw()
		
		
	def on_left_down(self, widget, event):
		"""
		Callback that responds to left mouse down over the wave view.
		"""
		mx,my = int(event.x), int(event.y)
		self.grab_add()
		self.redraw()

	def on_left_up(self, widget, event):
		"""
		Callback that responds to left mouse up over the wave view.
		"""
		self.grab_remove()
		self.redraw()

	def on_motion(self, widget, event):		
		"""
		Callback that responds to mouse motion over the wave view.
		"""
		mx,my,state = self.window.get_pointer()
		mx,my = int(mx),int(my)
		self.redraw()

	def update(self):
		"""
		Updates the envelope view based on the sample selected in the sample list.		
		"""
		sel = self.wavetable.get_sample_selection()
		if sel:
			self.wave = player.get_wave(sel[0])
		self.redraw()
		
	def set_sensitive(self, enable):
		gtk.DrawingArea.set_sensitive(self, enable)
		self.redraw()

	def draw(self, ctx):
		"""
		Overriding a L{Canvas} method that paints onto an offscreen buffer.
		Draws the envelope view graphics.
		"""	
		w,h = self.get_client_size()
		cfg = config.get_config()
		
		bgbrush = cfg.get_float_color('WE BG')
		pen = cfg.get_float_color('WE Line')
		brush = cfg.get_float_color('WE Fill')
		gridpen = cfg.get_float_color('WE Grid')
		
		ctx.translate(0.5,0.5)
		ctx.set_source_rgb(*bgbrush)
		ctx.rectangle(0,0,w,h)
		ctx.fill()
		ctx.set_line_width(1)
		
		level = self.wave.get_level(self.level)
		import ctypes
		minbuffer = (ctypes.c_float * w)()
		maxbuffer = (ctypes.c_float * w)()
		ampbuffer = (ctypes.c_float * w)()
		#~ print level.get_samples_digest(0, minbuffer, maxbuffer, ampbuffer, w)

		#~ if not self.envelope:
			#~ return
		#~ # 4096 (16)
		#~ # 8192 (8)
		#~ xlines = 16
		#~ ylines = 8
		#~ xf = 65535.0 / float(xlines)
		#~ yf = 65535.0 / float(ylines)
		#~ ctx.set_source_rgb(*gridpen)
		#~ for xg in range(xlines+1):
			#~ pt1 = self.env_to_pixel(xg*xf,0)
			#~ ctx.move_to(pt1[0],0)
			#~ ctx.line_to(pt1[0],h)
			#~ ctx.stroke()
		#~ for yg in range(ylines+1):
			#~ pt1 = self.env_to_pixel(0,yg*yf)
			#~ ctx.move_to(0,pt1[1])
			#~ ctx.line_to(w,pt1[1])
			#~ ctx.stroke()
		#~ if not self.get_property('sensitive'):
			#~ return
		#~ points = self.get_translated_points()
		#~ envp = None
		#~ ctx.move_to(*self.env_to_pixel(0,0))
		#~ for i in xrange(len(points)):
			#~ pt1 = points[max(i,0)]
			#~ ctx.line_to(pt1[0],pt1[1])
			#~ if pt1[2] & zzub.zzub_envelope_flag_sustain:
				#~ envp = pt1
		#~ ctx.line_to(*self.env_to_pixel(65535,0))
		#~ ctx.set_source_rgba(*(brush + (0.6,)))
		#~ ctx.fill_preserve()
		#~ ctx.set_source_rgb(*pen)
		#~ ctx.stroke()
		#~ if envp:
			#~ ctx.set_source_rgb(*sustainpen)
			#~ ctx.move_to(envp[0],0)
			#~ ctx.line_to(envp[0],h)
			#~ ctx.set_dash([4.0,2.0], 0.5)
			#~ ctx.stroke()
			#~ ctx.set_dash([], 0.0)
		#~ if self.showpoints:
			#~ for i in reversed(range(len(points))):
				#~ pt1 = points[max(i,0)]
				#~ pt2 = points[min(i+1,len(points)-1)]
				#~ if i == self.currentpoint:
					#~ ctx.set_source_rgb(*selectbrush)
				#~ else:
					#~ ctx.set_source_rgb(*dotbrush)
				#~ import math
				#~ ctx.arc(pt1[0],pt1[1],int((DOTSIZE/2.0)+0.5),0.0,math.pi*2)
				#~ ctx.fill()
