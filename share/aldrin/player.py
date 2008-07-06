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
from aldrincom import com

import gobject
import os,sys 
import time
from config import get_plugin_aliases, get_plugin_blacklist

class AldrinPlayer(Player):
	__aldrin__ = dict(
		id = 'aldrin.core.player',
		singleton = True,
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
		zzub_event_type_pre_delete_pattern = dict(args=None),
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
		self._hevtimes = 0		
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
				for argname,argtype in zzub.EventData._fields_:
					if argname == '': # union
						union = argtype
						break
				for argname,argtype in union._fields_:
					if argname == membername:
						datatype = argtype
				assert datatype, "couldn't find member %s in zzub_event_data_t" % membername
				for argname,argtype in datatype._fields_:
					args.append(argname)
			self.event_id_to_name[val] = (eventname, membername, args)
			#print "'%s', # ( %s )" % (eventname, ','.join(args + ["..."]))
		# load blacklist file and add blacklist entries
		for name in get_plugin_blacklist():
			self.blacklist_plugin(name)
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
		self.playstarttime = time.time()		
		self.document_unchanged()
		eventbus = com.get('aldrin.core.eventbus')
		gobject.timeout_add(int(1000/25), self.on_handle_events)
		
	def on_handle_events(self):
		"""
		Handler triggered by the default timer. Asks the player to fill
		the event queue and fetches events from the queue to pass them to handle_event.
		"""
		player = com.get('aldrin.core.player')
		t1 = time.time()
		player.handle_events()
		event = player.get_next_event()
		while event:
			try:
				self.handle_event(event)
			except:
				import errordlg
				errordlg.print_exc()
			event = player.get_next_event()
		t2 = time.time() - t1
		self._hevtimes = (self._hevtimes * 0.9) + (t2 * 0.1)
		self._hevcalls += 1
		t = time.time()
		if (t - self._cbtime) > 1:
			#print self._hevcalls, self._cbcalls, "%.2fms" % (self._hevtimes*1000)
			self._cbcalls = 0
			self._hevcalls = 0
			self._cbtime = t
		#called only if loop pattern is off when song ends:
		#~ if player.get_state() != zzub.zzub_player_state_playing and self.btnplay.get_active():
			#~ self.btnplay.set_active(False)
		return True
		
	def play(self):
		self.playstarttime = time.time()
		Player.play(self)
		
	def stop(self):
		if self.get_state() != zzub.zzub_player_state_playing:
			self.set_position(0)
		else:
			Player.stop(self)
		
	def handle_event(self, data):
		"""
		Default handler for ui events sent by zzub.
		
		@param data: event data.
		@type data: zzub_event_data_t
		"""
		eventbus = com.get('aldrin.core.eventbus')
		# prepare arguments for the specific callback
		eventname,membername,argnames = self.event_id_to_name[data.type]
		args = []
		if membername:
			specdata = getattr(data,'')
			specdata = getattr(specdata,membername)
			for argname in argnames:
				args.append(getattr(specdata, argname))
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
		

__aldrin__ = dict(
	classes = [
		AldrinPlayer,
	],
)

if __name__ == '__main__':
	com.load_packages()
	player = com.get('aldrin.core.player')
