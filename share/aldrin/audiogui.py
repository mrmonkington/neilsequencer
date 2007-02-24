#encoding: latin-1

# Aldrin
# Modular Sequencer
# Copyright (C) 2006 Leonard Ritter <contact@leonard-ritter.com>
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
Provides controls usually found on front panels of audio hardware.

See the bottom for some examples.
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

def draw_round_rectangle(ctx, x, y, w, h, arc, ratio):
	arch = arc*ratio # arc handle length
	ctx.move_to(x+arc, y)
	ctx.line_to(x+w-arc, y)
	ctx.curve_to(x+w-arch, y, x+w, y+arch, x+w, y+arc)
	ctx.line_to(x+w, y+h-arc)
	ctx.curve_to(x+w, y+h-arch, x+w-arch, y+h, x+w-arc, y+h)
	ctx.line_to(x+arc, y+h)
	ctx.curve_to(x+arch, y+h, x, y+h-arch, x, y+h-arc)
	ctx.line_to(x, y+arc)
	ctx.curve_to(x, y+arch, x+arch, y, x+arc, y)
	ctx.close_path()

def get_peaks(f, tolerance=0.01, maxd=0.01, mapfunc=map_coords_linear):
	corners = 360
	yc = 1.0/corners
	peaks = []
	x0,y0 = 0.0,0.0
	t0 = -9999.0
	i0 = 0
	for i in xrange(int(corners)):
		p = i*yc
		a = f(p)
		x,y = mapfunc(p, a)
		if i == 0:
			x0,y0 = x,y
		t = math.atan2((y0 - y), (x0 - x)) / (2*math.pi)
		td = t - t0
		if (abs(td) >= tolerance):
			t0 = t
			peaks.append((x,y))
		x0,y0 = x,y
	return peaks

def make_knobshape(gaps, gapdepth):
	def knobshape_func(x):
		x = (x*gaps)%1.0
		w = 0.5
		g1 = 0.5 - w*0.5
		g2 = 0.5 + w*0.5
		if (x >= g1) and (x < 0.5):
			x = (x-g1)/(w*0.5)
			return 0.5 - gapdepth * x * 0.9
		elif (x >= 0.5) and (x < g2):
			x = (x-0.5)/(w*0.5)
			return 0.5 - gapdepth * (1-x) * 0.9
		else:
			return 0.5
	return get_peaks(knobshape_func, 0.01, 0.05, map_coords_spheric)
	
def hls_to_color(h,l,s):
	r,g,b = hls_to_rgb(h,l,s)
	return gtk.gdk.color_parse('#%04X%04X%04X' % (int(r*65535),int(g*65535),int(b*65535)))

