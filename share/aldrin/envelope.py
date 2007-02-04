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

from wximport import wx
import os, sys
from utils import prepstr, db2linear, linear2db, note2str
from utils import read_int, write_int
from canvas import Canvas
import zzub
import config

# size of border
BORDER = 5
# size of the envelope dots
DOTSIZE = 8
# matches existing points exactly or approximately
EXACT = 0
NEXT = 1

class EnvelopeView(Canvas):
	"""
	Envelope viewer.
	
	A drawing surface where you can specify how the volume of a sample changes over time.
	"""
	SUSTAIN = wx.NewId()
	DELETE = wx.NewId()
	RESET = wx.NewId()
	LOAD = wx.NewId()
	SAVE = wx.NewId()
	
	def __init__(self, wavetable, *args, **kwds):
		"""
		Initialization.
		"""
		self.envelope = None
		kwds['style'] = wx.SUNKEN_BORDER
		self.parent = wavetable
		self.currentpoint = None
		self.dragging = False
		self.showpoints = False
		Canvas.__init__(self, *args, **kwds)
		wx.EVT_LEFT_DOWN(self, self.on_left_down)
		wx.EVT_LEFT_UP(self, self.on_left_up)
		wx.EVT_CONTEXT_MENU(self, self.on_context_menu)
		wx.EVT_MOTION(self, self.on_motion)
		wx.EVT_MENU(self, self.SUSTAIN, self.on_set_sustain)
		wx.EVT_MENU(self, self.DELETE, self.on_delete_point)
		wx.EVT_MENU(self, self.RESET, self.on_reset)
		wx.EVT_MENU(self, self.LOAD, self.on_load)
		wx.EVT_MENU(self, self.SAVE, self.on_save)
		wx.EVT_ENTER_WINDOW(self, self.on_enter)
		wx.EVT_LEAVE_WINDOW(self, self.on_leave) 
		WILDCARD = '|'.join([
			"Buzz Envelope File (*.BEF)", "*.BEF",
		])
		self.open_dlg = wx.FileDialog(
			self, 
			message="Open", 
			wildcard = WILDCARD,
			style=wx.OPEN | wx.FILE_MUST_EXIST)
		self.save_dlg = wx.FileDialog(
			self, 
			message="Save", 
			wildcard = WILDCARD,
			style=wx.SAVE | wx.OVERWRITE_PROMPT)
			
	def on_enter(self, event):
		"""
		Called when the mouse enters the envelope editor.
		"""
		self.showpoints = True
		self.ReDraw()
		
	def on_leave(self, event):
		"""
		Called when the mouse leaves the envelope editor.
		"""
		self.showpoints = False
		self.ReDraw()
		
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
		ds = int((DOTSIZE / 2.0)+0.5)
		bestindex = None
		for i, point in enumerate(points):
			px,py,f = point
			if (bestindex == None) and (px > x):
				bestindex = i
			rc = wx.Rect(px-ds,py-ds,DOTSIZE,DOTSIZE)
			if rc.Inside((x,y)):
				return i,EXACT
		return bestindex,NEXT
		
	def on_left_down(self, event):
		"""
		Callback that responds to left mouse down over the envelope view.
		
		@param event: MouseEvent event
		@type event: wx.MouseEvent
		"""
		i,location = self.get_point_at((event.m_x, event.m_y))
		if i == None:
			return
		self.CaptureMouse()
		self.currentpoint = i
		self.dragging = True
		if location == NEXT:
			# no direct hit, create a new one
			self.envelope.insert_point(self.currentpoint)
			x,y,f = self.envelope.get_point(self.currentpoint)
			x,y = self.pixel_to_env((event.m_x, event.m_y))
			self.envelope.set_point(self.currentpoint, x, y, f)
		self.ReDraw()

	def on_left_up(self, event):
		"""
		Callback that responds to left mouse up over the envelope view.
		
		@param event: MouseEvent event
		@type event: wx.MouseEvent
		"""
		if self.dragging:
			self.ReleaseMouse()
			self.dragging = False
			self.currentpoint = None
			self.ReDraw()

	def on_motion(self, event):		
		"""
		Callback that responds to mouse motion over the envelope view.
		
		@param event: MouseEvent event
		@type event: wx.MouseEvent
		"""
		if self.dragging:
			x,y,f = self.envelope.get_point(self.currentpoint)
			x,y = self.pixel_to_env((event.m_x, event.m_y))
			self.envelope.set_point(self.currentpoint, x, y, f)
			self.ReDraw()
		else:
			i,location = self.get_point_at((event.m_x, event.m_y))
			if location != EXACT:
				i = None
			if (self.currentpoint != i):
				self.currentpoint = i
				self.ReDraw()

	def on_context_menu(self, event):
		"""
		Callback that responds to context menu activation.
		
		@param event: CommandEvent event
		@type event: wx.CommandEvent
		"""
		mx,my = self.ScreenToClientXY(*event.GetPosition())
		i,location = self.get_point_at((mx, my))
		if i == None:
			return
		menu = wx.Menu()
		if location == EXACT:
			self.currentpoint = i
			self.ReDraw()
			if (i > 0) and (i < (self.envelope.get_point_count()-1)):
				menu.Append(self.DELETE, "&Delete Point", "", wx.ITEM_NORMAL)
			menu.Append(self.SUSTAIN, "&Sustain", "", wx.ITEM_NORMAL)
		else:
			menu.Append(self.RESET, "&Reset", "", wx.ITEM_NORMAL)
			menu.AppendSeparator()
			menu.Append(self.LOAD, "&Load...", "", wx.ITEM_NORMAL)
			menu.Append(self.SAVE, "&Save...", "", wx.ITEM_NORMAL)
		self.PopupMenuXY(menu, mx, my)
		self.currentpoint = None
		self.ReDraw()

	def on_set_sustain(self, event):
		"""
		Callback responding to the context menu item that sets the current point to sustain mode.
		
		@param event: CommandEvent event
		@type event: wx.CommandEvent		
		"""
		# remove old sustain point
		for i in range(self.envelope.get_point_count()):
			x,y,f = self.envelope.get_point(i)
			if f & zzub.zzub_envelope_flag_sustain:
				f = f ^ (f & zzub.zzub_envelope_flag_sustain)
				self.envelope.set_point(i,x,y,f)
		x,y,f = self.envelope.get_point(self.currentpoint)
		self.envelope.set_point(self.currentpoint,x,y,f | zzub.zzub_envelope_flag_sustain)
		self.currentpoint = None
		self.ReDraw()
		
	def on_reset(self, event):
		"""
		Callback responding to the "reset" context menu item.
		
		@param event: CommandEvent event
		@type event: wx.CommandEvent		
		"""
		while self.envelope.get_point_count() > 2:
			self.envelope.delete_point(1)
		self.ReDraw()

	def on_load(self, event):
		"""
		Callback responding to the "load" context menu item.
		
		@param event: CommandEvent event
		@type event: wx.CommandEvent		
		"""
		if self.open_dlg.ShowModal() == wx.ID_OK:
			while self.envelope.get_point_count() > 2:
				self.envelope.delete_point(1)
			points,sustainindex = load_envelope(self.open_dlg.GetPath())
			spoint, epoint = points[0], points[-1]
			self.envelope.set_point(0, spoint[0], spoint[1], 0)
			self.envelope.set_point(1, epoint[0], epoint[1], 0)
			def add_point(x,y,f=0):
				i = self.envelope.get_point_count()-1
				self.envelope.insert_point(i)
				self.envelope.set_point(i,x,y,f)
				return i
			for x,y in points[1:-1]:
				add_point(x,y,0)
			if sustainindex != -1:
				x,y,f = self.envelope.get_point(sustainindex)
				self.envelope.set_point(sustainindex,x,y,f | zzub.zzub_envelope_flag_sustain)
			self.ReDraw()

	def on_save(self, event):
		"""
		Callback responding to the "save" context menu item.
		
		@param event: CommandEvent event
		@type event: wx.CommandEvent		
		"""
		if self.save_dlg.ShowModal() == wx.ID_OK:
			points = []
			sustainindex = -1
			for i,(x,y,f) in enumerate(self.envelope.get_point_list()):
				if f & zzub.zzub_envelope_flag_sustain:
					sustainindex = i
				points.append((x,y))
			save_envelope(self.save_dlg.GetPath(), points, sustainindex)

	def on_delete_point(self, event):
		"""
		Callback responding to the context menu item that deletes the current point of the envelope.
		
		@param event: CommandEvent event
		@type event: wx.CommandEvent		
		"""
		self.envelope.delete_point(self.currentpoint)
		self.currentpoint = None
		self.ReDraw()

	def update(self):
		"""
		Updates the envelope view based on the sample selected in the sample list.		
		"""
		self.currentpoint = None
		self.dragging = False
		self.envelope = None
		sel = self.parent.get_sample_selection()
		if sel:
			w = player.get_wave(sel[0])
			if w.get_envelope_count():
				self.envelope = w.get_envelope(0)
		self.ReDraw()
		
	def env_to_pixel(self, x, y):
		w,h = self.GetClientSize()
		xf = (w-(2*BORDER)-1) / 65535.0
		yf = (h-(2*BORDER)-1) / 65535.0
		return int(x*xf) + BORDER,int((65535 - y)*yf) + BORDER
		
	def pixel_to_env(self, position):
		"""
		Converts a (x,y) pixel coordinate into an envelope point value.
		
		@param position: Pixel coordinate.
		@type position: (int, int)
		@return: (time, amplitude) point on the envelope.
		@rtype: (int, int)
		"""
		x,y = position
		w,h = self.GetClientSize()
		xf = 65535.0 / (w-(2*BORDER)-1)
		yf = 65535.0 / (h-(2*BORDER)-1)
		return max(min(int((x-BORDER)*xf),65535),0),65535 - max(min(int((y-BORDER)*yf),65535),0)
		
	def get_translated_points(self):
		"""
		Converts the envelope values to a list of pixel values.
		
		@return: Pixel values.
		@rtype: list		
		"""
		return [self.env_to_pixel(x,y) + (f,) for x,y,f in self.envelope.get_point_list()]
		
	def Enable(self, enable):
		if enable != self.IsEnabled():
			Canvas.Enable(self, enable)
		self.ReDraw()

	def DrawBuffer(self):
		"""
		Overriding a L{Canvas} method that paints onto an offscreen buffer.
		Draws the envelope view graphics.
		"""	
		dc = self.buffer
		cfg = config.get_config()
		
		bgbrush = cfg.get_brush('EE BG')
		dotbrush = cfg.get_brush('EE Dot')
		selectbrush = cfg.get_brush('EE Dot Selected')
		pen = cfg.get_pen('EE Line')
		brush = cfg.get_brush('EE Fill')
		sustainpen = wx.Pen(cfg.get_color('EE Sustain'), 1, wx.SHORT_DASH)
		gridpen = cfg.get_pen('EE Grid')
		dc.SetBackground(bgbrush)
		dc.Clear()
		dc.SetBrush(dotbrush)		
		if not self.envelope:
			return
		w,h = self.GetClientSize()
		# 4096 (16)
		# 8192 (8)
		dc.SetPen(gridpen)
		xlines = 16
		ylines = 8
		xf = 65535.0 / float(xlines)
		yf = 65535.0 / float(ylines)
		for xg in range(xlines+1):
			pt1 = self.env_to_pixel(xg*xf,0)
			dc.DrawLine(pt1[0],0,pt1[0],h)
		for yg in range(ylines+1):
			pt1 = self.env_to_pixel(0,yg*yf)
			dc.DrawLine(0,pt1[1],w,pt1[1])
		if not self.IsEnabled():
			return
		dc.SetPen(pen)
		dc.SetBrush(brush)
		points = self.get_translated_points()
		ppoints = [None] * len(points)
		envp = None
		for i in range(len(points)):
			pt1 = points[max(i,0)]
			#pt2 = points[min(i+1,len(points)-1)]
			ppoints[i] = wx.Point(pt1[0],pt1[1])
			#dc.DrawLine(pt1[0], pt1[1], pt2[0], pt2[1])
			if pt1[2] & zzub.zzub_envelope_flag_sustain:
				envp = pt1
		ppoints.append(wx.Point(*self.env_to_pixel(65535,0)))
		ppoints.append(wx.Point(*self.env_to_pixel(0,0)))
		dc.DrawPolygon(ppoints)
		if envp:
			dc.SetPen(sustainpen)
			dc.DrawLine(envp[0],0,envp[0],h)
		if self.showpoints:
			dc.SetPen(wx.TRANSPARENT_PEN)
			for i in reversed(range(len(points))):
				pt1 = points[max(i,0)]
				pt2 = points[min(i+1,len(points)-1)]
				if i == self.currentpoint:
					dc.SetBrush(selectbrush)
				else:
					dc.SetBrush(dotbrush)
				dc.DrawCircle(pt1[0],pt1[1], int((DOTSIZE/2.0)+0.5))
		
