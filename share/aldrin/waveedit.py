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
		self.level = None
		self.dragging = False
		gtk.DrawingArea.__init__(self)
		self.add_events(gtk.gdk.ALL_EVENTS_MASK)
		self.connect('button-press-event', self.on_left_down)
		self.connect('button-release-event', self.on_left_up)
		self.connect('motion-notify-event', self.on_motion)
		self.connect('enter-notify-event', self.on_enter)
		self.connect('leave-notify-event', self.on_leave)
		self.connect('scroll-event', self.on_mousewheel)
		self.connect("expose_event", self.expose)
			
	def expose(self, widget, event):
		self.context = widget.window.cairo_create()
		self.draw(self.context)
		return False

	def get_client_size(self):
		rect = self.get_allocation()
		return rect.width, rect.height
		
	def delete_range(self):
		if self.selection:
			begin,end = self.selection
			self.level.remove_range(begin,end)
			self.selection = None
			begin,end = self.range
			self.set_range(begin,end)
			self.redraw()

	def redraw(self):
		if self.window:
			w,h = self.get_client_size()
			self.window.invalidate_rect((0,0,w,h), False)
			
	def set_range(self, begin, end):
		print "set_range"
		begin = max(min(begin, self.level.get_sample_count()-1), 0)
		end = max(min(end, self.level.get_sample_count()), begin+1)
		self.range = [begin, end]
		w,h = self.get_client_size()
		self.minbuffer, self.maxbuffer, self.ampbuffer = self.level.get_samples_digest(0, self.range[0], self.range[1],  w)
		self.redraw()
		
	def set_selection(self, begin, end):
		begin = max(min(begin, self.level.get_sample_count()-1), 0)
		end = max(min(end, self.level.get_sample_count()), begin+1)
		self.selection = [begin, end]
		self.redraw()
		
	def client_to_sample(self, x, y):
		w,h = self.get_client_size()
		sample = self.range[0] + (float(x) / float(w)) * (self.range[1] - self.range[0])
		amp = (float(y) / float(h)) * 2.0 - 1.0
		return int(sample+0.5),amp
		
	def sample_to_client(self, s, a):
		w,h = self.get_client_size()
		x = ((float(s) - self.range[0]) / (self.range[1] - self.range[0])) * w
		y = (a + 1.0) * float(h) / 2.0
		return x,y
			
	def on_mousewheel(self, widget, event):
		"""
		Callback that responds to mousewheeling in pattern view.
		"""		
		mx,my = int(event.x), int(event.y)
		s,a = self.client_to_sample(mx,my)
		b,e = self.range
		diffl = s - b
		diffr = e - s
		if event.direction == gtk.gdk.SCROLL_DOWN:
			diffl *= 2
			diffr *= 2
		elif event.direction == gtk.gdk.SCROLL_UP:
			diffl /= 2
			diffr /= 2
		self.set_range(s - diffl, s + diffr)
		self.redraw()
			
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
		self.dragging = True
		mx,my = int(event.x), int(event.y)
		s,a = self.client_to_sample(mx,my)
		if self.selection:
			begin,end = self.selection
			if abs(s - begin) < abs(s - end):
				self.startpos = end
			else:
				self.startpos = begin
		else:
			self.startpos = s
		self.grab_add()
		self.redraw()

	def on_left_up(self, widget, event):
		"""
		Callback that responds to left mouse up over the wave view.
		"""
		if self.dragging == True:
			self.dragging = False
			self.grab_remove()
			mx,my = int(event.x), int(event.y)
			s,a = self.client_to_sample(mx,my)
			if s < self.startpos:
				self.set_selection(s, self.startpos)
			else:
				self.set_selection(self.startpos, s)
			self.redraw()

	def on_motion(self, widget, event):		
		"""
		Callback that responds to mouse motion over the wave view.
		"""
		mx,my,state = self.window.get_pointer()
		mx,my = int(mx),int(my)
		if self.dragging == True:
			mx,my = int(event.x), int(event.y)
			s,a = self.client_to_sample(mx,my)
			if s < self.startpos:
				self.set_selection(s, self.startpos)
			else:
				self.set_selection(self.startpos, s)
			self.redraw()

	def update(self):
		"""
		Updates the envelope view based on the sample selected in the sample list.		
		"""
		sel = self.wavetable.get_sample_selection()
		self.wave = None
		self.level = None
		self.selection = None
		if sel:
			self.wave = player.get_wave(sel[0])
			if self.wave.get_level_count() >= 1:
				self.level = self.wave.get_level(0)
				self.set_range(0,self.level.get_sample_count())
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
		brush2 = cfg.get_float_color('WE Peak Fill')
		gridpen = cfg.get_float_color('WE Grid')
		selbrush = cfg.get_float_color('WE Selection')
		
		ctx.translate(0.5,0.5)
		ctx.set_source_rgb(*bgbrush)
		ctx.rectangle(0,0,w,h)
		ctx.fill()
		ctx.set_line_width(1)
		
		if self.level == None:
			return
		
		ctx.set_source_rgb(*gridpen)
		rb,re = self.range
		rsize = re - rb
		spb = (60.0 * float(self.level.get_samples_per_second())) / float(player.get_bpm()) # samples per beat
		ppb = (float(w) * spb)/ rsize # pixels begin
		l = 0
		while True:
			tb = int((rb / spb)) * spb
			te = int((re / spb)+1) * spb
			xp = tb
			a = 0.2
			ctx.set_source_rgba(*(gridpen + (a,)))
			while xp < te:
				x1 = self.sample_to_client(xp, 0.0)[0]
				ctx.move_to(x1, 0)
				ctx.line_to(x1, h)
				ctx.stroke()
				xp += spb
			spb *= 0.5
			ppb *= 0.5
			l += 1
			if (l > 5) or (ppb < 16):
				break
		
		#ctx.move_to(
		
		minbuffer, maxbuffer, ampbuffer = self.minbuffer, self.maxbuffer, self.ampbuffer

		ctx.set_source_rgba(*(brush2 + (0.5,)))
		ctx.move_to(0, h-1)
		for x in xrange(w):
			a = 1.0 + linear2db(ampbuffer[x],-80.0) / 80.0
			ctx.line_to(x, h-1-(h*a))
		ctx.line_to(w-1, h-1)
		ctx.fill()

		ctx.set_source_rgba(*(brush + (0.5,)))
		hm = h/2
		ctx.move_to(0, hm)
		for x in xrange(w):
			ctx.line_to(x, hm - h*maxbuffer[x]*0.5)
		for x in xrange(w):
			ctx.line_to(w-x, hm - h*minbuffer[w-x-1]*0.5)
		ctx.fill_preserve()
		ctx.set_source_rgb(*pen)
		ctx.stroke()
		
		if self.selection:
			begin, end = self.selection
			x1 = self.sample_to_client(begin, 0.0)[0]
			x2 = self.sample_to_client(end, 0.0)[0]
			if (x2 >= 0) and (x1 <= w):
				ctx.set_source_rgba(*selbrush + (0.3,))
				ctx.rectangle(x1, 0, x2-x1, h)
				ctx.fill()

if __name__ == '__main__':
	import sys, utils
	from main import run
	run(sys.argv + [utils.filepath('demosongs/paniq-knark.ccm')])
