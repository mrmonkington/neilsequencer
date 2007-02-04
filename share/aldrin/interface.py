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
Interfaces, classes and constants for contracted communication with UI extensions.
"""

from wximport import wx
import inspect, new

#
# Service URIs
####

MAINFRAME_SERVICE = "@zzub.org/ui/mainframe" # IMainFrame

#
# Interfaces
####

class Interface(object):
	"""
	Base class for all interfaces, which is responsible
	for making sure contracts between host and client
	are not violated.
	"""
	def __init__(self):
		"""
		Compares implemented methods and warns on
		discrepancies in argument count, argument
		names or default values.
		"""
		classname = self.__class__.__name__
		ifaces = self.get_interfaces()
		for iface in ifaces:
			ifacename = iface.__name__
			for name in dir(iface):
				clselement = getattr(iface,name)
				if not name.startswith('_') and inspect.ismethod(clselement):
					element = getattr(self,name)
					assert inspect.ismethod(element), "%s: attribute %s is not a method." % (classname, name)
					# get wanted and having args
					args,varargs,varkw,defaults = inspect.getargspec(clselement)
					myargs,myvarargs,myvarkw,mydefaults = inspect.getargspec(element)
					if args:
						assert len(args) == len(myargs), "%s: %s.%s takes %i argument(s), not %i." % (classname, ifacename, name, len(args), len(myargs))
						idx = 0
						for a,b in zip(args,myargs):
							idx += 1
							assert a == b, "%s: %s.%s argument %i is named %s, not %s." % (classname, ifacename, name, idx, a, b)
						if defaults:
							assert len(defaults) == len(mydefaults), "%s: %s.%s has %i default argument(s), not %i." % (classname, ifacename, name, len(defaults), len(mydefaults))
							idx = 0
							for a,b in zip(defaults,mydefaults):
								idx += 1
								assert a == b, "%s: %s.%s argument %i defaults to %r, not %r." % (classname, ifacename, name, idx, a, b)
		self.__validated__ = True
		
	def create_protected_proxy(self, ifaces = None):
		"""
		Returns a protected proxy object that only
		exports methods defined in interfaces.
		
		@param ifaces: List of interfaces to expose or None.
		@type ifaces: [class, ...]
		"""
		assert hasattr(self,'__validated__') and self.__validated__ == True, "%r: Interface constructor was not called. Object is not validated." % self
		if not ifaces:
			ifaces = self.get_interfaces()
		cls = new.classobj("%sProxy" % self.__class__.__name__, tuple(ifaces), {})
		obj = cls()
		for iface in ifaces:
			for name in dir(iface):
				element = getattr(self,name)
				if not name.startswith('_') and inspect.ismethod(element):
					setattr(obj, name, element)
		return obj
		
	def get_interfaces(self):
		"""
		Returns all supported interfaces.
		"""
		basemrolen = len(inspect.getmro(Interface))
		mro = inspect.getmro(self.__class__)
		if len(mro) == basemrolen+1:
			return [self.__class__]
		ifaces = []
		for cls in mro:
			if not issubclass(cls,Interface):
				continue
			clsmro = inspect.getmro(cls)
			if len(clsmro) != basemrolen+1: # interface, object
				continue
			ifaces.append(cls)
		return ifaces

class IExtension(Interface):
	"""
	Base class for Aldrin UI extensions. Any extension
	should export a module which contains one class
	inherited from this interface.
	"""
	
	def realize(self, extensionhost):
		"""
		Called when the extension is asked to
		realize itself.
		
		In this method you should store the pointer to
		the extension host and initialize your
		extension i.e. add buttons or menu entries
		through the extension manager.
		
		@param extensionhost: The extension host object.
		@type extensionhost: {IExtensionHost} instance
		"""
	
	def finalize(self):
		"""
		Called when the extension is asked to
		close.
		
		You must remove your reference to the
		extension manager object and delete
		any allocated resources, if neccessary.
		"""

class IExtensionHost(Interface):
	"""
	Base class for the Aldrin UI extension host. An
	extension host object will be passed to extension
	classes on initialization, which can be queried
	for subsequent services.
	"""
	
	def get_extension_manager(self):
		"""
		Returns the extension manager object.
		
		@return: Extension manager object.
		@rtype: {IExtensionManager}
		"""
		
	def resolve_path(self, path):
		"""
		Resolves a relative path to an absolute
		one, where the location of the manifest.xml
		is regarded as the base directory.
		
		@param path: Relative path to file or dir.
		@type path: str
		@return: Absolute path to file or dir.
		@rtype: str
		"""

class IExtensionManager(Interface):
	"""
	UI extension manager interface. Enumerates extensions, creates 
	extension hosts and returns services.
	"""
	
	def get_service(self, uri):
		"""
		returns an object providing the service
		requested by the uri or raises an
		UnknownServiceException object.
		
		@param uri: The uri of the service requested.
		@type uri: str
		"""
		
class IMainFrame(Interface):
	"""
	Base interface for the mainframe window.
	"""
	
	def get_window(self):
		"""
		Returns the window object associated with the mainframe.
		
		@return: Window object.
		@rtype: wx.Window
		"""
		
	def add_menuitem(self, label, description = "", kind = wx.ITEM_NORMAL):
		"""
		Adds a new menuitem to the tools menu and returns the identifier.
		
		@param label: Label of the item.
		@type label: str
		@param description: Description for Status bar.
		@type description: str
		@param kind: One of wx.ITEM_NORMAL, wx.ITEM_CHECK or wx.ITEM_RADIO
		@type kind: int
		@return: Identifier of the menuitem.
		@rtype: int
		"""
		
	def add_submenu(self, label, submenu, description = ""):
		"""
		Adds a new submenu to the tools menu and returns the identifier.
		
		@param label: Label of the item.
		@type label: str
		@param submenu: The submenu which to add.
		@type submenu: wx.Menu
		@param description: Description for Status bar.
		@type description: str
		@return: Identifier of the menuitem.
		@rtype: int
		"""

	def add_tool_button(self, label, bitmap1, bitmap2 = wx.NullBitmap, kind = wx.ITEM_NORMAL, tooltip = "", description = ""):
		"""
		Adds a new tool to the toolbar and returns the identifier.
		
		@param label: Label of the button. Will not be visible on all systems.
		@type label: str
		@param bitmap1: Bitmap for the button.
		@type bitmap1: wx.Bitmap
		@param bitmap2: Bitmap for disabled button.
		@type bitmap2: wx.Bitmap
		@param kind: One of wx.ITEM_NORMAL, wx.ITEM_CHECK or wx.ITEM_RADIO
		@type kind: int
		@param tooltip: Tooltip Text
		@type tooltip: str
		@param description: Description for Status bar.
		@type description: str
		@return: Identifier of the toolbar button.
		@rtype: int
		"""
		
	def add_click_handler(self, toolid, func):
		"""
		Adds a handler for when a tool is being clicked by the user.
		
		@param toolid: Id of the tool as returned by add_tool()
		@type toolid: int
		@param func: Function to call. The function should take
					an additional event parameter.
		@type func: callable
		"""		

class UnknownServiceException(Exception):
	pass

if __name__ == '__main__':
	class IDemoInterface(Interface):
		def you_can_call_me(self):
			pass
			
		def also_implement_this_one(self):
			pass
	
	class Demo(IDemoInterface):
		def you_can_call_me(self):
			print "you called me."
			
		def but_this_one_is_private(self):
			assert 0, "you should not be able to call this function."
			
	d = Demo()
	print d.get_interfaces()
	print dir(d.create_protected_proxy([IDemoInterface]))

	d2 = d.create_protected_proxy()
	print d2, dir(d2)

	d2.you_can_call_me()
	d2.but_this_one_is_private()
