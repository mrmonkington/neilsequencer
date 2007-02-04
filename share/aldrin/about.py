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
Contains the information displayed in the about box.
"""

import sys
from utils import prepstr

abouttext = """
Aldrin 0.10.1 (Venus)

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

http://trac.zeitherrschaft.org/zzub/wiki/Aldrin

Head Developer
	Leonard Ritter <contact@leonard-ritter.com>

Developers
	Pieter Holtzhausen <13682857@sun.ac.za>
	Anders Ervik <calvin@countzero.no>
	
Plugin Developers
	Carsten Sørensen
	Joachim Michaelis <jm@binarywerks.dk>
	Aaron Oxford <aaron@hardwarehookups.com.au>
	Laurent De Soras <laurent.de.soras@club-internet.fr>
	Leonard Ritter <contact@leonard-ritter.com>
	Greg Raue <graue@oceanbase.org>
	
Icons taken from http://www.famfamfam.com/lab/icons/silk/

Special thanks to phed for cleaning up documentation.

A big thank you to all users and testers of Aldrin. That means you!
"""

abouttext = prepstr(abouttext)

__all__ = [
]
