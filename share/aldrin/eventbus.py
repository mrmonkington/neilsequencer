#encoding: latin-1

"""
Supplies all user interface modules with a common bus
to handle status updates.

this is the order of operation:

1. on first time loading, GuiEventManager registers all possible event types (add new ones here).
2. main window instantiates all other ui objects
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

class EventBus:
  def __init__(self):
    self.mapping = {
      'ping': [], # (int, ... ). all connected objects should print out a random message.
      'finalize': [], # (hub, ... ). all module are loaded and available, finalize any cross-dependencies.
      'shutdown-request': [], # ( ... ). asks for quit. it is possible that quitting might not happen.
      'shutdown': [], # ( ... ). receiver should deinitialize and prepare for application exit.
      #~ 'song-opened': [], # ( ... ). called when the entire song changes in such a way that all connected objects should update.
      #~ 'pattern-created': [], # (pattern, ... ). called when a new pattern is created.
      #~ 'pattern-removed': [], # ( ... ). called after a pattern has been removed.
      #~ 'pattern-changed': [], # (pattern, ... ). called when the contents of a pattern have changed.
      #~ 'pattern-size-changed': [], #(pattern, ...). called when a patterns size has changed.
      #~ 'pattern-name-changed': [], #(pattern, ...). challed when a patterns name has changed.
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
        print "  no connections."
      for func,args in self.mapping[idstr]:
        print "  => %r%r" % (func,args)
  
  def __call__(self, idstr, *cargs):
    assert self.mapping.has_key(idstr), "there is no event registered with id '%s'" % idstr
    for func,args in self.mapping[idstr]:
      try:
        result = func(*(cargs + args))
        if result:
          return result
      except:
        sys.excepthook(*sys.exc_info())
  
eventbus = EventBus()

__all__ = [
'eventbus',
]

if __name__ == '__main__':
  class MyHandler:
    def on_bang(self, otherbang, mybang):
      print "BANG! otherbang=",otherbang,"mybang=",mybang
      
  handler1 = MyHandler()
  handler2 = MyHandler()
  eventbus.connect('ping', handler1.on_bang, 50)
  eventbus.connect('ping', handler1.on_bang, 60)
  eventbus.print_mapping()
  eventbus('ping', 25)
