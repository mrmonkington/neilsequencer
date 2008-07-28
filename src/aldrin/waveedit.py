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
Provides dialogs, classes and controls to display/load/save envelopes
"""

import gtk
import os, sys
from utils import prepstr, db2linear, linear2db, note2str, file_filter
from utils import read_int, write_int, add_scrollbars, new_image_button, filepath
import zzub
import config
import common
from common import MARGIN, MARGIN2, MARGIN3

import aldrin.com as com

# size of border
BORDER = 5
# size of the envelope dots
DOTSIZE = 8
# matches existing points exactly or approximately
EXACT = 0
NEXT = 1

class WaveEditPanel(gtk.VBox):
	def __init__(self, wavetable):
		gtk.VBox.__init__(self, False, MARGIN)
		self.wavetable = wavetable
		self.view = WaveEditView(wavetable)
		self.set_border_width(MARGIN)
		self.waveedscrollwin = add_scrollbars(self.view)
		self.pack_start(self.waveedscrollwin)
		waveedbuttons = gtk.HBox(False, MARGIN)
		self.btndelrange = gtk.Button("Delete Range")
		waveedbuttons.pack_start(self.btndelrange, expand=False)
		self.btnstoresel = gtk.Button("Save Sel/Slices")
		waveedbuttons.pack_start(self.btnstoresel, expand=False)
		self.btnapplyslices = gtk.Button("Apply Slices")
		waveedbuttons.pack_start(self.btnapplyslices, expand=False)
		self.pack_end(waveedbuttons, expand=False)
		self.btndelrange.connect('clicked', self.on_delete_range)
		self.btnstoresel.connect('clicked', self.on_store_range)
		self.btnapplyslices.connect('clicked', self.on_apply_slices)
		
	def update(self, *args):
		self.view.update()
		
	def on_store_range(self, widget):
		self.view.store_range()
		
	def on_apply_slices(self, widget):
		self.view.apply_slices()

	def on_delete_range(self, widget):
		self.view.delete_range()
		self.wavetable.update_sampleprops()

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
		self.offpeak = 0.4
		self.onpeak = 0.9
		self.dragging = False
		self.stretching = False
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
		player = com.get('aldrin.core.player')
		if self.selection:
			begin,end = self.selection
			self.level.remove_sample_range(begin,end)
			self.selection = None
			player.history_commit("remove sample range")
			self.sample_changed()

	def redraw(self):
		if self.window:
			w,h = self.get_client_size()
			self.window.invalidate_rect((0,0,w,h), False)
			
	def update_digest(self):
		w,h = self.get_client_size()
		self.minbuffer, self.maxbuffer, self.ampbuffer = self.level.get_samples_digest(0, self.range[0], self.range[1],  w)
		
	def fix_range(self):
		begin,end = self.range
		begin = max(min(begin, self.level.get_sample_count()-1), 0)
		end = max(min(end, self.level.get_sample_count()), begin+1)
		self.range = [begin, end]
			
	def set_range(self, begin, end):
		self.range = [begin,end]
		self.view_changed()
		
	def set_selection(self, begin, end):
		if begin == end:
			self.selection = None
		else:
			begin = max(min(begin, self.level.get_sample_count()-1), 0)
			end = max(min(end, self.level.get_sample_count()), begin+1)
			self.selection = [begin, end]
		self.redraw()
		
	def client_to_sample(self, x, y, db=False):
		w,h = self.get_client_size()
		sample = self.range[0] + (float(x) / float(w)) * (self.range[1] - self.range[0])
		if db:
			amp = 1.0 - (float(y) / float(h))
		else:
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
		
	def apply_slices(self):
		self.level.clear_slices()
		for i,x in enumerate(self.peaks):
			res = self.level.add_slice(x)
			assert res == 0
		self.redraw()
		
	def store_range(self):
		w = self.wave
		origpath = w.get_path().replace('/',os.sep).replace('\\',os.sep)
		if origpath:
			filename = os.path.splitext(os.path.basename(origpath))[0]
		else:
			filename = w.get_name()
		if self.selection:
			filename += '_selection'
		else:
			filename += '_slice'
		filename += '.wav'
		if self.selection:
			title = "Export Selection"
		else:
			title = "Export Slices"
		dlg = gtk.FileChooserDialog(title, parent=self.get_toplevel(), action=gtk.FILE_CHOOSER_ACTION_SAVE,
			buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_SAVE, gtk.RESPONSE_OK))
		dlg.set_current_name(filename)
		dlg.set_do_overwrite_confirmation(True)
		dlg.add_filter(file_filter('Wave Files (*.wav)', '*.wav'))
		response = dlg.run()
		filepath = dlg.get_filename()
		dlg.destroy()
		if response != gtk.RESPONSE_OK:
			return
		if self.selection:
			begin,end = self.selection
			self.wave.save_sample_range(0, filepath, begin, end)
		else:
			filename, fileext = os.path.splitext(filepath)
			for i,x in enumerate(self.peaks):
				if i == (len(self.peaks)-1):
					end = self.level.get_sample_count()
				else:
					end = self.peaks[i+1]
				filepath = "%s%03i%s" % (filename, i, fileext)
				print filepath
				self.wave.save_sample_range(0, filepath, x, end)
			
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
		if (event.button == 1):
			s,a = self.client_to_sample(mx,my)
			if (event.state & gtk.gdk.SHIFT_MASK):
				if self.selection:
					begin,end = self.selection
					self.stretching = True
					self.stretchbegin = max(min(s, end),begin)
					self.stretchend = self.stretchbegin
					self.grab_add()
					self.redraw()
			elif (event.type == gtk.gdk._2BUTTON_PRESS):
				self.selection = None
				self.dragging = False
				self.redraw()
			else:
				self.dragging = True
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
		elif (event.button == 2):
			s,a = self.client_to_sample(mx,my,True)
			if (event.state & gtk.gdk.SHIFT_MASK):
				self.offpeak = a
			else:
				self.onpeak = a
			self.update_peaks()
			self.redraw()
				
	def sample_changed(self):
		self.update_peaks()
		self.view_changed()

	def view_changed(self):
		self.fix_range()		
		self.update_digest()
		self.redraw()
		
	def on_left_up(self, widget, event):
		"""
		Callback that responds to left mouse up over the wave view.
		"""
		if self.stretching == True:
			self.grab_remove()
			self.stretching = False
			begin,end = self.selection
			xo,xn = self.stretchbegin, self.stretchend # old pos, new pos
			if xo != xn:
				print "begin=%i, end=%i, xo=%i, xn=%i" % (begin,end,xo,xn)
				if end > xo:
					print "stretch 2nd part (from %i to %i (%i -> %i)" % (xo,end,end-xo,end-xn)
					self.level.stretch_range(xo,end,end - xn)
				if begin < xo:					
					print "stretch 1st part (from %i to %i (%i -> %i)" % (begin,xo,xo - begin, xn - begin)
					self.level.stretch_range(begin,xo,xn - begin)
				#~ self.level.stretch_range(self.range[0],self.range[1],(self.range[1] - self.range[0])*2)
			self.sample_changed()
		elif self.dragging == True:
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
		mx,my = int(event.x), int(event.y)
		s,a = self.client_to_sample(mx,my)
		if self.stretching == True:
			if self.selection:
				begin,end = self.selection
				self.stretchend = max(min(s, end),begin)
				self.redraw()
		elif self.dragging == True:
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
		player = com.get('aldrin.core.player')
		if sel:
			self.wave = player.get_wave(sel[0])
			if self.wave.get_level_count() >= 1:
				self.level = self.wave.get_level(0)
				self.range = [0,self.level.get_sample_count()]
				self.sample_changed()
		self.redraw()
	
	def update_peaks(self):
		print "updating peaks..."
		samplerate = int(self.level.get_samples_per_second())
		blocksize = min(self.level.get_sample_count(), 44)
		peaksize = self.level.get_sample_count() / blocksize
		minbuf, maxbuf, ampbuf = self.level.get_samples_digest(0, 0, self.level.get_sample_count(), peaksize)
		self.peaks = []
		minpeak = 1.0
		maxpeak = 0.0
		mind = 1.0 # min delta
		maxd = -1.0 # max delta
		os = 0.0
		power = 0
		for s in ampbuf:
			s = 1.0 + linear2db(s,-80.0) / 80.0
			power += s
			d = s - os
			os = s
			mind = min(mind, d)
			maxd = max(maxd, d)
			minpeak = min(minpeak, s)
			maxpeak = max(maxpeak, s)
		power = power / len(ampbuf)
		t = 0.2
		td = 0.01
		li = 0
		op = 0.0
		falloff = 1.0 / ((0.075 * 44100.0) / blocksize) # go to 0 peak after 75ms
		minb = (44100.0 / 20.0) / blocksize # dont divide below wavelengths of 20hz
		p = 0.0
		sleeping = True
		center = (power - minpeak) / (maxpeak - minpeak)
		peaktop = (self.onpeak - minpeak) / (maxpeak - minpeak)
		peakbottom = (self.offpeak - minpeak) / (maxpeak - minpeak)
		for i,s in enumerate(ampbuf):
			s = 1.0 + linear2db(s,-80.0) / 80.0
			# normalize peak sample
			s = (s - minpeak) / (maxpeak - minpeak)
			p = s#max(p - falloff, s)
			if sleeping: # we are sleeping
				if (p >= peaktop): # noise reaches threshold
					sleeping = False # wake up
					pos = max(i*blocksize - blocksize/2, 0)
					minb, maxb, ampb = self.level.get_samples_digest(0, pos, pos+blocksize, blocksize)
					bestmin = 1.0 + linear2db(ampb[0],-80.0) / 80.0
					bestpos = 0
					for j,t in enumerate(ampb):
						t = 1.0 + linear2db(t,-80.0) / 80.0
						if t < bestmin:
							bestpos = j
							bestmin = t
					self.peaks.append(pos + bestpos)
			else: # we are awake
				if (p <= peakbottom): # noise goes below threshold
					sleeping = True # sleep
		print "done."
		
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
		stretchbrush = cfg.get_float_color('WE Stretch Cue')
		splitbar = cfg.get_float_color('WE Split Bar')
		slicebar = cfg.get_float_color('WE Slice Bar')
		onpeak = cfg.get_float_color('WE Wakeup Peaks')
		offpeak = cfg.get_float_color('WE Sleep Peaks')
		
		ctx.translate(0.5,0.5)
		ctx.set_source_rgb(*bgbrush)
		ctx.rectangle(0,0,w,h)
		ctx.fill()
		ctx.set_line_width(1)
		
		if self.level == None:
			return
		
		player = com.get('aldrin.core.player')
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
				xp += spb
			ctx.stroke()
			spb *= 0.5
			ppb *= 0.5
			l += 1
			if (l > 5) or (ppb < 16):
				break
		
		#ctx.move_to(
		
		if len(self.ampbuffer) != w:
			print "updating peaks"
			self.update_digest()
		minbuffer, maxbuffer, ampbuffer = self.minbuffer, self.maxbuffer, self.ampbuffer

		ctx.set_source_rgba(*(brush2 + (0.5,)))
		ctx.move_to(0, h-1)
		for x in xrange(w):
			a = 1.0 + linear2db(ampbuffer[x],-80.0) / 80.0
			ctx.line_to(x, h-1-(h*a))
		ctx.line_to(w-1, h-1)
		ctx.fill()

		for x in self.peaks:
			x1 = self.sample_to_client(x, 0.0)[0]
			ctx.set_source_rgb(*splitbar)
			if (x1 >= 0) and (x1 <= w):
				ctx.move_to(x1, 0)
				ctx.line_to(x1, h)
				ctx.stroke()

#		print self.level.get_slice_count()
#		print self.level.get_slices()
#		for x in self.level.get_slices():
#			x1 = self.sample_to_client(x, 0.0)[0]
#			ctx.set_source_rgb(*slicebar)
#			if (x1 >= 0) and (x1 <= w):
#				ctx.move_to(x1, 0)
#				ctx.line_to(x1, h)
#				ctx.stroke()

		minp, maxp = self.offpeak, self.onpeak
		ctx.set_source_rgb(*offpeak)
		ctx.move_to(0, h-1-(h*minp))
		ctx.line_to(w-1, h-1-(h*minp))
		ctx.stroke()
		ctx.set_source_rgb(*onpeak)
		ctx.move_to(0, h-1-(h*maxp))
		ctx.line_to(w-1, h-1-(h*maxp))
		ctx.stroke()

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
				
		if self.stretching:
			x1 = self.sample_to_client(self.stretchbegin, 0.0)[0]
			x2 = self.sample_to_client(self.stretchend, 0.0)[0]
			ctx.set_source_rgb(*stretchbrush)
			if (x1 >= 0) and (x1 <= w):
				ctx.move_to(x1, 0)
				ctx.line_to(x1, h)
				ctx.stroke()
			if (x2 >= 0) and (x2 <= w):
				ctx.move_to(x2, 0)
				ctx.line_to(x2, h)
				ctx.stroke()
				ctx.rectangle(x2-4, (h/2)-4, 8, 8)
				ctx.stroke()


