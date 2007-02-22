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
Extension manager for aldrin. Manages extensions to functionality
and the user interface.
"""

from gtkimport import gtk
import interface
from interface import UnknownServiceException
import imp
import inspect
import utils
from utils import prepstr, error
import sys
import config
import os
from xml.dom.minidom import parse

class RealizeExtensionException(Exception):
	pass

class ExtensionHost(interface.IExtensionHost):
	"""
	Acts as an agent between extension and manager and
	stores metainformation. An extension host
	object will be passed to extension classes on 
	initialization, which can be queried for subsequent
	services.
	"""
	def __init__(self, basepath, element):
		""""
		Loads metadata for this extension.
		
		@param basepath: Base directory of the extension.
		@type basepath: str
		@param element: XML DOM element "extension"
		@type element: xml.Element
		"""
		self.basepath = basepath
		self.module = None
		self.instance = None
		for attrname in ('uri','name','description','author','minversion','maxversion','version'):
			setattr(self, attrname, str(element.getAttribute(attrname)))
		assert self.uri, "uri attribute is empty."
		assert self.name, "name attribute is empty."
		assert self.version, "version attribute is empty."
		modules = element.getElementsByTagName("module")
		assert len(modules) == 1, "Need exactly one module element to load extension."
		if modules[0].hasAttribute("language"):
			assert modules[0].getAttribute("language") == "python", "Wrong module language. Need python."
		self.modulepath = os.path.join(self.basepath,str(modules[0].getAttribute("path")))
		assert os.path.isfile(self.modulepath), "can't find extension module: %s" % self.modulepath

	def get_extension_manager(self):
		"""
		Returns the extension manager object.
		
		@return: Extension manager object.
		@rtype: {IExtensionManager}
		"""
		return get_extension_manager()
		
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
		return os.path.abspath(os.path.normpath(os.path.join(self.basepath, path)))

	def realize(self):
		"""
		Instantiates the extension.
		"""
		if not self.module:
			self.module = sys.modules.get(self.uri,None)
		if not self.module:
			moduledir = os.path.dirname(self.modulepath)
			modulename = os.path.splitext(os.path.basename(self.modulepath))[0]
			sys_path = sys.path[:]
			try:
				sys.path = [moduledir]
				self.module = __import__(modulename)				
			finally:
				sys.path = sys_path
			# remove module entry in sys.modules and add
			# a new one instead
			for k,v in sys.modules.iteritems():
				if v == self.module:
					del sys.modules[k]
					break
			sys.modules[self.uri] = self.module
		if not self.instance:
			for name in dir(self.module):
				element = getattr(self.module,name)
				if inspect.isclass(element) and \
					element != interface.IExtension and \
					issubclass(element,interface.IExtension) and \
					getattr(element,'__uri__') == self.uri:
					self.instance = element()
					self.instance.realize(self)
		assert self.instance, "Extension class for %s not found." % self.uri
		return self.instance

class UIMessage(interface.IUIMessage):
	"""
	Interface for displaying info or error messages or asking questions.
	"""
	def error(self, message):
		"""
		Displays a modal messagebox configured to show an error message.
		
		@param message: The message to display (you can use markup).
		@type message: str
		"""
		return utils.error(self.parent.get_toplevel(), message)
		
	def message(self, message):
		"""
		Displays a modal messagebox configured to show an informative message.
		
		@param message: The message to display (you can use markup).
		@type message: str
		"""
		return utils.message(self.parent.get_toplevel(), message)

	def warning(self, message):
		"""
		Displays a modal messagebox configured to show a warning message.
		
		@param message: The message to display (you can use markup).
		@type message: str
		"""
		return utils.warning(self.parent.get_toplevel(), message)

	def question(self, message):
		"""
		Displays a modal messagebox configured to show a question.
		
		@param message: The message to display (you can use markup).
		@type message: str
		@return: either gtk.RESPONSE_YES or gtk.RESPONSE_NO
		@rtype: int
		"""
		return utils.question(self.parent.get_toplevel(), message, False)

	def choice(self, message):
		"""
		Displays a modal messagebox configured to show a choice. It
		is similar to question, except that it also allows to cancel.
		
		@param message: The message to display (you can use markup).
		@type message: str
		@return: either gtk.RESPONSE_YES, gtk.RESPONSE_NO or gtk.RESPONSE_CANCEL
		@rtype: int
		"""
		return utils.question(self.parent.get_toplevel(), message, True)

class ExtensionManager(interface.IExtensionManager):
	"""
	UI extension manager. Enumerates extensions, creates 
	extension hosts and returns services.
	"""
	def __init__(self):
		self.extensions = []
		self.services = {}
		self.classes = {}
		if not 'aldrin' in sys.modules:
			m = imp.new_module("aldrin")
			m.interface = interface
			sys.modules['aldrin'] = m
		sys.modules['aldrin.interface'] = interface
		uris = []
		extpaths = [
			os.path.join(config.get_config().get_settings_folder(),'extensions'),
			utils.filepath('extensions')
		]
		for extpath in extpaths:
			if os.path.isdir(extpath):
				for name in os.listdir(extpath):
					fullpath = os.path.join(extpath,name)
					if os.path.isdir(fullpath):
						manifestpath = os.path.join(fullpath,'manifest.xml')
						if os.path.isfile(manifestpath):
							root = parse(manifestpath).documentElement
							assert root.tagName == 'aldrin', "Invalid root node name."
							extensions = root.getElementsByTagName('extension')
							for extension in extensions:
								uri = extension.getAttribute("uri")
								if uri in uris:
									print >> sys.stderr, "duplicate extension URI: %s" % uri
								else:
									uris.append(uri)								
									host = ExtensionHost(fullpath, extension)
									self.extensions.append(host)
									
	def get_service(self, uri):
		"""
		returns an object providing the service
		requested by the uri or raises an
		UnknownServiceException object.
		
		@param uri: The uri of the service requested.
		@type uri: str
		"""
		svc = self.services.get(uri,None)
		if svc:
			return svc
		raise UnknownServiceException, uri
		
	def register_service(self, uri, instance, iface = None):
		"""
		Registers a service for access by extensions.
		
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
		if not iface:
			ifaces = instance.get_interfaces()
		else:
			ifaces = [iface]
		self.services[uri] = instance.create_protected_proxy(ifaces)
		
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
		cats = self.classes.get(classuri,[])
		if not uri in cats:
			cats.append(uri)
		self.classes[classuri] = cats
		
	def enumerate_services(self, classuri):
		"""
		Returns a list of services registered for a specific class.
		
		@param classui: class uri
		@type classui: str
		@return: list of service objects.
		@rtype: object list
		"""
		return [self.get_service(uri) for uri in self.classes.get(classuri,[])]
		
	def realize_extensions(self, parent):
		"""
		Instantiates and realizes all enabled extensions.
		
		@param parent: Parent window.
		@type parent: wx.Window
		"""
		self.uimessage = UIMessage()
		self.uimessage.parent = parent
		self.register_service(interface.SERVICE_MESSAGE, self.uimessage)
		uris = config.get_config().get_enabled_extensions()
		for ext in self.extensions:
			if ext.uri in uris:
				print "realizing extension %s..." % ext.uri
				try:
					ext.realize()
				except:
					import traceback
					traceback.print_exc()
					error(parent, 'An error occurred while loading extension "%s".' % prepstr(ext.name))

extman = None

def get_extension_manager():
	"""
	Returns the global extension manager instance.
	"""
	global extman
	if not extman:
		extman = ExtensionManager()
	return extman

ExtensionManager()

__all__ = [
	'ExtensionManager',
	'ExtensionHost',
	'get_extension_manager',
]
