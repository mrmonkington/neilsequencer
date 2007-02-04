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
Provides dialog class for hd recorder control.
"""


from wximport import wx
import utils, os, stat

class HDRecorderDialog(wx.Dialog):
	"""
	This Dialog shows the HD recorder, which allows recording
	the audio output to a wave file.
	"""
	SAVEAS = wx.NewId()
	REC = wx.NewId()
	STOP = wx.NewId()
	AUTOSTARTSTOP = wx.NewId()
	
	WILDCARD = "PCM Waves (*.wav)|*.wav"
	
	def __init__(self, *args, **kwds):
		"""
		Initializer.
		"""
		wx.Dialog.__init__(self, *args, **kwds)
		self.master = player.get_plugin(0)
		self.SetTitle("Hard Disk Recorder")
		btnsaveas = wx.Button(self, self.SAVEAS, "Save As...", style=wx.SUNKEN_BORDER)
		textposition = wx.StaticText(self, -1, "")
		btnrec = wx.Button(self, self.REC, "&Rec")
		btnstop = wx.Button(self, self.STOP, "&Stop")
		chkauto = wx.CheckBox(self, self.AUTOSTARTSTOP, "&Auto start/stop")
		self.btnsaveas = btnsaveas
		self.textposition = textposition
		self.btnrec = btnrec
		self.btnstop = btnstop
		self.chkauto = chkauto
		self.chkauto.SetValue(self.master.get_auto_write())
		sizer = wx.BoxSizer(wx.VERTICAL)
		
		sizer.Add(btnsaveas, 0, wx.EXPAND | wx.TOP | wx.LEFT | wx.RIGHT, border=5)
		sizer.Add(textposition, 0, wx.EXPAND | wx.TOP | wx.LEFT | wx.RIGHT, border=5)
		sizer2 = wx.BoxSizer(wx.HORIZONTAL)
		sizer2.Add(btnrec, 0, wx.ALIGN_LEFT)
		sizer2.Add(btnstop, 0, wx.ALIGN_LEFT | wx.LEFT, border=5)
		sizer2.Add(chkauto, 0, wx.ALIGN_LEFT | wx.LEFT, border=5)
		sizer.Add(sizer2, 0, wx.ALIGN_LEFT | wx.ALL, border=5)
		self.SetAutoLayout(True)
		self.SetSizer(sizer)
		self.Layout()
		self.Fit()
		self.Centre()
		wx.EVT_BUTTON(self, self.SAVEAS, self.on_saveas)
		wx.EVT_BUTTON(self, self.REC, self.on_rec)
		wx.EVT_BUTTON(self, self.STOP, self.on_stop)
		wx.EVT_CHECKBOX(self, self.AUTOSTARTSTOP, self.on_autostartstop)
		self.filename = ''
		self.timer = wx.Timer(self, -1)
		self.timer.Start(100)
		wx.EVT_TIMER(self, self.timer.GetId(), self.on_timer)
		
	def on_autostartstop(self, event):
		"""
		Handles clicks on the auto start/stop checkbox. Updates the masters
		auto_write property.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		self.master.set_auto_write(event.IsChecked())
		
	def on_timer(self, event):
		"""
		Called by timer event. Updates controls according to current
		state of recording.
		
		@param event: timer event
		@type event: wx.TimerEvent
		"""
		if self.IsShown():
			bpm = self.master.get_parameter_value(1, 0, 1)
			tpb = self.master.get_parameter_value(1, 0, 2)
			rectime = utils.ticks_to_time(self.master.get_ticks_written(), bpm, tpb)
			if os.path.isfile(self.filename):
				recsize = os.stat(self.filename)[stat.ST_SIZE]
			else:
				recsize = 0
			self.textposition.SetLabel("Time:  %s     Size: %.2fM" % (utils.format_time(rectime), float(recsize)/(1<<20)))
		
	def on_saveas(self, event):
		"""
		Handler for the "Save As..." button.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		dlg = wx.FileDialog(
			self, 
			message="Save", 
			defaultFile = self.filename,
			wildcard = self.WILDCARD,
			style=wx.SAVE | wx.OVERWRITE_PROMPT)
		if dlg.ShowModal() == wx.ID_OK:
			self.filename = dlg.GetPath()
			self.btnsaveas.SetLabel(self.filename)
			master = player.get_plugin(0)
			master.set_wave_file_path(self.filename)

	def on_rec(self, event):
		"""
		Handler for the "Rec" button.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		self.master.set_write_wave(1)

	def on_stop(self, event):
		"""
		Handler for the "Stop" button.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		self.master.set_write_wave(0)

__all__ = [
'HDRecorderDialog',
]

if __name__ == '__main__':
	import sys, utils
	from main import run
	sys.argv.append(utils.filepath('demosongs/test.bmx'))
	run(sys.argv)
