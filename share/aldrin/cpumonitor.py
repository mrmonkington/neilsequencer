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
Provides dialog class for cpu monitor.
"""

from gtkimport import gtk
import gobject
from utils import prepstr, add_scrollbars
import utils, os, stat
import common
player = common.get_player()

class CPUMonitorDialog(gtk.Dialog):
	"""
	This Dialog shows the CPU monitor, which allows monitoring
	CPU usage and individual plugin CPU consumption.
	"""
	def __init__(self, parent):
		"""
		Initializer.
		"""
		gtk.Dialog.__init__(self, parent=parent.get_toplevel())
		self.set_size_request(200,300)
		self.set_title("CPU Monitor")
		self.pluginlist = gtk.ListStore(str, str)
		scrollwin = gtk.ScrolledWindow()
		scrollwin.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
		self.pluginlistview = gtk.TreeView(self.pluginlist)
		self.pluginlistview.set_rules_hint(True)
		self.tvplugin = gtk.TreeViewColumn("Plugin")
		self.tvplugin.set_resizable(True)
		self.tvload = gtk.TreeViewColumn("CPU Load")
		self.cellplugin = gtk.CellRendererText()
		self.cellload = gtk.CellRendererText()
		self.tvplugin.pack_start(self.cellplugin)
		self.tvload.pack_start(self.cellload)
		self.tvplugin.add_attribute(self.cellplugin, 'text', 0)
		self.tvload.add_attribute(self.cellload, 'text', 1)
		self.pluginlistview.append_column(self.tvplugin)
		self.pluginlistview.append_column(self.tvload)
		self.pluginlistview.set_search_column(0)
		self.tvplugin.set_sort_column_id(0)
		self.tvload.set_sort_column_id(1)
		self.labeltotal = gtk.Label("100%")
		self.gaugetotal = gtk.ProgressBar()
		sizer = gtk.VBox()
		sizer.pack_start(add_scrollbars(self.pluginlistview))
		hsizer = gtk.HBox()
		hsizer.pack_start(self.gaugetotal, padding=5)
		hsizer.pack_start(self.labeltotal, expand=False, padding=5)
		sizer.pack_start(hsizer, expand=False, padding=5)
		self.vbox.add(sizer)
		gobject.timeout_add(1000, self.on_timer)

	def on_timer(self):
		"""
		Called by timer event. Updates CPU usage statistics.
		"""
		if self.window and self.window.is_visible():
			player.lock_tick()
			cpu = 0.0
			cpu_loads = {}
			try:
				cpu = player.audiodriver_get_cpu_load()
				for mp in player.get_plugin_list():
					cpu_loads[mp.get_name()] = mp.get_last_worktime()
			except:
				import traceback
				traceback.print_exc()
			player.unlock_tick()
			total_workload = max(sum(cpu_loads.values()),0.001)
			self.gaugetotal.set_fraction(cpu / 100.0)
			self.labeltotal.set_label("%i%%" % int(cpu + 0.5))
			
			def update_node(store, level, item, udata):
				name = self.pluginlist.get_value(item, 0)
				if name in cpu_loads:
					relperc = (cpu_loads[name] / total_workload) * cpu
					store.set_value(item, 1, "%.1f%%" % relperc)
					del cpu_loads[name]
				else:
					store.remove(item)
				
			self.pluginlist.foreach(update_node, None)
				
			for k,v in cpu_loads.iteritems():
				k = prepstr(k)
				relperc = (v / total_workload) * cpu
				self.pluginlist.append([k, "%.1f%%" % relperc])
			self.pluginlistview.columns_autosize()
		return True

__all__ = [
'CPUMonitorDialog',
]

if __name__ == '__main__':
	import testplayer, utils
	player = testplayer.get_player()
	player.load_ccm(utils.filepath('demosongs/paniq-knark.ccm'))
	window = testplayer.TestWindow()
	window.show_all()
	dlg = CPUMonitorDialog(window)
	dlg.connect('destroy', lambda widget: gtk.main_quit())
	dlg.show_all()
	gtk.main()
