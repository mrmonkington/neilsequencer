#encoding: latin-1

# Neil
# Modular Sequencer
# Copyright (C) 2006,2007,2008 The Neil Development Team
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
Provides application class and controls used in the neil main window.
"""

import pathconfig
import gobject
gobject.threads_init()

import pygtk
pygtk.require('2.0')
import gtk

import sys, os

import neil.contextlog as contextlog
import neil.errordlg as errordlg

import neil.com as com

def shutdown():
  gtk.main_quit()

def init_neil():
  """
  Loads the categories neccessary to visualize neil.
  """
  com.get_from_category('driver')	
  com.get_from_category('rootwindow')

def run(argv, initfunc = init_neil):
  """
  Starts the application and runs the mainloop.

  @param argv: command line arguments as passed by sys.argv.
  @type argv: str list
  @param initfunc: a function to call before gtk.main() is called.
  @type initfunc: callable()
  """
  contextlog.init()
  errordlg.install()
  com.init()
  options = com.get('neil.core.options')
  options.parse_args(argv)
  eventbus = com.get('neil.core.eventbus')
  eventbus.shutdown += shutdown
  options = com.get('neil.core.options')
  app_options, app_args = options.get_options_args()
  initfunc()
  if app_options.profile:
    import cProfile
    cProfile.runctx('gtk.main()', globals(), locals(), app_options.profile)
  else:
    gtk.main()

__all__ = [
'CancelException',
'AboutDialog',
'NeilFrame',
'AmpView',
'MasterPanel',
'TimePanel',
'NeilApplication',
'main',
'run',
]

if __name__ == '__main__':
  run(sys.argv)
