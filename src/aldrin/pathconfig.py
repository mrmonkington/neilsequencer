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
Organizes finding Aldrins resources across the system.
"""

from ConfigParser import ConfigParser
import sys, os

class PathConfig(ConfigParser):
	CFG_PATHS = [
		'etc/debugpath.cfg', # assume we are in the repository
		'~/.aldrin/path.cfg', # is it in home dir config folder?
		'/etc/aldrin/path.cfg', # take the absolute path
	]
	
	def __init__(self):
		ConfigParser.__init__(self)
		self.basedir = os.path.normpath(os.path.join(os.path.dirname(__file__), '../..'))
		CFG_PATH = None
		for path in self.CFG_PATHS:
			path = os.path.expanduser(path)
			if not os.path.isabs(path):
				path = os.path.normpath(os.path.join(self.basedir,path))
			print "searching " + path
			if os.path.isfile(path):
				print "using " + path
				CFG_PATH = path
				break
		assert CFG_PATH, "Unable to find path.cfg"
		self.read([CFG_PATH])
		site_packages = self.get_path('site_packages')
		if not site_packages in sys.path:
			print site_packages + "  missing in sys.path, prepending"
			sys.path = [site_packages] + sys.path
		
	def get_path(self, pathid, append=''):
		if not self.has_option('Paths', pathid):
			return None
		value = os.path.expanduser(self.get('Paths', pathid))
		if not os.path.isabs(value):
			value = os.path.normpath(os.path.join(self.basedir, value))
		if append:
			value = os.path.join(value, append)
		return value

path_cfg = PathConfig()

