#encoding: latin-1

# Neil
# Modular Sequencer
# Copyright (C) 2006 The Neil Development Team
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

opts = Variables('options.conf', ARGUMENTS )
opts.Add("PREFIX", 'Set the install "prefix" ( /path/to/PREFIX )', "/usr/local")
opts.Add("DESTDIR", 'Set the root directory to install into ( /path/to/DESTDIR )', "")
opts.Add("ETCDIR", 'Set the configuration dir "prefix" ( /path/to/ETC )', "/etc")

env = Environment(ENV = os.environ, options=opts)

env.SConsignFile()

######################################
# build settings
######################################

distutils_prefix = "%s%s" % (env['DESTDIR'], env['PREFIX'])

env['ROOTPATH'] = os.getcwd()
env['SITE_PACKAGE_PATH'] = \
    distutils.sysconfig.get_python_lib(prefix=distutils_prefix)
env['APPLICATIONS_PATH'] = '${DESTDIR}${PREFIX}/share/applications'
env['BIN_PATH'] = '${DESTDIR}${PREFIX}/bin'
env['SHARE_PATH'] = '${DESTDIR}${PREFIX}/share/neil'
env['DOC_PATH'] = '${DESTDIR}${PREFIX}/share/doc/neil'
env['ETC_PATH'] = '${DESTDIR}${ETCDIR}/neil'
env['ICONS_NEIL_PATH'] = '${DESTDIR}${PREFIX}/share/icons/neil'
env['ICONS_HICOLOR_PATH'] = '${DESTDIR}${PREFIX}/share/icons/hicolor'
env['PIXMAPS_PATH'] = '${DESTDIR}${PREFIX}/share/pixmaps/neil'

CONFIG_PATHS = dict(
	site_packages = 'SITE_PACKAGE_PATH',
	applications = 'APPLICATIONS_PATH',
	bin = 'BIN_PATH',
	share = 'SHARE_PATH',
	doc = 'DOC_PATH',
	icons_neil = 'ICONS_NEIL_PATH',
	icons_hicolor = 'ICONS_HICOLOR_PATH',
	pixmaps = 'PIXMAPS_PATH',
	etc = 'ETC_PATH',
)

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
env.Alias(target='install', source="${DESTDIR}${ETCDIR}")

def install_recursive(target, path, mask):
    for f in glob.glob(os.path.join(path, mask)):
	install(target, f)
    for filename in os.listdir(path):
	fullpath = os.path.join(path, filename)
	if os.path.isdir(fullpath):
	    install_recursive(os.path.join(target, filename), fullpath, mask)

def build_path_config(target, source, env):
    outpath = str(target[0])
    from StringIO import StringIO
    from ConfigParser import ConfigParser
    s = StringIO()
    cfg = ConfigParser()
    cfg.add_section('Paths')
    remove_prefix = '${DESTDIR}'
    for key, value in CONFIG_PATHS.iteritems():
	value = env[value]
	if value.startswith(remove_prefix):
	    value = value[len(remove_prefix):]
	cfg.set('Paths', key, os.path.abspath(str(env.Dir(value))))
    cfg.write(s)
    file(outpath, 'w').write(s.getvalue())

builders = dict(
    BuildPathConfig = Builder(action=build_path_config),
)

env['BUILDERS'].update(builders)

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

