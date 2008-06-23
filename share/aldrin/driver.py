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
Contains tool functions to deal with audio and midi drivers.
"""

import config

from aldrincom import com
from utils import error

class MidiDriver:
	__aldrin__ = dict(
		id = 'aldrin.core.driver.midi',
		singleton = True,
		categories = [
			'driver',
		],		
	)
	
	class MidiInitException(Exception):
		pass
	def __init__(self):
		self.enabled = False
		eventbus = com.get('aldrin.core.eventbus')
		eventbus.shutdown += self.destroy
		self.init_failed = False
		try:
			self.init()
		except self.MidiInitException:
			self.init_failed = True
			error(self, "<b><big>Aldrin was unable to initialize MIDI output.</big></b>\n\nPlease check your MIDI settings.")

	def destroy(self):
		if not self.enabled:
			return
		print "uninitializing midi driver..."
		player = com.get('aldrin.core.player')
		player.mididriver_close_all()
		self.enabled = False
		
	def init(self):
		if self.enabled:
			self.destroy()
		midiinputs = config.get_config().get_mididriver_inputs()
		midioutputs = config.get_config().get_mididriver_outputs()
		player = com.get('aldrin.core.player')
		for i in range(player.mididriver_get_count()):
			drivername = player.mididriver_get_name(i).strip() 
			if player.mididriver_is_input(i) and drivername in midiinputs:
				print "Opening MIDI input device '%s'..." % drivername
				if player.mididriver_open(i) != 0:
					raise self.MidiInitException
			elif player.mididriver_is_output(i) and drivername in midioutputs:
				print "Opening MIDI output device '%s'..." % drivername
				if player.mididriver_open(i) != 0:
					raise self.MidiInitException
		self.enabled = True

class AudioDriver:
	__aldrin__ = dict(
		id = 'aldrin.core.driver.audio',
		singleton = True,
		categories = [
			'driver',
		],		
	)
	
	class AudioInitException(Exception):
		pass
	
	def __init__(self):
		self.enabled = False
		self.samplerate = 44100
		self.buffersize = 256
		self.driver = None
		eventbus = com.get('aldrin.core.eventbus')
		eventbus.shutdown += self.destroy
		self.init_failed = False
		try:
			self.init()
		except self.AudioInitException:
			self.init_failed = True
			error(self, "<b><big>Aldrin was unable to initialize audio output.</big></b>\n\nPlease check your audio settings in the preferences dialog.")
		
	def destroy(self):
		if not self.enabled:
			return
		print "uninitializing audio driver..."
		self.driver.destroy()
		self.enabled = False
		
	def get_cpu_load(self):
		return self.driver.get_cpu_load()
		
	def get_latency(self):
		return float(self.buffersize) / float(self.samplerate)
		
	def get_count(self):
		return self.driver.get_count()
		
	def get_name(self, *args,**kargs):
		return self.driver.get_name(*args,**kargs)
		
	def enable(self, *args,**kargs):
		self.driver.enable(*args,**kargs)
		
	def init(self):
		if self.enabled:
			self.destroy()
		inputname, outputname, samplerate, buffersize = config.get_config().get_audiodriver_config()
		player = com.get('aldrin.core.player')
		self.driver = player.audiodriver_create()
		if not self.driver.get_count():
			raise self.AudioInitException
		print "available drivers:"
		bestpick = -1
		for i in range(self.driver.get_count()):
			io = ''
			if self.driver.is_input(i):
				io += 'I'
			else:
				io += ' '
			if self.driver.is_output(i):
				io += 'O'
			else:
				io += ' '
			print "#%i: %s [%s]" % (i,self.driver.get_name(i), io)
			if self.driver.get_name(i) == outputname:
				bestpick = i
		print "best output pick:"
		print self.driver.get_name(bestpick)
		self.driver.set_samplerate(samplerate)
		self.driver.set_buffersize(buffersize)
		idriver = -1
		if self.driver.is_input(bestpick):
			idriver = bestpick
		initres = self.driver.create_device(bestpick, idriver)
		if initres != 0:
			raise self.AudioInitException
		self.driver.enable(1)
		self.samplerate = samplerate
		self.buffersize = buffersize
		self.enabled = True

__aldrin__ = dict(
	classes = [
		AudioDriver,
		MidiDriver,
	],
)

if __name__ == '__main__':
	com.load_packages()
	com.get_from_category('driver')
