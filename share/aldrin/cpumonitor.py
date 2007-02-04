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

from wximport import wx
from utils import prepstr
import utils, os, stat

class CPUMonitorDialog(wx.Dialog):
	"""
	This Dialog shows the CPU monitor, which allows monitoring
	CPU usage and individual plugin CPU consumption.
	"""
	def __init__(self, *args, **kwds):
		"""
		Initializer.
		"""
		kwds['style'] = wx.RESIZE_BORDER | wx.DEFAULT_DIALOG_STYLE
		wx.Dialog.__init__(self, *args, **kwds)
		self.SetMinSize((200,300))
		self.SetTitle("CPU Monitor")
		self.pluginlist = wx.ListCtrl(self, -1, style=wx.SUNKEN_BORDER | wx.LC_REPORT)
		self.pluginlist.InsertColumn(0, "Plugin", wx.LIST_AUTOSIZE)
		self.pluginlist.InsertColumn(1, "CPU Load", wx.LIST_AUTOSIZE)
		self.labeltotal = wx.StaticText(self, -1, "100%")
		self.gaugetotal = wx.Gauge(self, -1, style=wx.GA_SMOOTH|wx.GA_HORIZONTAL)
		self.gaugetotal.SetRange(100)
		sizer = wx.BoxSizer(wx.VERTICAL)
		sizer.Add(self.pluginlist, 1, wx.EXPAND)
		hsizer = wx.BoxSizer(wx.HORIZONTAL)
		hsizer.Add(self.gaugetotal, 1, wx.ALIGN_CENTER_VERTICAL | wx.RIGHT, 10)
		hsizer.Add(self.labeltotal, 0, wx.ALIGN_CENTER_VERTICAL)
		sizer.Add(hsizer, 0, wx.EXPAND | wx.ALL, border=5)
		self.SetAutoLayout(True)
		self.SetSizer(sizer)
		self.Layout()
		self.Fit()
		self.Centre()
		self.timer = wx.Timer(self, -1)
		self.timer.Start(250)
		wx.EVT_SIZE(self, self.on_size)
		wx.EVT_TIMER(self, self.timer.GetId(), self.on_timer)
		
	def on_size(self, event):
		"""
		Called when the panel is being resized.
		"""
		#event.Skip()
		self.Layout()
		x,y,w,h = self.pluginlist.GetClientRect()
		self.pluginlist.SetColumnWidth(0, w/2)
		self.pluginlist.SetColumnWidth(1, w/2)

	def on_timer(self, event):
		"""
		Called by timer event. Updates CPU usage statistics.
		
		@param event: timer event
		@type event: wx.TimerEvent
		"""
		if self.IsShown():
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
			total_workload = sum(cpu_loads.values())
			self.gaugetotal.SetValue(int(cpu + 0.5))
			self.labeltotal.SetLabel("%i%%" % int(cpu + 0.5))
			item = -1
			keys = []
			while True:
				item = self.pluginlist.GetNextItem(item, wx.LIST_NEXT_ALL, wx.LIST_STATE_DONTCARE)
				if item == -1:
					break
				name = self.pluginlist.GetItem(item,0).GetText()
				if name in cpu_loads:
					relperc = (cpu_loads[name] / total_workload) * cpu
					self.pluginlist.SetStringItem(item, 1, "%.1f%%" % relperc, -1)
					self.pluginlist.SetItemData(item, len(keys))
					keys.append(name)
					del cpu_loads[name]
				else:
					self.pluginlist.DeleteItem(item)
			for k,v in cpu_loads.iteritems():
				index = self.pluginlist.GetItemCount()
				k = prepstr(k)
				self.pluginlist.InsertStringItem(index, k)
				relperc = (v / total_workload) * cpu
				self.pluginlist.SetStringItem(index, 1, "%.1f%%" % relperc, -1)
				self.pluginlist.SetItemData(index, len(keys))
				keys.append(k)
			if cpu_loads.keys():
				def sort_cmp_func(a, b):
					a = keys[a].lower()
					b = keys[b].lower()
					return cmp(a,b)
				self.pluginlist.SortItems(sort_cmp_func)

__all__ = [
'CPUMonitorDialog',
]

if __name__ == '__main__':
	import sys, utils
	from main import run
	sys.argv.append(utils.filepath('demosongs/test.bmx'))
	run(sys.argv)
