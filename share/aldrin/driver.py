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
Contains tool functions to deal with audio and midi drivers.
"""

import config
import common
player = common.get_player()

class AudioInitException(Exception):
	pass

class MidiInitException(Exception):
	pass
	
class MidiDriver:
	def __init__(self):
		self.enabled = False

	def destroy(self):
		if not self.enabled:
			return
		print "uninitializing midi driver..."
		player.mididriver_close_all()
		self.enabled = False
		
	def init(self):
		if self.enabled:
			self.destroy()
		midiinputs = config.get_config().get_mididriver_inputs()
		midioutputs = config.get_config().get_mididriver_outputs()
		for i in range(player.mididriver_get_count()):
			drivername = player.mididriver_get_name(i).strip() 
			if player.mididriver_is_input(i) and drivername in midiinputs:
				print "Opening MIDI input device '%s'..." % drivername
				if player.mididriver_open(i) != 0:
					raise MidiInitException
			elif player.mididriver_is_output(i) and drivername in midioutputs:
				print "Opening MIDI output device '%s'..." % drivername
				if player.mididriver_open(i) != 0:
					raise MidiInitException
		self.enabled = True

class AudioDriver:
	def __init__(self):
		self.enabled = False
		self.samplerate = 44100
		self.buffersize = 256
		
	def destroy(self):
		if not self.enabled:
			return
		print "uninitializing audio driver..."
		player.audiodriver_destroy()
		self.enabled = False
		
	def get_latency(self):
		return float(self.buffersize) / float(self.samplerate)
		
	def init(self):
		if self.enabled:
			self.destroy()
		inputname, outputname, samplerate, buffersize = config.get_config().get_audiodriver_config()
		if not player.audiodriver_get_count():
			raise AudioInitException
		print "available drivers:"
		bestpick = -1
		for i in range(player.audiodriver_get_count()):
			io = ''
			if player.audiodriver_is_input(i):
				io += 'I'
			else:
				io += ' '
			if player.audiodriver_is_output(i):
				io += 'O'
			else:
				io += ' '
			print "#%i: %s [%s]" % (i,player.audiodriver_get_name(i), io)
			if player.audiodriver_get_name(i) == outputname:
				bestpick = i
		print "best output pick:"
		print player.audiodriver_get_name(bestpick)
		player.audiodriver_set_samplerate(samplerate)
		player.audiodriver_set_buffersize(buffersize)
		idriver = -1
		if player.audiodriver_is_input(bestpick):
			idriver = bestpick
		initres = player.audiodriver_create(bestpick, idriver)
		if initres != 0:
			raise AudioInitException
		player.audiodriver_enable(1)
		self.samplerate = samplerate
		self.buffersize = buffersize
		self.enabled = True
		
audiodriver = AudioDriver()
mididriver = MidiDriver()

def get_audiodriver():
	return audiodriver
	
def get_mididriver():
	return mididriver

