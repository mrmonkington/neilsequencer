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
provides controls usually found on front panels of audio hardware.
"""

import gtk
import pango
import gobject
import cairo
import math
from colorsys import hls_to_rgb

def map_coords_linear(x,y):
	return x,1.0-y

def map_coords_spheric(x,y):
	nx = math.cos(x * 2 * math.pi) * y
	ny = -math.sin(x * 2 * math.pi) * y
	return nx, ny
	
def supershape(m, n1, n2, n3, phi):
	t1 = abs(math.cos(m * phi / 4.0))**n2
	t2 = abs(math.sin(m * phi / 4.0))**n3
	
	r = (t1+t2)**(1.0/n1)
	
	if r == 0:
		return 0,0
	r = 1.0/r
	return r

def get_peaks(f, tolerance=0.01, maxd=0.01, mapfunc=map_coords_linear):
	corners = 10000
	yc = 1.0/corners
	peaks = []
	x0,y0 = 0.0,0.0
	t0 = -9999.0
	i0 = 0
	s = 0.0
	for i in xrange(int(corners)):
		p = i*yc
		a = f(p)
		x,y = mapfunc(p, a)
		if i == 0:
			x0,y0 = x,y
		t = math.atan2((y0 - y), (x0 - x)) / (2*math.pi)
		s += (((x0 - x)**2) + ((y0 - y)**2))**0.5
		td = t - t0
		if (abs(td) >= tolerance):
			t0 = t
			peaks.append((x,y))
			s = 0.0
		x0,y0 = x,y
	return peaks

def make_knobshape(gaps=6, gapdepth=0.1):
	def knobshape_func(x):
		r = math.sin(x * 2 * math.pi * (gaps/2.0))
		r = (r ** 36)
		return 0.5 - r * gapdepth
	return get_peaks(knobshape_func, 0.1, 0.05, map_coords_spheric)

class Knob(gtk.DrawingArea):
	def __init__(self):
		gtk.DrawingArea.__init__(self)
		self.connect('expose-event', self.on_expose)
		self.hue = 0.0
		self.gapdepth = 0.1
		self.gaps = 6
		self.value = 0.0
		self.min_value = 0.0
		self.max_value = 1.0
		self.sat = 0.0
		self.dragging = False
		self.start = 0.0
		self.angle = (5.0/6.0) * 2 * math.pi
		self.knobshape = None
		self.add_events(gtk.gdk.ALL_EVENTS_MASK)
		self.connect('button-press-event', self.on_left_down)
		self.connect('button-release-event', self.on_left_up)
		self.connect('motion-notify-event', self.on_motion)
		self.connect('realize', self.on_realize)
		
	def on_realize(self, widget):
		self.knobshape = make_knobshape(self.gaps, self.gapdepth)
		
	def update_knobshape(self):
		self.knobshape = make_knobshape(self.gaps, self.gapdepth)
		
	def set_value(self, value):
		self.value = value
		self.refresh()
		
	def get_value(self):
		return self.value
		
	def set_color(self, hue, saturation):
		self.hue = hue
		self.sat = saturation
		self.refresh()
		
	def get_color(self):
		return self.hue, self.sat
		
	def set_gaps(self, gaps):
		self.gaps = gaps
		self.knobshape = None
		self.refresh()
		
	def get_gaps(self):
		return self.gaps
		
	def set_gap_depth(self, gapdepth):
		self.gapdepth = gapdepth
		self.knobshape = None
		self.refresh()
		
	def get_gap_depth(self):
		return self.gapdepth
		
	def set_angle(self, angle):
		self.angle = angle
		self.refresh()
		
	def get_angle(self):
		return self.angle
		
	def on_left_down(self, widget, event):
		if event.button == 1:
			self.startvalue = self.value
			self.start = event.y
			self.dragging = True
			self.grab_add()

	def on_left_up(self, widget, event):
		if event.button == 1:
			self.dragging = False
			self.grab_remove()

	def on_motion(self, widget, event):
		x,y,state = self.window.get_pointer()
		if self.dragging:
			rc = self.get_allocation()
			range = self.max_value - self.min_value
			value = self.startvalue - ((y - self.start)*range)/rc.height
			self.value = min(max(value, self.min_value), self.max_value)
			self.refresh()

	def draw_points(self, ctx, peaks):
		ctx.move_to(*peaks[0])
		for peak in peaks[1:]:
			ctx.line_to(*peak)
		
	def draw(self, ctx):
		if not self.knobshape:
			self.knobshape = make_knobshape(self.gaps, self.gapdepth)
		angle = self.value*self.angle + math.pi*1.5 - self.angle*0.5
		rc = self.get_allocation()
		size = min(rc.width, rc.height)
		kh = 4 # knob height
		ps = 1.0/size # pixel size
		ss = ps * kh # shadow size
		# draw spherical
		ctx.translate(0.5,0.5)
		ctx.translate(size*0.5, size*0.5)
		ctx.scale(size-(2*kh)-1, size-(2*kh)-1)
		ctx.save()
		ctx.translate(ss, ss)
		ctx.rotate(angle)
		self.draw_points(ctx, self.knobshape)
		ctx.close_path()
		ctx.restore()
		ctx.set_source_rgba(0,0,0,0.3)
		ctx.fill()
		pat = cairo.LinearGradient(-0.5, -0.5, 0.5, 0.5)
		pat.add_color_stop_rgb(1.0, 0.2,0.2,0.2)
		pat.add_color_stop_rgb(0.0, 0.3,0.3,0.3)
		ctx.set_source(pat)
		ctx.rotate(angle)
		self.draw_points(ctx, self.knobshape)
		ctx.close_path()
		ctx.fill_preserve()
		ctx.set_source_rgba(0.1,0.1,0.1,1)
		ctx.save()
		ctx.identity_matrix()
		ctx.set_line_width(1.0)
		ctx.stroke()
		ctx.restore()
		ctx.arc(0.0, 0.0, 0.5-self.gapdepth, 0.0, math.pi*2.0)
		ctx.set_source_rgb(*hls_to_rgb(self.hue, 0.3, self.sat))
		ctx.fill()
		ctx.arc(0.0, 0.0, 0.5-self.gapdepth-ps, 0.0, math.pi*2.0)
		ctx.set_source_rgb(*hls_to_rgb(self.hue, 0.7, self.sat))
		ctx.fill()
		ctx.arc(0.0, 0.0, 0.5-self.gapdepth-(2*ps), 0.0, math.pi*2.0)
		ctx.set_source_rgb(*hls_to_rgb(self.hue, 0.6, self.sat))
		ctx.fill()
		ctx.move_to(0.1, 0.0)
		ctx.line_to(0.5-self.gapdepth-ps, 0.0)
		ctx.save()
		ctx.identity_matrix()
		ctx.translate(0.5,0.5)
		ctx.set_line_width(5)
		ctx.set_source_rgb(*hls_to_rgb(self.hue, 0.7, self.sat))
		ctx.stroke_preserve()
		ctx.set_line_width(3)
		ctx.set_source_rgb(*hls_to_rgb(self.hue, 0.3, self.sat))
		ctx.stroke()
		ctx.restore()
		
	def refresh(self):
		rect = self.get_allocation()
		if self.window:
			self.window.invalidate_rect((0,0,rect.width,rect.height), False)
		return True
		
	def on_expose(self, widget, event):
		self.context = widget.window.cairo_create()
		self.draw(self.context)
		return False

if __name__ == '__main__':
	window = gtk.Window()
	window.connect('destroy', lambda widget: gtk.main_quit())
	vbox = gtk.VBox()
	hbox = gtk.HBox(False)
	def add_knob(size, value, hue, sat, gaps, gapdepth):
		knob = Knob()
		knob.set_value(value)
		knob.set_color(hue, sat)
		knob.set_gaps(gaps)
		knob.set_gap_depth(gapdepth)
		knob.set_size_request(size,size)
		vbox = gtk.VBox()
		vbox.pack_start(gtk.VBox())
		vbox.pack_start(knob, expand=False)
		vbox.pack_start(gtk.VBox())
		hbox.pack_start(vbox, expand=False)
	s = 0.3
	add_knob(64, 0.0, 0.0, s, 12, 0.05)
	add_knob(64, 0.0, 0.1, s, 12, 0.05)
	add_knob(48, 0.0, 0.2, s, 9, 0.067)
	add_knob(48, 0.0, 0.3, s, 9, 0.067)
	add_knob(48, 0.0, 0.4, s, 9, 0.067)
	add_knob(48, 0.5, 0.5, s, 9, 0.067)
	add_knob(32, 0.0, 0.6, s, 6, 0.1)
	add_knob(32, 0.0, 0.7, s, 6, 0.1)
	add_knob(32, 0.0, 0.8, s, 6, 0.1)
	add_knob(32, 1.0, 0.9, s, 6, 0.1)
	vbox.pack_start(hbox, expand=False)
	window.add(vbox)
	window.show_all()
	gtk.main()
