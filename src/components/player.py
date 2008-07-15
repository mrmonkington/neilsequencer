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

import zzub
from zzub import Player
import aldrin.com as com

import aldrin.common as common
import gobject
import os,sys 
import time
from aldrin.utils import is_generator, is_effect, is_root, is_controller, is_streamer
from config import get_plugin_aliases, get_plugin_blacklist

import gtk

DOCUMENT_UI = dict(
	# insert persistent members at this level, in the format
	#
	# <member name> = <descriptor dict>
	#
	# <member name> is the base name of the getter/setter/property values
	# and the private member variable.
	#
	# keys you can enter into the dict:
	#
	# default: initialization value. if omitted, a default will be deduced
	# from the value type.
	#
	# vtype: value type as type object. if omitted, the type will be deduced from
	#        the default value. allowed types are int, bool, str, float.
	#
	# doc: a doc string describing the meaning of the setting. name it so that
	#      it can be appended to "Returns ..." and "Sets ...". if omitted,
	#      a default will be used.
	# 
	# event: name of the global event to be triggered. usually generated
	#        by default if not given.
	#
	# onset: an optional function to be called before the value is assigned. 
	# 	     the function should have the signature
	#
	#        def func(value): -> value
	#
	# onget: an optional function to be called before the value is returned
	#        to the calling function.
	#
	#        def func(value): -> value
	#
	# for the setting below, active_plugins, you can access player.get_active_plugins(),
	# player.set_active_plugins(plugins), and player.active_plugins as a property.
	# when changed, the event active_plugins_changed will be triggered.
	active_plugins = dict(vtype=zzub.Plugin,list=True,doc="the list of active plugins."),
	active_patterns = dict(vtype=(zzub.Plugin,int),list=True,doc="the list of active patterns (zzub.Plugin, index)."),
	active_waves = dict(vtype=zzub.Wave,list=True,doc="the list of active waves."),
	octave = dict(vtype=int,default=4,doc="the current octave to be used for keyjazz."),
	autoconnect_target = dict(vtype=zzub.Plugin,doc="the plugin to connect to automatically when creating a new plugin."),
	sequence_step = dict(vtype=int,default=64,doc="the current step size for sequencers."),
	plugin_origin = dict(vtype=int,list=True,default=[0.0,0.0],doc="the origin position for new plugins."),
	solo_plugin = dict(vtype=zzub.Plugin,doc="if set, the plugin that is currently set to solo."),
	document_path = dict(vtype=str,doc="path to the current document."),
)

