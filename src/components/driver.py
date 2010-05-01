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
import zzub

import aldrin.com as com
from aldrin.utils import error

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
		zzub.Mididriver.close_all(player)
		self.enabled = False
		
	def init(self):
		if self.enabled:
			self.destroy()
		midiinputs = config.get_config().get_mididriver_inputs()
		midioutputs = config.get_config().get_mididriver_outputs()
		player = com.get('aldrin.core.player')
		for i in range(zzub.Mididriver.get_count(player)):
			drivername = zzub.Mididriver.get_name(player,i).strip() 
			if zzub.Mididriver.is_input(player,i) and drivername in midiinputs:
				print "Opening MIDI input device '%s'..." % drivername
				if zzub.Mididriver.open(player,i) != 0:
					raise self.MidiInitException
			elif zzub.Mididriver.is_output(player,i) and drivername in midioutputs:
				print "Opening MIDI output device '%s'..." % drivername
				if zzub.Mididriver.open(player,i) != 0:
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
			error(None, "<b><big>Aldrin was unable to initialize audio output.</big></b>\n\nPlease check your audio settings in the preferences dialog.")
		
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
		print inputname, outputname
		player = com.get('aldrin.core.player')
		self.driver = zzub.Audiodriver.create(player)
		if not self.driver.get_count():
			raise self.AudioInitException
		print "available drivers:"
		input = -1
		output = -1
		for i in range(self.driver.get_count()):
			io = ''
			if self.driver.is_output(i):
				io += 'O'
			else:
				io += ' '
			if self.driver.is_input(i):
				io += 'I'
			else:
				io += ' '
			drivername = self.driver.get_name(i)
			print "#%i: %s [%s]" % (i,drivername, io)
			if drivername == inputname:
				input = i
			if drivername == outputname:
				output = i
				
		# second round: if we didnt find them from the config,
		# pick good alternatives.
		
		if output == -1:
			for i in range(self.driver.get_count()):
				if self.driver.is_output(i):
					output = i
		
		# regardless of what was chosen as input, if output
		# has an input as well, prefer that one first.
		if self.driver.is_input(output):
			input = output
		
		# take output channel if it supports input		
		if input == -1:
			for i in range(self.driver.get_count()):
				if self.driver.is_input(i):
					input = i
			
		if output == -1:
			raise self.AudioInitException
		print "best input/output pick:"
		print self.driver.get_name(input),"/",self.driver.get_name(output)
		self.driver.set_samplerate(samplerate)
		self.driver.set_buffersize(buffersize)
		initres = self.driver.create_device(input, output)
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
