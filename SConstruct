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

VERSION = "0.10.1"

import os, glob, sys, time

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

env = Environment(ENV = os.environ, options=opts)

env.SConsignFile()

######################################
# build settings
######################################

env['ROOTPATH'] = os.getcwd()

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

rootpath = "${DESTDIR}${PREFIX}"
binpath = "${DESTDIR}${PREFIX}/bin"
libpath = "${DESTDIR}${PREFIX}${LIBDIR}"
includepath = "${DESTDIR}${PREFIX}/include"
pluginpath = "${DESTDIR}${PREFIX}${LIBDIR}/zzub"

def install(target, source, perm=None):
	if not perm:
		env.Install(dir=env.Dir(target), source=source)
	else:
		env.InstallPerm(dir=env.Dir(target), source=source, perm=perm)

env.Alias(target='install', source=rootpath)

################################################
#
# aldrin tracker
#
################################################

aldrinpath = "${DESTDIR}${PREFIX}/share/aldrin"
aldrinrespath = "${DESTDIR}${PREFIX}/share/aldrin/res"
iconpath = "${DESTDIR}${PREFIX}/share/icons/hicolor"
apppath = "${DESTDIR}${PREFIX}/share/applications"
pixmappath = "${DESTDIR}${PREFIX}/share/pixmaps"

env['ALDRIN_SRC_PATH'] = '${ROOTPATH}/share/aldrin'
env['ALDRIN_INSTALL_PATH'] = '${DESTDIR}${PREFIX}/share/aldrin'
env['DOC_INSTALL_PATH'] = '${DESTDIR}${PREFIX}/share/doc'
env['DOC_SRC_PATH'] = '${ROOTPATH}/share/doc'

for f in glob.glob('share/aldrin/*.py'):
	install(aldrinpath, f)
install(aldrinpath, 'share/aldrin/index.xml')
install(aldrinpath, 'share/aldrin/blacklist.txt')
install(aldrinpath, 'share/aldrin/aliases.txt')
install('${DESTDIR}${PREFIX}/bin', 'bin/aldrin')
if win32:
	install('${DESTDIR}${PREFIX}', 'aldrin.bat')
	
# docs
install('${DOC_INSTALL_PATH}/aldrin/html', 
	glob.glob('share/doc/aldrin/html/*.html')
	+ glob.glob('share/doc/aldrin/html/*.css'))
install('${DOC_INSTALL_PATH}/aldrin/images', 
	glob.glob('share/doc/aldrin/images/*.gif')
	+ glob.glob('share/doc/aldrin/images/*.png'))

# icon theme
install(iconpath + '/16x16/apps', 'share/icons/hicolor/16x16/apps/aldrin.png')
install(iconpath + '/22x22/apps', 'share/icons/hicolor/22x22/apps/aldrin.png')
install(iconpath + '/24x24/apps', 'share/icons/hicolor/24x24/apps/aldrin.png')
install(iconpath + '/32x32/apps', 'share/icons/hicolor/32x32/apps/aldrin.png')
install(iconpath + '/48x48/apps', 'share/icons/hicolor/48x48/apps/aldrin.png')
install(iconpath + '/scalable/apps', 'share/icons/hicolor/scalable/apps/aldrin.svg')
#install(pixmappath, 'share/icons/hicolor/scalable/apps/aldrin.svg')

# applications info
install(apppath, 'share/applications/aldrin.desktop')

# demo songs
install(aldrinpath + "/demosongs", glob.glob("share/aldrin/demosongs/*.ccm"))

# themes
install(aldrinpath + "/themes", glob.glob("share/aldrin/themes/*.col"), 0644)

# presets
install(aldrinpath + "/presets", glob.glob("share/aldrin/presets/*.prs"), 0644)

################################################
#
# aldrin resources
#
################################################

for f in glob.glob('share/aldrin/res/*.png'):
	install(aldrinrespath, f)
for f in glob.glob('share/aldrin/res/*.ico'):
	install(aldrinrespath, f)

################################################
#
# aldrin extensions
#
################################################

Export('env', 'install')
env.SConscript('${ROOTPATH}/share/aldrin/extensions/SConscript')


