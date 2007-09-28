#encoding: latin-1

# pyzzub
# Python bindings for libzzub
# Copyright (C) 2006 The libzzub Development Team
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

from zzub import Player, zzub_player_state_playing, zzub_player_state_stopped
import os, sys

if __name__ == "__main__":
	if len(sys.argv) != 2:
		print "syntax: testzzub.py <path to bmx or ccm>"
		raise SystemExit
		
	filepath = sys.argv[1]
	if not os.path.isfile(filepath):
		print "no such file: %s" % filepath
		raise SystemExit, 1
	
	ext = os.path.splitext(filepath)[1]
	if not ext in ('.bmx','.ccm'):
		print "unknown filetype: %s" % ext
		raise SystemExit, 1
	
	p = Player()
	p.add_plugin_path("/usr/local/lib64/zzub/")
	p.add_plugin_path("/usr/local/lib/zzub/")
	p.add_plugin_path("/usr/lib64/zzub/")
	p.add_plugin_path("/usr/lib/zzub/")

	print "initializing zzub player..."
	if p.initialize(44100):
		print "error initializing zzub."
		raise SystemExit, 1

	p.audiodriver_set_samplerate(44100)
	res = p.audiodriver_create(-1, -1) # best pick
	if res:
		print "error creating audiodriver."
		raise SystemExit, 1

	print "Loading %s... " % filepath
	if ext == '.bmx':
		res = p.load_bmx(filepath)
	elif ext == '.ccm':
		res = p.load_ccm(filepath)
	else:
		res = -1
	if res:
		print "Failed loading"
		raise SystemExit, 1

	print "enabling audiodriver..."
	p.audiodriver_enable(True)
	
	print "playing. press a key to quit."

	p.play()

	sys.stdin.readline()

	print "stopping..."
	p.stop()

	print "destroying audiodriver..."
	p.audiodriver_destroy()
	print "finished playing."