class ADSRPanel(wx.Panel):
	"""
	ADSR Panel.
	
	Allows to sculpt Attack/Decay/Sustain/Release based envelopes.
	"""
	
	ATTACK_MODES = ["Linear", "48dB Logarithmic", "64dB Logarithmic", "96dB Logarithmic"]
	DECAY_MODES = ["Linear", "Logarithmic"]
	RELEASE_MODES = ["Linear", "-48dB Logarithmic", "-64dB Logarithmic", "-96dB Logarithmic"]
	
	def __init__(self, wavetable, *args, **kwds):
		"""
		Initialization.
		"""
		self.wavetable = wavetable
		wx.Panel.__init__(self, *args, **kwds)
		self.SetTitle("ADSR Envelope Editor")
		topgrid = wx.FlexGridSizer(5,3,5,5)
		self.attack = wx.Slider(self, -1)
		w,h = self.attack.GetMinSize()
		self.attack.SetMinSize((150,h))
		self.cbattacktype = wx.Choice(self, -1)
		self.decay = wx.Slider(self, -1)
		self.cbdecaytype = wx.Choice(self, -1)
		self.sustain = wx.Slider(self, -1)
		self.chksustain = wx.CheckBox(self, -1, "Sustain")
		self.release = wx.Slider(self, -1)
		self.cbreleasetype = wx.Choice(self, -1)
		#self.resolution = wx.Slider(self, -1)
		self.attack.SetMin(0); self.attack.SetMax(16384)
		self.decay.SetMin(0); self.decay.SetMax(16384)
		self.sustain.SetMin(0); self.sustain.SetMax(16384)
		self.release.SetMin(0); self.release.SetMax(16384)
		#self.resolution.SetMin(1); self.resolution.SetMax(48)
		for mode in self.ATTACK_MODES:
			self.cbattacktype.Append(mode)
		for mode in self.DECAY_MODES:
			self.cbdecaytype.Append(mode)
		for mode in self.RELEASE_MODES:
			self.cbreleasetype.Append(mode)
		self.attack.SetValue(0)
		self.decay.SetValue(0)
		self.sustain.SetValue(16384)
		self.release.SetValue(16383)
		#self.resolution.SetValue(8) # how much points between?
		self.cbattacktype.SetSelection(0)
		self.cbdecaytype.SetSelection(0)
		self.cbreleasetype.SetSelection(0)
		topgrid.AddGrowableCol(1)
		topgrid.Add(wx.StaticText(self, -1, "Attack Time"), 0, wx.ALIGN_RIGHT|wx.ALIGN_CENTER_VERTICAL|wx.LEFT|wx.TOP, 5)
		topgrid.Add(self.attack, 1, wx.EXPAND | wx.ALIGN_CENTER_VERTICAL|wx.TOP, 5)
		topgrid.Add(self.cbattacktype, 0, wx.EXPAND|wx.ALIGN_CENTER_VERTICAL|wx.TOP|wx.RIGHT, 5)
		topgrid.Add(wx.StaticText(self, -1, "Decay Time"), 0, wx.ALIGN_RIGHT|wx.ALIGN_CENTER_VERTICAL|wx.LEFT, 5)
		topgrid.Add(self.decay, 1, wx.EXPAND | wx.ALIGN_CENTER_VERTICAL)
		topgrid.Add(self.cbdecaytype, 0, wx.EXPAND|wx.ALIGN_CENTER_VERTICAL|wx.RIGHT, 5)
		topgrid.Add(wx.StaticText(self, -1, "Sustain Level"), 0, wx.ALIGN_RIGHT|wx.ALIGN_CENTER_VERTICAL|wx.LEFT, 5)
		topgrid.Add(self.sustain, 1, wx.EXPAND | wx.ALIGN_CENTER_VERTICAL)
		topgrid.Add(self.chksustain, 0, wx.EXPAND|wx.ALIGN_CENTER_VERTICAL|wx.RIGHT, 5)
		topgrid.Add(wx.StaticText(self, -1, "Release Time"), 0, wx.ALIGN_RIGHT|wx.ALIGN_CENTER_VERTICAL|wx.LEFT, 5)
		topgrid.Add(self.release, 1, wx.EXPAND | wx.ALIGN_CENTER_VERTICAL)
		topgrid.Add(self.cbreleasetype, 0, wx.EXPAND|wx.ALIGN_CENTER_VERTICAL|wx.RIGHT, 5)
		#topgrid.Add(wx.StaticText(self, -1, "Resolution"), 0, wx.ALIGN_CENTER_VERTICAL|wx.LEFT|wx.BOTTOM, 5)
		#topgrid.Add(self.resolution, 1, wx.EXPAND | wx.ALIGN_CENTER_VERTICAL|wx.RIGHT|wx.BOTTOM, 5)
		self.SetSizerAndFit(topgrid)
		self.SetAutoLayout(True)
		self.Layout()
		wx.EVT_SCROLL_THUMBTRACK(self, self.update_envelope)
		wx.EVT_SCROLL_CHANGED(self, self.update_envelope)
		wx.EVT_CHOICE(self, self.cbattacktype.GetId(), self.update_envelope)
		wx.EVT_CHOICE(self, self.cbdecaytype.GetId(), self.update_envelope)
		wx.EVT_CHOICE(self, self.cbreleasetype.GetId(), self.update_envelope)
		wx.EVT_CHECKBOX(self, self.chksustain.GetId(), self.update_envelope)
		
	def update(self):
		"""
		Update slider values from presets.
		"""
		iswave = False
		sel = self.wavetable.get_sample_selection()
		if sel:
			w = player.get_wave(sel[0])
			iswave = w.get_level_count() >= 1
			if w.get_envelope_count():
				env = w.get_envelope(0)
				iswave = env.is_enabled()
				self.attack.SetValue(env.get_attack())
				self.decay.SetValue(env.get_decay())
				self.sustain.SetValue(env.get_sustain())
				self.release.SetValue(env.get_release())
			else:
				iswave = False
		self.attack.Enable(iswave)
		self.decay.Enable(iswave)
		self.sustain.Enable(iswave)
		self.release.Enable(iswave)
		self.chksustain.Enable(iswave)
		self.cbattacktype.Enable(iswave)
		self.cbdecaytype.Enable(iswave)
		self.cbreleasetype.Enable(iswave)

	def update_envelope(self, event=None):
		"""
		Updates the envelope from current slider values.
		"""
		sel = self.wavetable.samplelist.GetSelections()
		if sel:
			w = player.get_wave(sel[0])
			if w.get_envelope_count():
				env = w.get_envelope(0)
				env.set_attack(self.attack.GetValue())
				env.set_decay(self.decay.GetValue())
				env.set_sustain(self.sustain.GetValue())
				env.set_release(self.release.GetValue())
				a = self.attack.GetValue()*65535 / 16384
				d = max(self.decay.GetValue()*65535 / 16384, 1)
				s = self.sustain.GetValue()*65535 / 16384
				r = max(self.release.GetValue()*65535 / 16384, 1)
				p = 9 #self.resolution.GetValue()+1
				atype = self.cbattacktype.GetSelection()
				rtype = self.cbreleasetype.GetSelection()
				while env.get_point_count() > 2:
					env.delete_point(1)
				def add_point(x,y,f=0):
					i = env.get_point_count()-1
					env.insert_point(i)
					env.set_point(i,x,y,f)
					return i
				x = 0
				f = 0
				i = 0
				if self.chksustain.IsChecked():
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
							y = float(idx+1) / p
							limit = [-48,-64,-96][atype-1]
							y = db2linear((1-y)*limit, limit)
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
					limit = [-48,-64,-96][rtype-1]
					fac = linear2db(s / 65535.0, limit)
					for idx in range(p):
						x += stp
						y = float(idx+1) / p
						y = db2linear((y*limit) + fac, limit)
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
		points.append((x,65535 - y))
	f.close()
	return points, sustainindex
	
def save_envelope(path, points, sustainindex = -1):
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
	for x,y in points:
		write_int(f, x)
		write_int(f, 65535 - y)
	f.close()

if __name__ == '__main__':
	import sys, utils
	from main import run
	run(sys.argv)
