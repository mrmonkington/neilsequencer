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

VERSION = "0.13"

import os, glob, sys, time
import distutils.sysconfig

posix = os.name == 'posix'
win32 = os.name == 'nt'
mac = os.name == 'mac'

######################################
#
# init environment and define options
#
######################################

def tools_converter(value):
	return value.split(',')
	
def bool_converter(value):
	if value == 'True':
		return True
	elif value == 'False':
		return False
	return bool(value)

opts = Options( 'options.conf', ARGUMENTS )
opts.Add("PREFIX", 'Set the install "prefix" ( /path/to/PREFIX )', "/usr/local")
opts.Add("DESTDIR", 'Set the root directory to install into ( /path/to/DESTDIR )', "")
opts.Add("ETCDIR", 'Set the configuration dir "prefix" ( /path/to/ETC )', "/etc")

env = Environment(ENV = os.environ, options=opts)

env.SConsignFile()

######################################
# build settings
######################################

env['ROOTPATH'] = os.getcwd()
env['PYTHON_SITE_PACKAGES'] = distutils.sysconfig.get_python_lib(prefix="${DESTDIR}${PREFIX}")

######################################
# save config
######################################

opts.Save('options.conf', env)
Help( opts.GenerateHelpText( env ) )

######################################
# install paths
######################################

try:
	umask = os.umask(022)
	#print 'setting umask to 022 (was 0%o)' % umask
except OSError:     # ignore on systems that don't support umask
	pass

import SCons
from SCons.Script.SConscript import SConsEnvironment
SConsEnvironment.Chmod = SCons.Action.ActionFactory(os.chmod,
		lambda dest, mode: 'Chmod: "%s" with 0%o' % (dest, mode))

def InstallPerm(env, dir, source, perm):
	obj = env.Install(dir, source)
	for i in obj:
		env.AddPostAction(i, env.Chmod(str(i), perm))
	return dir

SConsEnvironment.InstallPerm = InstallPerm

def install(target, source, perm=None):
	if not perm:
		env.Install(dir=env.Dir(target), source=source)
	else:
		env.InstallPerm(dir=env.Dir(target), source=source, perm=perm)

env.Alias(target='install', source="${DESTDIR}${PREFIX}")

def install_recursive(target, path, mask):
	for f in glob.glob(os.path.join(path, mask)):
		install(target, f)
	for filename in os.listdir(path):
		fullpath = os.path.join(path, filename)
		if os.path.isdir(fullpath):
			install_recursive(os.path.join(target,filename), fullpath, mask)

Export(
	'env', 
	'install',
	'install_recursive',
	'win32', 'mac', 'posix',
)

env.SConscript('applications/SConscript')
env.SConscript('bin/SConscript')
env.SConscript('doc/SConscript')
env.SConscript('demosongs/SConscript')
env.SConscript('etc/SConscript')
env.SConscript('icons/SConscript')
env.SConscript('pixmaps/SConscript')
env.SConscript('presets/SConscript')
env.SConscript('src/SConscript')
env.SConscript('themes/SConscript')