class Knob(gtk.VBox):
	def __init__(self):
		gtk.VBox.__init__(self)
		self.gapdepth = 0.1
		self.gaps = 6
		self.value = 0.0
		self.min_value = 0.0
		self.max_value = 127.0
		self.fg_hls = 0.0, 0.5, 0.0
		self.dragging = False
		self.start = 0.0
		self.digits = 0
		self.angle = (3.0/4.0) * 2 * math.pi
		self.knobshape = None
		self.set_double_buffered(True)
		self.connect('realize', self.on_realize)
		self.connect('expose-event', self.on_expose)
		
	def on_realize(self, widget):
		self.root = self.get_toplevel()
		self.root.add_events(gtk.gdk.ALL_EVENTS_MASK)
		self.root.connect('scroll-event', self.on_mousewheel)
		self.root.connect('button-press-event', self.on_left_down)
		self.root.connect('button-release-event', self.on_left_up)
		self.root.connect('motion-notify-event', self.on_motion)
		self.update_knobshape()
		
	def update_knobshape(self):
		self.knobshape = make_knobshape(self.gaps, self.gapdepth)
	
	def set_range(self, minvalue, maxvalue):
		self.min_value = minvalue
		self.max_value = maxvalue
		self.set_value(self.value)
		
	def quantize_value(self, value):
		scaler = 10**self.digits
		value = int((value*scaler)+0.5) / float(scaler)
		return value
		
	def set_value(self, value):
		oldval = self.value
		self.value = min(max(self.quantize_value(value), self.min_value), self.max_value)
		if self.value != oldval:
			self.refresh()
		
	def get_value(self):
		return self.value
		
	def set_top_color(self, h, l, s):
		self.fg_hls = h,l,s
		self.refresh()
		
	def get_top_color(self):
		return self.fg_hls
		
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
		if not sum(self.get_allocation().intersect((int(event.x), int(event.y), 1, 1))):
			return
		if event.button == 1:
			self.startvalue = self.value
			self.start = event.y
			self.dragging = True
			self.grab_add()

	def on_left_up(self, widget, event):
		if not self.dragging:
			return
		if event.button == 1:
			self.dragging = False
			self.grab_remove()

	def on_motion(self, widget, event):
		if self.dragging:
			x,y,state = self.window.get_pointer()
			rc = self.get_allocation()
			range = self.max_value - self.min_value
			scale = rc.height*2
			if event.state & gtk.gdk.SHIFT_MASK:
				scale = rc.height*8
			value = self.startvalue - ((y - self.start)*range)/scale
			oldval = self.value
			self.set_value(value)
			if oldval != self.value:
				self.start = y
				self.startvalue = self.value
			
	def on_mousewheel(self, widget, event):
		if not sum(self.get_allocation().intersect((int(event.x), int(event.y), 1, 1))):
			return
		range = self.max_value - self.min_value
		minstep = 1.0 / (10**self.digits)
		if event.state & gtk.gdk.SHIFT_MASK:
			step = minstep
		else:
			step = max(self.quantize_value(range/25.0), minstep)
		value = self.value
		if event.direction == gtk.gdk.SCROLL_UP:
			value += step
		elif event.direction == gtk.gdk.SCROLL_DOWN:
			value -= step
		self.set_value(value)

	def draw_points(self, ctx, peaks):
		ctx.move_to(*peaks[0])
		for peak in peaks[1:]:
			ctx.line_to(*peak)
		
	def draw(self, ctx):
		if not self.knobshape:
			self.update_knobshape()
		angle = (self.value/(self.max_value - self.min_value))*self.angle + math.pi*1.5 - self.angle*0.5
		rc = self.get_allocation()
		size = min(rc.width, rc.height)
		kh = self.get_border_width() # knob height
		ps = 1.0/size # pixel size
		ss = ps * kh # shadow size
		# draw spherical
		ctx.translate(rc.x, rc.y)
		ctx.translate(0.5,0.5)
		ctx.translate(size*0.5, size*0.5)
		ctx.scale(size-(2*kh)-1, size-(2*kh)-1)
		if kh:
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
		ctx.set_source_rgb(*hls_to_rgb(self.fg_hls[0], max(self.fg_hls[1]*0.4,0.0), self.fg_hls[2]))
		ctx.fill()
		ctx.arc(0.0, 0.0, 0.5-self.gapdepth-ps, 0.0, math.pi*2.0)
		ctx.set_source_rgb(*hls_to_rgb(self.fg_hls[0], min(self.fg_hls[1]*1.2,1.0), self.fg_hls[2]))
		ctx.fill()
		ctx.arc(0.0, 0.0, 0.5-self.gapdepth-(2*ps), 0.0, math.pi*2.0)
		ctx.set_source_rgb(*hls_to_rgb(*self.fg_hls))
		ctx.fill()
		ctx.move_to(0.1, 0.0)
		ctx.line_to(0.5-self.gapdepth-ps, 0.0)
		ctx.save()
		ctx.identity_matrix()
		ctx.translate(0.5,0.5)
		ctx.set_line_width(5)
		ctx.set_source_rgb(*hls_to_rgb(self.fg_hls[0], min(self.fg_hls[1]*1.2,1.0), self.fg_hls[2]))
		ctx.stroke_preserve()
		ctx.set_line_width(3)
		ctx.set_source_rgb(*hls_to_rgb(self.fg_hls[0], max(self.fg_hls[1]*0.4,0.0), self.fg_hls[2]))
		ctx.stroke()
		ctx.restore()
		
	def refresh(self):
		rect = self.get_allocation()
		if self.window:
			self.window.invalidate_rect(rect, False)
		return True
		
	def on_expose(self, widget, event):
		self.context = self.window.cairo_create()
		self.draw(self.context)
		return False
		
