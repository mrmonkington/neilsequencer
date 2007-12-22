#encoding: latin-1

# AudioGUI
# Provides GTK+ controls usually found on front panels of audio hardware.
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

def draw_round_rectangle(ctx, x, y, w, h, arc1, arc2=None, arc3=None, arc4=None, ratio=0.618, tabwidth=0, tabheight=0):
	if arc2 == None:
		arc2 = arc1
	if arc3 == None:
		arc3 = arc1
	if arc4 == None:
		arc4 = arc1
	arch1 = arc1*ratio # arc handle length
	arch2 = arc2*ratio # arc handle length
	arch3 = arc3*ratio # arc handle length
	arch4 = arc4*ratio # arc handle length
	ctx.move_to(x+arc1, y)
	if tabwidth and tabheight:
		curvewidth = tabheight*2
		ctx.line_to(x+tabwidth, y)
		arch = curvewidth*0.5
		ctx.curve_to(x+tabwidth+arch, y, x+tabwidth+arch, y+tabheight, x+tabwidth+curvewidth, y+tabheight)
	ctx.line_to(x+w-arc2, y+tabheight)
	ctx.curve_to(x+w-arch2, y+tabheight, x+w, y+tabheight+arch2, x+w, y+tabheight+arc2)
	ctx.line_to(x+w, y+h-arc3)
	ctx.curve_to(x+w, y+h-arch3, x+w-arch3, y+h, x+w-arc3, y+h)
	ctx.line_to(x+arc4, y+h)
	ctx.curve_to(x+arch4, y+h, x, y+h-arch4, x, y+h-arc4)
	ctx.line_to(x, y+arc1)
	ctx.curve_to(x, y+arch1, x+arch1, y, x+arc1, y)
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
	return get_peaks(knobshape_func, 0.03, 0.05, map_coords_spheric)
	
def hls_to_color(h,l,s):
	r,g,b = hls_to_rgb(h,l,s)
	return gtk.gdk.color_parse('#%04X%04X%04X' % (int(r*65535),int(g*65535),int(b*65535)))
	
MARKER_NONE = ''
MARKER_LINE = 'line'
MARKER_ARROW = 'arrow'
MARKER_DOT = 'dot'
	
LEGEND_NONE = ''
LEGEND_DOTS = 'dots' # painted dots
LEGEND_LINES = 'lines' # painted ray-like lines
LEGEND_RULER = 'ruler' # painted ray-like lines + a circular one
LEGEND_RULER_INWARDS = 'ruler-inwards' # same as ruler, but the circle is on the outside
LEGEND_LED_SCALE = 'led-scale' # an LCD scale
LEGEND_LED_DOTS = 'led-dots' # leds around the knob

class KnobTooltip:
	def __init__(self):
		self.tooltip_window = gtk.Window(gtk.WINDOW_POPUP)
		self.tooltip = gtk.Label()
		self.tooltip.modify_fg(gtk.STATE_NORMAL, hls_to_color(0.0, 1.0, 0.0))
		self.tooltip_timeout = None
		vbox = gtk.VBox()
		vbox2 = gtk.VBox()
		vbox2.add(self.tooltip)
		vbox2.set_border_width(2)
		vbox.add(vbox2)
		self.tooltip_window.add(vbox)
		vbox.connect('expose-event', self.on_tooltip_expose)

	def show_tooltip(self, knob):
		text = knob.format_value(knob.value)
		self.tooltip.set_text(knob.format_value(knob.max_value))
		rc = knob.get_allocation()
		x,y = knob.window.get_origin()
		self.tooltip_window.show_all()
		w,h = self.tooltip_window.get_size()
		wx,wy = x+rc.x-w, y+rc.y+rc.height/2-h/2
		self.tooltip_window.move(wx,wy)
		rc = self.tooltip_window.get_allocation()
		self.tooltip_window.window.invalidate_rect((0,0,rc.width,rc.height), False)
		self.tooltip.set_text(text)
		if self.tooltip_timeout:
			gobject.source_remove(self.tooltip_timeout)
		self.tooltip_timeout = gobject.timeout_add(500, self.hide_tooltip)
			
	def hide_tooltip(self):
		self.tooltip_window.hide_all()
		
	def on_tooltip_expose(self, widget, event):
		ctx = widget.window.cairo_create()
		rc = widget.get_allocation()
		ctx.set_source_rgb(*hls_to_rgb(0.0, 0.5, 1.0))
		ctx.paint()
		ctx.set_source_rgb(*hls_to_rgb(0.0, 1.0, 0.0))
		ctx.translate(0.5, 0.5)
		ctx.set_line_width(1)
		ctx.rectangle(rc.x, rc.y,rc.width-1,rc.height-1)
		ctx.stroke()
		return False



