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
Provides application class and controls used in the aldrin main window.
"""

import gobject
gobject.threads_init()

import sys, os
# add the directory that the main module resides in to the python path,
# if its not already contained
#modulepath = os.path.dirname(__file__)
#if not modulepath in sys.path:
#	sys.path = [modulepath] + sys.path

import contextlog

import errordlg

from gtkimport import gtk

import aldrin.com as com

from ConfigParser import ConfigParser

app = None
path_cfg = None

class AldrinApplication:
	"""
	Application class. This one will be instantiated as a singleton.
	"""
	def main(self):
		"""
		Called when the main loop initializes.
		"""
		eventbus = com.get('aldrin.core.eventbus')
		eventbus.shutdown += self.shutdown
		
		# instantiate the drivers
		com.get_from_category('driver')
		
		com.get_from_category('rootwindow')
		
		gtk.main()
		return 1
	
	def shutdown(self):
		gtk.main_quit()

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
		print pathid, append, "=",value
		return value

def run(argv):
	"""
	Starts the application and runs the mainloop.
	
	@param argv: command line arguments as passed by sys.argv.
	@type argv: str list
	"""
	global app
	global path_cfg
	contextlog.init()
	errordlg.install()
	path_cfg = PathConfig()
	component_path = [
		path_cfg.get_path('components') or os.path.join(path_cfg.get_path('share'), 'components'),
		os.path.expanduser('~/.aldrin/components'),
	]
	com.init(component_path)
	for key in com.com.factories.keys():
		print key
	options = com.get('aldrin.core.options')
	options.parse_args(argv)
	app_options, app_args = options.get_options_args()
	global app
	app = AldrinApplication()
	if app_options.profile:
		import cProfile
		cProfile.runctx('app.main()', globals(), locals(), app_options.profile)
	else:
		app.main()

__all__ = [
'CancelException',
'AboutDialog',
'AldrinFrame',
'AmpView',
'MasterPanel',
'TimePanel',
'AldrinApplication',
'main',
'run',
]

if __name__ == '__main__':
	run(sys.argv)
