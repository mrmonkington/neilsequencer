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

import config

DEFAULT_PACKAGES = [
	'+eventbus',
	'+driver',
	'+router',
	'+patterns',
	'+sequencer',
	'+wavetable',
	'+rack',
	'+info',
	'+mainwindow',
	'+about',
	'+masterpanel',
	'+transportpanel',
	'+player',
	'+options',
	'+pkgbrowser',
	'+iconfactory',
	'+preferences',
]

# aldrin component object model
class ComponentManager:
	def __init__(self):
		self.singletons = {}
		self.factories = {}
		self.categories = {}
		self.packages = []
		self.factory_excludes = []
		self.register(config.__aldrin__)
		
	def load_packages(self):		
		self.packages = []
		includes = []
		excludes = []
		for modulename in DEFAULT_PACKAGES + self.get('aldrin.core.config').packages:
			if modulename.startswith('+'):
				modulename = modulename[1:]
				includes.append(modulename)
			elif modulename.startswith('-'):
				modulename = modulename[1:]
				if modulename.startswith(':'):
					moduleid = modulename[1:]
					self.factory_excludes.append(moduleid)
				else:
					excludes.append(modulename)
			else:
				includes.append(modulename)
		for modulename in includes:
			if modulename in excludes:
				print "excluding module %s" % modulename
				continue
			print "importing module %s" % modulename
			module_ = __import__(modulename)
			names = modulename.split('.')
			for name in names[1:]:
				module_ = getattr(module_, name)
			if not hasattr(module_, '__aldrin__'):
				continue
			self.packages.append(modulename)
			self.register(module_.__aldrin__, modulename)
					
	def register(self, pkginfo, modulename=None):
		# enumerate class factories
		for class_ in pkginfo.get('classes', []):
			if not hasattr(class_, '__aldrin__'):
				continue
			classinfo = class_.__aldrin__
			classid = classinfo['id']
			if classid in self.factory_excludes:
				print "excluding factory '%s'" % id
				continue
			self.factories[classid] = dict(classobj=class_, modulename=modulename)
			self.factories[classid].update(classinfo)
			# register categories
			for category in classinfo.get('categories', []):
				catlist = self.categories.get(category, [])
				catlist.append(classid)
				self.categories[category] = catlist
					
	def get(self, id, *args, **kwargs):
		# try to get a singleton first
		instance = self.singletons.get(id, False)
		if instance != False:
			assert instance, "got empty instance for classid '%s'. recursive loop?" % id
			return instance
		# get metainfo for object
		metainfo = self.factories.get(id, None)
		if not metainfo:
			print "no factory metainfo found for classid '%s'" % id
			return None
		# create a new object
		class_ = metainfo.get('classobj', None)
		if not class_:
			print "no factory found for classid '%s'" % id
			return None
		if class_.__aldrin__.get('singleton',False):
			self.singletons[id] = None # fill with an empty slot so we get no recursive loop
		obj = class_(*args,**kwargs)
		if class_.__aldrin__.get('singleton',False):
			self.singletons[id] = obj # register as singleton
		return obj
		
	def get_ids_from_category(self, category):
		return self.categories.get(category,[])
		
	def get_from_category(self, category, *args, **kwargs):
		return [self.get(classid, *args, **kwargs) for classid in self.categories.get(category,[])]

com = ComponentManager()

__all__ = [
	'com',
]

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
		
		def __init__(self, y=0):
			import random
			self.x = y or random.random()
			
	class MyClass2(MyClass):
		__aldrin__ = dict(
			id = 'aldrin.hub.myclass.singleton',
			singleton = True,
			categories = [
				'uselessclass',
				'uselessclass2',
			]
		)
	
	pkginfo = dict(
		classes = [
			MyClass,
			MyClass2,
		],
	)
	com.register(pkginfo)
	print com.get('aldrin.hub.myclass').x
	print com.get('aldrin.hub.myclass').x
	print com.get('aldrin.hub.myclass').x
	print com.get('aldrin.hub.myclass.singleton').x
	print com.get('aldrin.hub.myclass.singleton').x
	print com.get('aldrin.hub.myclass.singleton').x
	print com.get_from_category('uselessclass')
	