knob_tooltip = None
def get_knob_tooltip():
	global knob_tooltip
	if not knob_tooltip:
		knob_tooltip = KnobTooltip()
	return knob_tooltip

class Knob(gtk.VBox):
	def __init__(self):
		gtk.VBox.__init__(self)
		self.gapdepth = 6
		self.gaps = 6
		self.value = 0.0
		self.min_value = 0.0
		self.max_value = 127.0
		self.fg_hls = 0.0, 0.5, 0.0
		self.legend_hls = 0.0, 1.0, 0.0
		self.dragging = False
		self.start = 0.0
		self.digits = 0
		self.segments = 13
		self.label = ''
		self.marker = MARKER_LINE
		self.angle = (3.0/4.0) * 2 * math.pi
		self.knobshape = None
		self.legend = LEGEND_NONE
		self.lsize = 2
		self.lscale = False
		self.set_double_buffered(True)
		self.connect('realize', self.on_realize)
		self.connect('expose-event', self.on_expose)
		
	def format_value(self, value):
		return ("%%.%if" % self.digits) % value

	def show_tooltip(self):
		get_knob_tooltip().show_tooltip(self)
		
	def on_realize(self, widget):
		self.root = self.get_toplevel()
		self.root.add_events(gtk.gdk.ALL_EVENTS_MASK)
		self.root.connect('scroll-event', self.on_mousewheel)
		self.root.connect('button-press-event', self.on_left_down)
		self.root.connect('button-release-event', self.on_left_up)
		self.root.connect('motion-notify-event', self.on_motion)
		self.update_knobshape()
		
	def update_knobshape(self):
		rc = self.get_allocation()
		b = self.get_border_width()
		size = min(rc.width, rc.height) - 2*b
		gd = float(self.gapdepth*0.5) / size
		self.gd = gd
		self.knobshape = make_knobshape(self.gaps, gd)
		
	def set_legend_scale(self, scale):
		self.lscale = scale
		self.refresh()
		
	def set_legend_line_width(self, width):
		self.lsize = width
		self.refresh()
		
	def set_segments(self, segments):
		self.segments = segments
		self.refresh()
		
	def set_marker(self, marker):
		self.marker = marker
		self.refresh()
	
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
		
	def set_legend_color(self, h, l, s):
		self.legend_hls = h,l,s
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
		
	def set_legend(self, legend):
		self.legend = legend
		self.refresh()
		
	def get_legend(self):
		return self.legend
		
	def on_left_down(self, widget, event):
		if not sum(self.get_allocation().intersect((int(event.x), int(event.y), 1, 1))):
			return
		if event.button == 1:
			self.startvalue = self.value
			self.start = event.y
			self.dragging = True
			self.show_tooltip()
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
			scale = rc.height
			if event.state & gtk.gdk.SHIFT_MASK:
				scale = rc.height*8
			value = self.startvalue - ((y - self.start)*range)/scale
			oldval = self.value
			self.set_value(value)
			self.show_tooltip()
			if oldval != self.value:
				self.start = y
				self.startvalue = self.value
			
	def on_mousewheel(self, widget, event):
		if not sum(self.get_allocation().intersect((int(event.x), int(event.y), 1, 1))):
			return
		range = self.max_value - self.min_value
		minstep = 1.0 / (10**self.digits)
		if event.state & (gtk.gdk.SHIFT_MASK | gtk.gdk.BUTTON1_MASK):
			step = minstep
		else:
			step = max(self.quantize_value(range/25.0), minstep)
		value = self.value
		if event.direction == gtk.gdk.SCROLL_UP:
			value += step
		elif event.direction == gtk.gdk.SCROLL_DOWN:
			value -= step
		self.set_value(value)
		self.show_tooltip()

	def draw_points(self, ctx, peaks):
		ctx.move_to(*peaks[0])
		for peak in peaks[1:]:
			ctx.line_to(*peak)
		
	def draw(self, ctx):
		if not self.knobshape:
			self.update_knobshape()
		startangle = math.pi*1.5 - self.angle*0.5
		angle = (self.value/(self.max_value - self.min_value))*self.angle + startangle
		rc = self.get_allocation()
		size = min(rc.width, rc.height)
		kh = self.get_border_width() # knob height
		ps = 1.0/size # pixel size
		ps2 = 1.0 / (size-(2*kh)-1) # pixel size inside knob
		ss = ps * kh # shadow size
		lsize = ps2 * self.lsize # legend line width
		# draw spherical
		ctx.translate(rc.x, rc.y)
		ctx.translate(0.5,0.5)
		ctx.translate(size*0.5, size*0.5)
		ctx.scale(size-(2*kh)-1, size-(2*kh)-1)
		if self.legend == LEGEND_DOTS:
			ctx.save()
			ctx.set_source_rgb(*hls_to_rgb(*self.legend_hls))
			dots = self.segments
			for i in xrange(dots):
				s = float(i)/(dots-1)
				a = startangle + self.angle*s
				ctx.save()
				ctx.rotate(a)
				r = lsize*0.5
				if self.lscale:
					r = max(r*s,ps2)
				ctx.arc(0.5+lsize, 0.0, r, 0.0, 2*math.pi)
				ctx.fill()
				ctx.restore()
			ctx.restore()
		elif self.legend in (LEGEND_LINES, LEGEND_RULER, LEGEND_RULER_INWARDS):
			ctx.save()
			ctx.set_source_rgb(*hls_to_rgb(*self.legend_hls))
			dots = self.segments
			n = ps2*(kh-1)
			for i in xrange(dots):
				s = float(i)/(dots-1)
				a = startangle + self.angle*s
				ctx.save()
				ctx.rotate(a)
				r = n*0.9
				if self.lscale:
					r = max(r*s,ps2)
				ctx.move_to(0.5+ps2+n*0.1, 0.0)
				ctx.line_to(0.5+ps2+n*0.1+r, 0.0)
				ctx.set_line_width(lsize)
				ctx.stroke()
				ctx.restore()
			ctx.restore()
			if self.legend == LEGEND_RULER:
				ctx.save()
				ctx.set_source_rgb(*hls_to_rgb(*self.legend_hls))
				ctx.set_line_width(lsize)
				ctx.arc(0.0, 0.0, 0.5+ps2+n*0.1, startangle, startangle+self.angle)
				ctx.stroke()
				ctx.restore()
			elif self.legend == LEGEND_RULER_INWARDS:
				ctx.save()
				ctx.set_source_rgb(*hls_to_rgb(*self.legend_hls))
				ctx.set_line_width(lsize)
				ctx.arc(0.0, 0.0, 0.5+ps2+n, startangle, startangle+self.angle)
				ctx.stroke()
		if kh:
			ctx.save()
			ctx.translate(ss, ss)
			ctx.rotate(angle)
			self.draw_points(ctx, self.knobshape)
			ctx.close_path()
			ctx.restore()
			ctx.set_source_rgba(0,0,0,0.3)
			ctx.fill()
		if self.legend in (LEGEND_LED_SCALE, LEGEND_LED_DOTS):
			ch,cl,cs = self.legend_hls
			n = ps2*(kh-1)
			ctx.save()
			ctx.set_line_cap(cairo.LINE_CAP_ROUND)
			ctx.set_source_rgb(*hls_to_rgb(ch,cl*0.2,cs))
			ctx.set_line_width(lsize)
			ctx.arc(0.0, 0.0, 0.5+ps2+n*0.5, startangle, startangle+self.angle)
			ctx.stroke()
			ctx.set_source_rgb(*hls_to_rgb(ch,cl,cs))
			if self.legend == LEGEND_LED_SCALE:
				ctx.set_line_width(lsize-ps2*2)
				ctx.arc(0.0, 0.0, 0.5+ps2+n*0.5, startangle, angle)
				ctx.stroke()
			elif self.legend == LEGEND_LED_DOTS:
				dots = self.segments
				dsize = lsize-ps2*2
				seg = self.angle/dots
				endangle = startangle + self.angle
				for i in xrange(dots):
					s = float(i)/(dots-1)
					a = startangle + self.angle*s
					if ((a-seg*0.5) > angle) or (angle == startangle):
						break
					ctx.save()
					ctx.rotate(a)
					r = dsize*0.5
					if self.lscale:
						r = max(r*s,ps2)
					ctx.arc(0.5+ps2+n*0.5, 0.0, r, 0.0, 2*math.pi)
					ctx.fill()
					ctx.restore()
			ctx.restore()
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
		
		ctx.arc(0.0, 0.0, 0.5-self.gd, 0.0, math.pi*2.0)
		ctx.set_source_rgb(*hls_to_rgb(self.fg_hls[0], max(self.fg_hls[1]*0.4,0.0), self.fg_hls[2]))
		ctx.fill()
		ctx.arc(0.0, 0.0, 0.5-self.gd-ps, 0.0, math.pi*2.0)
		ctx.set_source_rgb(*hls_to_rgb(self.fg_hls[0], min(self.fg_hls[1]*1.2,1.0), self.fg_hls[2]))
		ctx.fill()
		ctx.arc(0.0, 0.0, 0.5-self.gd-(2*ps), 0.0, math.pi*2.0)
		ctx.set_source_rgb(*hls_to_rgb(*self.fg_hls))
		ctx.fill()
		
		#~ ctx.set_line_cap(cairo.LINE_CAP_ROUND)
		#~ ctx.move_to(0.5-0.3-self.gd-ps, 0.0)
		#~ ctx.line_to(0.5-self.gd-ps*5, 0.0)
		
		if self.marker == MARKER_LINE:
			ctx.set_line_cap(cairo.LINE_CAP_BUTT)
			ctx.move_to(0.5-0.3-self.gd-ps, 0.0)
			ctx.line_to(0.5-self.gd-ps, 0.0)
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
		elif self.marker == MARKER_DOT:
			ctx.arc(0.5-0.05-self.gd-ps*5, 0.0, 0.05, 0.0, 2*math.pi)
			ctx.save()
			ctx.identity_matrix()
			ctx.set_source_rgb(*hls_to_rgb(self.fg_hls[0], min(self.fg_hls[1]*1.2,1.0), self.fg_hls[2]))
			ctx.stroke_preserve()
			ctx.set_line_width(1)
			ctx.set_source_rgb(*hls_to_rgb(self.fg_hls[0], max(self.fg_hls[1]*0.4,0.0), self.fg_hls[2]))
			ctx.fill()
			ctx.restore()
		elif self.marker == MARKER_ARROW:
			ctx.set_line_cap(cairo.LINE_CAP_BUTT)
			ctx.move_to(0.5-0.3-self.gd-ps, 0.1)
			ctx.line_to(0.5-0.1-self.gd-ps, 0.0)
			ctx.line_to(0.5-0.3-self.gd-ps, -0.1)
			ctx.close_path()
			ctx.save()
			ctx.identity_matrix()
			#~ ctx.set_source_rgb(*hls_to_rgb(self.fg_hls[0], min(self.fg_hls[1]*1.2,1.0), self.fg_hls[2]))
			#~ ctx.stroke_preserve()
			ctx.set_line_width(1)
			ctx.set_source_rgb(*hls_to_rgb(self.fg_hls[0], max(self.fg_hls[1]*0.4,0.0), self.fg_hls[2]))
			ctx.fill()
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
		self.arc1 = 0.0
		self.arc2 = None
		self.arc3 = None
		self.arc4 = None
		self.fg_hls = 0.0, 1.0, 0.0
		self.bg_hls = 0.0, 0.3, 0.618
		self.text_hls = 0.0, 1.0, 1.0
		self.filled = False
		self.tabwidth = 0
		self.tabheight = 0
		self.thickness = 0.0
		self.ratio = 0.382
		self.alpha = 1.0
		self.set_app_paintable(True)
		self.connect('expose-event', self.on_expose)
		self.vbox = gtk.VBox()
		hbox = gtk.HBox()
		self.pack_start(hbox, expand=False)
		self.pack_start(self.vbox)
		self.tabbox = hbox
		
	def update_tabbox_size(self):
		self.tabbox.set_size_request(-1, self.tabheight+10)

	def set_label(self, text):
		self.label = text
		self.update_tabbox_size()
		self.refresh()

	def set_thickness(self, thickness):
		self.thickness = thickness
		self.refresh()
		
	def set_tab_size(self, width, height):
		self.tabwidth = width
		self.tabheight = height
		self.update_tabbox_size()
		self.refresh()
	
	def get_thickness(self):
		return self.thickness
		
	def set_roundness_ratio(self, ratio):
		self.ratio = ratio
		self.refresh()
		
	def get_roundness_ratio(self):
		return self.ratio
		
	def set_roundness(self, topleft, topright=None, bottomleft=None, bottomright=None):
		self.arc1 = topleft
		self.arc2 = topright
		self.arc3 = bottomleft
		self.arc4 = bottomright
		self.refresh()

	def set_alpha(self, alpha):
		self.alpha = alpha
		self.refresh()
		
	def get_alpha(self):
		return self.alpha

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

	def set_label_color(self, h, l, s):
		self.text_hls = h,l,s
		self.refresh()

	def get_label_color():
		return self.text_hsl
		
	def refresh(self):
		rc = self.get_allocation()
		if self.window:
			self.window.invalidate_rect(rc, False)
		return True
		
	def configure_font(self, ctx):
		ctx.select_font_face(
			"Bitstream Sans Vera", 
			cairo.FONT_SLANT_NORMAL,
			cairo.FONT_WEIGHT_BOLD)
		ctx.set_font_size(9)
		fo = cairo.FontOptions()
		fo.set_antialias(cairo.ANTIALIAS_GRAY)
		fo.set_hint_style(cairo.HINT_STYLE_NONE)
		fo.set_hint_metrics(cairo.HINT_METRICS_DEFAULT)
		ctx.set_font_options(fo)

	def draw(self, ctx):
		rc = self.get_allocation()
		x,y,w,h = rc.x, rc.y, rc.width, rc.height
		bw = self.get_border_width()
		x += bw
		y += bw
		w -= bw*2
		h -= bw*2
		
		label = self.label.upper()
		ctx.push_group()
		self.configure_font(ctx)
		xb, yb, fw, fh, xa, ya = ctx.text_extents(label)
		
		tw = max(self.tabwidth,xa+self.arc1+self.tabheight)

		ctx.set_source_rgb(*hls_to_rgb(*self.bg_hls))
		draw_round_rectangle(ctx, x, y, w, h, 
			arc1=self.arc1, arc2=self.arc2, arc3=self.arc3, arc4=self.arc4, 
			ratio=self.ratio, tabwidth=tw, tabheight=self.tabheight)

		ctx.fill()
		if self.thickness:
			ctx.set_source_rgb(*hls_to_rgb(*self.fg_hls))
			th = self.thickness*0.5
			draw_round_rectangle(ctx, x+th, y+th, w-self.thickness, h-self.thickness, 
				arc1=self.arc1, arc2=self.arc2, arc3=self.arc3, arc4=self.arc4, 
				ratio=self.ratio, tabwidth=tw, tabheight=self.tabheight)
			ctx.set_line_width(self.thickness)
			ctx.stroke()
			
		if label:
			ctx.set_source_rgb(*hls_to_rgb(*self.text_hls))
			ctx.translate(x+self.arc1*0.5,y+self.tabheight*0.5-yb)
			if self.thickness:
				ctx.translate(self.thickness+1, self.thickness+1)
			ctx.show_text(label)
			ctx.fill()
		ctx.pop_group_to_source()
		ctx.paint_with_alpha(self.alpha)

	def on_expose(self, widget, event):
		self.context = widget.window.cairo_create()
		self.draw(self.context)
		return False

