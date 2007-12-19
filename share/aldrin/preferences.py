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
Contains panels and dialogs related to application preferences.
"""

import sys, os
from gtkimport import gtk
import zzub
import extman
import webbrowser

from utils import prepstr, buffersize_to_latency, filepath, error, add_scrollbars, new_listview
import utils
import config
import common
from common import MARGIN, MARGIN2, MARGIN3
player = common.get_player()

from controller import learn_controller

samplerates = [96000,48000,44100]
buffersizes = [32768,16384,8192,4096,2048,1024,512,256,128,64,32,16]

class CancelException(Exception):
	"""
	Is being thrown when the user hits cancel in a sequence of
	modal UI dialogs.
	"""

class GeneralPanel(gtk.VBox):
	"""
	Panel which allows changing of general settings.
	"""
	def __init__(self):
		"""
		Initializing.
		"""
		gtk.VBox.__init__(self)
		self.set_border_width(MARGIN)
		frame1 = gtk.Frame("General Settings")
		fssizer = gtk.VBox(False, MARGIN)
		fssizer.set_border_width(MARGIN)
		frame1.add(fssizer)
		audioeditor = config.get_config().get_audioeditor_command()
		incsave = config.get_config().get_incremental_saving()
		#rackpanel = config.get_config().get_experimental('RackPanel')
		self.audioeditor = gtk.Entry()
		self.incsave = gtk.CheckButton()
		self.rackpanel = gtk.CheckButton()
		self.audioeditor.set_text(audioeditor)
		self.incsave.set_active(int(incsave))
		#self.rackpanel.set_active(rackpanel)
		sg1 = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
		sg2 = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
		def add_row(c1, c2):
			row = gtk.HBox(False, MARGIN)
			c1.set_alignment(1, 0.5)
			sg1.add_widget(c1)
			sg2.add_widget(c2)
			row.pack_start(c1, expand=False)
			row.pack_end(c2)
			fssizer.pack_start(row, expand=False)
		add_row(gtk.Label("External Sample Editor"), self.audioeditor)
		add_row(gtk.Label("Incremental Saves"), self.incsave)
		#add_row(gtk.Label("Rack Panel View (After Restart)"), self.rackpanel)
		self.add(frame1)
		
	def apply(self):
		"""
		Writes general config settings to file.
		"""
		audioeditor = self.audioeditor.get_text()
		config.get_config().set_audioeditor_command(audioeditor)
		config.get_config().set_incremental_saving(self.incsave.get_active())
		#config.get_config().set_experimental('RackPanel', self.rackpanel.get_active())
	

class DriverPanel(gtk.VBox):
	"""
	Panel which allows to see and change audio driver settings.
	"""
	def __init__(self):
		"""
		Initializing.
		"""
		gtk.VBox.__init__(self)
		self.set_border_width(MARGIN)
		self.cboutput = gtk.combo_box_new_text()
		self.cbsamplerate = gtk.combo_box_new_text()
		self.cblatency = gtk.combo_box_new_text()
		size_group = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
		def add_row(c1, c2):
			row = gtk.HBox(False, MARGIN)
			size_group.add_widget(c1)
			c1.set_alignment(1, 0.5)
			row.pack_start(c1, expand=False)
			row.pack_start(c2)
			return row
			
		sizer1 = gtk.Frame("Audio Output")
		vbox = gtk.VBox(False, MARGIN)
		vbox.pack_start(add_row(gtk.Label("Driver"), self.cboutput), expand=False)
		vbox.pack_start(add_row(gtk.Label("Samplerate"), self.cbsamplerate), expand=False)
		vbox.pack_start(add_row(gtk.Label("Latency"), self.cblatency), expand=False)
		vbox.set_border_width(MARGIN)
		sizer1.add(vbox)
		inputname, outputname, samplerate, buffersize = config.get_config().get_audiodriver_config()
		if not outputname:
			outputname = player.audiodriver_get_name(-1)
		for i in range(player.audiodriver_get_count()):
			name = prepstr(player.audiodriver_get_name(i))
			self.cboutput.append_text(name)
			if player.audiodriver_get_name(i) == outputname:
				self.cboutput.set_active(i)
		for sr in samplerates:
			self.cbsamplerate.append_text("%iHz" % sr)
		self.cbsamplerate.set_active(samplerates.index(samplerate))
		for bs in buffersizes:
			self.cblatency.append_text("%.1fms" % buffersize_to_latency(bs, 44100))
		self.cblatency.set_active(buffersizes.index(buffersize))
		self.add(sizer1)
		
	def apply(self):
		"""
		Validates user input and reinitializes the driver with current
		settings. If the reinitialization fails, the user is being
		informed and asked to change the settings.
		"""
		sr = self.cbsamplerate.get_active()
		if sr == -1:
			error(self, "You did not pick a valid sample rate.")
			raise CancelException
		sr = samplerates[sr]
		bs = self.cblatency.get_active()
		if bs == -1:
			error(self, "You did not pick a valid latency.")
			raise CancelException
		bs = buffersizes[bs]
		o = self.cboutput.get_active()
		if o == -1:
			error(self, "You did not select a valid output device.")
			raise CancelException
		iname = ""
		oname = player.audiodriver_get_name(o)
		inputname, outputname, samplerate, buffersize = config.get_config().get_audiodriver_config()
		if (oname != outputname) or (samplerate != sr) or (bs != buffersize):
			config.get_config().set_audiodriver_config(iname, oname, sr, bs) # write back
			import driver
			try:
				driver.get_audiodriver().init()
			except driver.AudioInitException:
				import traceback
				traceback.print_exc()
				error(self, "<b><big>There was an error initializing the audio driver.</big></b>\n\nChange settings and try again.")
				raise CancelException

class WavetablePanel(gtk.HBox):
	"""
	Panel which allows to see and change paths to sample libraries.
	"""
	def __init__(self):
		gtk.HBox.__init__(self, False, MARGIN)
		self.set_border_width(MARGIN)
		#~ frame1 = gtk.Frame("Sound Folders")
		#~ vsizer = gtk.VBox(False, MARGIN)
		#~ vsizer.set_border_width(MARGIN)
		#~ frame1.add(vsizer)
		#~ self.pathlist, self.pathstore, columns = new_listview([
			#~ ('Path', str),
		#~ ])
		#~ vsizer.pack_start(add_scrollbars(self.pathlist))
		#~ self.btnadd = gtk.Button(stock=gtk.STOCK_ADD)
		#~ self.btnremove = gtk.Button(stock=gtk.STOCK_REMOVE)
		#~ hsizer = gtk.HButtonBox()
		#~ hsizer.set_spacing(MARGIN)
		#~ hsizer.set_layout(gtk.BUTTONBOX_START)
		#~ hsizer.pack_start(self.btnadd, expand=False)
		#~ hsizer.pack_start(self.btnremove, expand=False)
		#~ vsizer.pack_end(hsizer, expand=False)
		frame2 = gtk.Frame("The Freesound Project")
		fssizer = gtk.VBox(False, MARGIN)
		fssizer.set_border_width(MARGIN)
		frame2.add(fssizer)
		uname, passwd = config.get_config().get_credentials("Freesound")
		self.edusername = gtk.Entry()
		self.edusername.set_text(uname)
		self.edpassword = gtk.Entry()
		self.edpassword.set_visibility(False)
		self.edpassword.set_text(passwd)
		logo = gtk.Image()
		logo.set_from_file(filepath("res/fsbanner.png"))
		lalign = gtk.HBox()
		lalign.pack_start(logo, expand=False)
		fssizer.pack_start(lalign, expand=False)
		fsintro = gtk.Label("The Freesound Project is a public library of Creative Commons licensed sounds and samples.")
		fsintro.set_alignment(0, 0)
		fsintro.set_line_wrap(True)
		fssizer.pack_start(fsintro, expand=False)
		sg1 = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
		sg2 = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
		def add_row(c1, c2):
			row = gtk.HBox(False, MARGIN)
			c1.set_alignment(1, 0.5)
			sg1.add_widget(c1)
			sg2.add_widget(c2)
			row.pack_start(c1, expand=False)
			row.pack_end(c2)
			fssizer.pack_start(row, expand=False)
		add_row(gtk.Label("Username"), self.edusername)
		add_row(gtk.Label("Password"), self.edpassword)
		fspwddesc = gtk.Label("You will need an username and a password in order to search and download freesound samples.")
		fspwddesc.set_alignment(0, 0)
		fspwddesc.set_line_wrap(True)
		fssizer.pack_start(fspwddesc, expand=False)
		fsvisit = gtk.Button("Visit website")
		lalign = gtk.HBox()
		lalign.pack_start(fsvisit, expand=False)
		fssizer.pack_start(lalign, expand=False)
		#~ self.add(frame1)
		self.add(frame2)
		#~ self.btnadd.connect('clicked', self.on_add_path)
		#~ self.btnremove.connect('clicked', self.on_remove_path)
		fsvisit.connect('clicked', lambda widget: webbrowser.open("http://freesound.iua.upf.edu/"))
		#~ for path in config.get_config().get_wavetable_paths():
			#~ self.pathstore.append([path])

	def apply(self):
		"""
		Stores list of paths back to config.
		"""
		olduname,oldpasswd = config.get_config().get_credentials("Freesound")
		uname = self.edusername.get_text()
		passwd = self.edpassword.get_text()
		if (uname != olduname) or (oldpasswd != passwd):
			import freesound
			fs = freesound.Freesound()
			try:
				if uname and passwd:
					fs.login(uname,passwd)
				config.get_config().set_credentials("Freesound",uname,passwd)
			except:
				import traceback
				traceback.print_exc()
				utils.error(self, "<b><big>There was an error logging into freesound.</big></b>\n\nPlease make sure username and password are correct.")
				raise CancelException
		#~ pathlist = []
		#~ for row in self.pathstore:
			#~ pathlist.append(row[0])
		#~ config.get_config().set_wavetable_paths(pathlist)
		
	def on_add_path(self, event):
		"""
		Handles 'Add' button click. Opens a directory selection dialog.
		"""
		dlg = gtk.FileChooserDialog(
			"Add Sound Folder", None,
			gtk.FILE_CHOOSER_ACTION_SELECT_FOLDER,
			(gtk.STOCK_OK, gtk.RESPONSE_OK, gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL))
		response = dlg.run()
		if response == gtk.RESPONSE_OK:
			found = [row for row in self.pathstore if row[0] == dlg.get_filename()]
			if not found:
				self.pathstore.append([dlg.get_filename()])
		dlg.destroy()
		
	def on_remove_path(self, event):
		"""
		Handles 'Remove' button click. Removes the selected path from list.
		"""
		store, sel = self.pathlist.get_selection().get_selected_rows()
		refs = [gtk.TreeRowReference(store, row) for row in sel]
		for ref in refs:
			store.remove(store.get_iter(ref.get_path()))

class ControllerPanel(gtk.VBox):
	"""
	Panel which allows to set up midi controller mappings.
	"""
	def __init__(self, rootwindow):
		self.rootwindow = rootwindow
		self.sort_column = 0
		gtk.VBox.__init__(self)
		self.set_border_width(MARGIN)
		frame1 = gtk.Frame("Controllers")
		sizer1 = gtk.VBox(False, MARGIN)
		sizer1.set_border_width(MARGIN)
		frame1.add(sizer1)
		self.controllers, self.store, columns = new_listview([
			('Name', str),
			('Channel', str),
			('Controller', str),
		])
		self.controllers.get_selection().set_mode(gtk.SELECTION_MULTIPLE)
		sizer1.add(add_scrollbars(self.controllers))
		self.btnadd = gtk.Button(stock=gtk.STOCK_ADD)
		self.btnremove = gtk.Button(stock=gtk.STOCK_REMOVE)
		hsizer = gtk.HButtonBox()
		hsizer.set_spacing(MARGIN)
		hsizer.set_layout(gtk.BUTTONBOX_START)
		hsizer.pack_start(self.btnadd, expand=False)
		hsizer.pack_start(self.btnremove, expand=False)
		sizer1.pack_start(hsizer, expand=False)
		self.add(frame1)
		self.btnadd.connect('clicked', self.on_add_controller)
		self.btnremove.connect('clicked', self.on_remove_controller)
		self.update_controllers()		
		
	def update_controllers(self):
		"""
		Updates the controller list.
		"""
		self.store.clear()
		for name,channel,ctrlid in config.get_config().get_midi_controllers():
			self.store.append([name, str(channel), str(ctrlid)])
		
	def on_add_controller(self, event):
		"""
		Handles 'Add' button click. Opens a popup that records controller events.
		"""
		res = learn_controller(self, self.rootwindow)
		if res:
			name, channel, ctrlid = res
			self.store.append([name, str(channel), str(ctrlid)])
		
	def on_remove_controller(self, event):
		"""
		Handles 'Remove' button click. Removes the selected controller from list.
		"""
		store, sel = self.controllers.get_selection().get_selected_rows()
		refs = [gtk.TreeRowReference(store, row) for row in sel]
		for ref in refs:
			store.remove(store.get_iter(ref.get_path()))
		
	def apply(self):
		"""
		Validates user input and reinitializes the driver with current
		settings. If the reinitialization fails, the user is being
		informed and asked to change the settings.
		"""
		ctrllist = []
		item = -1
		for row in self.store:
			name = row[0]
			channel = int(row[1])
			ctrlid = int(row[2])
			ctrllist.append((name,channel,ctrlid))
		config.get_config().set_midi_controllers(ctrllist)

class MidiPanel(gtk.VBox):
	"""
	Panel which allows to see and change a list of used MIDI output devices.
	"""
	def __init__(self):
		gtk.VBox.__init__(self, False, MARGIN)
		self.set_border_width(MARGIN)
		frame1 = gtk.Frame("MIDI Input Devices")
		sizer1 = gtk.VBox()
		sizer1.set_border_width(MARGIN)
		frame1.add(sizer1)
		self.idevicelist, self.istore, columns = new_listview([
			("Use", bool),
			("Device", str),
		])
		self.idevicelist.set_property('headers-visible', False)
		inputlist = config.get_config().get_mididriver_inputs()
		for i in range(player.mididriver_get_count()):
			if player.mididriver_is_input(i):
				name = prepstr(player.mididriver_get_name(i))
				use = name.strip() in inputlist
				self.istore.append([use, name])
		sizer1.add(add_scrollbars(self.idevicelist))
		frame2 = gtk.Frame("MIDI Output Devices")
		sizer2 = gtk.VBox()
		sizer2.set_border_width(MARGIN)
		frame2.add(sizer2)
		self.odevicelist, self.ostore, columns = new_listview([
			("Use", bool),
			("Device", str),
		])
		self.odevicelist.set_property('headers-visible', False)
		outputlist = config.get_config().get_mididriver_outputs()
		for i in range(player.mididriver_get_count()):
			if player.mididriver_is_output(i):
				name = prepstr(player.mididriver_get_name(i))
				use = name in outputlist
				self.ostore.append([use,name])
		sizer2.add(add_scrollbars(self.odevicelist))
		self.add(frame1)
		self.add(frame2)
		label = gtk.Label("Checked MIDI devices will be used the next time you start Aldrin.")
		label.set_alignment(0, 0)
		self.pack_start(label, expand=False)

	def apply(self):
		"""
		Adds the currently selected drivers to the list.
		"""
		inputlist = []
		for row in self.istore:
			if row[0]:
				inputlist.append(row[1])
		config.get_config().set_mididriver_inputs(inputlist)
		outputlist = []
		for row in self.ostore:
			if row[0]:
				outputlist.append(row[1])
		config.get_config().set_mididriver_outputs(outputlist)

class KeyboardPanel(gtk.VBox):
	"""
	Panel which allows to see and change the current keyboard configuration.
	"""
	
	KEYMAPS = [
		('en', 'English (QWERTY)'),
		('de', 'Deutsch (QWERTZ)'),
		('dv', 'Dvorak (\',.PYF)'),
		('fr', 'French (AZERTY)')
	]
	
	def __init__(self):
		gtk.VBox.__init__(self, False, MARGIN)
		self.set_border_width(MARGIN)
		hsizer = gtk.HBox(False, MARGIN)
		hsizer.pack_start(gtk.Label("Keyboard Map"), expand=False)
		self.cblanguage = gtk.combo_box_new_text()
		sel = 0
		lang = config.get_config().get_keymap_language()
		index = 0
		for kmid, name in self.KEYMAPS:
			self.cblanguage.append_text(name)
			if lang == kmid:
				sel = index
			index += 1
		hsizer.add(self.cblanguage)
		self.pack_start(hsizer, expand=False)
		self.cblanguage.set_active(sel)

	def apply(self):
		"""
		applies the keymap.
		"""
		config.get_config().set_keymap_language(self.KEYMAPS[self.cblanguage.get_active()][0])

class ExtensionsPanel(gtk.VBox):
	"""
	Panel which allows to enable and disable extensions.
	"""
	
	def __init__(self):
		gtk.VBox.__init__(self, False, MARGIN)
		self.set_border_width(MARGIN)
		#~ self.extlist = ExtensionListBox(self, -1, style=wx.SUNKEN_BORDER)
		self.extman = extman.get_extension_manager()
		self.cfg = config.get_config()
		self.extlist, self.extstore, columns = new_listview([
			("Use", bool),
			("Name", str, dict(markup=True,wrap=True)),
		])
		self.extlist.set_property('headers-visible', False)
		exts = config.get_config().get_enabled_extensions()
		for ext in self.extman.extensions:
			name = prepstr(ext.name)
			checked = ext.uri in exts
			markup = ''
			markup += '<b><big>%s</big></b>\n' % ext.name
			markup += '<i>Version %s, by %s</i>\n' % (ext.version, ext.author)
			markup += '\n'
			markup += '%s\n' % ext.description
			self.extstore.append([checked,markup])
		self.pack_start(add_scrollbars(self.extlist))
		label = gtk.Label("Click OK and restart Aldrin to apply changes.")
		label.set_alignment(0, 0.5)
		self.pack_start(label, expand=False)
		
	def on_extlist_select(self, event):
		"""
		Called when an extension is selected. Updates the html description.
		
		@param event: Event.
		@type event: wx.Event
		"""
		ext = self.extman.extensions[event.GetSelection()]
		out = """<html><head></head>
		<style type="text/css">
		body {
			font-size: 8pt;
		}
		</style>
		<body><font size="-4">"""
		out += "</font></body></html>"
		self.htmldesc.SetPage(out)
		
	def apply(self):
		"""
		Updates the config object with the currently selected extensions.
		"""
		exts = []
		for i,row in enumerate(self.extstore):
			if row[0]:
				ext = self.extman.extensions[i]
				exts.append(ext.uri)
		config.get_config().set_enabled_extensions(exts)

class PreferencesDialog(gtk.Dialog):
	"""
	This Dialog aggregates the different panels and allows
	the user to switch between them using a tab control.
	"""
	def __init__(self, rootwindow,parent):
		gtk.Dialog.__init__(self,
			"Preferences",
			parent,
			gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT)
		nb = gtk.Notebook()
		nb.set_border_width(MARGIN)
		self.generalpanel = GeneralPanel()
		self.driverpanel = DriverPanel()
		self.wavetablepanel = WavetablePanel()
		self.midipanel = MidiPanel()
		self.controllerpanel = ControllerPanel(rootwindow)
		self.keyboardpanel = KeyboardPanel()
		self.extensionspanel = ExtensionsPanel()
		nb.append_page(self.driverpanel, gtk.Label("Audio"))
		nb.append_page(self.midipanel, gtk.Label("MIDI"))
		nb.append_page(self.controllerpanel, gtk.Label("Controllers"))
		nb.append_page(self.keyboardpanel, gtk.Label("Keyboard"))
		nb.append_page(self.wavetablepanel, gtk.Label("Sound Library"))
		nb.append_page(self.generalpanel, gtk.Label("General"))
		nb.append_page(self.extensionspanel, gtk.Label("Extensions"))
		self.vbox.add(nb)
		
		btnok = self.add_button(gtk.STOCK_OK, gtk.RESPONSE_OK)
		self.add_button(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL)
		btnapply = self.add_button(gtk.STOCK_APPLY, gtk.RESPONSE_APPLY)
		btnok.grab_default()

		self.connect('response', self.on_response)
		self.show_all()
		
	def on_response(self, dialog, response):
		if response == gtk.RESPONSE_OK:
			self.on_ok()
		elif response == gtk.RESPONSE_APPLY:
			self.on_apply()
		else:
			self.destroy()
		
	def apply(self):
		"""
		Apply changes in settings without closing the dialog.
		"""
		self.generalpanel.apply()
		self.wavetablepanel.apply()
		self.extensionspanel.apply()
		self.keyboardpanel.apply()
		self.driverpanel.apply()
		self.controllerpanel.apply()
		self.midipanel.apply()
		
	def on_apply(self):
		"""
		Event handler for apply button.
		"""
		try:
			self.apply()
		except CancelException:
			pass

	def on_ok(self):
		"""
		Event handler for OK button. Calls apply
		and then closes the dialog.
		"""
		try:
			self.apply()		
			self.destroy()
		except CancelException:
			pass

def show_preferences(rootwindow, parent):
	"""
	Shows the {PreferencesDialog}.
	
	@param rootwindow: The root window which receives zzub callbacks.
	@type rootwindow: wx.Frame
	@param parent: Parent window.
	@type parent: wx.Window
	"""
	dlg = PreferencesDialog(rootwindow, parent)

__all__ = [
'CancelException',
'DriverPanel',
'WavetablePanel',
'MidiInPanel',
'MidiOutPanel',
'KeyboardPanel',
'ExtensionsPanel',
'PreferencesDialog',
'show_preferences',
]

if __name__ == '__main__':
	import testplayer, utils
	player = testplayer.get_player()
	window = testplayer.TestWindow()
	window.show_all()
	show_preferences(window, window)
	gtk.main()
