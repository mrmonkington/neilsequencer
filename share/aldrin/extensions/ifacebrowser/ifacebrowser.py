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
A view that allows browsing available extension interfaces and documentation.

This module can also be executed standalone.
"""

if __name__ == '__main__':
	import sys
	sys.path.append('../..')
	import interface
else:
	import aldrin.interface
	from aldrin.interface import IExtension, IUIBuilder
	from aldrin import interface
	

import gtk
import gobject
import os
import inspect

MARGIN = 6

class InterfaceBrowser(gtk.Dialog):
	def __init__(self, exthost):
		gtk.Dialog.__init__(self,
			"Extension Interface Browser")
		self.resize(500,400)
		self.ifacestore = gtk.TreeStore(gtk.gdk.Pixbuf, str, gobject.TYPE_PYOBJECT)
		self.ifacelist = gtk.TreeView(self.ifacestore)
		self.ifacelist.set_property('headers-visible', False)
		column = gtk.TreeViewColumn("Item")
		cell = gtk.CellRendererPixbuf()
		column.pack_start(cell, False)
		column.set_attributes(cell, pixbuf=0)
		cell = gtk.CellRendererText()
		column.pack_start(cell, True)
		column.set_attributes(cell, markup=1)
		self.ifacelist.append_column(column)
		
		def resolve_path(path):
			if exthost:
				return exthost.resolve_path(path)
			else:
				return path
		icon_iface = gtk.gdk.pixbuf_new_from_file(resolve_path('interface.png'))
		icon_method = gtk.gdk.pixbuf_new_from_file(resolve_path('method.png'))
		icon_class = gtk.gdk.pixbuf_new_from_file(resolve_path('class.png'))
		self.desc = gtk.Label()
		self.desc.set_line_wrap(True)
		#~ self.desc.set_size_request(150,-1)
		self.desc.set_justify(gtk.JUSTIFY_FILL)
		self.desc.set_selectable(True)
		self.desc.set_alignment(0,0)

		rootnode = self.ifacestore.append(None, [None,"<b>Interfaces</b>",None])
		for name in dir(interface):
			element = getattr(interface,name)
			if inspect.isclass(element) and issubclass(element,interface.Interface):
				classname = element.__name__
				ifacenode = self.ifacestore.append(rootnode, [icon_iface, "<i>%s</i>" % classname, element])
				for ename in dir(element):
					eelement = getattr(element,ename)
					if not ename.startswith('_') and inspect.ismethod(eelement):
						methodnode = self.ifacestore.append(ifacenode, [icon_method, ename, eelement])
		hsizer = gtk.HPaned()
		hsizer.set_border_width(MARGIN)
		scrollwin = gtk.ScrolledWindow()
		scrollwin.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		scrollwin.set_shadow_type(gtk.SHADOW_IN)
		scrollwin.add(self.ifacelist)
		hsizer.pack1(scrollwin)
		hsizer.pack2(self.desc)
		hsizer.set_position(150)
		self.vbox.add(hsizer)
		self.ifacelist.get_selection().connect('changed', self.on_ifacelist_sel_changed)
		
	def cleanup_docstr(self, docstr):
		"""
		returns a cleaned up epydoc-compatible docstring suitable
		for output in an editbox.
		"""
		def wrap(lines):
			if lines and not lines[0]:
				del lines[0]
			if lines and not lines[-1]:
				del lines[-1]
			s = ''
			for l in lines:
				l = l.strip()
				if s and not l:
					s += '\n'
				elif l:
					if s:
						s += ' '
					s += l
			return s
		
		import re
		rcmp = re.compile(r'^\@([\s\w]+):(.*)$')
		desc = None
		cb = []
		targets = []
		ctarg = None
		for line in docstr.split('\n'):
			line = line.strip()
			m = rcmp.match(line)
			if m:
				if not desc:
					desc = wrap(cb)
					cb = []
				if ctarg:
					targets.append((ctarg,wrap(cb)))
					ctarg = None
					cb = []
				t,line = m.group(1), m.group(2)					
				ctarg = t
			cb.append(line)
		if not desc:
			desc = wrap(cb)
		if ctarg:
			targets.append((ctarg,wrap(cb)))
		params = {}
		pcmp1 = re.compile(r'^\s*(\w+)\s+(\w+)\s*$')
		for ptarg,pdesc in targets:
			m = pcmp1.match(ptarg)
			if not m:
				params[ptarg] = pdesc
			else:
				params[(m.group(1),m.group(2))] = pdesc
		return desc,params
		
	def on_ifacelist_sel_changed(self, selection):
		"""
		Handles changes in the treeview. Updates meta information.
		"""
		self.desc.set_markup('<i>Select an interface or a method in the treeview to see a description.</i>')
		store, rows = selection.get_selected()
		if not rows:
			return
		obj = store.get(rows, 2)[0]
		if not obj:
			return
		markup = ''
		keyw = 'weight="bold"'
		paramc = 'style="italic"'
		funcc = 'underline="single"'
		defvc = 'style="italic"'
		if inspect.ismethod(obj):
			docstr = ""
			if hasattr(obj, '__doc__'):
				docstr = obj.__doc__
			markup += '<span %s>def</span> ' % keyw
			args,varargs,varkw,defaults = inspect.getargspec(obj)
			if not defaults:
				defaults = []
			if len(defaults) < args:
				defaults = [None]*(len(args)-len(defaults)) + list(defaults)
			markup += '<span %s>%s</span>(' % (funcc, obj.im_func.func_name)
			index = 0
			for arg,df in zip(args,defaults):
				if index:
					markup += ', '
				markup += arg
				if df != None:
					markup += '<span %s>=%r</span>' % (defvc, df)
				index += 1
			markup += ')\n\n'
		elif inspect.isclass(obj) and issubclass(obj, interface.Interface):
			docstr = ""
			if hasattr(obj, '__doc__'):
				docstr = obj.__doc__
			markup += '<span %s>interface</span> %s:\n\n' % (keyw, obj.__name__)
		else:
			return
		desc,params = self.cleanup_docstr(docstr)
		if desc:
			markup += '%s\n\n' % desc
		if params:
			args,varargs,varkw,defaults = inspect.getargspec(obj)
			for arg in args[1:]:
				desc = params.get(('param',arg),'No description.')
				typedesc = params.get(('type',arg),'Unknown')
				markup += '<span %s>%s (%s):</span>\t%s\n' % (paramc, arg, typedesc, desc)
			desc = params.get('return','No description.')
			typedesc = params.get('rtype','Unknown')
			markup += '<span %s>returns (%s):</span>\t%s\n' % (paramc, typedesc, desc)
		self.desc.set_markup(markup)

if __name__ != '__main__': # extension mode
	class Extension(IExtension, IUIBuilder):
		__uri__ = '@zzub.org/extension/ifacebrowser;1'
		SERVICE_URI = '@zzub.org/extension/ifacebrowser/uibuilder'
		
		def realize(self, extensionhost):
			# store the host reference
			self.exthost = extensionhost
			# get the extension manager
			extman = extensionhost.get_extension_manager()
			# register our service
			extman.register_service(self.SERVICE_URI, self)
			# add our service to the ui builder class list
			extman.add_service_class(aldrin.interface.CLASS_UI_BUILDER, self.SERVICE_URI)
			# create browser
			self.browser = InterfaceBrowser(self.exthost)
			self.browser.connect('delete-event', self.browser.hide_on_delete)

		# IUIBuilder.extend_menu
		def extend_menu(self, menuuri, menu, **kargs):
			if menuuri == aldrin.interface.UIOBJECT_MAIN_MENU_TOOLS:
				# create a menu item
				item = gtk.MenuItem(label="Show _Interface Browser")
				# connect the menu item to our handler
				item.connect('activate', self.on_menuitem_activate)
				# append the item to the menu
				menu.append(item)
				return True
				
		def on_menuitem_activate(self, widget):
			self.browser.show_all()
		
		def finalize(self):
			# get rid of the host reference
			del self.exthost
			
else: # running standalone
	browser = InterfaceBrowser(None)
	browser.connect('destroy', lambda widget: gtk.main_quit())
	browser.show_all()
	gtk.main()
