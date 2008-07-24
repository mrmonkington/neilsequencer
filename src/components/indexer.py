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
Reads and writes index files and directory structures.
"""

if __name__ == '__main__':
	import os,sys
	sys.path.insert(0, '/home/paniq/devel/aldrin/src')

import fnmatch, glob, os, sys
from aldrin.utils import prepstr
import zzub
from xml.dom.minidom import parse
import aldrin.com as com
import aldrin.pathconfig as pathconfig
from ConfigParser import ConfigParser

DIRECTORIES = [
	('Samplers', '', ['Tracker','Sampler']),
	('Synthesizers', '', ['Synth']),
	('-', '', []),
	('Generators', '', ['Generator']),
	('Effects', '', ['Effect']),
	('Controllers', '', ['Controller']),
	('-', '', []),
	('DSSI Plugins', '', ['DSSIPlugin']),
	('LADSPA Plugins', '', ['LADSPAPlugin']),
	('Psycle Plugins', '', ['PsyclePlugin']),
	('-', '', []),
	('Unsorted', '', ['']),
]

class BaseItem:
	def is_directory(self):
		return isinstance(self, Directory)
	def is_separator(self):
		return isinstance(self, Separator)
	def is_reference(self):
		return isinstance(self, Reference)

class Separator(BaseItem):
	"""
	Separates plugins.
	"""
	def get_xml(self):
		return "<Separator/>\n"

class Reference(BaseItem):
	"""
	Resembles a plugin reference.
	"""
	name = ""
	uri = ""
	icon = ""
	pluginloader = None
	def get_xml(self):
		return '<Reference name="%s" uri="%s"/>\n' % (self.name, self.uri)

class Directory(BaseItem):
	"""
	Resembles a folder or a submenu containing plugin references.
	"""
	def __init__(self, name = '', children = []):
		self.name = name
		self.children = list(children)
		
	def sort_children(self):
		def cmp_items(a,b):
			if a.is_directory() and not b.is_directory():
				return -1
			elif not a.is_directory() and b.is_directory():
				return 1
			return cmp(a.name.lower(), b.name.lower())
		self.children = sorted(self.children, cmp_items)
		
	def is_empty(self):
		for child in self.children:
			if isinstance(child,Reference) and child.pluginloader:
				return False
			elif isinstance(child,Directory) and not child.is_empty():
				return False
		return True
		
	def get_xml(self):
		xml = '<Directory name="%s">\n' % self.name
		for child in self.children:
			subxml = child.get_xml()
			xml += '\n'.join(['\t'+n for n in subxml.split('\n')])
		xml += '</Directory>\n'
		return xml

def parse_index_old(player, filepath, root = None):
	"""
	Parses an index.xml file and returns a plugin directory structure.
	
	@param filepath: Path to index.xml
	@type filepath: str
	"""
	def fill_tree(node, element, special_nodes):
		for item in element.childNodes:
			if not item.nodeType == item.ELEMENT_NODE:
				continue
			if item.tagName == "group":
				subnode = Directory()
				label = item.getAttribute("label")
				ref = item.getAttribute("ref")
				if ref == "__unsorted__":
					special_nodes['unsorted'] = subnode
				subnode.name = label
				node.children.append(subnode)
				fill_tree(subnode, item, special_nodes)
			elif item.tagName == "separator":
				node.children.append(Separator())
			elif item.tagName == "refs":
				pattern = item.getAttribute("pattern")
				for pl in sorted([uri2plugin[uri] for uri in fnmatch.filter(uri2plugin.keys(),pattern)], \
					cmp_pl_nocase):
					ref = Reference()
					ref.pluginloader = pl
					ref.name = pl.get_name()
					ref.uri = pl.get_uri()
					node.children.append(ref)
					uriused.append(pl.get_uri())
			elif item.tagName == "ref":
				label = item.getAttribute("label")
				uri = item.getAttribute("uri")
				icon = item.getAttribute("icon")
				if uri in uri2plugin:
					if label:
						menuname = label
					else:
						menuname = pl.get_name()
					ref = Reference()
					ref.pluginloader = uri2plugin[uri]
					ref.name = menuname
					ref.icon = icon
					ref.uri = ref.pluginloader.get_uri()
					node.children.append(ref)
					uriused.append(ref.uri)
				elif label:
					ref = Reference()
					ref.name = label
					ref.icon = icon
					ref.uri = uri
					node.children.append(ref)
	name2uri = {}
	uri2plugin = {}
	uriused = []
	for pl in player.get_pluginloader_list():
		if not (pl.get_flags() & zzub.zzub_plugin_flag_is_root):
			name = pl.get_name()
			uri = pl.get_uri()
			name2uri[name] = uri
			uri2plugin[uri] = pl
	def cmp_pl_nocase(a,b):
		if a.get_name().lower() == b.get_name().lower():
			return 0
		if a.get_name().lower() < b.get_name().lower():
			return -1
		return 1
	special_nodes = {}
	if not root:
		root = Directory()
	xmldoc = parse(filepath)
	assert xmldoc.documentElement.tagName == "pluginindex"
	try:
		fill_tree(root, xmldoc.documentElement, special_nodes)
	except StopIteration:
		pass
	unsorted_node = special_nodes.get('unsorted',None)
	if unsorted_node:
		for pl in sorted(uri2plugin.values(),cmp_pl_nocase):
			if pl.get_uri() not in uriused:
				ref = Reference()
				ref.pluginloader = pl
				ref.name = pl.get_name()
				ref.uri = pl.get_uri()
				unsorted_node.children.append(ref)
	return root

class PluginTreeIndex(Directory):
	__aldrin__ = dict(
		id = 'aldrin.core.plugintree',
		singleton = True,
		categories = [
		],
	)
	
	def __init__(self):
		Directory.__init__(self)
		#parse_index(com.get('aldrin.core.player'), com.get('aldrin.core.config').get_index_path(), self)
		player = com.get('aldrin.core.player')
		self.name2uri = {}
		self.uri2plugin = {}
		self.categories = {}
		for pl in player.get_pluginloader_list():
			if not (pl.get_flags() & zzub.zzub_plugin_flag_is_root):
				name = pl.get_name()
				uri = pl.get_uri()
				self.name2uri[name] = uri
				self.uri2plugin[uri] = pl
		self.parse_indices()
		
	def add_reference(self, cats, ref):
		"""
		add a reference to given categories
		"""
		for cat in cats:
			if not cat in self.categories:
				print "unknown index category:",cat
		cats = [cat for cat in cats if cat in self.categories]
		if not cats:
			cats = ['']
		for cat in cats:
			self.categories[cat].children.append(ref)
		
	def parse_indices(self):
		"""
		Searches for index files in a list of paths and builds a plugin directory structure.
		"""
		self.children = []
		self.categories = {}
		for label,icon,cats in DIRECTORIES:
			if label == '-':
				self.children.append(Separator())
				continue
			else:
				d = Directory(label)
				d.icon = icon
				for cat in cats:
					self.categories[cat] = d
				self.children.append(d)
		uriused = []
		paths = pathconfig.path_cfg.get_paths('index')
		special_nodes = {}
		for path in paths:
			for filename in glob.glob(path + '/*.aldrin-index'):
				SECTION = "Aldrin Plugin"
				cfg = ConfigParser()
				cfg.read([filename])
				cats = []
				icon = ''
				label = ''
				uri = ''
				pattern = ''
				if cfg.has_option(SECTION, 'Categories'):
					cats = cfg.get(SECTION, 'Categories').strip(';').split(';')
				if cfg.has_option(SECTION, 'Icon'):
					icon = cfg.get(SECTION, 'Icon')
				if cfg.has_option(SECTION, 'Label'):
					label = cfg.get(SECTION, 'Label')
				if cfg.has_option(SECTION, 'URI'):
					uri = cfg.get(SECTION, 'URI')
				if cfg.has_option(SECTION, 'URIPattern'):
					pattern = cfg.get(SECTION, 'URIPattern')
				if pattern:
					for pl in [self.uri2plugin[uri] for uri in fnmatch.filter(self.uri2plugin.keys(),pattern)]:
						ref = Reference()
						ref.pluginloader = pl
						ref.name = pl.get_name()
						ref.uri = pl.get_uri()
						self.add_reference(cats, ref)
						uriused.append(pl.get_uri())
				elif uri in self.uri2plugin: # plugin exists
					menuname = label or pl.get_name()
					ref = Reference()
					ref.pluginloader = self.uri2plugin[uri]
					ref.name = menuname
					ref.icon = icon
					ref.uri = ref.pluginloader.get_uri()
					self.add_reference(cats, ref)
					uriused.append(ref.uri)
				elif label: # if we have a label
					ref = Reference()
					ref.name = label
					ref.icon = icon
					ref.uri = uri
					self.add_reference(cats, ref)
		# add plugins not referenced
		for pl in self.uri2plugin.values():
			if pl.get_uri() not in uriused:
				ref = Reference()
				ref.pluginloader = pl
				ref.name = pl.get_name()
				ref.uri = pl.get_uri()
				self.add_reference([], ref)
		for child in self.children:
			if child.is_directory():
				child.sort_children()

__aldrin__ = dict(
	classes = [
		PluginTreeIndex,
	],
)

if __name__ == '__main__':
	com.init()
	d = com.get('aldrin.core.plugintree')

