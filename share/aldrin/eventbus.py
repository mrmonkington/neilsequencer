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

import sys, weakref

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
	'plugin-created', # (plugin, ... ) called when a plugin is created by the user.
	'show-plugin', # (plugin, ...) called when a plugin should be visualized overall.
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
		ref = None
		funcname = None
		if hasattr(func, 'im_self'):
			ref = weakref.ref(func.im_self)
			funcname = func.__name__
		else:
			ref = weakref.ref(func)
		handler = EventHandlerList(self.name, self.handlers)
		handler.handlers.append((ref,funcname,args))
		return handler
		
	def __len__(self):
		return len(self.handlers)
		
	def filter_dead_references(self):
		"""
		filter handlers from dead references
		"""
		self.handlers = [(ref,funcname,args) for ref,funcname,args in self.handlers if ref()]
		
	def __call__(self, *cargs):
		print 'call event [%s]' % self.name
		self.filter_dead_references()
		for ref,funcname,args in self:
			if funcname:
				func = getattr(ref(), funcname)
			else:
				func = ref()
			try:
				fargs = cargs + args
				funcname = func.__name__
				if hasattr(func, 'im_class'):
					funcname = func.im_class.__name__ + '.' + funcname
				print " => %s(%s)" % (funcname,','.join([repr(x) for x in fargs]))
				result = func(*fargs)
				if result:
					return result
			except:
				sys.excepthook(*sys.exc_info())
				
	def __iter__(self):
		return iter(self.handlers)
		
	def print_mapping(self):
		self.filter_dead_references()
		print "event [%s]" % self.name
		for ref,funcname,args in self.handlers:
			if funcname:
				func = getattr(ref(), funcname)
			else:
				func = ref()
			funcname = func.__name__
			if hasattr(func, 'im_class'):
				funcname = func.im_class.__name__ + '.' + funcname
			print " => %s(%s)" % (funcname,','.join([repr(x) for x in args]))

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

class AldrinEventBus(EventBus):
	__aldrin__ = dict(
		id = 'aldrin.core.eventbus',
		singleton = True,
	)	
	
	names = EVENTS

__all__ = [
'GlobalEventBus',
]

__aldrin__ = dict(
	classes = [
		AldrinEventBus,
	],
)

if __name__ == '__main__':
	class MyHandler:
		def on_bang(self, otherbang, mybang):
			print self,"BANG! otherbang=",otherbang,"mybang=",mybang
			
	def on_bang(otherbang, mybang):
		print "GLOBAL BANG! otherbang=",otherbang,"mybang=",mybang
			
	handler1 = MyHandler()
	handler2 = MyHandler()
	eventbus = AldrinEventBus()
	eventbus.ping += handler1.on_bang, 50
	eventbus.ping += handler2.on_bang, 60
	eventbus.ping += on_bang, 70
	eventbus.print_mapping()
	print "* 3 bangs:"
	eventbus.ping(25)
	del handler1
	del on_bang
	print "* 1 bang:"
	eventbus.ping(25)
