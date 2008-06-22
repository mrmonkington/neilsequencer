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

from zzub import Player
from aldrincom import com

import os,sys 
import time
from config import get_plugin_aliases, get_plugin_blacklist

class AldrinPlayer(Player):
	__aldrin__ = dict(
		id = 'aldrin.core.player',
		singleton = True,
	)
	
	def __init__(self):
		Player.__init__(self)
		# load blacklist file and add blacklist entries
		for name in get_plugin_blacklist():
			self.blacklist_plugin(name)
		# load aliases file and add aliases
		# 0.3: DEAD
		#~ for name,uri in get_plugin_aliases():
			#~ self.add_plugin_alias(name, uri)
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
		
	def init_lunar(self):
		"""
		Initializes the lunar dsp scripting system
		"""
		pc = self.get_plugincollection_by_uri("@zzub.org/plugincollections/lunar")

		# return if lunar is missing
		if not pc._handle:
			print >> sys.stderr, "not supporting lunar."
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