class AldrinPlayer(Player):
	__aldrin__ = dict(
		id = 'aldrin.core.player',
		singleton = True,
		categories = [
			'pythonconsole.locals',
		],
	)
	
	_exclude_event_debug_ = [
		zzub.zzub_event_type_parameter_changed,
	]
		
	_event_types_ = dict(
		zzub_event_type_double_click = dict(args=None),
		zzub_event_type_new_plugin = dict(args='new_plugin'),
		zzub_event_type_delete_plugin = dict(args='delete_plugin'),
		zzub_event_type_pre_delete_plugin = dict(args='delete_plugin'),
		zzub_event_type_disconnect = dict(args='disconnect_plugin'),
		zzub_event_type_connect = dict(args='connect_plugin'),
		zzub_event_type_plugin_changed = dict(args='plugin_changed'),
		zzub_event_type_parameter_changed = dict(args='change_parameter'),
		zzub_event_type_set_tracks = dict(args=None),
		zzub_event_type_set_sequence_tracks = dict(args='set_sequence_tracks'),
		zzub_event_type_set_sequence_event = dict(args='set_sequence_event'),
		zzub_event_type_new_pattern = dict(args='new_pattern'),
		zzub_event_type_delete_pattern = dict(args='delete_pattern'),
		zzub_event_type_pre_delete_pattern = dict(args='delete_pattern'),
		zzub_event_type_edit_pattern = dict(args='edit_pattern'),
		zzub_event_type_pattern_changed = dict(args='pattern_changed'),
		zzub_event_type_pattern_insert_rows = dict(args='pattern_insert_rows'),
		zzub_event_type_pattern_remove_rows = dict(args='pattern_remove_rows'),
		zzub_event_type_sequencer_add_track = dict(args=None),
		zzub_event_type_sequencer_remove_track = dict(args=None),
		zzub_event_type_sequencer_changed = dict(args=None),
		zzub_event_type_pre_disconnect = dict(args=None),
		zzub_event_type_pre_connect = dict(args=None),
		zzub_event_type_post_connect = dict(args=None),
		zzub_event_type_pre_set_tracks = dict(args=None),
		zzub_event_type_post_set_tracks = dict(args=None),
		zzub_event_type_envelope_changed = dict(args=None),
		zzub_event_type_slices_changed = dict(args=None),
		zzub_event_type_wave_changed = dict(args='change_wave'),
		zzub_event_type_delete_wave = dict(args='delete_wave'),
		zzub_event_type_load_progress = dict(args=None),
		zzub_event_type_midi_control = dict(args='midi_message'),
		zzub_event_type_wave_allocated = dict(args='allocate_wavelevel'),
		zzub_event_type_player_state_changed = dict(args='player_state_changed'),
		zzub_event_type_osc_message = dict(args='osc_message'),
		zzub_event_type_vu = dict(args='vu'),
		zzub_event_type_custom = dict(args='custom'),
		zzub_event_type_all = dict(args='all'),	
	)
	
	def __init__(self):
		Player.__init__(self, Player.create())
		
		self._cbtime = time.time()
		self._cbcalls = 0
		self._hevcalls = 0
		self._hevtime = 0
		self.__lazy_commits = False
		self.__event_stats = False
		# enumerate zzub_event_types and prepare unwrappers for the different types
		self.event_id_to_name = {}		
		for enumname,cfg in self._event_types_.iteritems():
			val = getattr(zzub, enumname)
			assert val not in self.event_id_to_name, "value %s (%s) already registered." % (val,eventname)
			eventname = 'zzub_' + enumname[len('zzub_event_type_'):]
			membername = cfg.get('args',None)
			args = []
			if membername:
				union = None
				datatype = None
				ed = zzub.EventData()
				for argname in dir(ed):
					if argname == membername:
						datatype = getattr(ed, argname).__class__
				assert datatype, "couldn't find member %s in zzub_event_data_t" % membername
				for argname,argtype in datatype._fields_:
					args.append(argname)
			self.event_id_to_name[val] = (eventname, membername, args)
			#print "'%s', # ( %s )" % (eventname, ','.join(args + ["..."]))
		config = com.get('aldrin.core.config')
		pluginpath = os.environ.get('ALDRIN_PLUGIN_PATH',None)
		if pluginpath:
			pluginpaths = pluginpath.split(os.pathsep)
		else:
			pluginpaths = []
			paths = os.environ.get('LD_LIBRARY_PATH',None) # todo or PATH on mswindows
			if paths: paths = paths.split(os.pathsep)
			else: paths = []
			paths.extend([
					'/usr/local/lib64',
					'/usr/local/lib',
					'/usr/lib64',
					'/usr/lib',
				])
			for path in [os.path.join(path, 'zzub') for path in paths]:
				if os.path.exists(path) and not path in pluginpaths: pluginpaths.append(path)
		for pluginpath in pluginpaths:
			print 'plugin path:', pluginpath
			self.add_plugin_path(pluginpath + os.sep)
		inputname, outputname, samplerate, buffersize = config.get_audiodriver_config()
		self.initialize(samplerate)
		self.init_lunar()
		self.__stream_ext_uri_mappings = {}
		self.__streamplayer = None
		self.enumerate_stream_plugins()
		self.playstarttime = time.time()		
		self.document_unchanged()
		eventbus = com.get('aldrin.core.eventbus')
		eventbus.zzub_pre_delete_plugin += self.on_pre_delete_plugin
		eventbus.zzub_pre_delete_pattern += self.on_pre_delete_pattern
		self._callback = zzub.zzub_callback_t(self.handle_event)
		self.set_callback(self._callback, None)
		gobject.timeout_add(int(1000/50), self.on_handle_events)
		
	def enumerate_stream_plugins(self):
		self.__stream_ext_uri_mappings = {}
		for pluginloader in self.get_pluginloader_list():
			if is_streamer(pluginloader):
				uri = pluginloader.get_uri()
				for j in xrange(pluginloader.get_stream_format_count()):
					ext = '.' + pluginloader.get_stream_format_ext(j)
					if ext in self.__stream_ext_uri_mappings:
						print >> sys.stderr, "Found another mapping for " + ext + "! Skipping " + uri
						continue
					self.__stream_ext_uri_mappings[ext] = uri
					
	def delete_stream_player(self):
		if not self.__streamplayer:
			return
		self.__streamplayer.destroy()
		self.__streamplayer = None
		
	def set_machine_non_song(self, plugin, enable):
		pi = common.get_plugin_infos().get(plugin)
		pi.songplugin = not enable
		
	def create_stream_player(self, uri):
		# create a stream player plugin and keep it out of the undo buffer
		assert not self.__streamplayer
		
		loader = self.get_pluginloader_by_name(uri)
		if not loader:
			print >> sys.stderr, "Can't find streamplayer plugin loader."
			return
		
		self.__streamplayer = zzub.Player.create_plugin(self, None, 0, "_PreviewPlugin", loader)
		if not self.__streamplayer:
			print >> sys.stderr, "Can't create streamplayer plugin instance."
			return
		self.get_plugin(0).add_input(self.__streamplayer, zzub.zzub_connection_type_audio)
		self.set_machine_non_song(self.__streamplayer, True)
		
	def preview_file(self, filepath):
		base,ext = os.path.splitext(filepath)
		if not ext in self.__stream_ext_uri_mappings:
			return False
		uri = self.__stream_ext_uri_mappings[ext]
		self.play_stream((4 << 4)+1, uri, filepath)
		return True
	
	def preview_wave(self, w):
		self.play_stream((4 << 4)+1, "@zzub.org/stream/wavetable;1", str(w.get_index()+1))
		
	def stop_preview(self):
		if self.__streamplayer:
			self.__streamplayer.play_midi_note(zzub.zzub_note_value_off, (4 << 4)+1, 0)
			
	def load_wave(self, wave, filepath):
		stream = zzub.Input.open_file(filepath)
		if not stream:
			return False
		res = wave.load_sample(0, 0, 0, filepath, stream)
		stream.destroy()
		if res != 0:
			self.active_waves = [wave]
		return res != 0
		
	def save_wave(self, wave, filepath):
		stream = zzub.Output.create_file(filepath)
		res = wave.save_sample(0, stream)
		stream.destroy()
		
	def play_stream(self, note, plugin_uri, data_url):
		if self.__streamplayer:
			pluginloader = self.__streamplayer.get_pluginloader()
			if pluginloader.get_uri() != plugin_uri:
				self.delete_stream_player()
		if not self.__streamplayer:
			self.create_stream_player(plugin_uri)
		if self.__streamplayer:
			self.__streamplayer.set_stream_source(data_url)
		self.flush(None,None)
		if self.__streamplayer:
			self.__streamplayer.play_midi_note(note, 0, 0)
		self.history_flush_last()
		
	def register_locals(self, locs):
		locs.update(dict(
			lazy_commits = self._enable_lazy_commits,
			event_stats = self._enable_event_stats,
		))
		
	def _enable_event_stats(self, enable=True):
		self.__event_stats = enable
		
	def _enable_lazy_commits(self, enable=True): 
		"""not to be used outside of tests."""
		self.__lazy_commits = enable
		
	def on_pre_delete_pattern(self, plugin, index):
		sel = self.active_patterns
		pair = (plugin,index)
		if pair in sel:
			sel.remove(pair)
			self.active_patterns = sel
		
	def on_pre_delete_plugin(self, plugin):
		sel = self.active_plugins
		if plugin in sel:
			sel.remove(plugin)
			self.active_plugins = sel
		sel = self.active_patterns
		for selplugin,index in sel:
			if selplugin == plugin:
				sel.remove((selplugin,index))
		self.active_patterns = sel
		
	def load_bmx(self, filename):
		res = zzub.Player.load_bmx(self, filename)
		if not res:
			self.document_path = filename
			eventbus = com.get('aldrin.core.eventbus')
			eventbus.document_loaded()
		return res
		
	def load_ccm(self, filename):
		res = zzub.Player.load_ccm(self, filename)
		if not res:
			self.document_path = filename
			eventbus = com.get('aldrin.core.eventbus')
			eventbus.document_loaded()
		return res
		
	def save_ccm(self, filename):
		res = zzub.Player.save_ccm(self, filename)
		if not res:
			self.document_path = filename
		return res
			
	def clear(self):
		zzub.Player.clear(self)
		self.document_path = ''
		
	def getter(self, membername, kwargs):
		value = getattr(self, '__' + membername)
		onget = kwargs.get('onget',None)
		if onget:
			value = onget(value)
		return value
		
	def setter(self, membername, kwargs, value):		
		onset = kwargs.get('onset',None)
		if onset:
			value = onset(value)
		setattr(self, '__' + membername, value)
		eventname = kwargs.get('event', membername + '_changed')
		eventbus = com.get('aldrin.core.eventbus')
		getattr(eventbus, eventname)(value)
		
	def listgetter(self, membername, kwargs):
		value = getattr(self, '__' + membername)
		onget = kwargs.get('onget',None)
		if onget:
			value = onget(value)
		return value[:]
		
	def listsetter(self, membername, kwargs, values):
		onset = kwargs.get('onset',None)
		if onset:
			values = onset(values)
		setattr(self, '__' + membername, values)
		eventname = kwargs.get('event', membername + '_changed')
		eventbus = com.get('aldrin.core.eventbus')
		getattr(eventbus, eventname)(values[:])
				
	def on_handle_events(self):
		"""
		Handler triggered by the default timer. Asks the player to fill
		the event queue and fetches events from the queue to pass them to handle_event.
		"""
		if not self.__lazy_commits:
			ucopcount = self.history_get_uncomitted_operations()
			if ucopcount:
				# you should commit your actions
				import aldrin.errordlg
				msg = "%i operation(s) left uncommitted." % ucopcount
				aldrin.errordlg.error(None, "<b>Internal Program Error</b>", msg)
				self.history_commit("commit leak")
		player = com.get('aldrin.core.player')
		t1 = time.time()
		player.handle_events()
		t2 = time.time() - t1
		self._hevtime += t2
		self._hevcalls += 1
		t = time.time()
		if self.__event_stats and ((t - self._cbtime) > 1):
			print self._hevcalls, self._cbcalls, "%.2fms" % (self._hevtime*1000)
			self._cbcalls = 0
			self._hevcalls = 0
			self._hevtime = 0
			self._cbtime = t
		return True
		
	def play(self):
		self.playstarttime = time.time()
		self.set_state(zzub.zzub_player_state_playing)
		
	def stop(self):
		if self.get_state() != zzub.zzub_player_state_playing:
			self.set_position(0)
		else:
			self.set_state(zzub.zzub_player_state_stopped)
		
	def handle_event(self, player, plugin, data, tag):
		"""
		Default handler for ui events sent by zzub.
		
		@param data: event data.
		@type data: zzub_event_data_t
		"""
		eventbus = com.get('aldrin.core.eventbus')
		data = data.contents
		# prepare arguments for the specific callback
		eventname,membername,argnames = self.event_id_to_name[data.type]
		args = []
		if membername:
			specdata = getattr(data,membername)
			for argname in argnames:
				value = getattr(specdata, argname)
				if hasattr(value, 'contents'):
					class_ = value.contents.__class__
					value = class_._wrapper_._new_from_handle(value)
				elif 'contents' in dir(value):
					value = None
				args.append(value)
		if not data.type in self._exclude_event_debug_:
			print "[%s](%s)" % (eventname,','.join([('%s=%r' % (a,b)) for a,b in zip(argnames,args)]))
		result = getattr(eventbus, eventname)(*args) or False
		self._cbcalls += 1
		return result

	def document_unchanged(self):
		self.last_history_pos = self.history_get_position()
		
	def can_undo(self):
		pos = self.history_get_position()
		historysize = self.history_get_size()
		return (pos > 0) and ((pos-1) < historysize)
		
	def can_redo(self):
		pos = self.history_get_position()
		historysize = self.history_get_size()
		return (pos < historysize)
		
	def document_changed(self):
		return self.last_history_pos != self.history_get_position()
		
	def get_track_list(self):
		"""
		Returns a list of sequences
		"""
		return [self.get_sequence(i) for i in xrange(self.get_sequence_track_count())]
		
	def get_current_sequencer(self):
		return self
		
	def init_lunar(self):
		"""
		Initializes the lunar dsp scripting system
		"""
		pc = zzub.Plugincollection.get_by_uri(self, "@zzub.org/plugincollections/lunar")

		# return if lunar is missing
		if not pc:
			print >> sys.stderr, "lunar plugin collection not found, not supporting lunar."
			return

		config = com.get('aldrin.core.config')
		userlunarpath = os.path.join(config.get_settings_folder(),'lunar')
		if not os.path.isdir(userlunarpath):
			print "folder %s does not exist, creating..." % userlunarpath
			os.makedirs(userlunarpath)
		pc.configure("local_storage_dir", userlunarpath)
		
	def solo(self, plugin):	
		if not plugin or plugin == self.solo_plugin:
			# soloing deactived so apply muted states
			self.solo_plugin = None
			for plugin, info in common.get_plugin_infos().iteritems():
				plugin.set_mute(info.muted)
				info.reset_plugingfx()
		elif is_generator(plugin):
			# mute all plugins except solo plugin
			self.solo_plugin = plugin			
			for plugin, info in common.get_plugin_infos().iteritems():
				if plugin != self.solo_plugin and is_generator(plugin):
					plugin.set_mute(True)
					info.reset_plugingfx()
				elif plugin == self.solo_plugin:
					plugin.set_mute(info.muted)
					info.reset_plugingfx()
		
	def toggle_mute(self, plugin):
		pi = common.get_plugin_infos().get(plugin)
		pi.muted = not pi.muted
		print pi.muted
		# make sure a machine muted by solo is not unmuted manually
		if not self.solo_plugin or plugin == self.solo_plugin or is_effect(plugin):
			plugin.set_mute(pi.muted)
		pi.reset_plugingfx()
		
	def create_plugin(self, pluginloader):
		
		# find an unique name for the new plugin
		basename = pluginloader.get_short_name()
		name = pluginloader.get_short_name()
		basenumber = 2
		while True:
			found = False
			for mp in self.get_plugin_list():
				if mp.get_name() == name:
					found = True
					name = "%s%i" % (basename, basenumber)
					basenumber += 1
					break
			if not found:
				break
				
		# create the new plugin
		mp = zzub.Player.create_plugin(self, None, 0, name, pluginloader)
		assert mp
		
		active_plugins = []
		active_patterns = []
		
		# if it is a generator and has global or track parameters,
		# create a new default pattern, and autoconnect the plugin.
		if is_generator(mp) and \
			(pluginloader.get_parameter_count(1) or pluginloader.get_parameter_count(2)):
			pattern = mp.create_pattern(self.sequence_step)
			pattern.set_name('00')
			mp.add_pattern(pattern)
			active_plugins = [mp]
			active_patterns = [(mp, 0)]
			t=self.create_sequence(mp)
			t.set_event(0,16)
			mask=gtk.get_current_event_state()
			if not(mask & gtk.gdk.SHIFT_MASK):
				if self.autoconnect_target:
					self.autoconnect_target.add_input(mp, zzub.zzub_connection_type_audio)
				else:
					self.get_plugin(0).add_input(mp, zzub.zzub_connection_type_audio)
					
		# position the plugin at the default location
		mp.set_position(*self.plugin_origin)
		
		if 0: # TODO: disabled for now, rewrite for 0.3 interface
			# if we have a context plugin, prepend connections
			if 'plugin' in kargs:
				plugin = kargs['plugin']
				inplugs = []
				# record all connections
				while True:
					conns = plugin.get_input_connection_list()
					if not conns:
						break
					conn = conns.pop()
					input = conn.get_input()
					for i in range(conn.get_output().get_input_connection_count()):
							if conn.get_output().get_input_connection(i)==conn:
								break
					try:
						aconn = conn.get_audio_connection()
						amp = aconn.get_amplitude()
						pan = aconn.get_panning()
						inplugs.append((input,amp,pan))
					except:
						import traceback
						print traceback.format_exc()
					plugin.delete_input(input)
				# restore
				for inplug,amp,pan in inplugs:
					mp.add_audio_input(inplug, amp, pan)
				plugin.add_audio_input(mp, 16384, 16384)
		
		if 0: # TODO: disabled for now, rewrite for 0.3 interface
			# if we have a context connection, replace that one
			if 'conn' in kargs:
				conn = kargs['conn']
				for i in range(conn.get_output().get_input_connection_count()):
						if conn.get_output().get_input_connection(i)==conn:
							break
				try:
					aconn = conn.get_audio_connection()
					amp = aconn.get_amplitude()
					pan = aconn.get_panning()
					minput = conn.get_input()
					moutput = conn.get_output()
					moutput.delete_input(minput)
					mp.add_audio_input(minput, amp, pan)
					moutput.add_audio_input(mp, 16384, 16384)
				except:
					import traceback
					print traceback.format_exc()
					
		self.history_commit("new plugin")
		if active_plugins:
			self.active_plugins = active_plugins
		if active_patterns:
			self.active_patterns = active_patterns
		
	def delete_plugin(self, mp):
		# add plugin information
		common.get_plugin_infos().add_plugin(mp)
		
		inplugs = []
		outplugs = []
		if 0: # todo: rewrite for 0.3
			# record all connections
			while True:
				conns = mp.get_input_connection_list()
				if not conns:
					break
				conn = conns.pop()
				input = conn.get_input()
				for i in range(conn.get_output().get_input_connection_count()):
						if conn.get_output().get_input_connection(i)==conn:
							break
				try:
					aconn = conn.get_audio_connection()
					amp = aconn.get_amplitude()
					pan = aconn.get_panning()
					inplugs.append((input,amp,pan))
				except:
					import traceback
					print traceback.format_exc()
				mp.delete_input(input)
			while True:
				conns = mp.get_output_connection_list()
				if not conns:
					break
				conn = conns.pop()
				output = conn.get_output()
				for i in range(conn.get_output().get_input_connection_count()):
						if conn.get_output().get_input_connection(i)==conn:
							break
				try:
					aconn = conn.get_audio_connection()
					amp = aconn.get_amplitude()
					pan = aconn.get_panning()
					outplugs.append((output,amp,pan))
				except:
					import traceback
					print traceback.format_exc()
				output.delete_input(mp)
			# and now restore them
			for inplug,iamp,ipan in inplugs:
				for outplug,oamp,opan in outplugs:
					newamp = (iamp*oamp)/16384
					newpan = ipan
					outplug.add_audio_input(inplug, newamp, newpan)
		del common.get_plugin_infos()[mp]
		mp.destroy()
		self.history_commit("delete plugin")


