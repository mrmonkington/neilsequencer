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

import gtk
import gobject
import os
import inspect

import aldrin.com as com
import aldrin.utils as utils

import aldrin.contextlog as contextlog

import pango

MARGIN = 6

class PackageBrowserDialog(gtk.Dialog):
	__aldrin__ = dict(
		id = 'aldrin.componentbrowser.dialog',
		singleton = True,
	)
	
	def __init__(self, hide_on_delete=True):
		gtk.Dialog.__init__(self,
			"Component Browser")
		if hide_on_delete:
			self.connect('delete-event', self.hide_on_delete)
		self.resize(600,500)
		#self.ifacestore = gtk.TreeStore(gtk.gdk.Pixbuf, str, gobject.TYPE_PYOBJECT)
		self.ifacestore = gtk.TreeStore(str, gobject.TYPE_PYOBJECT)
		self.ifacelist = gtk.TreeView(self.ifacestore)
		self.ifacelist.set_property('headers-visible', False)
		column = gtk.TreeViewColumn("Item")
		#~ cell = gtk.CellRendererPixbuf()
		#~ column.pack_start(cell, False)
		#~ column.set_attributes(cell, pixbuf=0)
		cell = gtk.CellRendererText()
		column.pack_start(cell, True)
		column.set_attributes(cell, markup=0)
		self.ifacelist.append_column(column)
		
		def resolve_path(path):
			if exthost:
				return exthost.resolve_path(path)
			else:
				return path
		self.desc = gtk.TextView()
		self.desc.set_wrap_mode(gtk.WRAP_WORD)
		self.desc.set_editable(False)
		#self.desc.set_justification(gtk.JUSTIFY_FILL)
		textbuffer = self.desc.get_buffer()
		textbuffer.create_tag("i", style=pango.STYLE_ITALIC)
		textbuffer.create_tag("u", underline=pango.UNDERLINE_SINGLE)
		textbuffer.create_tag("b", weight=pango.WEIGHT_BOLD)

		rootnode = self.ifacestore.append(None, ["<b>Aldrin Components</b>",None])
		packagenode = self.ifacestore.append(rootnode, ["<b>By Modules</b>",None])
		pkgnodes = {}
		pkgnodes['(unknown)'] = self.ifacestore.append(packagenode, ["<i>(unknown)</i>", None])
		for name in sorted(com.get_packages()):
			pkgnodes[name] = self.ifacestore.append(packagenode, ["<b>module</b> <i>%s</i>" % name, None])
		categorynode = self.ifacestore.append(rootnode, ["<b>By Categories</b>",None])
		catnodes = {}
		catnodes['(unknown)'] = self.ifacestore.append(categorynode, ["<i>(unknown)</i>", None])
		for name in sorted(com.get_categories()):
			catnodes[name] = self.ifacestore.append(categorynode, ["<b>category</b> <i>%s</i>" % name, None])
		allnode = self.ifacestore.append(rootnode, ["<b>By Alphabet</b>",None])
		def create_classnode(parent, metainfo):
			element = metainfo.get('classobj',None)			
			if not element:
				return
			classname = element.__aldrin__['id']
			ifacenode = self.ifacestore.append(parent, ["<b>component</b> %s" % classname, element])
			for ename in dir(element):
				eelement = getattr(element,ename)
				if not ename.startswith('_') and inspect.ismethod(eelement):
					methodnode = self.ifacestore.append(ifacenode, ["<b>method</b> %s" % ename, eelement])
		for name in sorted(com.get_factories()):
			metainfo = com.get_factories()[name]
			create_classnode(allnode, metainfo)
			modulename = metainfo.get('modulename', None) or '(unknown)'
			if modulename in pkgnodes:
				create_classnode(pkgnodes[modulename], metainfo)
			for category in metainfo.get('categories', []):
				if category in catnodes:
					create_classnode(catnodes[category], metainfo)
		hsizer = gtk.HPaned()
		hsizer.set_border_width(MARGIN)
		scrollwin = gtk.ScrolledWindow()
		scrollwin.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		scrollwin.set_shadow_type(gtk.SHADOW_IN)
		scrollwin.add(self.ifacelist)
		hsizer.pack1(scrollwin)
		scrollwin = gtk.ScrolledWindow()
		scrollwin.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		scrollwin.set_shadow_type(gtk.SHADOW_IN)
		scrollwin.add(self.desc)
		hsizer.pack2(scrollwin)
		hsizer.set_position(300)
		self.vbox.add(hsizer)
		self.ifacelist.get_selection().connect('changed', self.on_ifacelist_sel_changed)
		self.ifacelist.connect('row-activated', self.on_ifacelist_row_activated)
		self.on_ifacelist_sel_changed(self.ifacelist.get_selection())
		
	def cleanup_docstr(self, docstr):
		"""
		returns a cleaned up epydoc-compatible docstring suitable
		for output in an editbox.
		"""
		assert docstr != None
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
		
	def on_ifacelist_row_activated(self, widget, path, column):
		store, rows = widget.get_selection().get_selected()
		if not rows:
			return
		obj = store.get(rows, 1)[0]
		if not obj:
			return
		try:
			filepath = inspect.getsourcefile(obj)
			source, line = inspect.getsourcelines(obj)
		except TypeError:
			return
		os.spawnlp(os.P_NOWAIT, 'scite', 'scite', '-open:'+filepath, '-goto:'+str(line))
		
	def on_ifacelist_sel_changed(self, selection):
		"""
		Handles changes in the treeview. Updates meta information.
		"""
		buffer = self.desc.get_buffer()
		buffer.set_text("")
		iter = buffer.get_iter_at_offset(0)		
		def insert(a,b=None):
			if b:
				buffer.insert_with_tags_by_name(iter,a,b)
			else:
				buffer.insert(iter, a)
		store, rows = selection.get_selected()
		if not rows:
			insert("Select a component or method to see a description.", 'i')
			return
		obj = store.get(rows, 1)[0]
		if not obj:
			insert("Select a component or method to see a description.", 'i')
			return
		keyw = 'b'
		paramc = 'i'
		funcc = 'u'
		defvc = 'i'
		docstr = ""
		filepath = ''
		source, line = [],1
		try:
			filepath = inspect.getsourcefile(obj)
			source, line = inspect.getsourcelines(obj)
		except TypeError:
			pass
		if filepath:
			# print reference to stdout so devs can click the line from
			# within SciTE.
			contextlog.clean_next_line()
			print "%s:%s:%r" % (filepath, line, obj)
		insert('File "%s", Line %s\n\n' % (filepath, line), 'i')
		if inspect.ismethod(obj):
			docstr = ""
			if hasattr(obj, '__doc__'):
				docstr = obj.__doc__ or ""
			insert("def ", keyw)
			args,varargs,varkw,defaults = inspect.getargspec(obj)
			if not defaults:
				defaults = []
			if len(defaults) < args:
				defaults = [None]*(len(args)-len(defaults)) + list(defaults)
			insert(obj.im_func.func_name, funcc)
			insert("(")
			index = 0
			for arg,df in zip(args,defaults):
				if index:
					insert(', ')
				insert(arg)
				if df != None:
					insert('=%r' % df, defvc)
				index += 1
			insert(')\n\n')
		elif inspect.isclass(obj):
			docstr = ""
			if hasattr(obj, '__doc__'):
				docstr = obj.__doc__ or ""
			insert('class ', keyw)
			insert(obj.__name__)
			insert(':\n\n')
		else:
			return
		desc,params = self.cleanup_docstr(docstr)
		if desc:
			insert('%s\n\n' % desc)
		if params:
			args,varargs,varkw,defaults = inspect.getargspec(obj)
			for arg in args[1:]:
				desc = params.get(('param',arg),'No description.')
				typedesc = params.get(('type',arg),'Unknown')
				insert('%s (%s):' % (arg, typedesc), paramc)
				insert('\t%s\n' % desc)
			desc = params.get('return','No description.')
			typedesc = params.get('rtype','Unknown')
			insert('returns (%s):' % typedesc, paramc)
			insert('\t%s\n' % desc)

class PackageBrowserMenuItem:
	__aldrin__ = dict(
		id = 'aldrin.componentbrowser.menuitem',
		singleton = True,
		categories = [
			'menuitem.tool'
		],
	)
	
	def __init__(self, menu):
		# create a menu item
		item = gtk.MenuItem(label="Show _Component Browser")
		# connect the menu item to our handler
		item.connect('activate', self.on_menuitem_activate)
		# append the item to the menu
		menu.append(item)
		
	def on_menuitem_activate(self, widget):
		browser = com.get('aldrin.componentbrowser.dialog')
		browser.show_all()

__aldrin__ = dict(
	classes = [
		PackageBrowserDialog,
		PackageBrowserMenuItem,
	],
)

if __name__ == '__main__': # extension mode
	import contextlog
	contextlog.init()
	com.load_packages()
	# running standalone
	browser = com.get('aldrin.componentbrowser.dialog', False)
	browser.connect('destroy', lambda widget: gtk.main_quit())
	browser.show_all()
	gtk.main()
