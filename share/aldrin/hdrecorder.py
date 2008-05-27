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
Provides dialog class for hd recorder control.
"""


from gtkimport import gtk
import gobject
import utils, os, stat
import common
from aldrincom import com
from common import MARGIN, MARGIN2, MARGIN3

class HDRecorderDialog(gtk.Dialog):
	"""
	This Dialog shows the HD recorder, which allows recording
	the audio output to a wave file.
	"""

	def __init__(self, parent):
		"""
		Initializer.
		"""
		gtk.Dialog.__init__(self, 
			"Hard Disk Recorder",
			parent.get_toplevel())
		#self.add_button(gtk.STOCK_CLOSE, gtk.RESPONSE_CLOSE)
		#self.set_size_request(250,-1)
		self.set_resizable(False)
		player = com.get('aldrin.core.player')
		self.master = player.get_plugin(0)
		btnsaveas = gtk.Button(stock=gtk.STOCK_SAVE_AS)
		btnsaveas.connect("clicked", self.on_saveas)
		textposition = gtk.Label("")
		btnrec = gtk.Button(stock=gtk.STOCK_MEDIA_RECORD)
		btnrec.connect("clicked", self.on_rec)
		btnstop = gtk.Button(stock=gtk.STOCK_MEDIA_STOP)
		btnstop.connect("clicked", self.on_stop)
		chkauto = gtk.CheckButton("_Auto start/stop")
		chkauto.connect("toggled", self.on_autostartstop)
		self.btnsaveas = btnsaveas
		self.textposition = textposition
		self.btnrec = btnrec
		self.btnstop = btnstop
		self.chkauto = chkauto
		self.chkauto.set_active(self.master.get_auto_write())
		sizer = gtk.VBox(False, MARGIN)		
		sizer.pack_start(btnsaveas, expand=False)
		sizer.pack_start(textposition, expand=False)
		sizer2 = gtk.HBox(False,MARGIN)
		sizer3 = gtk.HButtonBox()
		sizer3.set_spacing(MARGIN)
		sizer3.set_layout(gtk.BUTTONBOX_START)
		sizer3.pack_start(btnrec, expand=False)
		sizer3.pack_start(btnstop, expand=False)
		sizer2.pack_start(sizer3, expand=False)
		sizer2.pack_start(chkauto, expand=False)
		sizer.pack_start(sizer2)
		sizer.set_border_width(MARGIN)
		self.vbox.add(sizer)
		self.filename = ''
		gobject.timeout_add(100, self.on_timer)
		
	def on_autostartstop(self, widget):
		"""
		Handles clicks on the auto start/stop checkbox. Updates the masters
		auto_write property.
		
		@param event: Command event.
		@type event: wx.CommandEvent
		"""
		self.master.set_auto_write(widget.get_active())
		
	def on_timer(self):
		"""
		Called by timer event. Updates controls according to current
		state of recording.
		"""
		if self.window and self.window.is_visible():
			bpm = self.master.get_parameter_value(1, 0, 1)
			tpb = self.master.get_parameter_value(1, 0, 2)
			rectime = utils.ticks_to_time(self.master.get_ticks_written(), bpm, tpb)
			if os.path.isfile(self.filename):
				recsize = os.stat(self.filename)[stat.ST_SIZE]
			else:
				recsize = 0
			self.textposition.set_markup("<b>Time</b> %s     <b>Size</b> %.2fM" % (utils.format_time(rectime), float(recsize)/(1<<20)))
		return True
		
	def on_saveas(self, widget):
		"""
		Handler for the "Save As..." button.
		"""
		dlg = gtk.FileChooserDialog(title="Save", parent=self, action=gtk.FILE_CHOOSER_ACTION_SAVE,
			buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_SAVE, gtk.RESPONSE_OK)
		)
		dlg.set_do_overwrite_confirmation(True)
		ffwav = gtk.FileFilter()
		ffwav.set_name("PCM Waves (*.wav)")
		ffwav.add_pattern("*.wav")
		dlg.set_filename(self.filename)
		dlg.add_filter(ffwav)
		result = dlg.run()
		if result == gtk.RESPONSE_OK:
			self.filename = dlg.get_filename()
			if (dlg.get_filter() == ffwav) and not (self.filename.endswith('.wav')):
				self.filename += '.wav'
			self.btnsaveas.set_label(self.filename)
			master = player.get_plugin(0)
			master.set_wave_file_path(self.filename)
		dlg.destroy()

	def on_rec(self, widget):
		"""
		Handler for the "Rec" button.
		"""
		self.master.reset_ticks_written()
		self.master.set_write_wave(1)

	def on_stop(self, widget):
		"""
		Handler for the "Stop" button.
		"""
		self.master.set_write_wave(0)

__all__ = [
'HDRecorderDialog',
]

if __name__ == '__main__':
	import testplayer, utils
	player = testplayer.get_player()
	player.load_ccm(utils.filepath('demosongs/paniq-knark.ccm'))
	window = testplayer.TestWindow()
	window.show_all()
	dlg = HDRecorderDialog(window)
	dlg.connect('destroy', lambda widget: gtk.main_quit())
	dlg.show_all()
	gtk.main()