import lcdfont

LCD_CHARWIDTH = 5 # lcd character width in tiles
LCD_CHARHEIGHT = 7 # lcd character height in tiles

class LCD(gtk.DrawingArea):
	def __init__(self):
		gtk.DrawingArea.__init__(self)
		self.font = lcdfont.charset_5x7
		self.fg_hls = 0.2, 0.7, 1.0
		self.bg_hls = 0.6, 0.2, 1.0
		self.contrast = 0.2
		self.brightness = 0.8
		self.rows = 2
		self.columns = 16
		self.border = 6
		self.calc_size()
		self.connect('expose-event', self.on_expose)
		self.connect('realize', self.on_realize)
		
	def calc_size(self):
		self.clear_text()
		self.charwidth = LCD_CHARWIDTH
		self.charheight = LCD_CHARHEIGHT
		self.panelwidth = self.columns*(self.charwidth+1)-1
		self.panelheight = self.rows*(self.charheight+1)-1
		self.set_size_request(
			int(self.border*2+self.panelwidth+0.5), 
			int(self.border*2+self.panelheight+0.5))
		self.chars = None
		
	def on_realize(self, widget):
		self.get_characters() # to initialize
			
	def get_characters(self):
		if self.chars:
			return self.chars
		# make tiles
		self.chars = []
		BITMASK = lcdfont.BITMASK
		for i in xrange(256):
			pm = gtk.gdk.Pixmap(self.window, self.charwidth, self.charheight, -1)
			self.chars.append(pm)
			ctx = pm.cairo_create()
			x,y,w,h = 0, 0, self.charwidth, self.charheight
			ctx.set_source_rgb(*hls_to_rgb(*self.bg_hls))
			ctx.paint()
			tbgcolor = hls_to_rgb(*self.fg_hls) + (self.contrast,)
			tcolor = hls_to_rgb(*self.fg_hls) + (1.0 -self.contrast,)
			ctx.push_group()
			ctx.save()
			for cy in xrange(LCD_CHARHEIGHT):
				ctx.save()
				bm = self.font[i]
				for cx in xrange(LCD_CHARWIDTH):
					if bm & (BITMASK>>(cy+(8*cx))):
						ctx.set_source_rgba(*tcolor)
					else:
						ctx.set_source_rgba(*tbgcolor)
					ctx.rectangle(0,0,1,1)
					ctx.fill()
					ctx.translate(1,0)
				ctx.restore()
				ctx.translate(0,1)
			ctx.restore()
			ctx.pop_group_to_source()
			ctx.paint_with_alpha(self.brightness)
		return self.chars
			
	def get_dimensions(self):
		return self.columns, self.rows
			
	def set_dimensions(self, columns, rows=1):
		self.columns = columns
		self.rows = rows
		self.calc_size()
		self.refresh()
			
	def set_brightness(self, brightness):
		self.brightness = brightness
		self.refresh()
		
	def set_border(self, border):
		self.border = border
		self.calc_size()
		self.refresh()
			
	def set_contrast(self, contrast):
		self.contrast = contrast
		self.refresh()
		
	def on_expose(self, widget, event):
		self.context = widget.window.cairo_create()
		self.draw(self.context)
		return False
		
	def set_fg_color(self, h,l,s):
		self.fg_hls = h,l,s
		self.refresh()

	def set_bg_color(self, h,l,s):
		self.bg_hls = h,l,s
		self.refresh()
		
	def clear_text(self):
		self.buffer = [[' ' for x in xrange(self.columns)] for y in xrange(self.rows)]

	def set_text(self, text, x=0, y=0):
		for c in text:
			if c == '\n':
				x = 0
				y += 1
			else:
				if (x >= 0) and (x < self.columns) and (y >= 0) and (y < self.rows):
					self.buffer[y][x] = c
				x += 1
		self.refresh()

	def draw(self, ctx):
		gc = self.window.new_gc()
		chars = self.get_characters()
		rc = self.get_allocation()
		x,y,w,h = 0, 0, rc.width, rc.height
		ctx.set_source_rgb(*hls_to_rgb(*self.bg_hls))
		ctx.paint()
		x = int(w/2 + 0.5) - int(self.panelwidth/2 + 0.5)
		y = int(h/2 + 0.5) - int(self.panelheight/2 + 0.5)
		rx,ry = x,y
		for row in self.buffer:
			rx = x
			for c in row:
				self.window.draw_drawable(gc, chars[ord(c)], 0, 0, int(rx), int(ry), -1, -1)
				rx += self.charwidth+1
			ry += self.charheight+1

	def refresh(self):
		rc = self.get_allocation()
		if self.window:
			self.window.invalidate_rect((0,0,rc.width,rc.height), False)
		return True

