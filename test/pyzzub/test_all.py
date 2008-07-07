
"""
Test Suite for PyZZUB
"""

import os
os.environ['LD_LIBRARY_PATH'] = '../../lib'
import sys
sys.path = ['../../src/pyzzub'] + sys.path

import time
from unittest import TestCase, main
import zzub
from zzub import *

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

# enumerate zzub_event_types and prepare unwrappers for the different types
event_id_to_name = {}		
for enumname,cfg in _event_types_.iteritems():
	val = getattr(zzub, enumname)
	assert val not in event_id_to_name, "value %s (%s) already registered." % (val,eventname)
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
	event_id_to_name[val] = (eventname, membername, args)

class Test(TestCase):
	def setUp(self):
		self.player = Player.create()
		self.player.add_plugin_path('../../lib/zzub/')
		self.player.initialize(44100)
		self.driver = Audiodriver.create(self.player)
		self.driver.set_buffersize(1024)
		self.driver.set_samplerate(44100)
		res = self.driver.create_device(-1, -1)
		self.assertTrue(res == 0)
		self.driver.enable(True)
		self.events = []
		
	def tearDown(self):
		self.driver.enable(False)
		self.driver.destroy()
		self.player.destroy()
		del self.player
		
	def _handle_events(self):
		self.player.handle_events()
		events = []
		event = self.player.get_next_event()
		while event:
			# prepare arguments for the specific callback
			eventname,membername,argnames = event_id_to_name[event.type]
			args = []
			if membername:
				specdata = getattr(event,membername)
				for argname in argnames:
					value = getattr(specdata, argname)
					if hasattr(value, 'contents'):
						class_ = value.contents.__class__
						value = class_._wrapper_._new_from_handle(value)
					elif 'contents' in dir(value):
						value = None
					args.append(value)
			print "[%s](%s)" % (eventname,','.join([('%s=%r' % (a,b)) for a,b in zip(argnames,args)]))
			events.append(event)
			event = self.player.get_next_event()
		self.events = events
		
	def _check_protocol(self, eventlist):
		events = self.events
		for name,kargs in eventlist:
			found = False
			for i,event in enumerate(events):
				eventname,membername,argnames = event_id_to_name[event.type]
				if eventname == name:
					found = True
					events = events[i+1:]
					if membername:
						specdata = getattr(event,membername)
						for key,value in kargs.iteritems():
							gotval = getattr(specdata, key)
							if hasattr(gotval, 'contents'):
								class_ = gotval.contents.__class__
								gotval = class_._wrapper_._new_from_handle(gotval)
							elif 'contents' in dir(gotval):
								gotval = None
							self.assertTrue(gotval == value, "event %s: %s != %s (%s)" % (name, gotval, value, key))
					else:
						self.assertTrue(not kargs)
					break
			self.assertTrue(found == True, "no luck finding event %s" % name)
		
	def test_state_changed(self):
		"""
		run player, check if state changed event is being sent,
		then stop, then check if state changed event is being sent again.
		"""
		self.player.set_state(zzub_player_state_playing)
		self._handle_events()
		self._check_protocol([
			('zzub_player_state_changed', dict(player_state=zzub_player_state_playing)),
		])
		self.player.set_state(zzub_player_state_stopped)
		self._handle_events()
		self._check_protocol([
			('zzub_player_state_changed', dict(player_state=zzub_player_state_stopped)),
		])
		
	def test_undo(self):
		"""
		create plugin, connect to master, disconnect, then undo/redo/undo until the beginning.
		"""
		self.assertTrue(self.player.get_plugin_count() == 1)
		master = self.player.get_plugin_by_id(0)
		self.assertTrue(master == self.player.get_plugin(0))
		self.assertTrue(master.get_input_connection_count() == 0)
		self.assertTrue(self.player.history_get_size() == 0)
		self.assertTrue(self.player.history_get_position() == 0)
		pluginloader = self.player.get_pluginloader_by_name('@krzysztof_foltman/generator/infector;1')
		self.assertTrue(pluginloader)
		plugin = self.player.create_plugin(None, 0, "test", pluginloader)
		self.assertTrue(self.player.get_plugin_count() == 2)
		self.assertTrue(self.player.history_get_size() == 0)
		master.add_input(plugin, zzub_connection_type_audio)
		self.player.history_commit("create plugin and connect")
		self._handle_events()
		self._check_protocol([
			('zzub_pre_connect', dict()),
			('zzub_new_plugin', dict(plugin=plugin)),
			('zzub_connect', dict(from_plugin=plugin, to_plugin=master, type=0)),
			('zzub_post_connect', dict()),
		])
		self.assertTrue(self.player.history_get_position() == 1)
		self.assertTrue(self.player.history_get_size() == 1)
		self.assertTrue(master.get_input_connection_count() == 1)
		self.assertTrue(master.get_input_connection_plugin(0) == plugin)
		self.assertTrue(master.get_input_connection_type(0) == zzub_connection_type_audio)
		master.delete_input(plugin, zzub_connection_type_audio)
		self.assertTrue(master.get_input_connection_count() == 0)
		self.player.history_commit("disconnect")
		self._handle_events()
		self._check_protocol([
			('zzub_pre_disconnect', dict()),
			('zzub_disconnect', dict(from_plugin=plugin,to_plugin=master,type=0)),
		])
		self.assertTrue(self.player.history_get_position() == 2)
		self.assertTrue(master.get_input_connection_count() == 0)
		self.assertTrue(self.player.history_get_size() == 2)
		self.player.undo()
		self._handle_events()
		self._check_protocol([
			('zzub_pre_connect', dict()),
			('zzub_connect', dict(from_plugin=plugin, to_plugin=master, type=0)),
			('zzub_post_connect', dict()),
		])
		self.assertTrue(self.player.history_get_position() == 1)
		self.assertTrue(self.player.history_get_size() == 2)
		self.assertTrue(master.get_input_connection_count() == 1)
		self.assertTrue(master.get_input_connection_plugin(0) == plugin)
		self.assertTrue(master.get_input_connection_type(0) == zzub_connection_type_audio)
		self.player.redo()
		self._handle_events()
		self._check_protocol([
			('zzub_pre_disconnect', dict()),
			('zzub_disconnect', dict(from_plugin=plugin,to_plugin=master,type=0)),
		])
		self.assertTrue(self.player.history_get_position() == 2)
		self.assertTrue(master.get_input_connection_count() == 0)
		self.assertTrue(plugin.get_output_connection_count() == 0)
		self.player.undo()
		self._handle_events()
		self._check_protocol([
			('zzub_pre_connect', dict()),
			('zzub_connect', dict(from_plugin=plugin, to_plugin=master, type=0)),
			('zzub_post_connect', dict()),
		])
		self.assertTrue(self.player.history_get_position() == 1)
		self.assertTrue(master.get_input_connection_count() == 1)
		self.assertTrue(master.get_input_connection_plugin(0) == plugin)
		self.assertTrue(master.get_input_connection_type(0) == zzub_connection_type_audio)
		self.assertTrue(self.player.get_plugin_count() == 2)
		self.player.undo()
		self._handle_events()
		self._check_protocol([
			('zzub_pre_disconnect', dict()),
			('zzub_pre_delete_plugin', dict(plugin=plugin)),
			('zzub_disconnect', dict(from_plugin=plugin,to_plugin=master,type=0)),
			('zzub_delete_plugin', dict(plugin=plugin)),
		])
		self.assertTrue(self.player.history_get_position() == 0)
		self.assertTrue(self.player.history_get_size() == 2)
		self.assertTrue(self.player.get_plugin_count() == 1)
		self.player.history_flush()
		self.assertTrue(self.player.get_plugin_count() == 1)
		self.assertTrue(self.player.history_get_position() == 0)
		self.assertTrue(self.player.history_get_size() == 0)
		self._handle_events()
		self.assertFalse(self.events)

if __name__ == '__main__':
	main()

