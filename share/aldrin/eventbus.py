#encoding: latin-1

"""
Supplies all user interface modules with a common bus
to handle status updates.

this is the order of operation:

1. on first time loading, GlobalEventBus registers all possible event types (add new ones below).
2. main window instantiates all other ui objects
3. no other ui object may register any new event id.
4. upon creation, all ui objects listening to events must connect their handlers using 
   eventbus.<event name> += <function>[,<arg1>[,...]]
5. call events using
   eventbus.<event name>([<arg1>[,...]])

please note:

* event handling is slow and shouldn't be used for things
	that need to be called several times per second.
* event handler calls should always have the declared number of parameters, with the 
	specified types.
* for clarity, don't register functions of another object in your object. always have
	objects connect their own methods.
"""

import sys

EVENTS = [
	'ping', # (int, ... ). all connected objects should print out a random message.
	'finalize', # (hub, ... ). all module are loaded and available, finalize any cross-dependencies.
	'shutdown-request', # ( ... ). asks for quit. it is possible that quitting might not happen.
	'shutdown', # ( ... ). receiver should deinitialize and prepare for application exit.
	'song-opened', # ( ... ). called when the entire song changes in such a way that all connected objects should update.
	'pattern-created', # (pattern, ... ). called when a new pattern is created.
	'pattern-removed', # ( ... ). called after a pattern has been removed.
	'pattern-changed', # (pattern, ... ). called when the contents of a pattern have changed.
	'pattern-size-changed', #(pattern, ...). called when a patterns size has changed.
	'pattern-name-changed', #(pattern, ...). challed when a patterns name has changed.
	'connection-changed', #() called when a connection between machines is deleted.
]

class EventHandlerList:
	def __init__(self, name, handlers=None):
		self.name = name
		if not handlers:
			handlers = []
		self.handlers = handlers
		
	def __add__(self, funcargs):
		func = None
		args = ()
		if isinstance(funcargs, list) or isinstance(funcargs, tuple):
			if len(funcargs) >= 2:
				func,args = funcargs[0],funcargs[1:]
			else:
				func = funcargs[0]
		else:
			func = funcargs
		assert callable(func), "object %r must be callable." % func
		handler = EventHandlerList(self.name, self.handlers)
		handler.handlers.append((func,args))
		return handler
		
	def __len__(self):
		return len(self.handlers)
		
	def __call__(self, *cargs):
		for func,args in self:
			try:
				result = func(*(cargs + args))
				if result:
					return result
			except:
				sys.excepthook(*sys.exc_info())
				
	def __iter__(self):
		return iter(self.handlers)
		
	def print_mapping(self):
		print "event '%s':" % self.name
		if not self.handlers:
			print "    no connections."
		for func,args in self.handlers:
			print "    => %r%r" % (func,args)

class EventBus(object):
	__readonly__ = False
	
	def __init__(self):
		self.handlers = []
		for name in self.names:
			attrname = name.replace('-','_')
			self.handlers.append(attrname)
			setattr(self, attrname, EventHandlerList(name))
		self.__readonly__ = True
			
	def __setattr__(self, name, value):
		if name.startswith('__'):
			self.__dict__[name] = value
			return
		assert name in self.__dict__ or not self.__readonly__, "can't set attribute when object is read only"
		if name in self.__dict__ and isinstance(self.__dict__[name], EventHandlerList) and not isinstance(value, EventHandlerList):
			raise Exception, "did you mean +=?"
		self.__dict__[name] = value
		
	def print_mapping(self):
		for idstr in sorted(self.handlers):
			handlerlist = getattr(self, idstr)
			handlerlist.print_mapping()

class GlobalEventBus(EventBus):
	names = EVENTS

eventbus = GlobalEventBus()

__all__ = [
'EventBus',
'eventbus',
]

if __name__ == '__main__':
	class MyHandler:
		def on_bang(self, otherbang, mybang):
			print self,"BANG! otherbang=",otherbang,"mybang=",mybang
			
	handler1 = MyHandler()
	handler2 = MyHandler()
	eventbus.ping += handler1.on_bang, 50
	eventbus.ping += handler2.on_bang, 60
	eventbus.print_mapping()
	eventbus.ping(25)
