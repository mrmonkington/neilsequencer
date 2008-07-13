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

import pathconfig
import gobject
gobject.threads_init()

import pygtk
pygtk.require('2.0')
import gtk

import sys, os

import aldrin.contextlog as contextlog
import aldrin.errordlg as errordlg

import aldrin.com as com

app = None

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
		
		for key in sorted(sys.modules.keys()):
			print key,"=",sys.modules[key]
		gtk.main()
		return 1
	
	def shutdown(self):
		gtk.main_quit()

def run(argv):
	"""
	Starts the application and runs the mainloop.
	
	@param argv: command line arguments as passed by sys.argv.
	@type argv: str list
	"""
	global app
	contextlog.init()
	errordlg.install()
	com.init()
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