if __name__ == '__main__':
	window = gtk.Window()
	window.connect('destroy', lambda widget: gtk.main_quit())
	s = 0.9
	
	
	class scrollinfo1:
		text = "This LCD screen works pretty much like a real one, " \
		"except that it's made out pixmaps. It also doesn't display " \
		"those little lines between the tiles, because it is " \
		"so tiny. You can adjust contrast and brightness and " \
		"place text anywhere on the screen."
		x = 0
		y = 0

	class scrollinfo2:
		text = "Dies ist ein deutscher Text, der präsentieren soll, " \
		"dass dank dem ASCII-Zeichensatz auch Umlaute unterstützt werden. " \
		"Ausserdem scrollt er ein wenig schneller, um zu demonstrieren, " \
		"dass Zeilen auch unabhängig voneinander aktualisiert werden können."
		x = 0
		y = 1
		
	class scrollinfo3:
		text = "This line serves no particular purpose."
		x = 0
		y = 2

	def scroller(lcd, si):
		lcd.set_text(' '*19, 0, si.y)
		lcd.set_text(si.text,19 - si.x, si.y)
		si.x = (si.x+1) % (len(si.text)+19)
		return True
	
	def new_vbox(text):
		vbox = DecoBox()
		vbox.set_label(text)
		hbox = gtk.HBox()
		vbox.vbox.pack_start(hbox, expand=False)
		return vbox, hbox
	def new_knob(size, value, hue, sat, gaps):
		knob = Knob()
		knob.set_gaps(gaps)
		knob.set_border_width(6)
		knob.set_value(value)
		knob.set_top_color(hue, 0.8, sat)
		knob.set_size_request(size,size)
		if size == 32:
			knob.set_legend(LEGEND_DOTS)
		if size == 64:
			knob.set_legend_scale(True)
			knob.set_legend(LEGEND_LINES)
			knob.set_legend_color(0.0, 0.0, 0.0)
			knob.set_gap_depth(0)
			knob.set_segments(36)
		return knob
	window.modify_bg(gtk.STATE_NORMAL, hls_to_color(0.0, 0.4, s))
	hbox = gtk.HBox(False, 6)
	hbox.set_border_width(6)
	vb, hb = new_vbox("LMAO")
	vb.set_roundness(6, 6, 6, 6)
	vb.set_tab_size(16,6)
	vb.set_label_color(0.0, 0.0, 0.0)
	vb.set_thickness(1)
	vb.set_bg_color(0.0, 0.9, 0.0)
	vb.set_fg_color(0.0, 1.0, 0.0)
	vb.set_alpha(0.8)
	def wrap_vb(vb):
		vbox = gtk.VBox()
		vbox.pack_start(vb, expand=False)
		return vbox
	hbox.pack_start(wrap_vb(vb), expand=False)
	def wrap_border(knob):
		hbox = gtk.HBox()
		hbox.pack_start(knob, expand=False)
		hbox.set_border_width(6)
		return hbox
	knob = new_knob(64, 0.0, 0.0, 0.1, 9)
	knob.set_legend_scale(False)
	knob.set_marker(MARKER_DOT)
	hb.pack_start(wrap_border(knob), expand=False)
	hb.pack_start(wrap_border(new_knob(64, 0.0, 0.0, 0.1, 9)), expand=False)
	titles = ["LOL", "ROFL", "WTF", "YBG"]
	for x,i in enumerate((0.1, 0.2, 0.4, 0.6)):
		title = titles.pop()
		vb, hb = new_vbox(title)
		vb.set_thickness(3)
		r = 12
		if x != 3:
			vb.set_roundness(0, r, 0, r)
		vb.set_bg_color(0.0, 0.4, s)
		vb.set_fg_color(0.0, 1.0, 1.0)
		vb.set_alpha(0.9)
		hbox.pack_start(wrap_vb(vb), expand=False)
		knob = new_knob(48, 0.1, i, 1.0, 6)
		knob.set_border_width(9)
		knob.set_angle(math.pi)
		if x == 0:
			knob.set_angle(1.5*math.pi)
			knob.set_legend(LEGEND_RULER_INWARDS)
			knob.set_legend_line_width(2)
			knob.set_segments(7)
		elif x == 1:
			vb.set_alpha(1.0)
			vb.set_fg_color(0.0, 0.4, 0.0)
			vb.set_bg_color(0.0, 0.2, 0.0)
			knob.set_angle(1.5*math.pi)
			knob.set_legend(LEGEND_LED_SCALE)
			knob.set_legend_color(0.6, 0.5, 1.0)
			knob.set_legend_line_width(6)
			knob.set_top_color(0.0, 0.8, 0.0)
			knob.set_gap_depth(1)
			knob.set_marker(MARKER_NONE)
		elif x == 2:
			vb.set_alpha(1.0)
			vb.set_fg_color(0.0, 0.4, 0.0)
			vb.set_bg_color(0.0, 0.2, 0.0)
			knob.set_angle(1.5*math.pi)
			knob.set_legend(LEGEND_LED_DOTS)
			knob.set_legend_color(0.01, 0.5, 1.0)
			knob.set_legend_line_width(6)
			knob.set_top_color(0.0, 0.8, 0.0)
			knob.set_gap_depth(1)
			knob.set_segments(13)
			knob.set_marker(MARKER_ARROW)
		else:
			knob.set_angle(math.pi)
			knob.set_legend(LEGEND_RULER)
			knob.set_legend_line_width(2)
			knob.set_segments(7)
		hb.pack_start(wrap_border(knob), expand=False)
	vb, hb = new_vbox("BRB")
	vb.set_roundness(6, 6, 6, 6)
	vb.set_tab_size(16,6)
	vb.set_thickness(1)
	vb.set_bg_color(0.0, 0.3, s)
	vb.set_fg_color(0.0, 0.28, s)
	vbox = gtk.VBox(False, 6)
	vbox.pack_start(vb, expand=False)
	lcd = LCD()
	lcd.set_contrast(0.1)
	lcd.set_dimensions(19,3)
	vbox.pack_start(lcd, expand=False)
	lcd1 = lcd
	lcd = LCD()
	lcd.set_fg_color(0.4, 0.5, 1.0)
	lcd.set_bg_color(0.4, 0.05, 1.0)
	lcd.set_contrast(0.05)
	lcd.set_brightness(0.8)
	lcd.set_text("Slider 2\n 41 CC12  1  U12")
	vbox.pack_start(lcd, expand=False)
	hbox.pack_start(vbox, expand=False)
	hb.pack_start(new_knob(32, 0.0, 0.6, 0.1, 3), expand=False)
	hb.pack_start(new_knob(32, 0.0, 0.6, 0.1, 3), expand=False)
	hb.pack_start(new_knob(32, 0.0, 0.6, 0.1, 3), expand=False)
	hb.pack_start(new_knob(32, 1.0, 0.6, 0.1, 3), expand=False)
	window.add(hbox)
	window.show_all()
	gobject.timeout_add(150, scroller, lcd1, scrollinfo1())
	gobject.timeout_add(100, scroller, lcd1, scrollinfo2())
	gobject.timeout_add(40, scroller, lcd1, scrollinfo3())
	gtk.main()
