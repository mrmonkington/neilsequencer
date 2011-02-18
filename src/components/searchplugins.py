#encoding: latin-1

# Neil
# Modular Sequencer
# Copyright (C) 2006,2007,2008,2009,2010 The Neil Development Team
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, 
# Boston, MA  02110-1301, USA.

import gtk
import gobject
import neil.utils as utils, os, stat
from neil.utils import new_stock_image_toggle_button, ObjectHandlerGroup
from neil.utils import is_effect,is_generator,is_controller,\
     is_root
from neil.utils import prepstr, filepath, db2linear, linear2db,\
     is_debug, filenameify, get_item_count, question, error,\
     new_listview, add_scrollbars, get_clipboard_text,\
     set_clipboard_text, gettext, new_stock_image_button,\
     new_liststore, add_vscrollbar
import neil.common as common
import neil.com as com
import zzub

DRAG_FORMAT_PLUGIN_URI = 0

DRAG_FORMATS = [
	('application/x-neil-plugin-uri', 0, DRAG_FORMAT_PLUGIN_URI)
]

class SearchPluginsDialog(gtk.Window):

    __neil__ = dict(
        id = 'neil.core.searchplugins',
        singleton = True,
        categories = [
            'viewdialog',
            'view',
            ]
	)
	
    __view__ = dict(
        label = "Search Plugins",
        order = 0,
        toggle = True,
	)

    def __init__(self):
	gtk.Window.__init__(self)
        self.set_default_size(250, -1)
        self.vbox = gtk.VBox()
        self.add(self.vbox)
        self.set_title("Search Plugins")
        self.connect('delete-event', self.hide_on_delete)
	com.get("neil.core.icons") # make sure theme icons are loaded
	self.searchterms = ['']
	self.searchbox = gtk.Entry()
	self.treeview = gtk.TreeView()
	new_liststore(self.treeview, [
		('Icon', gtk.gdk.Pixbuf),
		('Name', str, dict(markup=True)),
		(None, object),
		(None, str, dict(markup=True)),
	])

	# checkboxes
	self.check_containers = [gtk.HBox() for i in range(2)]
	self.vbox.pack_end(self.check_containers[1], False, False)
	self.vbox.pack_end(self.check_containers[0], False, False)

	self.show_generators_button = gtk.CheckButton(label="Generators")
	self.check_containers[0].add(self.show_generators_button)
	self.show_generators_button.connect("toggled", self.on_checkbox_changed)

	self.show_effects_button = gtk.CheckButton(label="Effects")
	self.check_containers[0].add(self.show_effects_button)
	self.show_effects_button.connect("toggled", self.on_checkbox_changed)

	self.show_controllers_button = gtk.CheckButton(label="Controllers")
	self.check_containers[1].add(self.show_controllers_button)
	self.show_controllers_button.connect("toggled", self.on_checkbox_changed)

	self.show_nonnative_button = gtk.CheckButton(label="Non-native")
	self.check_containers[1].add(self.show_nonnative_button)
	self.show_nonnative_button.connect("toggled", self.on_checkbox_changed)

	self.store = self.treeview.get_model()
	self.treeview.set_headers_visible(False)
	self.treeview.set_rules_hint(True)
	self.populate()
	self.vbox.pack_start(add_scrollbars(self.treeview))
	self.vbox.pack_end(self.searchbox, False, False)
	self.searchbox.connect("changed", self.on_entry_changed)
	self.filter = self.store.filter_new()
	self.filter.set_visible_func(self.filter_item, data=None)
	self.treeview.set_model(self.filter)
	self.treeview.set_tooltip_column(3)
	self.treeview.drag_source_set(gtk.gdk.BUTTON1_MASK | gtk.gdk.BUTTON3_MASK,
		DRAG_FORMATS, gtk.gdk.ACTION_COPY | gtk.gdk.ACTION_MOVE)
	self.treeview.connect('drag_data_get', self.on_treeview_drag_data_get)
	cfg = com.get('neil.core.config')
	self.searchbox.set_text(cfg.pluginlistbrowser_search_term)
	self.show_generators_button.set_active(cfg.pluginlistbrowser_show_generators)
	self.show_effects_button.set_active(cfg.pluginlistbrowser_show_effects)
	self.show_controllers_button.set_active(cfg.pluginlistbrowser_show_controllers)
	self.show_nonnative_button.set_active(cfg.pluginlistbrowser_show_nonnative)
        self.set_size_request(-1, 500)

    def get_icon_name(self, pluginloader):
	uri = pluginloader.get_uri()
	if uri.startswith('@zzub.org/dssidapter/'):
	    return 'dssi'
	if uri.startswith('@zzub.org/ladspadapter/'):
	    return 'ladspa'
	if uri.startswith('@psycle.sourceforge.net/'):
	    return 'psycle'
	filename = pluginloader.get_name()
	filename = filename.strip().lower()
	for c in '():[]/,.!"\'$%&\\=?*#~+-<>`@ ':
	    filename = filename.replace(c, '_')
	while '__' in filename:
	    filename = filename.replace('__','_')
	filename = filename.strip('_')
	return filename

    def on_treeview_drag_data_get(self, widget, context, selection_data, info, time):
	if selection_data.target == 'application/x-neil-plugin-uri':
	    store, it = self.treeview.get_selection().get_selected()
	    child = store.get(it, 2)[0]
	    selection_data.set(selection_data.target, 8, child.get_uri())

    def on_entry_changed(self, widget):
	text = self.searchbox.get_text()
	cfg = com.get('neil.core.config')
	cfg.pluginlistbrowser_search_term = text
	terms = [word.strip().split(' ') for word in text.lower().strip().split(',')]
	self.searchterms = terms
	self.filter.refilter()

    def on_checkbox_changed(self, check):
	active = check.get_active()
	cfg = com.get('neil.core.config')
	lbl = check.get_label()
	if lbl == "Generators":
	    cfg.pluginlistbrowser_show_generators = active
	elif lbl == "Effects":
	    cfg.pluginlistbrowser_show_effects = active
	elif lbl == "Controllers":
	    cfg.pluginlistbrowser_show_controllers = active
	elif lbl == "Non-native":
	    cfg.pluginlistbrowser_show_nonnative = active
	self.filter.refilter()

    def filter_item(self, model, it, data):
	def is_native(pl):
	    name = pl.get_loader_name().lower()
	    if "ladspa" in name or "dssi" in name:
		return False
	    else:
		return True
	if not self.searchterms:
	    return True
	child = model.get(it, 2)[0]
	name = child.get_name().lower()
	if not self.show_nonnative_button.get_active() and not is_native(child):
	    return False
	if not self.show_generators_button.get_active() and is_generator(child):
	    return False
	if not self.show_effects_button.get_active() and is_effect(child):
	    return False
	if not self.show_controllers_button.get_active() and is_controller(child):
	    return False
	for group in self.searchterms:
	    found = True
	    for word in group:
		if not word in name:
		    found = False
		    break
	    if found:
		return True
	return False

    def populate(self):
	player = com.get('neil.core.player')
	plugins = {}
	cfg = com.get('neil.core.config')
	for pluginloader in player.get_pluginloader_list():
	    plugins[pluginloader.get_uri()] = pluginloader
	theme = gtk.icon_theme_get_default()
	def get_type_rating(n):
	    uri = n.get_uri()
	    if uri.startswith('@zzub.org/dssidapter/'):
		return 1
	    if uri.startswith('@zzub.org/ladspadapter/'):
		return 1
	    if uri.startswith('@psycle.sourceforge.net/'):
		return 2
	    return 0
	def get_icon_rating(n):
	    icon = self.get_icon_name(n)
	    if icon and theme.has_icon(icon):
		return 0
	    return 1

	def get_rating(n):
	    if is_generator(n):
		return 0
	    elif is_controller(n):
		return 1
	    elif is_effect(n):
		return 2
	    else:
		return 3
	def cmp_child(a,b):
	    c = cmp(get_type_rating(a), get_type_rating(b))
	    if c != 0:
		return c
	    c = cmp(get_icon_rating(a),get_icon_rating(b))
	    if c != 0:
		return c
	    c = cmp(get_rating(a),get_rating(b))
	    if c != 0:
		return c
	    return cmp(a.get_name().lower(),b.get_name().lower())
	def get_type_text(pl):
	    if is_generator(pl):
		return '<span color="' + cfg.get_color('MV Generator') + '">Generator</span>'
	    elif is_effect(pl):
		return '<span color="' + cfg.get_color('MV Effect') + '">Effect</span>'
	    elif is_controller(pl):
		return '<span color="' + cfg.get_color('MV Controller') + '">Controller</span>'
	    elif is_root(pl):
		return "Root"
	    else:
		return "Other"
	for pl in sorted(plugins.values(), cmp_child):
	    name = prepstr(pl.get_name())
	    text = '<b>' + name + '</b>\n<small>' + get_type_text(pl) + '</small>'
	    pixbuf = None
	    icon = self.get_icon_name(pl)
	    tooltip = '<b>' + name + '</b> <i>(' + get_type_text(pl) + ')</i>\n'
	    tooltip += '<small>' + prepstr(pl.get_uri()) + '</small>\n\n'
	    gpcount = pl.get_parameter_count(zzub.zzub_parameter_group_global)
	    tpcount = pl.get_parameter_count(zzub.zzub_parameter_group_track)
	    acount = pl.get_attribute_count()
	    tooltip += '%i global parameters, %i track parameters, %i attributes\n\n' % (gpcount, tpcount, acount)
	    author = pl.get_author().replace('<', '&lt;').replace('>', '&gt;')
	    tooltip += 'Written by ' + prepstr(author)
	    if icon and theme.has_icon(icon):
		pixbuf = theme.load_icon(icon, gtk.ICON_SIZE_MENU, 0)
	    self.store.append([pixbuf, text, pl, tooltip])
	

__all__ = [
    'SearchPluginsDialog',
]

__neil__ = dict(
    classes = [
        SearchPluginsDialog,
	],
)

if __name__ == '__main__':
    pass
