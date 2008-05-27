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

from eventbus import *
import config

DEFAULT_PACKAGES = [
	'router',
]

class Hub:
	def __init__(self):
		self.services = {}
		self.svcfactories = {}
		self.factories = {}
		self.categories = {}
		self.register(config.__aldrin__)
		
	def load_packages(self):
		packages = self.get_service('aldrin.core.config').packages
		for modulename in DEFAULT_PACKAGES + packages:
			print "importing module %s" % modulename
			module_ = __import__(modulename)
			names = modulename.split('.')
			for name in names[1:]:
				module_ = getattr(module_, name)
			self.register(module_.__aldrin__)
					
	def register(self, pkginfo):
		# enumerate class factories
		for class_ in pkginfo.get('classes', []):
			classinfo = class_.__aldrin__
			self.factories[classinfo['id']] = class_
			# register categories
			for category in classinfo.get('categories', []):
				catlist = self.categories.get(category, [])
				catlist.append(class_)
				self.categories[category]= catlist
		# enumerate service factories
		for serviceid,class_ in pkginfo.get('services', {}).iteritems():
			if not self.svcfactories.has_key(serviceid):
				self.svcfactories[serviceid] = class_
					
	def create_instance(self, id):
		class_ = self.factories.get(id, None)
		if not class_:
			return None
		return class_(self)
		
	def get_category_ids(self, category):
		return [class_.__aldrin__['id'] for class_ in self.categories.get(category,[])]
		
	def create_category_instances(self, category):
		return [class_(self) for class_ in self.categories.get(category,[]) if class_]
		
	def get_service(self, serviceid):
		if not serviceid in self.services:
			class_ = self.svcfactories.get(serviceid, None)
			if not class_:
				return None
			self.services[serviceid] = class_(self)
		return self.services.get(serviceid, None)

hub = Hub()

if __name__ == '__main__':
	class MyClass:
		__aldrin__ = dict(
			id = 'aldrin.hub.myclass',
			categories = [
				'uselessclass',
				'uselessclass2',
			]
		)
		
		def __repr__(self):
			return '<%s>' % repr(self.x)
		
		def __init__(self, hub):
			import random
			self.x = random.random()
	
	pkginfo = dict(
		classes = [
			MyClass,
		],
		services = {
			'randomnumber' : MyClass,
		},
	)
	hub.register(pkginfo)
	print hub.get_service('randomnumber').x
	print hub.create_instance('aldrin.hub.myclass').x
	print hub.get_service('randomnumber').x
	print hub.create_instance('aldrin.hub.myclass').x
	print hub.get_service('randomnumber').x
	print hub.create_instance('aldrin.hub.myclass').x
	print hub.create_category_instances('uselessclass')
	