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
A view that allows browsing available extension interfaces and documentation.

This module can also be executed standalone.
"""

from gtkimport import gtk
import gobject
import os
import inspect

from aldrincom import com
import utils

MARGIN = 6

class PackageBrowserDialog(gtk.Dialog):
	__aldrin__ = dict(
		id = 'aldrin.pkgbrowser.dialog',
		singleton = True,
	)
	
	def __init__(self, exthost):
		gtk.Dialog.__init__(self,
			"Package Browser")
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
		icon_iface = gtk.gdk.pixbuf_new_from_file(utils.filepath('res/interface.png'))
		icon_method = gtk.gdk.pixbuf_new_from_file(utils.filepath('res/method.png'))
		icon_class = gtk.gdk.pixbuf_new_from_file(utils.filepath('res/class.png'))
		self.desc = gtk.Label()
		self.desc.set_line_wrap(True)
		#~ self.desc.set_size_request(150,-1)
		self.desc.set_justify(gtk.JUSTIFY_FILL)
		self.desc.set_selectable(True)
		self.desc.set_alignment(0,0)

		rootnode = self.ifacestore.append(None, [None,"<b>Packages</b>",None])
		for name in sorted(com.factories):
			element = com.factories[name]
			classname = element.__aldrin__['id']
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
		elif inspect.isclass(obj):
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

class PackageBrowserMenuItem:
	__aldrin__ = dict(
		id = 'aldrin.pkgbrowser.menuitem',
		singleton = True,
		categories = [
			'menuitem.tool'
		],
	)
	
	def __init__(self, menu):
		# create a menu item
		item = gtk.MenuItem(label="Show _Package Browser")
		# connect the menu item to our handler
		item.connect('activate', self.on_menuitem_activate)
		# append the item to the menu
		menu.append(item)
		
	def on_menuitem_activate(self, widget):
		browser = com.get('aldrin.pkgbrowser.dialog', None)
		browser.connect('delete-event', browser.hide_on_delete)
		browser.show_all()

__aldrin__ = dict(
	classes = [
		PackageBrowserDialog,
		PackageBrowserMenuItem,
	],
	id = 'aldrin.core.dialog.pkgbrowser',
)

if __name__ == '__main__': # extension mode
	com.load_packages()
	# running standalone
	browser = PackageBrowserDialog(None)
	browser.connect('destroy', lambda widget: gtk.main_quit())
	browser.show_all()
	gtk.main()
