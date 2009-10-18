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
from aldrin.utils import prepstr
import gtk

NAME = "Neil"
VERSION = "0.9"
COPYRIGHT = "Copyright (C) 2006, 2007, 2008 The Aldrin Development Team"
COMMENTS = '"no insult, but i think you\'re too weird to really make the best music app ever created." -- rpfr'

LICENSE = """This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA."""

WEBSITE = "http://aldrin.eu"

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
'Pieter Holtzhausen <13682857@sun.ac.za>',
'Phed',
]

AUTHORS = [prepstr(x) for x in AUTHORS]

from aldrin.utils import filepath, imagepath

def about_visit_website(dialog, link, user_data):
    import webbrowser
    webbrowser.open_new(link)

def about_send_email(dialog, link, user_data):
    import webbrowser
    print link
    webbrowser.open_new('mailto:'+link)

gtk.about_dialog_set_url_hook(about_visit_website, None)
gtk.about_dialog_set_email_hook(about_send_email, None)

class AboutDialog(gtk.AboutDialog):
    """
    A simple about dialog with a text control and an OK button.
    """

    __aldrin__ = dict(
	    id = "aldrin.core.dialog.about",
    )

    def __init__(self, parent):
	"""
	Initialization.
	"""
	gtk.AboutDialog.__init__(self)
	self.set_name(NAME)
	self.set_version(VERSION)
	self.set_copyright(COPYRIGHT)
	self.set_comments(COMMENTS)
	self.set_license(LICENSE)
	self.set_wrap_license(True)
	self.set_website(WEBSITE)
	self.set_authors(AUTHORS)
	self.set_artists(ARTISTS)
	self.set_documenters(DOCUMENTERS)
	self.set_logo(gtk.gdk.pixbuf_new_from_file(imagepath("alien.png")))

    def show(self):
	self.run()
	self.destroy()

__aldrin__ = dict(
	classes = [
		AboutDialog,
	]
)

__all__ = [
]

if __name__ == '__main__':
    AboutDialog(None).run()
