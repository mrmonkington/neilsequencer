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
Supplies all user interface modules with a common bus
to handle status updates.

this is the order of operation:

1. on first time loading, GuiEventManager registers all possible event types (add new ones here).
2. main window instantiates all other ui objects (sequencer, pattern...)
3. no other ui object may register any new event id.
4. upon creation, all ui objects listening to events must connect their handlers
   using global_events.connect(). this works analogous to the way gtk does event handling.
5. there is no disconnect and hence connection can not be done dynamically.

please note:

* event handling is slow and shouldn't be used for things
  that need to be called several times per second.
* event handler calls should always have the declared number of parameters, with the 
  specified types.
* for clarity, don't register functions of another object in your object. always have
  objects connect their own methods.
"""

import sys

class GuiEventManager:
	def __init__(self):
		self.mapping = {
			'ping': [], # (int, ... ). all connected objects should print out a random message.
			'song-opened': [], # ( ... ). called when the entire song changes in such a way that all connected objects should update.
			'pattern-created': [], # (pattern, ... ). called when a new pattern is created.
			'pattern-removed': [], # ( ... ). called after a pattern has been removed.
			'pattern-changed': [], # (pattern, ... ). called when the contents of a pattern have changed.
			'pattern-size-changed': [], #(pattern, ...). called when a patterns size has changed.
			'pattern-name-changed': [], #(pattern, ...). challed when a patterns name has changed.
		}
		
	def connect(self, idstr, func, *args):
		assert self.mapping.has_key(idstr), "there is no event registered with id '%s'" % idstr
		funclist = self.mapping[idstr]
		funclist.append((func,args))
		self.mapping[idstr] = funclist
		
	def print_mapping(self):
		for idstr in sorted(self.mapping.keys()):
			print "event '%s':" % idstr
			if not self.mapping[idstr]:
				print "\tno connections."
			for func,args in self.mapping[idstr]:
				print "\t=> %r%r" % (func,args)
	
	def __call__(self, idstr, *cargs):
		assert self.mapping.has_key(idstr), "there is no event registered with id '%s'" % idstr
		for func,args in self.mapping[idstr]:
			try:
				result = func(*(cargs + args))
				if result:
					return result
			except:
				sys.excepthook(*sys.exc_info())
	
global_events = GuiEventManager()

__all__ = [
'global_events',
]

if __name__ == '__main__':
	class MyHandler:
		def on_bang(self, otherbang, mybang):
			print "BANG! otherbang=",otherbang,"mybang=",mybang
			
	handler1 = MyHandler()
	handler2 = MyHandler()
	global_events.connect('ping', handler1.on_bang, 50)
	global_events.connect('ping', handler1.on_bang, 60)
	global_events.print_mapping()
	global_events('ping', 25)
