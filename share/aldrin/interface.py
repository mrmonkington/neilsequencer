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
Interfaces, classes and constants for contracted communication with UI extensions.
"""

from gtkimport import gtk
import inspect, new, traceback

#
# Class URIs
# ############

# implement IUIBuilder for the following categories
CLASS_UI_BUILDER = "@zzub.org/class/ui_builder"

#
# UI object URIs
# ############

UIOBJECT_MAIN_TOOLBAR = "@aldrin.org/uiobject/main/toolbar"
UIOBJECT_MAIN_MENUBAR = "@aldrin.org/uiobject/main/menubar"
UIOBJECT_MAIN_MENU_TOOLS = "@aldrin.org/uiobject/main/menu/tools"
UIOBJECT_ROUTE_MENU_PLUGIN = "@aldrin.org/uiobject/route/menu/plugin"
UIOBJECT_ROUTE_MENU_CONNECTION = "@aldrin.org/uiobject/route/menu/connection"
UIOBJECT_ROUTE_MENU = "@aldrin.org/uiobject/route/menu"

#
# UI view URIs
# ############

UIVIEW_ALL = "@aldrin.org/uiview/all"

#
# Service URIs
# ############

# returns a IUIMessage object
SERVICE_MESSAGE = "@aldrin.org/service/message"
# return a IRootWindow object
SERVICE_ROOTWINDOW = "@aldrin.org/service/rootwindow"

#
# Interfaces
# ############

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
		
	def call_safe(self, name, defaultret, *args, **kargs):
		"""
		Makes a safe call that does only output a traceback, but
		does not terminate the entire callstack. If the call fails,
		defaultret is being returned.
		
		@param name: name of interface function to call.
		@type name: str
		@param defaultret: Default return value.
		@type defaultret: any
		"""
		try:
			return getattr(self, name)(*args, **kargs)
		except:
			traceback.print_exc()
			return defaultret
		
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
		
	def get_player(self):
		"""
		Returns the zzub.Player object allowing
		access to all song data. Please see
		the pyzzub documentation.
		
		@return: The global player object.
		@rtype: zzub.Player
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
		
	def add_service_class(self, classuri, uri):
		"""
		Registers a service uri to be listed in a class of services.
		
		The enumerate_services function returns a list of
		services for a class URI registered with this function.
		
		@param classui: class uri which is to be associated with the service.
		@type classui: str
		@param uri: uri of the service to be associated with the class.
		@type uri: str
		"""
		
	def enumerate_services(self, classuri):
		"""
		Returns a list of services registered for a specific class.
		
		@param classui: class uri
		@type classui: str
		@return: list of service objects.
		@rtype: object list
		"""
		
	def register_service(self, uri, instance, iface = None):
		"""
		Registers a service for public access.
		
		Instead of the real instance, a proxy object will
		be saved which exposes only interface methods.
		
		@param uri: uri by which the object can be retrieved.
		@type uri: str
		@param instance: The object to provide on request. The object must
		implement at least one interface.
		@type instance: any
		@param iface: Interface which exposes the methods required or None for all.
		@type iface: class
		"""
		
class IRootWindow(Interface):
	"""
	Interface for the root window.
	"""
	def refresh_view(self, target):
		"""
		Refreshes a view.
		
		@param target: The uri of the target.
		@type target: str
		"""

class IUIMessage(Interface):
	"""
	Interface for displaying info or error messages or asking questions.
	"""
	def error(self, message):
		"""
		Displays a modal messagebox configured to show an error message.
		
		@param message: The message to display (you can use markup).
		@type message: str
		"""
		
	def message(self, message):
		"""
		Displays a modal messagebox configured to show an informative message.
		
		@param message: The message to display (you can use markup).
		@type message: str
		"""

	def warning(self, message):
		"""
		Displays a modal messagebox configured to show a warning message.
		
		@param message: The message to display (you can use markup).
		@type message: str
		"""

	def question(self, message):
		"""
		Displays a modal messagebox configured to show a question.
		
		@param message: The message to display (you can use markup).
		@type message: str
		@return: either gtk.RESPONSE_YES or gtk.RESPONSE_NO
		@rtype: int
		"""

	def choice(self, message):
		"""
		Displays a modal messagebox configured to show a choice. It
		is similar to question, except that it also allows to cancel.
		
		@param message: The message to display (you can use markup).
		@type message: str
		@return: either gtk.RESPONSE_YES, gtk.RESPONSE_NO or gtk.RESPONSE_CANCEL
		@rtype: int
		"""

class IUIBuilder(Interface):
	"""
	Interface for extensions which need to add new menuitems
	or toolitems to any dialogue. Implement each function
	as you require.
	"""
	
	def extend_menubar(self, menuuri, menubar, **kargs):
		"""
		Called when a menu bar is being set up. Use this function
		to add new menuitems and submenus.
		
		@param menuuri: uri of the menu bar to be extended.
		@type menuuri: str
		@param menu: GTK+ menubar object.
		@type menu: gtk.MenuBar
		@return: return True if new items have been added.
		@rtype: bool
		"""

	def extend_menu(self, menuuri, menu, **kargs):
		"""
		Called when a popup menu is being set up. Use this function
		to add new menuitems and submenus to a popup menu.
		
		@param menuuri: uri of the menu to be extended.
		@type menuuri: str
		@param menu: GTK+ menu object.
		@type menu: gtk.Menu
		@return: return True if new items have been added.
		@rtype: bool
		"""

	def extend_toolbar(self, toolbaruri, toolbar, **kargs):
		"""
		Called when a tool bar is being set up. Use this function
		to add new buttons.
		
		@param toolbaruri: uri of the toolbar to be extended.
		@type toolbaruri: str
		@param toolbar: GTK+ toolbar object.
		@type toolbar: gtk.Toolbar
		@return: return True if new items have been added.
		@rtype: bool
		"""

	def extend_box(boxuri, box, **kargs):
		"""
		Called when a box is being set up. Use this function
		to add new controls and items to the box.
		
		@param boxuri: uri of the box to be extended.
		@type boxuri: str
		@param box: GTK+ box object.
		@type box: gtk.Box
		@return: return True if new items have been added.
		@rtype: bool
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