class DecoBox(gtk.VBox):
	def __init__(self):
		gtk.VBox.__init__(self)
		self.arc = 6.0
		self.fg_hls = 0.0, 1.0, 0.0
		self.bg_hls = 0.0, 0.3, 0.618
		self.filled = False
		self.thickness = 0.0
		self.ratio = 0.382
		self.set_app_paintable(True)
		self.connect('expose-event', self.on_expose)
		
	def set_thickness(self, thickness):
		self.thickness = thickness
		self.refresh()
	
	def get_thickness(self):
		return self.thickness
		
	def set_roundness_ratio(self, ratio):
		self.ratio = ratio
		self.refresh()
		
	def get_roundness_ratio(self):
		return self.ratio
		
	def set_roundness(self, roundness):
		self.arc = roundness
		self.refresh()
		
	def set_roundness(self):
		return self.arc

	def set_fg_color(self, h, l, s):
		self.fg_hls = h,l,s
		self.refresh()

	def get_fg_color():
		return self.fg_hsl

	def set_bg_color(self, h, l, s):
		self.bg_hls = h,l,s
		self.refresh()

	def get_bg_color():
		return self.bg_hsl

	def refresh(self):
		rect = self.get_allocation()
		if self.window:
			self.window.invalidate_rect((rc.x, rc.y,rect.width,rect.height), False)
		return True

	def draw(self, ctx):
		rc = self.get_allocation()
		x,y,w,h = rc.x, rc.y, rc.width, rc.height
		bw = self.get_border_width()
		x += bw
		y += bw
		w -= bw*2
		h -= bw*2
		ctx.set_source_rgb(*hls_to_rgb(*self.bg_hls))
		draw_round_rectangle(ctx, x, y, w, h, self.arc, self.ratio)
		ctx.fill()
		if self.thickness:
			ctx.set_source_rgb(*hls_to_rgb(*self.fg_hls))
			th = self.thickness*0.5
			draw_round_rectangle(ctx, x+th, y+th, w-self.thickness, h-self.thickness, self.arc, self.ratio)
			ctx.set_line_width(self.thickness)
			ctx.stroke()

	def on_expose(self, widget, event):
		self.context = widget.window.cairo_create()
		self.draw(self.context)
		return False
		
def render_stuff(self, widget):
	ctx = widget.window.cairo_create()
	ctx.set_source_rgb(1.0,0.0,0.0)
	ctx.paint()
	
if __name__ == '__main__':
	window = gtk.Window()
	window.connect('destroy', lambda widget: gtk.main_quit())
	s = 0.618
	def new_vbox():
		vbox = DecoBox()
		hbox = gtk.HBox()
		vbox.add(hbox)
		return vbox, hbox
	def new_knob(size, value, hue, sat, gaps, gapdepth):
		knob = Knob()
		knob.set_border_width(6)
		knob.set_value(value)
		knob.set_top_color(hue, 0.7, sat)
		knob.set_gaps(gaps)
		knob.set_gap_depth(gapdepth)
		knob.set_size_request(size,size)
		return knob
	window.modify_bg(gtk.STATE_NORMAL, hls_to_color(0.0, 0.2, s))
	hbox = gtk.HBox(False, 6)
	hbox.set_border_width(6)
	vb, hb = new_vbox()
	hbox.pack_start(vb, expand=False)
	hb.pack_start(new_knob(64, 0.0, 0.0, 0.1, 12, 0.05), expand=False)
	hb.pack_start(new_knob(64, 0.0, 0.0, 0.1, 12, 0.05), expand=False)
	vb, hb = new_vbox()
	vb.set_thickness(3)
	vb.set_fg_color(0.0, 0.4, s)
	hbox.pack_start(vb, expand=False)
	hb.pack_start(new_knob(48, 0.0, 0.0, s, 9, 0.067), expand=False)
	hb.pack_start(new_knob(48, 0.0, 0.0, s, 9, 0.067), expand=False)
	hb.pack_start(new_knob(48, 0.0, 0.0, s, 9, 0.067), expand=False)
	hb.pack_start(new_knob(48, 0.5, 0.0, s, 9, 0.067), expand=False)
	vb, hb = new_vbox()
	vb.set_bg_color(0.0, 0.4, s)
	hbox.pack_start(vb, expand=False)
	hb.pack_start(new_knob(32, 0.0, 0.6, 0.1, 6, 0.1), expand=False)
	hb.pack_start(new_knob(32, 0.0, 0.6, 0.1, 6, 0.1), expand=False)
	hb.pack_start(new_knob(32, 0.0, 0.6, 0.1, 6, 0.1), expand=False)
	hb.pack_start(new_knob(32, 1.0, 0.6, 0.1, 6, 0.1), expand=False)
	window.add(hbox)
	window.show_all()
	gtk.main()
