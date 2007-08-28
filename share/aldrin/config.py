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
Provides an object which eases access to the applications configuration.
"""

from gtkimport import gtk
import os, glob, re

from utils import filepath
import ConfigParser

# the key of this dictionary is the language code associated with the keyboard. the 
# value is a series of keyboard characters associated with each note, in the order 
# C, C#, D, E# and so on. two octaves are listed, separated by a vertical slash ("|").
#
# to make that configuration selectable, see share/aldrin/preferences.py.
# there is a dialog class named "KeyboardPanel", containing another KEYMAPS list, 
# associating the dict codes with readable names.
KEYMAPS = {
	'en' : 'ZSXDCVGBHNJM,|Q2W3ER5T6Y7UI9O0P',
	'de' : 'YSXDCVGBHNJM,|Q2W3ER5T6Z7UI9O0P',
	'dv' : ';OQEJKIXDBHMW|\'2,3.P5Y6F7GC9R0L',
	'fr' : 'WSXDCVGBHNJ,;|AéZ"ER(T-YèUIçOàP',
}

DEFAULT_THEME = {
	'MV Amp BG': 0xffffff,
	'MV Amp Border': 0x000000,
	'MV Amp Handle': 0x999999,
	'MV Arrow': 0x65d1b6,
	'MV Background': 0xeeeeee,
	'MV Effect': 0x617ccd,
	'MV Effect LED Border': 0x000000,
	'MV Effect LED Off': 0x435eaf,
	'MV Effect LED On': 0x377aff,
	'MV Effect Mute': 0x254091,
	'MV Effect Pan BG': 0x7f9aeb,
	'MV Generator': 0xf2bf55,
	'MV Generator LED Border': 0x000000,
	'MV Generator LED Off': 0xc3a137,
	'MV Generator LED On': 0xffff19,
	'MV Generator Mute': 0xb68319,
	'MV Generator Pan BG': 0xffdd73,
	'MV Line': 0x000000,
	'MV Machine Border': 0x000000,
	'MV Machine LED Warning': 0xff0000,
	'MV Machine Text': 0x000000,
	'MV Master': 0xcccccc,
	'MV Master LED Border': 0x000000,
	'MV Master LED Off': 0x666666,
	'MV Master LED On': 0xeeeeee,
	'MV Pan Handle': 0x000000,
	'PE BG': 0xeeeeee,
	'PE BG Dark': 0xcccccc,
	'PE BG Very Dark': 0x7f9aeb,
	'PE Sel BG': 0xaaaaaa,
	'PE Text': 0x303021,
	'SA Amp BG': 0x007d5d,
	'SA Amp Line': 0xffffff,
	'SA Freq BG': 0x007d5d,
	'SA Freq Line': 0xffffff,
	'SE BG': 0xeeeeee,
	'SE BG Dark': 0xcccccc,
	'SE BG Very Dark': 0x7f9aeb,
	'SE Line': 0x000000,
	'SE Loop Line' : 0x0000ff,
	'SE Sel BG': 0xaaaaaa,
	'SE Mute': 0xff0000,
	'SE Break': 0xff8000,
	'SE Text': 0x303021,
	'EE Line': 0x6060c0,
	'EE Fill': 0xe0e0ff,
	'EE Dot': 0x6060c0,
	'EE Sustain': 0xff6040,
	'EE Dot Selected': 0x000000,
	'EE BG': 0xffffff,
	'EE Grid': 0xe0e0e0,
}

class AldrinConfig(ConfigParser.ConfigParser):
	"""
	Streamlines access to the applications configuration. You should
	set all applications to and retrieve them from the config object.
	
	Do not instantiate this class, use {get_config} instead.
	
	On Windows, most settings will be saved in ~/aldrin/settings.cfg
	
	On Linux, most settings will be saved in ~/.aldrin/settings.cfg
	"""
	def __init__(self):
		"""
		Initializer.
		"""		
		ConfigParser.ConfigParser.__init__(self)
		self.filename = os.path.join(self.get_settings_folder(),'settings.cfg')
		self.read([self.filename])
		self._section = ''
		try:
			self.select_theme(self.get_active_theme())
		except:
			import traceback
			traceback.print_exc()
			self.select_theme(None)
			
	def set_section(self, section):
		self._section = section
		
	def delete_section(self, section):
		if not self.has_section(section):
			return
		self.remove_section(section)
		
	def get_values(self):
		assert self._section
		if not self.has_section(self._section):
			return []
		return sorted(self.items(self._section), lambda a,b: cmp(a[0],b[0]))
		
	def read_int_value(self, name, default=0):
		assert self._section
		if not self.has_section(self._section):
			return default
		if not self.has_option(self._section, name):
			return default
		return self.getint(self._section, name)
		
	def read_value(self, name, default=''):
		assert self._section
		if not self.has_section(self._section):
			return default
		if not self.has_option(self._section, name):
			return default
		return self.get(self._section, name)
		
	def write_value(self, name, value):
		assert self._section
		if not self.has_section(self._section):
			self.add_section(self._section)
		self.set(self._section,name,value)
		self.flush()
		
	def write_int_value(self, name, value):
		assert self._section
		if not self.has_section(self._section):
			self.add_section(self._section)
		self.set(self._section,name,str(value))
		self.flush()
		
	def flush(self):
		self.write(file(self.filename,'w'))
			
	def get_sample_preview_volume(self):
		"""
		Returns the volume with which samples shall be previewed.
		"""
		self.set_section('Global')
		vol = float(self.read_value('SamplePreviewVolume', '-12.0'))
		return vol
		
	def get_plugin_icon_path(self, name):
		"""
		Returns the plugin icon path for a specific icon name.
		"""
		path = os.path.join(filepath('icons'), name + '.svg')
		if not os.path.isfile(path):
			return ""
		return path
		
	def get_freesound_samples_folder(self):
		"""
		Returns the samples folder designated for samples downloaded from freesound.
		"""
		path = os.path.join(self.get_settings_folder(), "samples", "freesound")
		if not os.path.isdir(path):
			os.makedirs(path)
		return path
			
	def get_settings_folder(self):
		"""
		Returns the users settings folder.
		"""
		if os.name == 'nt':
			settingsfolder = os.path.expanduser('~/aldrin')
		elif os.name == 'posix':
			settingsfolder = os.path.expanduser('~/.aldrin')
		if not os.path.isdir(settingsfolder):
			os.makedirs(settingsfolder)
		return settingsfolder
		
	def get_active_theme(self):
		"""
		Returns the name of the currently active theme.
		
		@return: name of theme or None, if default
		@rtype: str or None
		"""
		self.set_section('Global')
		name = self.read_value('Theme', '')
		if not name:
			return None
		return name
		
	def select_theme(self, name):
		"""
		Selects a color theme with a specific name to be used.
		
		@param name: name of theme as returned by get_theme_names.
		@type name: str
		"""
		self.current_theme = dict(DEFAULT_THEME)		
		if not name:
			self.set_section('Global')
			self.write_value('Theme', '')
			return
		re_theme_attrib = re.compile('^([\w\s]+\w)\s+(\w+)$')
		for line in file(filepath('themes/'+name+'.col'),'r'):
			line = line.strip()
			if line and not line.startswith('#'):
				m = re_theme_attrib.match(line)
				assert m, "invalid line for theme %s: %s" % (name,line)
				key = m.group(1)
				value = int(m.group(2),16)
				assert key in self.current_theme.keys(), "no such key: %s" % key
				self.current_theme[key] = value
		self.set_section('Global')
		self.write_value('Theme', name)
			
	def get_brush(self, name):
		"""
		Returns a certain theme color as brush
		
		@param name: name of color theme key.
		@type name: str
		@return: brush
		@rtype: wx.Brush
		"""
		return wx.Brush(self.get_color(name),wx.SOLID)
			
	def get_pen(self, name):
		"""
		Returns a certain theme color as brush
		
		@param name: name of color theme key.
		@type name: str
		@return: brush
		@rtype: wx.Brush
		"""
		return wx.Pen(self.get_color(name), 1, wx.SOLID)

	def get_float_color(self, name):
		"""
		Returns a certain theme color as a float (r,g,b) tuple
		"""
		color = self.current_theme[name]
		r = ((color>>16) & 0xff) / 255.0
		g = ((color>>8) & 0xff) / 255.0
		b = (color & 0xff) / 255.0
		return r,g,b
		
	def get_color16(self, name):
		"""
		Returns a certain theme color as a 16-bit (r,g,b) tuple
		"""
		color = self.current_theme[name]
		r = ((color>>16) & 0xff) * 257
		g = ((color>>8) & 0xff)  * 257
		b = (color & 0xff) * 257
		return r,g,b

	def get_color(self, name):
		"""
		Returns a certain theme color as html hex color string.
		
		@param name: name of color theme key.
		@type name: str
		@return: color of key.
		@rtype: str
		"""
		return "#%06x" % self.current_theme[name]
		
	def get_theme_names(self):
		"""
		Returns a list of color theme names
		
		@return: A list of color theme names.
		@rtype: [str,...]
		"""
		return [os.path.splitext(os.path.basename(filename))[0] for filename in glob.glob(filepath('themes') + '/*.col')]
		
	def get_default_int(self, key, defval=0):
		"""
		Returns the default value for a UI setting.
		"""
		self.set_section('Defaults')
		return int(self.read_value(key, str(defval)))
		
	def set_default_int(self, key, val):
		"""
		Stores a default value for an UI setting.
		"""
		self.set_section('Defaults')
		self.write_value(key, str(val))

	def get_default(self, key, s=''):
		"""
		Returns the default value for a UI setting.
		"""
		self.set_section('Defaults')
		return self.read_value(key, s)
		
	def set_default(self, key, val):
		"""
		Stores a default value for an UI setting.
		"""
		self.set_section('Defaults')
		self.write_value(key, val)

	def set_keymap_language(self, lang):
		"""
		Sets the current keymap language
		
		@param lang: Language ('en', 'de')
		@type lang: str
		"""
		self.set_section('Global')
		self.write_value('KeymapLanguage', lang.lower())
		self.flush()
		
	def get_keymap_language(self):
		"""
		Returns the current keymap language
		"""
		self.set_section('Global')
		return self.read_value('KeymapLanguage', 'en').lower()
		
	def get_keymap(self):
		"""
		returns a keymap for the pattern editor, to be used
		for note input.
		"""
		return KEYMAPS.get(self.get_keymap_language(), KEYMAPS['en'])
		
	def get_credentials(self, service):
		"""
		returns the credentials required for a service (username/password).
		"""
		self.set_section('Credentials/'+service)
		return self.read_value('Username'), self.read_value('Password')
		
	def set_credentials(self, service, username, password):
		"""
		stores the credentials required for a service (username/password).
		"""
		self.set_section('Credentials/'+service)
		self.write_value('Username',username)
		self.write_value('Password',password)
		
	def get_index_path(self):
		"""
		yields a a tree of plugins, to be used
		in the machine view for menus. 
		
		On Posix platforms, ~/.aldrin/index.txt overrides ${PREFIX}/share/aldrin/index.txt.
		
		On Windows, <user folder>/aldrin/index.txt overrides <app folder>/share/aldrin/index.txt.
		
		@return: Path to the index file.
		@rtype: str
		"""
		indexpath = filepath('index.xml')
		userindexpath = os.path.join(self.get_settings_folder(),'index.xml')
		if userindexpath and os.path.isfile(userindexpath):
			return userindexpath
		else:
			return indexpath
			
	def get_mididriver_inputs(self):
		"""
		Returns the current list of MIDI input driver names.
		
		@return: List of driver names.
		@rtype: [str,...]
		"""
		self.set_section('MIDI/Inputs')
		inputlist = []
		for i,(name,value) in enumerate(config.get_values()):
			inputlist.append(value)
		return inputlist
		
	def set_mididriver_inputs(self, inputlist):
		"""
		Stores the current list of used MIDI input driver names.
		
		@param name: List of driver names.
		@type name: [str,...]
		"""
		self.delete_section('MIDI/Inputs')
		self.set_section('MIDI/Inputs')
		for i in range(len(inputlist)):
			self.write_value('Name%i' % i, inputlist[i])
		self.flush()
		
	def get_audiodriver_config(self):
		"""
		Retrieves current audiodriver settings.
		
		@return: A tuple containing input driver name, output driver name, samplerate and buffer size.
		@rtype: (str,str,int,int)
		"""
		self.set_section('AudioDevice')
		return self.read_value('InputName',''), self.read_value('OutputName','') or self.read_value('Name',''), self.read_int_value('SampleRate',44100), self.read_int_value('BufferSize',2048)
		
	def set_audiodriver_config(self, inputname, outputname, samplerate, buffersize):
		"""
		Stores audiodriver settings.
		
		@param name: The newly selected driver name.
		@type name: str
		@param samplerate: Selected samples per seconds.
		@type samplerate: int
		@param buffersize: Buffer size in samples.
		@type buffersize: int
		"""
		self.set_section('AudioDevice')
		self.write_value('InputName', inputname)
		self.write_value('OutputName', outputname)
		self.write_int_value('SampleRate',samplerate)
		self.write_int_value('BufferSize',buffersize)
		self.flush()
		
	def get_mididriver_outputs(self):
		"""
		Returns the current list of MIDI output driver names.
		
		@return: List of driver names.
		@rtype: [str,...]
		"""
		self.set_section('MIDI/Outputs')
		outputlist = []
		for i,(name,value) in enumerate(config.get_values()):
			outputlist.append(value)
		return outputlist
		
	def set_mididriver_outputs(self, outputlist):
		"""
		Stores the current list of used MIDI output driver names.
		
		@param name: List of driver names.
		@type name: [str,...]
		"""
		self.delete_section('MIDI/Outputs')
		self.set_section('MIDI/Outputs')
		for i in range(len(outputlist)):
			self.write_value('Name%i' % i, outputlist[i])
		self.flush()
		
	def get_enabled_extensions(self):
		"""
		Returns the current list of enabled extension uris.
		
		@return: List of uris.
		@rtype: [str,...]
		"""
		self.set_section('Extensions')
		uris = []
		for i,(name,value) in enumerate(config.get_values()):
			uris.append(value)
		return uris
		
	def set_enabled_extensions(self, uris):
		"""
		Stores the current list of enabled extension uris.
		
		@param uris: List of uris
		@type uris: [str,...]
		"""
		self.delete_section('Extensions')
		self.set_section('Extensions')
		for i in range(len(uris)):
			self.write_value('URI%i' % i, uris[i])
		self.flush()
		
		
	def set_wavetable_paths(self, pathlist):
		"""
		Sets the list of wavetable paths.
		
		@param pathlist: List of paths to directories containing samples.
		@type pathlist: [str,...]
		"""
		self.delete_section('WavetablePaths')
		self.set_section('WavetablePaths')
		for i in range(len(pathlist)):
			self.write_value('Path%i' % i, pathlist[i])
		self.flush()
		
	def set_plugin_presets(self, pluginloader, presets):
		"""
		Stores a preset collection for the given pluginloader.
		
		@param pluginloader: A pluginloader.
		@type pluginloader: zzub.Pluginloader
		@param presets: A preset collection
		@type presets: preset.PresetCollection
		"""
		from utils import filenameify
		uri = filenameify(pluginloader.get_uri())
		name = filenameify(pluginloader.get_name())
		presetpath = os.path.join(self.get_settings_folder(),'presets')
		if not os.path.isdir(presetpath):
			os.makedirs(presetpath)
		filename = os.path.join(presetpath,name + '.prs')
		presets.save(filename)
		
	def get_plugin_presets(self, pluginloader):
		"""
		Returns a PresetCollection for the given pluginloader.
		
		@param pluginloader: A pluginloader.
		@type pluginloader: zzub.Pluginloader
		@return: A preset collection.
		@rtype: preset.PresetCollection
		"""
		import preset
		from utils import filenameify
		uri = filenameify(pluginloader.get_uri())
		name = filenameify(pluginloader.get_name())
		presetpath = os.path.join(self.get_settings_folder(),'presets')
		presetfilepaths = [
			os.path.join(presetpath, uri + '.prs'),
			os.path.join(presetpath, name + '.prs'),
			filepath('presets/' + uri + '.prs'),
			filepath('presets/' + name + '.prs'),
		]
		presets = preset.PresetCollection()
		for path in presetfilepaths:
			print "searching preset '%s'..." % path
			if os.path.isfile(path):
				try:
					presets = preset.PresetCollection(path)
					break
				except:
					import traceback
					print traceback.format_exc()
		return presets
		
	def save_window_pos(self, windowid, window):
		"""
		Stores a windows position to the config.
		
		@param windowid: Name of window in config.
		@type windowid: str
		@param window: The window whose properties to save.
		@type window: wx.Window
		"""
		self.delete_section('Layout/'+windowid)
		self.set_section('Layout/'+windowid)
		if isinstance(window, gtk.Window):
			x,y = window.get_position()
			w,h = window.get_size()
			self.write_value("X", str(x))
			self.write_value("Y", str(y))
			self.write_value("W", str(w))
			self.write_value("H", str(h))
			if hasattr(window, 'IsMaximized'):
				if window.IsMaximized():
					maximize = 'true'
				else:
					maximize = 'false'
				self.write_value("Maximize", maximize)
		elif isinstance(window, gtk.Paned):
			self.write_value("SashPosition", str(window.get_position()))
		else:
			if window.window and window.get_property('visible'):
				visible = 'true'
			else:
				visible = 'false'
			self.write_value("Visible", visible)
		self.flush()
		
	def get_incremental_saving(self):
		"""
		Retrieves the incremental saving option.
		"""
		self.set_section('Global')
		value = self.read_value('IncrementalSaving', 'true')
		if value == 'true':
			return True
		else:
			return False
		
	def set_incremental_saving(self, value):
		"""
		Stores the incremental saving option.
		"""
		self.set_section('Global')
		if value:
			self.write_value('IncrementalSaving', 'true')
		else:
			self.write_value('IncrementalSaving', 'false')
		
	def load_window_pos(self, windowid, window):
		"""
		Retrieves a windows position from the config and applies it.
		
		@param windowid: Name of window in config.
		@type windowid: str
		@param window: The window whose properties to save.
		@type window: wx.Window
		"""
		self.set_section('Layout/'+windowid)
		if isinstance(window, gtk.Window):
			try:
				x = int(self.read_value('X'))
				y = int(self.read_value('Y'))
				w = int(self.read_value('W'))
				h = int(self.read_value('H'))
			except TypeError:
				return
			except ValueError:
				return
			window.move(x,y)
			window.resize(w,h)
			#~ if hasattr(window, 'IsMaximized'):
				#~ if self.read_value("Maximize") == 'true':
					#~ window.Maximize()
		elif isinstance(window, gtk.Paned):
			try:
				window.set_position(int(self.read_value("SashPosition")))
			except TypeError:
				pass
			except ValueError:
				pass
		else:
			visible = self.read_value("Visible")
			if visible:
				if visible == 'true':
					window.show()
				else:
					window.hide()
		
	def get_wavetable_paths(self):
		"""
		Returns the list of wavetable paths.
		@return: List of paths to directories containing samples.
		@rtype:[str,...]
		"""
		self.set_section('WavetablePaths')
		pathlist = []
		for i,(name,value) in enumerate(config.get_values()):
			pathlist.append(value)
		return pathlist
		
	def set_midi_controllers(self, ctrllist):
		"""
		Sets the list of mapped midi controllers.
		
		@param ctrllist: List of tuples containing name,channel and controller id
		@type ctrllist: [(str,int,int),...]
		"""
		self.delete_section('MIDIControllers')
		self.set_section('MIDIControllers')
		for i in range(len(ctrllist)):
			name,channel,ctrlid = ctrllist[i]
			self.write_value('Controller%i' % i, "%s|%i|%i" % (name.replace('|',''),channel,ctrlid))
		self.flush()
		
	def get_midi_controllers(self):
		"""
		Returns the list of mapped midi controllers.
		
		@return: List of tuples containing name, channel and controller id
		@rtype: [(str,int,int),...]
		"""
		self.set_section('MIDIControllers')
		ctrllist = []
		for i,(name,value) in enumerate(self.get_values()):
			name,channel,ctrlid = value.split('|')
			ctrllist.append((name,int(channel),int(ctrlid)))
		return ctrllist
	
	def get_recent_files_config(self):
		"""
		Retrieves list of recently used files.
		
		@return: List of file paths.
		@rtype: str list
		"""
		recent_files = []		
		self.set_section('RecentFiles')
		for i,(name,value) in enumerate(self.get_values()):
			if os.path.isfile(value):
				recent_files.append(value)
		return recent_files
		
	def add_recent_file_config(self, filename):
		"""
		Adds a filename to the list of recently used files, if not
		already included.
		
		@param filename: path to file.
		@type filename: str
		"""
		recent_files = self.get_recent_files_config()
		try:
			recent_files.remove(filename)
		except:
			pass
		while len(recent_files) >= 10:
			recent_files.pop()
		recent_files = [filename] + recent_files
		self.delete_section('RecentFiles')
		self.set_section('RecentFiles')
		for i in range(len(recent_files)):
			self.write_value("File%i" % i, recent_files[i])
		self.flush()
		
config = None


def get_config():
	"""
	Returns the global object singleton.
	
	@rtype: {AldrinConfig}.
	"""
	global config
	if not config:
		config = AldrinConfig()
	return config

def get_plugin_blacklist():
	"""
	Yields a list of blacklisted plugins.
	
	@return: A list of plugin uris.
	@rtype: [str,...]
	"""
	for line in file(filepath('blacklist.txt'), 'r'):
		line = line.strip()
		if line.startswith('#'):
			pass
		elif line:
			yield line

def get_plugin_aliases():
	"""
	Yields a list of aliases.
	
	@return: A list of alias tuples (name,uri)
	@rtype: [(str,str),...]
	"""
	for line in file(filepath('aliases.txt'), 'r'):
		line = line.strip()
		if line.startswith('#'):
			pass
		elif line:
			sep = line.index('=')
			yield line[:sep].strip(), line[sep+1:].strip()

__all__ = [
'AldrinConfig',
'get_config',
'get_plugin_aliases',
]

if __name__ == '__main__':
	cfg = get_config()
	print cfg.get_plugin_icon_path("matilde")
	print cfg.get_sample_preview_volume()
	print cfg.get_midi_controllers()
	print cfg.get_audiodriver_config()
	print "DEFAULT_THEME = {"
	for k in sorted(DEFAULT_THEME.keys()):
		v = DEFAULT_THEME[k]
		print '\t%r: 0x%06x,' % (k,int(cfg.get_color(k).replace('#',''),16))
	print "}"
