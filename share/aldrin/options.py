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

import optparse, sys

class OptionParser(optparse.OptionParser):
	__aldrin__ = dict(
		id = 'aldrin.core.options',
		singleton = True,
	)
	
	def __init__(self):
		optparse.OptionParser.__init__(self)
		self.add_option("--profile", metavar="profile", default='', help="Start Aldrin with profiling enabled, save results to <profile>.")
		
	def parse_args(self, *args, **kwargs):
		self._options, self._args = optparse.OptionParser.parse_args(self, *args, **kwargs)
		
	def get_options_args(self):
		return self._options, self._args

__aldrin__ = dict(
	classes = [
		OptionParser,
	],
)