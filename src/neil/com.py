#encoding: latin-1

# Neil
# Modular Sequencer
# Copyright (C) 2006,2007,2008 The Neil Development Team
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

from pathconfig import path_cfg
import os,sys,glob
from ConfigParser import ConfigParser

SECTION_NAME = 'Neil COM'
OPTIONS = [
    'Module',
    'Name',
    'Description',
    'Icon',
    'Authors',
    'Copyright',
    'Website',
]


class Package(ConfigParser):
    def __init__(self, path):
        ConfigParser.__init__(self)
        self.filename = path

    def parse(self):
        self.read([self.filename])
        if not self.has_section(SECTION_NAME):
            print "missing section " + SECTION_NAME + " in " + self.filename
            return False
        for option in OPTIONS:
            if not self.has_option(SECTION_NAME, option):
                print "missing option " + option + " in " + self.filename
                return False
            setattr(self, option.lower(), self.get(SECTION_NAME, option))
        #basepath = os.path.dirname(self.filename)
        return True


# neil component object model
class ComponentManager:
    def __init__(self):
        self.clear()

    def clear(self):
        self.singletons = {}
        self.factories = {}
        self.categories = {}
        self.packages = []

    def load_packages(self):
        self.packages = []
        packages = []
        names = []
        component_path = [
            path_cfg.get_path('components') or os.path.join(path_cfg.get_path('share'), 'components'),
            os.path.expanduser('~/.neil/components'),
        ]
        for path in component_path:
            print "scanning " + path + " for components"
            if os.path.isdir(path):
                if not path in sys.path:
                    sys.path = [path] + sys.path
                for filename in glob.glob(os.path.join(path, '*.neil-component')):
                    print filename
                    pkg = Package(filename)
                    if pkg.parse():
                        packages.append(pkg)
            else:
                print "no such path: " + path
        for pkg in packages:
            try:
                modulename = pkg.module
                print "importing module", modulename
                module_ = __import__(modulename)
                names = modulename.split('.')
                for name in names[1:]:
                    module_ = getattr(module_, name)
                if not hasattr(module_, '__neil__'):
                    print "module", modulename, "has no __neil__ metadict"
                    continue
                self.register(module_.__neil__, modulename)
                self.packages.append(pkg)
            except:
                import errordlg
                errordlg.print_exc()

    def register(self, pkginfo, modulename=None):
        # enumerate class factories
        for class_ in pkginfo.get('classes', []):
            if not hasattr(class_, '__neil__'):
                print "class", class_, "has no __neil__ metadict"
                continue
            classinfo = class_.__neil__
            classid = classinfo['id']
            self.factories[classid] = dict(classobj=class_, modulename=modulename)
            self.factories[classid].update(classinfo)
            # register categories
            for category in classinfo.get('categories', []):
                catlist = self.categories.get(category, [])
                catlist.append(classid)
                self.categories[category] = catlist

    def throw(self, id, arg):
        class_ = self.factory(id)
        raise class_(arg)

    def factory(self, id):
        # get metainfo for object
        metainfo = self.factories.get(id, None)
        if not metainfo:
            print "no factory metainfo found for classid '%s'" % id
            return None
        class_ = metainfo.get('classobj', None)
        if not class_:
            print "no factory found for classid '%s'" % id
            return None
        return class_

    def exception(self, id):
        return self.factory(id)

    def singleton(self, id):
        # try to get a singleton first
        instance = self.singletons.get(id, False)
        if instance != False:
            assert instance, "got empty instance for classid '%s'. recursive loop?" % id
            return instance
        return None

    def get(self, id, *args, **kwargs):
        instance = self.singleton(id)
        if instance:
            return instance
        # retrieve factory
        class_ = self.factory(id)
        if not class_:
            return None
        if class_.__neil__.get('singleton', False):
            self.singletons[id] = None  # fill with an empty slot so we get no recursive loop
        # create a new object
        obj = None
        try:
            obj = class_(*args, **kwargs)
        except:
            import traceback, errordlg
            traceback.print_exc()
            msg = "<b><big>Could not create component</big></b>"
            msg2 = "while trying to create '" + id + "'"
            errordlg.error(None, msg, msg2, traceback.format_exc(), offer_quit=True)
        if class_.__neil__.get('singleton', False):
            if not obj:
                del self.singletons[id]
            else:
                self.singletons[id] = obj  # register as singleton
        return obj

    def get_ids_from_category(self, category):
        return self.categories.get(category, [])

    def category(self, cat, *args, **kwargs):
        return self.get_from_category(cat, *args, **kwargs)

    def get_from_category(self, category, *args, **kwargs):
        return [self.get(classid, *args, **kwargs) for classid in self.categories.get(category, [])]

com = ComponentManager()
get = com.get
factory = com.factory
exception = com.exception
singleton = com.singleton
category = com.category
get_from_category = com.get_from_category
clear = com.clear


def get_packages():
    return com.packages


def get_categories():
    return com.categories


def get_factories():
    return com.factories


def init():
    com.load_packages()

__all__ = [
    'com',
]

if __name__ == '__main__':
    class MyClass:
        __neil__ = dict(
            id = 'neil.hub.myclass',
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
        __neil__ = dict(
            id = 'neil.hub.myclass.singleton',
            singleton = True,
            categories = [
                'uselessclass',
                'uselessclass2',
            ]
        )

    class CancelException(Exception):
        __neil__ = dict(
            id = 'neil.exception.cancel',
            exception = True,
            categories = [
            ]
        )

    pkginfo = dict(
        classes = [
            MyClass,
            MyClass2,
            CancelException,
        ],
    )
    com.register(pkginfo)
    print com.get('neil.hub.myclass').x
    print com.get('neil.hub.myclass').x
    print com.get('neil.hub.myclass').x
    print com.get('neil.hub.myclass.singleton').x
    print com.get('neil.hub.myclass.singleton').x
    print com.get('neil.hub.myclass.singleton').x
    print com.category('uselessclass')
    try:
        com.throw('neil.exception.cancel', "argh.")
    except com.exception('neil.exception.cancel'), test:
        print "passed.", test
    print "bleh."
