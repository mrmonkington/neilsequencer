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
Contains panels and dialogs related to application preferences.
"""

import os
import gtk
import webbrowser

from aldrin.utils import prepstr, buffersize_to_latency, filepath, error, add_scrollbars, new_listview
import config
from aldrin.common import MARGIN, MARGIN2, MARGIN3

from aldrin.controller import learn_controller
import aldrin.com as com

import zzub

samplerates = [96000,48000,44100]
buffersizes = [32768,16384,8192,4096,2048,1024,512,256,128,64,32,16]

class CancelException(Exception):
	"""
	Is being thrown when the user hits cancel in a sequence of
	modal UI dialogs.
	"""
	__aldrin__ = dict(
		id = 'aldrin.exception.cancel',
		exception = True,
		categories = [
		]
	)

class GeneralPanel(gtk.VBox):
	"""
	Panel which allows changing of general settings.
	"""
	
	__aldrin__ = dict(
		id = 'aldrin.core.pref.general',
		categories = [
			'aldrin.prefpanel',
		]
	)
	
	__prefpanel__ = dict(
		label = "General",
	)
	
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
		leddraw = config.get_config().get_led_draw()
		patnoteoff = config.get_config().get_pattern_noteoff()
		self.patternfont = gtk.FontButton(config.get_config().get_pattern_font())
		self.patternfont.set_use_font(True)
		self.patternfont.set_use_size(True)
		self.patternfont.set_show_style(True)
		self.patternfont.set_show_size(True)
		self.audioeditor = gtk.Entry()
		self.incsave = gtk.CheckButton()
		self.leddraw = gtk.CheckButton()
		self.patnoteoff = gtk.CheckButton()
		self.rackpanel = gtk.CheckButton()
		self.audioeditor.set_text(audioeditor)
		self.incsave.set_active(int(incsave))
		self.leddraw.set_active(int(leddraw))
		self.patnoteoff.set_active(int(patnoteoff))
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
		add_row(gtk.Label("Draw Amp LEDs in Router"), self.leddraw)
		add_row(gtk.Label("Auto Note-Off in Pattern Editor"), self.patnoteoff)
		add_row(gtk.Label("Pattern Font"), self.patternfont)
		#add_row(gtk.Label("Rack Panel View (After Restart)"), self.rackpanel)
		self.add(frame1)
		
	def apply(self):
		"""
		Writes general config settings to file.
		"""
		config.get_config().set_pattern_font(self.patternfont.get_font_name())
		audioeditor = self.audioeditor.get_text()
		config.get_config().set_audioeditor_command(audioeditor)
		config.get_config().set_incremental_saving(self.incsave.get_active())
		config.get_config().set_led_draw(self.leddraw.get_active())
		config.get_config().set_pattern_noteoff(self.patnoteoff.get_active())
		#config.get_config().set_experimental('RackPanel', self.rackpanel.get_active())

class DriverPanel(gtk.VBox):
	"""
	Panel which allows to see and change audio driver settings.
	"""
	
	__aldrin__ = dict(
		id = 'aldrin.core.pref.driver',
		categories = [
			'aldrin.prefpanel',
		]
	)
	
	__prefpanel__ = dict(
		label = "Audio",
	)
	
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
		audiodriver = com.get('aldrin.core.driver.audio')
		if not outputname:
			outputname = audiodriver.get_name(-1)
		for i in range(audiodriver.get_count()):
			name = prepstr(audiodriver.get_name(i))
			self.cboutput.append_text(name)
			if audiodriver.get_name(i) == outputname:
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
			raise com.exception('aldrin.exception.cancel')
		sr = samplerates[sr]
		bs = self.cblatency.get_active()
		if bs == -1:
			error(self, "You did not pick a valid latency.")
			raise com.exception('aldrin.exception.cancel')
		bs = buffersizes[bs]
		o = self.cboutput.get_active()
		if o == -1:
			error(self, "You did not select a valid output device.")
			raise com.exception('aldrin.exception.cancel')
		iname = ""
		audiodriver = com.get('aldrin.core.driver.audio')
		oname = audiodriver.get_name(o)
		inputname, outputname, samplerate, buffersize = config.get_config().get_audiodriver_config()
		if (oname != outputname) or (samplerate != sr) or (bs != buffersize):
			config.get_config().set_audiodriver_config(iname, oname, sr, bs) # write back
			try:
				audiodriver.init()
			except audiodriver.AudioInitException:
				import traceback
				traceback.print_exc()
				error(self, "<b><big>There was an error initializing the audio driver.</big></b>\n\nChange settings and try again.")
				raise com.exception('aldrin.exception.cancel')

class WavetablePanel(gtk.HBox):
	"""
	Panel which allows to see and change paths to sample libraries.
	"""
	
	__aldrin__ = dict(
		id = 'aldrin.core.pref.wavetable',
		categories = [
			'aldrin.prefpanel',
		]
	)
	
	__prefpanel__ = dict(
		label = "Sound Library",
	)
	
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
		fspwddesc = gtk.Label("You will need a username and a password in order to search and download freesound samples.")
		fspwddesc.set_alignment(0, 0)
		fspwddesc.set_line_wrap(True)
		fssizer.pack_start(fspwddesc, expand=False)
		fsvisit = gtk.Button("Visit website")
		lalign = gtk.HBox()
		lalign.pack_start(fsvisit, expand=False)
		fssizer.pack_start(lalign, expand=False)
		self.add(frame2)
		fsvisit.connect('clicked', lambda widget: webbrowser.open("http://www.freesound.org/"))

	def apply(self):
		"""
		Stores list of paths back to config.
		"""
		olduname,oldpasswd = config.get_config().get_credentials("Freesound")
		uname = self.edusername.get_text()
		passwd = self.edpassword.get_text()
		if (uname != olduname) or (oldpasswd != passwd):
			import aldrin.freesound as freesound
			fs = freesound.Freesound()
			try:
				if uname and passwd:
					fs.login(uname,passwd)
				config.get_config().set_credentials("Freesound",uname,passwd)
			except:
				import traceback
				traceback.print_exc()
				error(self, "<b><big>There was an error logging into freesound.</big></b>\n\nPlease make sure username and password are correct.")
				raise com.exception('aldrin.exception.cancel')

class ControllerPanel(gtk.VBox):
	"""
	Panel which allows to set up midi controller mappings.
	"""
	
	__aldrin__ = dict(
		id = 'aldrin.core.pref.controller',
		categories = [
			'aldrin.prefpanel',
		]
	)
	
	__prefpanel__ = dict(
		label = "Controllers",
	)
	
	def __init__(self):
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
		
	def on_add_controller(self, widget):
		"""
		Handles 'Add' button click. Opens a popup that records controller events.
		"""
		res = learn_controller(self)
		if res:
			name, channel, ctrlid = res
			self.store.append([name, str(channel), str(ctrlid)])
		
	def on_remove_controller(self, widget):
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
	
	__aldrin__ = dict(
		id = 'aldrin.core.pref.midi',
		categories = [
			'aldrin.prefpanel',
		]
	)
	
	__prefpanel__ = dict(
		label = "MIDI",
	)
	
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
		player = com.get('aldrin.core.player')
		for i in range(zzub.Mididriver.get_count(player)):
			if zzub.Mididriver.is_input(player,i):
				name = prepstr(zzub.Mididriver.get_name(player,i))
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
		for i in range(zzub.Mididriver.get_count(player)):
			if zzub.Mididriver.is_output(player,i):
				name = prepstr(zzub.Mididriver.get_name(player,i))
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

	__aldrin__ = dict(
		id = 'aldrin.core.pref.keyboard',
		categories = [
			'aldrin.prefpanel',
		]
	)

	__prefpanel__ = dict(
		label = "Keyboard",
	)
	
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

def cmp_prefpanel(a,b):
	a_order = (hasattr(a, '__prefpanel__') and a.__prefpanel__.get('label','')) or ''
	b_order = (hasattr(b, '__prefpanel__') and b.__prefpanel__.get('label','')) or ''
	return cmp(a_order, b_order)


class PreferencesDialog(gtk.Dialog):
	"""
	This Dialog aggregates the different panels and allows
	the user to switch between them using a tab control.
	"""
	
	__aldrin__ = dict(
		id = 'aldrin.core.prefdialog',
		categories = [
		]
	)
	
	def __init__(self, parent=None, visible_panel=None):
		gtk.Dialog.__init__(self,
			"Preferences",
			parent,
			gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT)
		self.nb = gtk.Notebook()
		self.nb.set_show_tabs(False)
		self.nb.set_border_width(MARGIN)
		self.nb.set_show_border(False)
		self.panels = sorted(com.get_from_category('aldrin.prefpanel'), cmp_prefpanel)
		starting_tab_index = 0
		for i, panel in enumerate(self.panels):
			if not hasattr(panel, '__prefpanel__'):
				continue
			cfg = panel.__prefpanel__
			label = cfg.get('label',None)
			if not label:
				continue
			if visible_panel == panel.__aldrin__['id']:
				starting_tab_index = i
			self.nb.append_page(panel, gtk.Label(label))
		self.tab_list, self.tab_list_store, columns = new_listview([('Name', str),])
		self.tab_list.set_headers_visible(False)
		self.tab_list.set_size_request(120, 100)
		# iterate through all tabs and add to tab list
		for i in range(self.nb.get_n_pages()):
			tab_label = self.nb.get_tab_label(self.nb.get_nth_page(i)).get_label()
			self.tab_list_store.append([tab_label])
		self.tab_list.connect('cursor-changed', self.on_tab_list_change)
		self.splitter = gtk.HPaned()
		self.splitter.pack1(add_scrollbars(self.tab_list))
		self.splitter.pack2(self.nb)
		self.vbox.add(self.splitter)
		
		btnok = self.add_button(gtk.STOCK_OK, gtk.RESPONSE_OK)
		self.add_button(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL)
		self.add_button(gtk.STOCK_APPLY, gtk.RESPONSE_APPLY)
		btnok.grab_default()

		self.connect('response', self.on_response)
		self.show_all()
		print starting_tab_index
		# select starting tab and adjust the list index
		self.nb.set_current_page(starting_tab_index)
		
	def on_response(self, widget, response):
		if response == gtk.RESPONSE_OK:
			self.on_ok()
		elif response == gtk.RESPONSE_APPLY:
			self.on_apply()
		else:
			self.destroy()

	def on_tab_list_change(self, treeview):
		self.nb.set_current_page(treeview.get_cursor()[0][0])
		
	def apply(self):
		"""
		Apply changes in settings without closing the dialog.
		"""
		for panel in self.panels:
			panel.apply()
		
	def on_apply(self):
		"""
		Event handler for apply button.
		"""
		try:
			self.apply()
		except com.exception('aldrin.exception.cancel'):
			pass

	def on_ok(self):
		"""
		Event handler for OK button. Calls apply
		and then closes the dialog.
		"""
		try:
			self.apply()		
			self.destroy()
		except com.exception('aldrin.exception.cancel'):
			pass

def show_preferences(parent, *args):
	"""
	Shows the {PreferencesDialog}.
	
	@param rootwindow: The root window which receives zzub callbacks.
	@type rootwindow: wx.Frame
	@param parent: Parent window.
	@type parent: wx.Window
	"""
	PreferencesDialog(parent, *args)

__aldrin__ = dict(
	classes = [
		CancelException,
		GeneralPanel,
		DriverPanel,
		WavetablePanel,
		ControllerPanel,
		MidiPanel,
		KeyboardPanel,
		PreferencesDialog,
	],
)

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
	import testplayer
	player = testplayer.get_player()
	window = testplayer.TestWindow()
	window.show_all()
	show_preferences(window)
	gtk.main()