def generate_ui_method(membername, kwargs):
	doc = kwargs.get('doc', '')
		
	onset = kwargs.get('onset', None)
	onget = kwargs.get('onget', None)
	
	if kwargs.get('list', False):
		vtype = kwargs['vtype']
		getter = lambda self: self.listgetter(membername,kwargs)
		setter = lambda self,value: self.listsetter(membername,kwargs,value)
		default = kwargs.get('default', [])
	else:
		if 'default' in kwargs:
			default = kwargs['default']
			vtype = kwargs.get('vtype', type(default))
		else:
			vtype = kwargs['vtype']
			default = {float: 0.0, int:0, long:0, str:'', unicode:u'', bool:False}.get(vtype, None)
		getter = lambda self,defvalue=kwargs.get(default,False): self.getter(membername,kwargs)
		setter = lambda self,value: self.setter(membername,kwargs,value)
	
	setattr(AldrinPlayer, '__' + membername, default)
	
	getter.__name__ = 'get_' + membername
	getter.__doc__ = 'Returns ' + doc
	setattr(AldrinPlayer, 'get_' + membername, getter)
	
	setter.__name__ = 'set_' + membername
	setter.__doc__ = 'Sets ' + doc
	setattr(AldrinPlayer, 'set_' + membername, setter)
	
	# add a property
	prop = property(getter, setter, doc=doc)
	setattr(AldrinPlayer, membername, prop)

def generate_ui_methods():
	# build getters and setters based on the options map
	for membername,kwargs in DOCUMENT_UI.iteritems():
		generate_ui_method(membername, kwargs)

generate_ui_methods()

__aldrin__ = dict(
	classes = [
		AldrinPlayer,
	],
)

if __name__ == '__main__':
	com.load_packages()
	player = com.get('aldrin.core.player')
	player.octave = 3
