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
	from aldrin.interface import IExtension, \
							MAINFRAME_SERVICE
	from aldrin import interface
	

import wx
import os
import inspect

class InterfaceBrowser(wx.Dialog):
	def __init__(self, exthost, *args, **kargs):
		kargs['style'] = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER
		wx.Dialog.__init__(self, *args, **kargs)
		self.SetMinSize((500,400))
		self.SetTitle("Extension Interface Browser")
		self.ifacelist = wx.TreeCtrl(self, -1, style=wx.SUNKEN_BORDER | wx.TR_NO_LINES | wx.TR_HIDE_ROOT | wx.TR_HAS_BUTTONS)
		self.imglist = wx.ImageList(16,16)
		def resolve_path(path):
			if exthost:
				return exthost.resolve_path(path)
			else:
				return path
		idx_iface = self.imglist.Add(wx.Bitmap(resolve_path('interface.png'), wx.BITMAP_TYPE_ANY))
		idx_method = self.imglist.Add(wx.Bitmap(resolve_path('method.png'), wx.BITMAP_TYPE_ANY))
		idx_class = self.imglist.Add(wx.Bitmap(resolve_path('class.png'), wx.BITMAP_TYPE_ANY))
		self.ifacelist.AssignImageList(self.imglist)
		self.desc = wx.TextCtrl(self, -1, style=wx.SUNKEN_BORDER | wx.TE_READONLY | wx.TE_MULTILINE | wx.TE_RICH2 | wx.TE_BESTWRAP)
		rootnode = self.ifacelist.AddRoot("Interfaces")
		for name in dir(interface):
			element = getattr(interface,name)
			if inspect.isclass(element) and issubclass(element,interface.Interface):
				classname = element.__name__
				ifacenode = self.ifacelist.AppendItem(rootnode, classname)
				self.ifacelist.SetItemImage(ifacenode, idx_iface)
				self.ifacelist.SetPyData(ifacenode, element)
				for ename in dir(element):
					eelement = getattr(element,ename)
					if not ename.startswith('_') and inspect.ismethod(eelement):
						methodnode = self.ifacelist.AppendItem(ifacenode, ename)
						self.ifacelist.SetItemImage(methodnode, idx_method)
						self.ifacelist.SetPyData(methodnode, eelement)
		vsizer = wx.BoxSizer(wx.HORIZONTAL)
		vsizer.Add(self.ifacelist, 1, wx.EXPAND|wx.ALL, 5)
		vsizer.Add(self.desc, 1, wx.EXPAND|wx.RIGHT|wx.TOP|wx.BOTTOM, 5)
		self.SetAutoLayout(True)
		self.SetSizerAndFit(vsizer)
		self.Layout()
		wx.EVT_TREE_SEL_CHANGED(self, self.ifacelist.GetId(), self.on_ifacelist_sel_changed)
		
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
		
	def on_ifacelist_sel_changed(self, event):
		"""
		Handles changes in the treeview. Updates meta information.
		
		@param event: Tree event.
		@type event: wx.TreeEvent
		"""
		obj = self.ifacelist.GetPyData(event.GetItem())
		if not obj:
			return
		self.desc.Clear()
		regular = wx.TextAttr('#000000')
		keyw = wx.TextAttr('#000080')
		paramc = wx.TextAttr('#800000')
		funcc = wx.TextAttr('#008080')
		defvc = wx.TextAttr('#808080')
		if inspect.ismethod(obj):
			docstr = ""
			if hasattr(obj, '__doc__'):
				docstr = obj.__doc__
			self.desc.SetDefaultStyle(keyw)
			self.desc.AppendText('def ')
			self.desc.SetDefaultStyle(funcc)
			args,varargs,varkw,defaults = inspect.getargspec(obj)
			if not defaults:
				defaults = []
			if len(defaults) < args:
				defaults = [None]*(len(args)-len(defaults)) + list(defaults)
			self.desc.AppendText(obj.im_func.func_name)
			self.desc.SetDefaultStyle(regular)
			self.desc.AppendText("(")
			index = 0
			for arg,df in zip(args,defaults):
				self.desc.SetDefaultStyle(regular)
				if index:
					self.desc.AppendText(", ")
				self.desc.AppendText(arg)
				if df != None:
					self.desc.SetDefaultStyle(defvc)
					self.desc.AppendText('=%r' % df)
				index += 1
			self.desc.SetDefaultStyle(regular)
			self.desc.AppendText(")\n\n")
		elif inspect.isclass(obj) and issubclass(obj, interface.Interface):
			docstr = ""
			if hasattr(obj, '__doc__'):
				docstr = obj.__doc__
			self.desc.SetDefaultStyle(keyw)
			self.desc.AppendText('interface ')
			self.desc.SetDefaultStyle(regular)
			self.desc.AppendText(obj.__name__ + ':\n\n')			
		else:
			return
		desc,params = self.cleanup_docstr(docstr)
		if desc:
			self.desc.SetDefaultStyle(regular)
			self.desc.AppendText(desc)
			self.desc.AppendText("\n\n")
		if params:
			self.desc.SetDefaultStyle(regular)
			args,varargs,varkw,defaults = inspect.getargspec(obj)
			for arg in args[1:]:
				desc = params.get(('param',arg),'No description.')
				typedesc = params.get(('type',arg),'Unknown')
				self.desc.SetDefaultStyle(paramc)
				self.desc.AppendText("%s (%s):\t" % (arg,typedesc))
				self.desc.SetDefaultStyle(regular)
				self.desc.AppendText("%s\n" % desc)
			desc = params.get('return','No description.')
			typedesc = params.get('rtype','Unknown')
			self.desc.SetDefaultStyle(paramc)
			self.desc.AppendText("returns (%s):\t" % (typedesc))
			self.desc.SetDefaultStyle(regular)
			self.desc.AppendText("%s\n" % desc)

if __name__ != '__main__': # extension mode
	class Extension(IExtension):
		__uri__ = '@zzub.org/extension/ifacebrowser;1'
		
		def realize(self, extensionhost):
			# store the host reference
			self.exthost = extensionhost
			# get the extension manager
			extman = extensionhost.get_extension_manager()
			# get the mainframe
			mainframe = extman.get_service(MAINFRAME_SERVICE)
			parent = mainframe.get_window()
			self.browser = InterfaceBrowser(extensionhost, parent, -1)
			# add a new button to the toolbar
			toolid = mainframe.add_menuitem(
				"Show Interface Browser", 	# label
				"Shows a window which allows to browse interfaces available to extensions.") # description
			# associate a handler
			mainframe.add_click_handler(toolid, self.on_button_click)
			
		def on_button_click(self, event):
			self.browser.Show()
		
		def finalize(self):
			# get rid of the host reference
			del self.exthost
else: # running standalone
	app = wx.App()
	browser = InterfaceBrowser(None, None, -1)
	browser.ShowModal()
