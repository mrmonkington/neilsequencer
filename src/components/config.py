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
Provides an object which eases access to the applications configuration.
"""

import gtk
import os, glob, re

from aldrin.utils import filepath, camelcase_to_unixstyle, etcpath, imagepath, iconpath, sharedpath, filenameify
import aldrin.preset as preset
import ConfigParser
import new

CONFIG_OPTIONS = dict(
	# insert all sections at this level, in the format
	#
	# <section name> = <descriptor dict>
	Global = dict(
		# insert options at this level, in the format
		#
		# <option name> = <descriptor dict>
		#
		# keys you can enter into the dict:
		#
		# func: name of the getter/setter/property values. if omitted, 
		#       the option name will be used in unix notation.
		#
		# default: default value to pass for get access if the value is not
		#          yet stored in the configuration. if omitted,
		#          a default will be deduced from the value type.
		#
		# vtype: value type as type object. if omitted, the type will be deduced from
		#        the default value. allowed types are int, bool, str, float.
		#
		# doc: a doc string describing the meaning of the option. name it so that
		#      it can be appended to "Returns ..." and "Sets ...". if omitted,
		#      a default will be used.
		#
		# onset: an optional function to be called before the value is written to
		#        the configuration. the function should have the signature
		#
		#        def func(value): -> value
		#
		# onget: an optional function to be called before the value is returned
		#        to the calling function.
		#
		#        def func(value): -> value
		#
		# for the setting below, SamplePreviewVolume, the default funcname is sample_preview_volume,
		# and so you can access config.get_sample_preview_volume([default]), config.set_sample_preview_volume(value),
		# and config.sample_preview_volume as a property.
		SamplePreviewVolume = dict(default=-12.0,doc="the volume with which samples shall be previewed."),
		Theme = dict(func='active_theme',default=None,vtype=str,onget=lambda v:v or None,doc="the name of the currently active theme."),
		KeymapLanguage = dict(default='en',onset=lambda s:s.lower(),onget=lambda s:s.lower(),doc="the current keymap language."),
		AudioEditorCommand = dict(func='audioeditor_command',default='audacity',doc="the audio editor command."),
		IncrementalSaving = dict(default=True,doc="the incremental saving option."),
		PatternFontName = dict(func='pattern_font',default='Monospace Bold 8',doc="the font used in the pattern editor."),
		LedDraw = dict(default=True,doc="the led draw option."),
		PatNoteOff = dict(func='pattern_noteoff',default=False,doc="pattern noteoff option."),
	),
	PluginListBrowser = dict(
		SearchTerm = dict(func='pluginlistbrowser_search_term',default='',vtype=str,doc="the current plugin search mask."),
		ShowGenerators = dict(func='pluginlistbrowser_show_generators',default='true',vtype=bool,doc="Show generators."),
		ShowEffects = dict(func='pluginlistbrowser_show_effects',default='true',vtype=bool,doc="Show effects."),
		ShowControllers = dict(func='pluginlistbrowser_show_controllers',default='true',vtype=bool,doc="Show controllers."),
		ShowNonNative = dict(func='pluginlistbrowser_show_nonnative',default='false',vtype=bool,doc="Show non-native plugins."),
	),
	WavetablePaths = dict(
		Path = dict(list=True,func='wavetable_paths',vtype=str,doc="the list of wavetable paths."),
	),
	Extensions = dict(
		URI = dict(list=True,func='enabled_extensions',vtype=str,doc="the current list of enabled extension uris."),
	),
	Packages = dict(
		package = dict(list=True,func='packages',vtype=str,doc="the list of python packages to be loaded."),
	),
	Debug = dict(
		Commands = dict(list=True,func='debug_commands',vtype=str,doc="the current list of debug commands for the python console."),
	)
)

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
	'MV Arrow': 0x8ae234,
	'MV Arrow Border In': 0xafeb73,
	'MV Arrow Border Out': 0x4e9a06,
	'MV Controller Arrow': 0xef2929,
	'MV Controller Arrow Border In': 0xf35d5d,
	'MV Controller Arrow Border Out': 0xa40000,
	'MV Background': 0xeeeeec,
	'MV Effect': 0x729fcf,
	'MV Effect LED Border': 0x96b8dc,
	'MV Effect LED Off': 0x204a87,
	'MV Effect LED On': 0x729fcf,
	'MV Effect Mute': 0x3465a4,
	'MV Effect Pan BG': 0x7f9aeb,
	'MV Effect Border In': 0x96b8dc,
	'MV Effect Border Out': 0x204a87,
	'MV Effect Border Selected': 0x204a87,
	'MV Effect LED Warning': 0xff0000,
	'MV Effect Text': 0x000000,
	'MV Generator': 0xfcaf3e,
	'MV Generator LED Border': 0xfdbf64,
	'MV Generator LED Off': 0xce5c00,
	'MV Generator LED On': 0xfcaf3e,
	'MV Generator Mute': 0xf57900,
	'MV Generator Pan BG': 0xffdd73,
	'MV Generator Border In': 0xfdbf64,
	'MV Generator Border Out': 0xce5c00,
	'MV Generator Border Selected': 0xce5c00,
	'MV Generator LED Warning': 0xff0000,
	'MV Generator Text': 0x000000,
	'MV Controller': 0xef2929,
	'MV Controller LED Border': 0xf35d5d,
	'MV Controller LED Off': 0xa40000,
	'MV Controller LED On': 0xef2929,
	'MV Controller Mute': 0xcc0000,
	'MV Controller Pan BG': 0xffdd73,
	'MV Controller Border In': 0xf35d5d,
	'MV Controller Border Out': 0xa40000,
	'MV Controller Border Selected': 0xa40000,
	'MV Controller LED Warning': 0xff0000,
	'MV Controller Text': 0x000000,
	'MV Line': 0x555753,
	'MV Master': 0xbabdb6,
	'MV Master Mute': 0xbabdb6,
	'MV Master LED Border': 0xd3d7cf,
	'MV Master LED Off': 0x555753,
	'MV Master LED On': 0xbabdb6,
	'MV Master Border In': 0xd3d7cf,
	'MV Master Border Out': 0x555753,
	'MV Master Border Selected': 0x555753,
	'MV Master LED Warning': 0xff0000,
	'MV Master Text': 0x000000,
	'MV CPU LED Border': 0x000000,
	'MV CPU LED Off': 0x001433,
	'MV CPU LED On': 0x0066ff,
	'MV CPU LED Warning': 0xff8000,
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
	'WE BG': 0xffffff,
	'WE Line': 0x6060c0,
	'WE Fill': 0xe0e0ff,
	'WE Peak Fill': 0x6060c0,
	'WE Grid': 0x008000,
	'WE Selection': 0xff0000,
	'WE Stretch Cue': 0x00ff00,
	'WE Split Bar': 0x800080,
	'WE Slice Bar': 0xFF0080,
	'WE Wakeup Peaks': 0xff0080,
	'WE Sleep Peaks': 0x8000ff,
}

class AldrinConfig(object, ConfigParser.ConfigParser):
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

    def getter(self, section, option, vtype, onget, defvalue):
	self.set_section(section)
	value = self.read_value(option, defvalue)
	if vtype == bool:
	    if value == 'true':
		value = True
	    elif value == 'false':
		value = False
	if onget:
	    value = onget(value)
	return value

    def setter(self, section, option, vtype, onset, value):
	if onset:
	    value = onset(value)
	assert type(value) == vtype		
	if vtype == bool:
	    if value:
		value = 'true'
	    else:
		value = 'false'
	self.set_section(section)
	self.write_value(option, str(value))
	self.flush()

    def listgetter(self, section, option, vtype, onget):
	self.set_section(section)
	values = []
	for i,(name,value) in enumerate(self.get_values()):
	    if name.lower().startswith(option.lower()):
		if vtype == bool:
		    if value == 'true':
			value = True
		    elif value == 'false':
			value = False
		if onget:
		    value = onget(value)
		values.append(value)
	return values

    def listsetter(self, section, option, vtype, onset, values):
	self.delete_section(section)
	self.set_section(section)
	for i,value in enumerate(values):
	    if onset:
		value = onset(value)
	    assert type(value) == vtype		
	    if vtype == bool:
		if value:
		    value = 'true'
		else:
		    value = 'false'
	    self.write_value('%s%04i' % (option,i), value)
	self.flush()

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

    def get_plugin_icon_path(self, name):
	"""
	Returns the plugin icon path for a specific icon name.
	"""
	path = os.path.join(iconpath(name + '.svg'))
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

    def select_theme(self, name):
	"""
	Selects a color theme with a specific name to be used.

	@param name: name of theme as returned by get_theme_names.
	@type name: str
	"""
	self.current_theme = dict(DEFAULT_THEME)
	if not name:
	    self.active_theme = ''
	    return
	re_theme_attrib = re.compile('^([\w\s]+\w)\s+(\w+)$')
	for line in file(sharedpath('themes/'+name+'.col'),'r'):
	    line = line.strip()
	    if line and not line.startswith('#'):
		m = re_theme_attrib.match(line)
		assert m, "invalid line for theme %s: %s" % (name,line)
		key = m.group(1)
		value = int(m.group(2),16)
		if key in self.current_theme.keys():
		    self.current_theme[key] = value
		else:
		    print "no such key: %s" % key
	self.active_theme = name

    def get_float_color(self, name):
	"""
	Returns a certain theme color as a float (r,g,b) tuple
	"""
	color = self.current_theme[name]
	r = ((color >> 16) & 0xff) / 255.0
	g = ((color >> 8) & 0xff) / 255.0
	b = (color & 0xff) / 255.0
	return r, g, b

    def get_color16(self, name):
	"""
	Returns a certain theme color as a 16-bit (r,g,b) tuple
	"""
	color = self.current_theme[name]
	r = ((color >> 16) & 0xff) * 257
	g = ((color >> 8) & 0xff)  * 257
	b = (color & 0xff) * 257
	return r, g, b

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
	return [os.path.splitext(os.path.basename(filename))[0] for filename in glob.glob(sharedpath('themes') + '/*.col')]

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

    def get_experimental(self, key, defval=0):
	"""
	Returns whether an experimental feature is enabled.
	"""
	self.set_section('ExperimentalFeatures')
	return int(self.read_value(key, str(defval))) == 1

    def set_experimental(self, key, val):
	"""
	Stores whether an experimental feature is enabled.
	"""
	self.set_section('ExperimentalFeatures')
	self.write_value(key, str(int(val)))

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
	indexpath = etcpath('index.xml')
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
	for i,(name,value) in enumerate(self.get_values()):
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
	@rtype: (str, str, int, int)
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
	for i,(name,value) in enumerate(self.get_values()):
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

    def set_plugin_presets(self, pluginloader, presets):
	"""
	Stores a preset collection for the given pluginloader.

	@param pluginloader: A pluginloader.
	@type pluginloader: zzub.Pluginloader
	@param presets: A preset collection
	@type presets: preset.PresetCollection
	"""
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
	uri = filenameify(pluginloader.get_uri())
	name = filenameify(pluginloader.get_name())
	#presetpath = os.path.join(sharedpath('presets'))
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

    def load_window_pos(self, windowid, window):
	"""
	Retrieves a windows position from the config and applies it.

	@param windowid: Name of window in config.
	@type windowid: str
	@param window: The window whose properties to save.
	@type window: wx.Window
	"""
	self.set_section('Layout/' + windowid)
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
	    window.move(x, y)
	    window.resize(w, h)
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


def generate_config_method(section, option, kwargs):
    funcname = kwargs.get('func', camelcase_to_unixstyle(option))
    doc = kwargs.get('doc', '')
    if not doc:
	doc = 'section %s, option %s, default %s, type %s.' % (section, option, default, vtype)

    onset = kwargs.get('onset', None)
    onget = kwargs.get('onget', None)

    if kwargs.get('list', False):
	vtype = kwargs['vtype']
	getter = lambda self: self.listgetter(section,option,vtype,onget)
	setter = lambda self,value: self.listsetter(section,option,vtype,onset,value)
    else:
	if 'default' in kwargs:
	    default = kwargs['default']
	    vtype = kwargs.get('vtype', type(default))
	else:
	    vtype = kwargs['vtype']
	    default = {float: 0.0, int:0, long:0, str:'', unicode:u'', bool:False}[vtype]
	getter = lambda self,defvalue=kwargs.get(default,False): self.getter(section,option,vtype,onget,default)
	setter = lambda self,value: self.setter(section,option,vtype,onset,value)

    getter.__name__ = 'get_' + funcname
    getter.__doc__ = 'Returns ' + doc
    setattr(AldrinConfig, 'get_' + funcname, getter)

    setter.__name__ = 'set_' + funcname
    setter.__doc__ = 'Sets ' + doc
    setattr(AldrinConfig, 'set_' + funcname, setter)

    # add a property
    prop = property(getter, setter, doc=doc)
    setattr(AldrinConfig, funcname, prop)

def generate_config_methods():
    # build getters and setters based on the options map
    for section,options in CONFIG_OPTIONS.iteritems():
	for option,kwargs in options.iteritems():
	    generate_config_method(section, option, kwargs)

generate_config_methods()

class AldrinConfigSingleton(AldrinConfig):
    __aldrin__ = dict(
	    id = 'aldrin.core.config',
	    singleton = True,
    )

def get_config(*args):
    """
    Returns the global object singleton.

    @rtype: {AldrinConfig}.
    """
    import aldrin.com
    return aldrin.com.get(AldrinConfigSingleton.__aldrin__['id'])

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
'get_config',
'get_plugin_aliases',
]

__aldrin__ = dict(
	classes = [
		AldrinConfigSingleton,
	],
)

if __name__ == '__main__':
    cfg = get_config()
    print cfg.get_enabled_extensions()
    print cfg.get_plugin_icon_path("matilde")
    print cfg.packages
    cfg.set_sample_preview_volume(-6.0)
    print "prop1:",cfg.sample_preview_volume
    cfg.sample_preview_volume = -9.0
    print cfg.active_theme
    print "prop2:",cfg.sample_preview_volume
    print "volume:",cfg.get_sample_preview_volume()
    cfg.set_sample_preview_volume(-12.0)
    print cfg.get_audiodriver_config()
    print "DEFAULT_THEME = {"
    for k in sorted(DEFAULT_THEME.keys()):
	v = DEFAULT_THEME[k]
	print '\t%r: 0x%06x,' % (k,int(cfg.get_color(k).replace('#',''),16))
    print "}"
