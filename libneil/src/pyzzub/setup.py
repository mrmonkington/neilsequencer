#!/usr/bin/python
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

from distutils.core import setup
setup(name='pyzzub',
		version='0.8.1',
		description = "Python Bindings For libzzub",
		author= "Leonard Ritter",
		author_email = "contact@leonard-ritter.com",
		url = "http://trac.zeitherrschaft.org/zzub",
		packages=['zzub'],
		)
