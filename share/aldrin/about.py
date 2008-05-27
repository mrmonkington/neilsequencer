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
Contains the information displayed in the about box.
"""

import sys
from utils import prepstr

NAME = "Aldrin"
VERSION = "0.13 (Mars)"
COPYRIGHT = "Copyright (C) 2006, 2007, 2008 The Aldrin Development Team"
COMMENTS = '"no insult, but i think you\'re too weird to really make the best music app ever created." -- rpfr'

LICENSE = """This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
"""
WEBSITE = "http://aldrin.sf.net"
AUTHORS = [
'Leonard Ritter <contact@leonard-ritter.com>',
'Pieter Holtzhausen <13682857@sun.ac.za>',
'Anders Ervik <calvin@countzero.no>',
'James Stone <jamesmstone@gmail.com>',
'James McDermott <jamesmichaelmcdermott@gmail.com>',
'Carsten Sørensen',
'Joachim Michaelis <jm@binarywerks.dk>',
'Aaron Oxford <aaron@hardwarehookups.com.au>',
'Laurent De Soras <laurent.de.soras@club-internet.fr>',
'Greg Raue <graue@oceanbase.org>',
]

ARTISTS = [
'famfamfam http://www.famfamfam.com/lab/icons/silk/',
]

DOCUMENTERS = [
'Leonard Ritter <contact@leonard-ritter.com>',
'Phed',
]

AUTHORS = [prepstr(x) for x in AUTHORS]

__all__ = [
]
