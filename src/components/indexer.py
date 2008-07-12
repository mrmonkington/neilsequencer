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

import fnmatch, os, sys
from aldrin.utils import prepstr
import zzub
from xml.dom.minidom import parse
import aldrin.com as com

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
	def __init__(self):
		self.name = ""
		self.children = []
		
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

def parse_index(player, filepath, root = None):
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
		parse_index(com.get('aldrin.core.player'), com.get('aldrin.core.config').get_index_path(), self)

__aldrin__ = dict(
	classes = [
		PluginTreeIndex,
	],
)

if __name__ == '__main__':
	from config import get_plugin_aliases, get_plugin_blacklist
	import utils, zzub, os
	aliases = {}
	player = zzub.Player()
	# load blacklist file and add blacklist entries
	for name in get_plugin_blacklist():
		player.blacklist_plugin(name)
	# load aliases file and add aliases
	for name,uri in get_plugin_aliases():
		aliases[name]=uri
		player.add_plugin_alias(name, uri)
	pluginpath = utils.filepath('/usr/lib/zzub') + os.sep
	print "pluginpath is '%s'" % pluginpath
	player.add_plugin_path(pluginpath)
	player.initialize(44100)
	node = parse_index(player,'./index.xml')
	print node.get_xml()
