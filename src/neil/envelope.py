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
Provides dialogs, classes and controls to display/load/save envelopes
"""

import gtk
import os, sys
from utils import prepstr, db2linear, linear2db, note2str
from utils import read_int, write_int
import zzub
import config
import common
from common import MARGIN, MARGIN2, MARGIN3

import neil.com as com

# size of border
BORDER = 5
# size of the envelope dots
DOTSIZE = 8
# matches existing points exactly or approximately
EXACT = 0
NEXT = 1

class BasicEnvelope(gtk.DrawingArea):
    def __init__(self):
        self.envelope = None
	self.currentpoint = None
	self.dragging = False
	self.showpoints = False
	gtk.DrawingArea.__init__(self)
	self.add_events(gtk.gdk.ALL_EVENTS_MASK)
	self.connect('button-press-event', self.on_button_down)
	self.connect('button-release-event', self.on_button_up)
	self.connect('motion-notify-event', self.on_motion)
	self.connect('enter-notify-event', self.on_enter)
	self.connect('leave-notify-event', self.on_leave)
	self.connect('expose_event', self.expose)

    def expose(self, widget, event):
	self.context = widget.window.cairo_create()
	self.draw(self.context)
	return False

    def get_client_size(self):
	rect = self.get_allocation()
	return rect.width, rect.height

    def redraw(self):
	if self.window:
	    w, h = self.get_client_size()
	    self.window.invalidate_rect((0, 0, w, h), False)

    def on_enter(self, widget, event):
	"""
	Called when the mouse enters the envelope editor.
	"""
	self.showpoints = True
	self.redraw()

    def on_leave(self, widget, event):
	"""
	Called when the mouse leaves the envelope editor.
	"""
	self.showpoints = False
	self.redraw()

    def get_point_at(self, position):
	"""
	Finds an existing envelope point that matches the given position the best.

	@param position: Pixel coordinate.
	@type position: (int, int)
	@return: (index, match) that represents the index of a point
	and whether it matches exactly or not (with EXACT and NEXT).
	@rtype: (int, int)
	"""
	x, y = position
	points = self.get_translated_points()
	ds = int((DOTSIZE / 2.0) + 0.5)
	bestindex = None
	for i, point in enumerate(points):
	    px, py, f = point
	    if (bestindex == None) and (px > x):
		bestindex = i
	    rc = gtk.gdk.Rectangle(px - ds, py - ds, DOTSIZE, DOTSIZE)
	    if sum(rc.intersect((x, y, 1, 1))):
		return i, EXACT
	return bestindex, NEXT

    def on_button_down(self, widget, event):
	"""
	Callback that responds to mouse down over the envelope view.
	"""
	if event.button == 1:
	    mx, my = int(event.x), int(event.y)
	    i, location = self.get_point_at((mx, my))
	    if i == None:
		return
	    self.grab_add()
	    self.currentpoint = i
	    self.dragging = True
	    if location == NEXT:
		# No direct hit, create a new one
		self.envelope.insert_point(self.currentpoint)
		x, y, f = self.envelope.get_point(self.currentpoint)
		x, y = self.pixel_to_env((mx, my))
		self.envelope.set_point(self.currentpoint, x, y, f)
	    self.redraw()
	elif event.button == 3:
	    mx, my = int(event.x), int(event.y)
	    i, location = self.get_point_at((mx, my))
	    if i == None:
		return
	    if location == NEXT:
                self.sustain.hide()
                self.delete.hide()
		self.context_menu.popup(None, None, None, event.button, event.time)
	    else:
                self.sustain.show()
                self.delete.show()
		self.context_menu.popup(None, None, None, event.button, event.time)

    def on_button_up(self, widget, event):
	"""
	Callback that responds to mouse up over the envelope view.
	"""
	if self.dragging:
	    self.grab_remove()
	    self.dragging = False
	    self.currentpoint = None
	    self.redraw()

    def on_motion(self, widget, event):		
	"""
	Callback that responds to mouse motion over the envelope view.
	"""
	mx, my, state = self.window.get_pointer()
	mx, my = int(mx), int(my)
	if self.dragging:
	    x, y, f = self.envelope.get_point(self.currentpoint)
	    x, y = self.pixel_to_env((mx, my))
	    self.envelope.set_point(self.currentpoint, x, y, f)
	    self.redraw()
	else:
	    i, location = self.get_point_at((mx, my))
	    if location != EXACT:
		i = None
	    if (self.currentpoint != i):
		self.currentpoint = i
		self.redraw()

class EnvelopeView(BasicEnvelope):
    """
    Envelope viewer.

    A drawing surface where you can specify how the volume of a sample changes over time.
    """
    def __init__(self, wavetable):
	"""
	Initialization.
	"""
	self.wavetable = wavetable
        BasicEnvelope.__init__(self)
	# Menu that get's activated when you click right mouse button.
	self.context_menu = gtk.Menu()
	
	self.sustain = gtk.MenuItem("Sustain")
	self.sustain.show()
	self.sustain.connect('button-press-event', self.on_set_sustain)
	self.context_menu.append(self.sustain)
	
	self.delete = gtk.MenuItem("Delete")
	self.delete.show()
	self.delete.connect('button-press-event', self.on_delete_point)
	self.context_menu.append(self.delete)

	self.reset = gtk.MenuItem("Reset")
	self.reset.show()
	self.reset.connect('button-press-event', self.on_reset)
	self.context_menu.append(self.reset)
	
	separator = gtk.SeparatorMenuItem()
	separator.show()
	self.context_menu.append(separator)

	self.load = gtk.MenuItem("Load")
	self.load.show()
	self.load.connect('button-press-event', self.on_load)
	self.context_menu.append(self.load)

	self.save = gtk.MenuItem("Save")
	self.save.show()
	self.save.connect('button-press-event', self.on_save)
	self.context_menu.append(self.save)

	self.open_dialog =\
	    gtk.FileChooserDialog(title="Open Envelope File",
				  action=gtk.FILE_CHOOSER_ACTION_OPEN,
				  buttons=(gtk.STOCK_CANCEL,
					   gtk.RESPONSE_CANCEL,
					   gtk.STOCK_OPEN,
					   gtk.RESPONSE_OK))
	self.save_dialog =\
	    gtk.FileChooserDialog(title="Save Envelope File",
				  action=gtk.FILE_CHOOSER_ACTION_SAVE,
				  buttons=(gtk.STOCK_CANCEL,
					   gtk.RESPONSE_CANCEL,
					   gtk.STOCK_OPEN,
					   gtk.RESPONSE_OK))

	self.filter_ = gtk.FileFilter()
	self.filter_.set_name("Buzz Envelope File (*.bef)")
	self.filter_.add_pattern("*.bef")
	self.open_dialog.add_filter(self.filter_)
	self.save_dialog.add_filter(self.filter_)
	
    def on_set_sustain(self, widget, event):
	"""
	Callback responding to the context menu item that sets the current
	point to sustain mode.
	"""
	# remove old sustain point
	for i in range(self.envelope.get_point_count()):
	    x, y, f = self.envelope.get_point(i)
	    if f & zzub.zzub_envelope_flag_sustain:
		f = f ^ (f & zzub.zzub_envelope_flag_sustain)
		self.envelope.set_point(i, x, y, f)
        if self.currentpoint != None:
            x, y, f = self.envelope.get_point(self.currentpoint)
            self.envelope.set_point(self.currentpoint, x, y, f |
                                    zzub.zzub_envelope_flag_sustain)
            self.currentpoint = None
            self.redraw()

    def on_reset(self, widget, event):
	"""
	Callback responding to the "reset" context menu item.
	"""
	while self.envelope.get_point_count() > 2:
	    self.envelope.delete_point(1)
	self.redraw()

    def on_load(self, widget, event):
	"""
	Callback responding to the 'load' context menu item.
	"""
	response = self.open_dialog.run()
	if response == gtk.RESPONSE_OK:
	    while self.envelope.get_point_count() > 2:
		self.envelope.delete_point(1)
	    filename = self.open_dialog.get_filename()
	    points, sustainindex = load_envelope(filename)
	    spoint, epoint = points[0], points[-1]
	    self.envelope.set_point(0, spoint[0], spoint[1], 0)
	    self.envelope.set_point(1, epoint[0], epoint[1], 0)
	    def add_point(x, y, f=0):
		i = self.envelope.get_point_count() - 1
		self.envelope.insert_point(i)
		self.envelope.set_point(i, x, y, f)
		return i
	    for x, y in points[1:-1]:
		add_point(x, y, 0)
	    if sustainindex != -1:
		x, y, f = self.envelope.get_point(sustainindex)
		self.envelope.set_point(sustainindex, x, y, f |\
					zzub.zzub_envelope_flag_sustain)
	self.open_dialog.hide()
	self.redraw()

    def on_save(self, widget, event):
	"""
	Callback responding to the 'save' context menu item.
	"""
	response = self.save_dialog.run()
	if response == gtk.RESPONSE_OK:
	    points = []
	    sustainindex = -1
	    for i, (x, y, f) in enumerate(self.envelope.get_point_list()):
		if f & zzub.zzub_envelope_flag_sustain:
		    sustainindex = i
		points.append((x, y))
	    filename = self.save_dialog.get_filename()
	    save_envelope(filename, points, sustainindex)
	self.save_dialog.hide()
	self.redraw()

    def on_delete_point(self, widget, event):
	"""
	Callback responding to the context menu item that deletes the current
	point of the envelope.
	"""
        if self.currentpoint != None:
            self.envelope.delete_point(self.currentpoint)
            self.currentpoint = None
            self.redraw()

    def update(self, *args):
	"""
	Updates the envelope view based on the sample selected in the sample list.		
	"""
	self.currentpoint = None
	self.dragging = False
	self.envelope = None
	sel = self.wavetable.get_sample_selection()
	if sel:
	    player = com.get('neil.core.player')
	    w = player.get_wave(sel[0])
	    if w.get_envelope_count():
		self.envelope = w.get_envelope(0)
	self.redraw()

    def env_to_pixel(self, x, y):
	w, h = self.get_client_size()
	xf = (w - (2 * BORDER) - 1) / 65535.0
	yf = (h - (2 * BORDER) - 1) / 65535.0
	return int(x * xf) + BORDER, int((65535 - y) * yf) + BORDER

    def pixel_to_env(self, position):
	"""
	Converts a (x,y) pixel coordinate into an envelope point value.

	@param position: Pixel coordinate.
	@type position: (int, int)
	@return: (time, amplitude) point on the envelope.
	@rtype: (int, int)
	"""
	x, y = position
	w, h = self.get_client_size()
	xf = 65535.0 / (w - (2 * BORDER) - 1)
	yf = 65535.0 / (h - (2 * BORDER) - 1)
	return max(min(int((x - BORDER) * xf), 65535), 0),\
	       65535 - max(min(int((y - BORDER) * yf), 65535), 0)

    def get_translated_points(self):
	"""
	Converts the envelope values to a list of pixel values.

	@return: Pixel values.
	@rtype: list		
	"""
	return [self.env_to_pixel(x, y) + (f,) for x, y, f in \
		self.envelope.get_point_list()]

    def set_sensitive(self, enable):
	gtk.DrawingArea.set_sensitive(self, enable)
	self.redraw()

    def draw(self, ctx):
	"""
	Overriding a L{Canvas} method that paints onto an offscreen buffer.
	Draws the envelope view graphics.
	"""	
	w, h = self.get_client_size()
	cfg = config.get_config()

	bgbrush = cfg.get_float_color('EE BG')
	dotbrush = cfg.get_float_color('EE Dot')
	selectbrush = cfg.get_float_color('EE Dot Selected')
	pen = cfg.get_float_color('EE Line')
	brush = cfg.get_float_color('EE Fill')
	sustainpen = cfg.get_float_color('EE Sustain')
	gridpen = cfg.get_float_color('EE Grid')

	ctx.translate(0.5, 0.5)
	ctx.set_source_rgb(*bgbrush)
	ctx.rectangle(0, 0, w, h)
	ctx.fill()
	ctx.set_line_width(1)

	if not self.envelope:
	    return
	# 4096 (16)
	# 8192 (8)
	xlines = 16
	ylines = 8
	xf = 65535.0 / float(xlines)
	yf = 65535.0 / float(ylines)
	ctx.set_source_rgb(*gridpen)
	for xg in range(xlines + 1):
	    pt1 = self.env_to_pixel(xg * xf, 0)
	    ctx.move_to(pt1[0], 0)
	    ctx.line_to(pt1[0], h)
	    ctx.stroke()
	for yg in range(ylines + 1):
	    pt1 = self.env_to_pixel(0, yg * yf)
	    ctx.move_to(0, pt1[1])
	    ctx.line_to(w, pt1[1])
	    ctx.stroke()
	if not self.get_property('sensitive'):
	    return
	points = self.get_translated_points()
	envp = None
	ctx.move_to(*self.env_to_pixel(0, 0))
	for i in xrange(len(points)):
	    pt1 = points[max(i, 0)]
	    ctx.line_to(pt1[0], pt1[1])
	    if pt1[2] & zzub.zzub_envelope_flag_sustain:
		envp = pt1
	ctx.line_to(*self.env_to_pixel(65535, 0))
	ctx.set_source_rgba(*(brush + (0.6,)))
	ctx.fill_preserve()
	ctx.set_source_rgb(*pen)
	ctx.stroke()
	if envp:
	    ctx.set_source_rgb(*sustainpen)
	    ctx.move_to(envp[0], 0)
	    ctx.line_to(envp[0], h)
	    ctx.set_dash([4.0, 2.0], 0.5)
	    ctx.stroke()
	    ctx.set_dash([], 0.0)
	if self.showpoints:
	    for i in reversed(range(len(points))):
		pt1 = points[max(i, 0)]
		pt2 = points[min(i + 1, len(points) - 1)]
		if i == self.currentpoint:
		    ctx.set_source_rgb(*selectbrush)
		else:
		    ctx.set_source_rgb(*dotbrush)
		import math
		ctx.arc(pt1[0], pt1[1],
			int((DOTSIZE / 2.0) + 0.5), 0.0, math.pi * 2)
		ctx.fill()

class ADSRPanel(gtk.VBox):
    """
    ADSR Panel.

    Allows to sculpt Attack/Decay/Sustain/Release based envelopes.
    """

    ATTACK_MODES = ["Linear", "48dB Logarithmic", "64dB Logarithmic", "96dB Logarithmic"]
    DECAY_MODES = ["Linear", "Logarithmic"]
    RELEASE_MODES = ["Linear", "-48dB Logarithmic", "-64dB Logarithmic", "-96dB Logarithmic"]

    def __init__(self, wavetable):
	"""
	Initialization.
	"""
	self.wavetable = wavetable
	gtk.VBox.__init__(self, False, MARGIN)
	#self.set_title("ADSR Envelope Editor")
	def new_scale():
	    scale = gtk.HScale()
	    scale.set_draw_value(False)
	    return scale
	self.attack = new_scale()
	self.cbattacktype = gtk.combo_box_new_text()
	self.decay = new_scale()
	self.cbdecaytype = gtk.combo_box_new_text()
	self.sustain = new_scale()
	self.chksustain = gtk.CheckButton("Sustain")
	self.release = new_scale()
	self.cbreleasetype = gtk.combo_box_new_text()
	self.attack.set_range(0, 16384)
	self.decay.set_range(0, 16384)
	self.sustain.set_range(0, 16384)
	self.release.set_range(0, 16384)
	for mode in self.ATTACK_MODES:
	    self.cbattacktype.append_text(mode)
	for mode in self.DECAY_MODES:
	    self.cbdecaytype.append_text(mode)
	for mode in self.RELEASE_MODES:
	    self.cbreleasetype.append_text(mode)
	self.attack.set_value(0)
	self.decay.set_value(0)
	self.sustain.set_value(16384)
	self.release.set_value(16383)
	self.cbattacktype.set_active(0)
	self.cbdecaytype.set_active(0)
	self.cbreleasetype.set_active(0)
	sg1 = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
	sg2 = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
	def add_row(c1, c2, c3):
	    row = gtk.HBox(False, MARGIN)
	    sg1.add_widget(c1)
	    sg2.add_widget(c3)
	    row.pack_start(c1, expand=False)
	    row.add(c2)
	    row.pack_end(c3, expand=False)
	    self.pack_start(row, expand=False)
	def new_label(name):
	    label = gtk.Label()
	    label.set_alignment(1, 0.5)
	    label.set_markup('<b>%s</b>' % name)
	    return label
	add_row(new_label("Attack Time"), self.attack, self.cbattacktype)
	add_row(new_label("Decay Time"), self.decay, self.cbdecaytype)
	add_row(new_label("Sustain Level"), self.sustain, self.chksustain)
	add_row(new_label("Release Time"), self.release, self.cbreleasetype)

	self.cbattacktype.connect('changed', self.update_envelope)
	self.cbdecaytype.connect('changed', self.update_envelope)
	self.cbreleasetype.connect('changed', self.update_envelope)
	self.chksustain.connect('clicked', self.update_envelope)
	self.attack.connect('change-value', self.update_envelope)
	self.decay.connect('change-value', self.update_envelope)
	self.sustain.connect('change-value', self.update_envelope)
	self.release.connect('change-value', self.update_envelope)

    def update(self):
	"""
	Update slider values from presets.
	"""
	iswave = False
	player = com.get('neil.core.player')
	sel = self.wavetable.get_sample_selection()
	if sel:
	    w = player.get_wave(sel[0])
	    iswave = w.get_level_count() >= 1
	    if w.get_envelope_count():
		env = w.get_envelope(0)
		iswave = env.is_enabled()
		self.attack.set_value(env.get_attack())
		self.decay.set_value(env.get_decay())
		self.sustain.set_value(env.get_sustain())
		self.release.set_value(env.get_release())
	    else:
		iswave = False
	self.attack.set_sensitive(iswave)
	self.decay.set_sensitive(iswave)
	self.sustain.set_sensitive(iswave)
	self.release.set_sensitive(iswave)
	self.chksustain.set_sensitive(iswave)
	self.cbattacktype.set_sensitive(iswave)
	self.cbdecaytype.set_sensitive(iswave)
	self.cbreleasetype.set_sensitive(iswave)

    def update_envelope(self, *args):
	"""
	Updates the envelope from current slider values.
	"""
	sel = self.wavetable.get_sample_selection()
	player = com.get('neil.core.player')
	if sel:
	    w = player.get_wave(sel[0])
	    if w.get_envelope_count():
		env = w.get_envelope(0)
		env.set_attack(int(self.attack.get_value()))
		env.set_decay(int(self.decay.get_value()))
		env.set_sustain(int(self.sustain.get_value()))
		env.set_release(int(self.release.get_value()))
		a = int(self.attack.get_value() * 65535 / 16384)
		d = int(max(self.decay.get_value() * 65535 / 16384, 1))
		s = int(self.sustain.get_value() * 65535 / 16384)
		r = int(max(self.release.get_value() * 65535 / 16384, 1))
		p = 9 #self.resolution.GetValue()+1
		atype = self.cbattacktype.get_active()
		rtype = self.cbreleasetype.get_active()
		while env.get_point_count() > 2:
		    env.delete_point(1)
		def add_point(x, y, f=0):
		    i = env.get_point_count() - 1
		    env.insert_point(i)
		    env.set_point(i, x, y, f)
		    return i
		x = 0
		f = 0
		i = 0
		if self.chksustain.get_active():
		    f = f | zzub.zzub_envelope_flag_sustain
		if a > 0:
		    env.set_point(0, 0, 0, 0)
		    if atype == 0:
			x += a
			i = add_point(x, 65535)
		    elif atype >= 1:
			stp = max(a / p, 1)
			for idx in range(p):
			    x += stp
			    y = float(idx + 1) / p
			    limit = [-48, -64, -96][atype - 1]
			    y = db2linear((1 - y) * limit, limit)
			    i = add_point(x, int(y * 65535))
		else:
		    env.set_point(0, 0, 65535, 0)
		x += d
		i = add_point(x, s, f)
		if rtype == 0:
		    x += r
		    add_point(x, 0)
		elif rtype >= 1:
		    stp = max(r / p, 1)
		    limit = [-48, -64, -96][rtype - 1]
		    fac = linear2db(s / 65535.0, limit)
		    for idx in range(p):
			x += stp
			y = float(idx + 1) / p
			y = db2linear((y * limit) + fac, limit)
			i = add_point(x, int(y * 65535))
		#self.envelope.set_point(self.currentpoint,x,y,f | zzub.zzub_envelope_flag_sustain)
	    self.wavetable.envelope.update()

def load_envelope(path):
    """
    Loads an envelope from a file and returns all points as a list of x,y tuples.

    @param path: path to envelope.
    @type path: str
    @return: A list of points and the index of the sustain point (or -1).
    @rtype: [(x,y),...]
    """
    f = file(path, 'rb')
    count = read_int(f)
    sustainindex = read_int(f)
    if sustainindex == 0xffffffff:
	sustainindex = -1
    points = []
    for i in range(count):
	x, y = read_int(f), read_int(f)
	points.append((x, 65535 - y))
    f.close()
    return points, sustainindex

def save_envelope(path, points, sustainindex=-1):
    """
    Saves an envelope to a file from a list of points.

    @param path: path to envelope.
    @type path: str	
    @param points: A list of points.
    @type points: [(x,y),...]
    @param sustainindex: Index of the sustain point (-1 for none).
    @type sustainindex: int
    """
    f = file(path, 'wb')
    write_int(f, len(points))
    if sustainindex == -1:
	sustainindex = 0xffffffff
    write_int(f, sustainindex)
    for x, y in points:
	write_int(f, x)
	write_int(f, 65535 - y)
    f.close()

if __name__ == '__main__':
    import sys, utils
    from main import run
    run(sys.argv)
