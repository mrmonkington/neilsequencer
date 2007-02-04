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
Contains all classes and functions needed to render the wavetable editor and the envelope viewer.
"""

from wximport import wx
import os, sys, stat
from utils import prepstr, db2linear, linear2db, note2str, filepath
import utils
from canvas import Canvas
import zzub
import config
from envelope import EnvelopeView, ADSRPanel
import freesound

def get_node(item, path):
	for element in path.split('/'):
		found = False
		for node in item.childNodes:
			if (node.nodeType == node.ELEMENT_NODE) and (node.tagName == element):
				found = True
				item = node
				break
		if not found:
			return None
	return item

def extract_text(item):
	if not item:
		return ''
	text = ''
	for node in item.childNodes:
		if node.nodeType == item.TEXT_NODE:
			text += node.nodeValue
	return text.strip()
	
def format_filesize(size):
	if (size / (1<<40)):
		return "%.2f TB" % (float(size) / (1<<40))
	elif (size / (1<<30)):
		return "%.2f GB" % (float(size) / (1<<30))
	elif (size / (1<<20)):
		return "%.2f MB" % (float(size) / (1<<20))
	elif (size / (1<<10)):
		return "%.2f KB" % (float(size) / (1<<10))
	else:
		return "%i bytes" % size

def format_duration(secs):
	mins = int(secs / 60)
	secs = int((secs % 60) + 0.5)
	return "%i:%02i" % (mins,secs)
	
class WrapStaticText(wx.StaticText):
	def __init__(self,*args,**kargs):
		self._text = ''
		wx.StaticText.__init__(self,*args,**kargs)
		wx.EVT_SIZE(self, self.on_size)
		
	def SetLabel(self, text):
		self._text = text
		wx.StaticText.SetLabel(self, text)
		w,h = self.GetClientSize()
		self.Wrap(w)
		
	def on_size(self, event):
		if self.GetLabel() != self._text:
			self.SetLabel(self._text)

class FreesoundPanel(wx.Panel):
	COL_NAME = 0
	COL_DURATION = 1
	COL_FILESIZE = 2
	COL_ADDEDBY = 3
	COL_ID = 4
	
	def __init__(self, wavetable, *args, **kwds):
		self.wavetable = wavetable
		wx.Panel.__init__(self, *args, **kwds)
		self.splitter = wx.SplitterWindow(self, -1, style=wx.SP_LIVE_UPDATE|wx.SP_NOBORDER)
		self.leftpanel = wx.Panel(self.splitter, -1)
		sizer = wx.BoxSizer(wx.HORIZONTAL)
		vsizer = wx.BoxSizer(wx.VERTICAL)
		vsearch = wx.StaticBoxSizer(wx.StaticBox(self.leftpanel, -1, "Search"), wx.VERTICAL)
		vdetails = wx.StaticBoxSizer(wx.StaticBox(self.leftpanel, -1, "Details"), wx.VERTICAL)
		vdetailgrid = wx.FlexGridSizer(4,2)
		vdetailgrid.AddGrowableCol(1)
		self.detailsitems = [vdetails.GetStaticBox()]
		boldfont = self.GetFont()
		boldfont.SetWeight(wx.FONTWEIGHT_BOLD)
		def add_detail(title, value=None):
			label = wx.StaticText(self.leftpanel, -1, title)
			mx,my = label.GetMinSize()
			if not value:
				value = WrapStaticText(self.leftpanel, -1)
			label.SetFont(boldfont)
			self.detailsitems.append(label)
			self.detailsitems.append(value)
			vdetailgrid.Add(label, 0, wx.ALIGN_RIGHT|wx.RIGHT, 5)
			vdetailgrid.Add(value, 0, wx.EXPAND|wx.ALIGN_LEFT)
			return value
		self.dtname = add_detail("Name:")
		self.dtuser = add_detail("Added by:")
		self.dtdownloads = add_detail("Downloads:")
		self.dtrating = add_detail("Rating:")
		self.dttype = add_detail("Type:")
		self.dtlength = add_detail("Duration:")
		self.dtsize = add_detail("Filesize:")
		self.dttags = add_detail("Tags:")
		self.dtdesc = add_detail("Description:")
		self.imgwave = wx.StaticBitmap(self.leftpanel, -1)
		self.detailsitems.append(self.imgwave)
		vdetails.Add(self.imgwave, 0, wx.EXPAND|wx.BOTTOM, 5)
		vdetails.Add(vdetailgrid, 1, wx.EXPAND)
		searchgroup = wx.BoxSizer(wx.HORIZONTAL)
		self.edsearch = wx.TextCtrl(self.leftpanel, -1, style=wx.TE_PROCESS_ENTER)
		self.progress = wx.Gauge(self.leftpanel, -1)
		searchgroup.Add(self.edsearch, 1, wx.ALIGN_CENTER_VERTICAL)
		self.btnsearchtags = wx.CheckBox(self.leftpanel, -1, "Tags")
		self.btnsearchdesc = wx.CheckBox(self.leftpanel, -1, "Descriptions")
		self.btnsearchfiles = wx.CheckBox(self.leftpanel, -1, "Files")
		self.btnsearchusers = wx.CheckBox(self.leftpanel, -1, "People")
		self.btncancel = wx.BitmapButton(self.leftpanel, -1, wx.Bitmap(filepath("res/cancel.png"), wx.BITMAP_TYPE_ANY), style=wx.NO_BORDER)
		self.btncancel.SetToolTip(wx.ToolTip("Cancel Search"))
		self.btnsearchtags.SetValue(True)
		imglist = wx.ImageList(16,16)
		self.IMG_FOLDER = imglist.Add(wx.Bitmap(filepath("res/folder.png"), wx.BITMAP_TYPE_ANY))
		self.IMG_WAVE = imglist.Add(wx.Bitmap(filepath("res/wave.png"), wx.BITMAP_TYPE_ANY))
		self.resultlist = wx.ListView(self.splitter, -1, style=wx.SUNKEN_BORDER | wx.LC_REPORT)
		self.resultlist.AssignImageList(imglist, wx.IMAGE_LIST_SMALL)
		self.resultlist.InsertColumn(self.COL_NAME, "Name", wx.LIST_FORMAT_LEFT, 250)
		self.resultlist.InsertColumn(self.COL_DURATION, "Duration", wx.LIST_FORMAT_RIGHT)
		self.resultlist.InsertColumn(self.COL_FILESIZE, "Filesize", wx.LIST_FORMAT_RIGHT)
		self.resultlist.InsertColumn(self.COL_ADDEDBY, "Added by")
		self.resultlist.InsertColumn(self.COL_ID, "#", wx.LIST_FORMAT_RIGHT)
		vsearch.Add(searchgroup, 0, wx.EXPAND)
		progressgroup = wx.BoxSizer(wx.HORIZONTAL)
		progressgroup.Add(self.progress, 1, wx.RIGHT, 5)
		progressgroup.Add(self.btncancel, 0, wx.EXPAND)
		vsearch.Add(self.btnsearchtags)
		vsearch.Add(self.btnsearchdesc)
		vsearch.Add(self.btnsearchfiles)
		vsearch.Add(self.btnsearchusers)
		vsearch.Add(progressgroup, 0, wx.EXPAND|wx.TOP|wx.LEFT|wx.RIGHT, 5)
		logo = wx.StaticBitmap(self.leftpanel, -1, wx.Bitmap(filepath("res/fsbanner.png"), wx.BITMAP_TYPE_ANY))
		logo.SetBackgroundColour('#ffffff')
		vsizer.Add(logo, 0, wx.EXPAND)
		vsizer.Add(vsearch, 0, wx.EXPAND|wx.TOP|wx.LEFT|wx.BOTTOM, 5)
		vsizer.Add(vdetails, 0, wx.EXPAND|wx.LEFT|wx.BOTTOM, 5)
		self.leftpanel.SetAutoLayout(True)
		self.leftpanel.SetSizer(vsizer)
		self.splitter.SetMinimumPaneSize(250)
		self.splitter.SplitVertically(self.leftpanel, self.resultlist, 250)
		sizer.Add(self.splitter, 1, wx.EXPAND)
		self.SetAutoLayout(True)
		self.SetSizer(sizer)
		self.Layout()
		self.progress.Hide()
		self.btncancel.Hide()
		self.cmds = []
		self.cancel = False
		self.populatetimer = wx.Timer(self, -1)
		self.populatetimer.Start(100)
		wx.EVT_TIMER(self, self.populatetimer.GetId(), self.on_populate)
		wx.EVT_BUTTON(self, self.btncancel.GetId(), self.on_cancel)
		wx.EVT_TEXT_ENTER(self, self.edsearch.GetId(), self.on_search)
		wx.EVT_LIST_ITEM_SELECTED(self, self.resultlist.GetId(), self.on_select)
		wx.EVT_LEFT_DCLICK(self.resultlist, self.on_resultlist_dclick)
		wx.EVT_KEY_DOWN(self.resultlist, self.on_resultlist_key_down)
		self.client = None
		self.results = {}
		self.imgcache = {}
		self.wavecache = {}
		self.show_details()
		import thread
		thread.start_new_thread(self.preload_freesound, ())
		
	def get_selection(self):
		"""
		Returns a list of selected indices.
		"""
		index = self.resultlist.GetFirstSelected()
		if index == -1:
			return []
		results = [index]
		while True:
			index = self.resultlist.GetNextSelected(index)
			if index == -1:
				break
			results.append(index)
		return results
		
	def on_resultlist_key_down(self, event):
		"""
		Called when the key is pressed in the result list.
		"""
		k = event.GetKeyCode()
		sellist = self.get_selection()
		sel = sellist and sellist[0] or 0
		if k in (wx.WXK_RIGHT,wx.WXK_NUMPAD_RIGHT):
			self.preview_current()
		elif k == wx.WXK_SPACE:
			self.preview_current()
			for s in sellist:
				self.resultlist.Select(s, False)
			sel = min(max(sel+1, 0), self.resultlist.GetItemCount()-1)
			self.resultlist.Focus(sel)
			self.resultlist.Select(sel, True)
			self.resultlist.EnsureVisible(sel)
		elif k in (wx.WXK_BACK, wx.WXK_RETURN):
			self.on_load_sample(None)
		elif k == wx.WXK_ESCAPE:
			self.wavetable.notebook.SetSelection(0)
			self.wavetable.samplelist.SetFocus()
		elif k in (wx.WXK_LEFT,wx.WXK_NUMPAD_LEFT):
			self.edsearch.SetSelection(-1,-1)
			self.edsearch.SetFocus()
		else:
			event.Skip()
			return
			
	def on_load_sample(self, event):
		"""
		Called to load samples based on the current file list selection.
		
		Samples will first be downloaded to the local freesound folder,
		and then into the wavetable.
		"""
		import thread
		files = []
		for index in self.get_selection():
			sampleid = self.resultlist.GetItemData(index)
			if sampleid > 0:
				sample = self.results[sampleid]
				orgfilename = extract_text(get_node(sample,'originalFilename'))
				ext = os.path.splitext(orgfilename)[1].lower()
				filename = str(sampleid) + ext
				fullpath = os.path.join(config.get_config().get_freesound_samples_folder(), filename)
				files.append((sampleid, fullpath))
		if files:
			progress = wx.ProgressDialog(
				"Downloading Samples...", 
				"Downloading samples from freesound...", 
				parent=self, 
				style=wx.PD_AUTO_HIDE | wx.PD_APP_MODAL | wx.PD_REMAINING_TIME)
			thread.start_new_thread(self.download_samples, (progress, files,))
			
	def import_samples(self, filenames):
		"""
		Imports a sequence of samples to the wavetable.
		"""
		self.wavetable.load_samples(filenames)
	
	def download_samples(self, progress, files):
		outfiles = []
		import re
		rx = re.compile(r'inline[;]\s*filename[=]["]([^"]*)["]')
		try:
			fs = self.get_freesound(warn=True)
			assert fs
			filecount = len(files)
			for index, (sampleid, path) in enumerate(files):
				value = (float(index) / filecount) * 100.0
				self.cmds.append((progress.Update, (value, "Downloading sample #%s (%s of %s)..." % (sampleid,index+1,filecount))))
				fname, headers = fs.download(sampleid, path)
				if headers['content-type'] == 'application/octet-stream':
					basedir = os.path.dirname(fname)
					newpath = os.path.join(basedir, rx.match(headers['content-disposition']).group(1))
					print "moving %s to %s..." % (fname, newpath)
					os.rename(fname, newpath)
					outfiles.append(newpath)
				else:
					print >> sys.stderr, "wrong content type for %s: %s" % (path, headers['content-type'])
		except:
			import traceback
			traceback.print_exc()
		self.cmds.append((progress.Destroy,()))
		self.cmds.append((self.import_samples,(outfiles,)))
				
	def preview_current(self):
		"""
		Preview currently focused item.
		"""
		import thread
		index = self.resultlist.GetFocusedItem()
		if index > -1:
			sampleid = self.resultlist.GetItemData(index)
			if sampleid > 0:
				sample = self.results[sampleid]
				thread.start_new_thread(self.load_preview_wave, (extract_text(get_node(sample,'preview')),))

		
	def on_resultlist_dclick(self, event):
		"""
		Called when a list entry is doubleclicked.
		"""
		self.preview_current()
		
	def preload_freesound(self):
		"""
		Loads freesound service and logs in.
		"""
		self.cmds.append((self.resultlist.InsertStringItem,(0, "Logging in...")))
		self.cmds.append((self.edsearch.SetEditable, (False,)))
		self.get_freesound()
		self.cmds.append((self.edsearch.SetEditable, (True,)))
		self.cmds.append((self.resultlist.DeleteAllItems,()))
		
	def on_cancel(self, event):
		self.cancel = True
		
	def preview_wave(self, url):
		if self.active_wave_url != url:
			print "hum."
			return
		path = self.wavecache[url].name
		print "playing %s (%s)" % (path, format_filesize(os.stat(path)[stat.ST_SIZE]))
		self.wavetable.preview_sample(path)
		
	def load_preview_wave(self, url):
		import urllib
		self.active_wave_url = url
		if not url in self.wavecache:
			import tempfile
			outf = tempfile.NamedTemporaryFile(suffix=".mp3")
			print url,"=>",outf.name
			fname, headers = self.get_freesound(warn=True).retrieve(url, outf.name)
			self.wavecache[url] = outf
		self.cmds.append((self.preview_wave, (url,)))
		
	def apply_image(self, url):
		if self.active_url != url:
			return
		bmp = wx.BitmapFromImage(self.imgcache[url])
		self.imgwave.SetBitmap(bmp)
		self.imgwave.Show()
		self.Layout()
		
	def load_wave_image(self, url):
		self.active_url = url
		if not url in self.imgcache:
			import StringIO
			bmpdata = self.get_freesound(warn=True).get(url)
			img = wx.ImageFromStream(StringIO.StringIO(bmpdata), wx.BITMAP_TYPE_PNG)
			img.Rescale(200, 48)
			self.imgcache[url] = img
		self.cmds.append((self.apply_image, (url,)))
		
	def show_details(self, sample=None):
		if sample:
			self.imgwave.SetBitmap(wx.NullBitmap)
			import thread
			thread.start_new_thread(self.load_wave_image, (extract_text(get_node(sample,'image')),))
			
			self.dtname.SetLabel(extract_text(get_node(sample,'originalFilename')))
			self.dtuser.SetLabel(extract_text(get_node(sample,'user/name')))
			self.dtdownloads.SetLabel(extract_text(get_node(sample,'statistics/downloads')))
			self.dtrating.SetLabel(extract_text(get_node(sample,'statistics/rating')))
			ext = extract_text(get_node(sample,'extension'))
			sr = extract_text(get_node(sample,'samplerate'))
			if sr:
				sr += 'Hz'
			br = extract_text(get_node(sample,'bitrate'))
			if br:
				br += 'kbps'
			bd = extract_text(get_node(sample,'bitdepth'))
			if bd:
				bd += ' bits'
			chans = {'':'Mono','1':'Mono','2':'Stereo'}[extract_text(get_node(sample,'channels'))]
			self.dttype.SetLabel(', '.join([x for x in [ext,sr,br,bd,chans] if x]))
			length = extract_text(get_node(sample,'duration'))
			if length:
				length = format_duration(float(length))
			self.dtlength.SetLabel(length)
			filesize = extract_text(get_node(sample,'filesize'))
			if filesize:
				filesize = format_filesize(int(filesize))
			self.dtsize.SetLabel(filesize)
			tags = []
			if get_node(sample,'tags'):
				for tag in get_node(sample,'tags').childNodes:
					if (tag.nodeType == tag.ELEMENT_NODE) and (tag.nodeName == 'tag'):
						tags.append(extract_text(tag))
			self.dttags.SetLabel(', '.join(tags))
			self.dtdesc.SetLabel(extract_text(get_node(sample,'descriptions/description/text')))
			for item in self.detailsitems:
				if item != self.imgwave:
					item.Show()
		else:
			for item in self.detailsitems:
				item.Hide()
		self.Fit()
		self.Layout()
		
	def insert_item(self, itemid, sample):
		originalFilename = extract_text(sample.getElementsByTagName('originalFilename')[0])
		addedby = extract_text(get_node(sample,'user/name'))
		rating = extract_text(sample.getElementsByTagName('rating')[0])
		downloads = extract_text(sample.getElementsByTagName('downloads')[0])
		date = extract_text(sample.getElementsByTagName('date')[0])
		duration = extract_text(get_node(sample,'duration'))
		if duration:
			duration = format_duration(float(duration))
		filesize = extract_text(get_node(sample,'filesize'))
		if filesize:
			filesize = format_filesize(int(filesize))
		index = self.resultlist.GetItemCount()
		self.resultlist.InsertStringItem(index, prepstr(originalFilename))
		self.resultlist.SetStringItem(index, self.COL_DURATION, duration)
		self.resultlist.SetStringItem(index, self.COL_FILESIZE, filesize)
		self.resultlist.SetStringItem(index, self.COL_ADDEDBY, addedby)
		self.resultlist.SetStringItem(index, self.COL_ID, itemid)
		self.resultlist.SetItemData(index, int(itemid))
		self.resultlist.SetItemImage(index, self.IMG_WAVE)
			
	def on_select(self, event):
		"""
		Called when the user selects an entry.
		"""
		index = self.resultlist.GetFocusedItem()
		if index > -1:
			sampleid = self.resultlist.GetItemData(index)
			if sampleid > 0:
				self.show_details(self.results[sampleid])
				return
		self.show_details()
		
	def get_freesound(self, warn=False):
		"""
		Returns a freesound client.
		"""
		if not self.client:
			uname, passwd = config.get_config().get_credentials("Freesound")
			if (not (uname and passwd)):
				if warn:
					self.cmds.append((utils.error,(self, "You have not provided login information for freesound yet.\n\nYou can set up your username and your password in preferences.")))
				return None
			client = freesound.Freesound()
			try:
				client.login(uname,passwd)
			except:
				self.cmds.append((utils.error,(self, "There was an error logging into freesound.")))
				return None
			self.client = client
		return self.client
		
	def on_populate(self, event):
		"""
		Populates the list control from results.
		"""
		while self.cmds:
			cmd,args = self.cmds[0]
			self.cmds = self.cmds[1:]
			try:
				cmd(*args)
			except:
				import traceback
				traceback.print_exc()
				
	def show_error(self, message):
		msgdlg = wx.MessageDialog(self, message=message, caption = "Aldrin", style = wx.ICON_ERROR|wx.OK|wx.CENTER)
		self.cmds.append((msgdlg.ShowModal,()))
		
	def cancel_search(self):
		self.cmds.append((self.progress.Hide,()))
		self.cmds.append((self.btncancel.Hide,()))
		self.cmds.append((self.Layout,()))
		
	def search_thread(self, args):
		self.cancel = False
		self.cmds.append((self.progress.SetValue,(0,)))
		self.cmds.append((self.progress.Show,()))
		self.cmds.append((self.btncancel.Show,()))
		self.cmds.append((self.Layout,()))
		try:
			fs = self.get_freesound(warn=True)
			assert fs
			if args['search'].startswith('#') and args['search'][1:].isdigit():
				items = [args['search'][1:]]
			else:
				self.result = fs.search(**args)
				items = [item.getAttribute("id") for item in self.result.childNodes if item.nodeType == item.ELEMENT_NODE and item.nodeName == 'sample']
			self.cmds.append((self.resultlist.DeleteAllItems,()))
			self.results = {}
			if len(items) > 1000:
				self.cmds.append((self.resultlist.InsertStringItem,(0, "Too many hits (max. 1000). Please refine your search.")))
				self.cancel_search()
				return
			elif not items:
				self.cmds.append((self.resultlist.InsertStringItem,(0, "No results matching your search terms.")))
			else:
				self.cmds.append((self.resultlist.InsertStringItem,(0, "Querying %s result(s)..." % len(items))))
			for index, itemid in enumerate(items):
				if self.cancel:
					break
				self.cmds.append((self.progress.SetValue,(int(100.0/len(items) * index + 0.5),)))
				#itemid = item.getAttribute("id")
				for i in range(3):
					try:
						sampleitem = fs.get_sample_info(itemid)
						if index == 0:
							self.cmds.append((self.resultlist.DeleteAllItems,()))
						sample = sampleitem.getElementsByTagName('sample')[0]
						self.results[int(itemid)] = sample
						self.cmds.append((self.insert_item, (itemid, sample)))
						break
					except:
						import traceback
						traceback.print_exc()
		except:
			import traceback
			traceback.print_exc()
			self.cmds.append((self.resultlist.DeleteAllItems,()))
		self.cancel_search()
		
	def on_search(self, event):
		"""
		Called when user hits return in search field.
		"""
		self.resultlist.DeleteAllItems()
		self.resultlist.InsertStringItem(0, "Searching...")
		args = dict(
			search=self.edsearch.GetValue(),
			searchDescriptions=int(self.btnsearchdesc.GetValue()),
			searchTags=int(self.btnsearchtags.GetValue()),
			searchFilenames=int(self.btnsearchfiles.GetValue()),
			searchUsernames=int(self.btnsearchusers.GetValue())
		)
		import thread
		thread.start_new_thread(self.search_thread, (args,))
		self.resultlist.SetFocus()
		

class WavetablePanel(wx.Panel):
	"""
	Wavetable editor.
	
	Contains a list of samples loaded in the song and a file list showing wave files in the file system. 
	It contains controls to transfer files between the song and the file system, and components that facilitate
	sample editing for example loops and envelopes.
	"""
	allowed_extensions = ['.wav','.mp3','.flac', '.aif']
	
	def __init__(self, rootwindow, *args, **kwds):
		"""
		Initialization.
		"""
		# begin wxGlade: SequencerFrame.__init__
		#kwds["style"] = wx.DEFAULT_PANEL_STYLE		
		self.working_directory = ''
		self.rootwindow = rootwindow
		self.files = []
		wx.Panel.__init__(self, *args, **kwds)
		self.notebook = wx.Notebook(self, -1, style = wx.NB_BOTTOM)
		self.instrpanel = wx.Panel(self.notebook, -1)
		self.libpanel = wx.Panel(self.notebook, -1)
		self.fspanel = FreesoundPanel(self, self.notebook, -1)
		self.notebook.InsertPage(0, self.instrpanel, "Instruments")
		self.notebook.InsertPage(1, self.libpanel, "Library")
		self.notebook.InsertPage(2, self.fspanel, "freesound")
		self.notebook.SetSelection(0)
		self.adsrpanel = ADSRPanel(self, self.instrpanel, -1)
		self.samplelist = wx.ListView(self.instrpanel, -1, style=wx.SUNKEN_BORDER | wx.LC_REPORT)
		self.samplelist.InsertColumn(0, "#", wx.LIST_FORMAT_RIGHT)
		self.samplelist.InsertColumn(1, "Name", wx.LIST_FORMAT_LEFT)
		imglist = wx.ImageList(16,16)
		self.IMG_SAMPLE_WAVE = imglist.Add(wx.Bitmap(filepath("res/wave.png"), wx.BITMAP_TYPE_ANY))
		self.samplelist.AssignImageList(imglist, wx.IMAGE_LIST_SMALL)
		#self.samplelist.SetMinSize((200,5))
		#~ self.subsamplelist = wx.ListCtrl(self, -1, style=wx.SUNKEN_BORDER | wx.LC_REPORT)
		buttonstyle = wx.NO_BORDER
		self.btnstoresample = wx.BitmapButton(self.instrpanel, -1, wx.Bitmap(filepath("res/storesample.png"), wx.BITMAP_TYPE_ANY), style=buttonstyle)
		self.btnstoresample.SetToolTip(wx.ToolTip("Save Instrument"))
		self.btnstop = wx.BitmapButton(self.instrpanel, -1, wx.Bitmap(filepath("res/control_stop.png"), wx.BITMAP_TYPE_ANY), style=buttonstyle)
		self.btnstop.SetToolTip(wx.ToolTip("Stop Preview"))
		self.btnplay = wx.BitmapButton(self.instrpanel, -1, wx.Bitmap(filepath("res/control_play.png"), wx.BITMAP_TYPE_ANY), style=buttonstyle)
		self.btnplay.SetToolTip(wx.ToolTip("Preview Sample"))
		self.btnrename = wx.BitmapButton(self.instrpanel, -1, wx.Bitmap(filepath("res/rename.png"), wx.BITMAP_TYPE_ANY), style=buttonstyle)
		self.btnrename.SetToolTip(wx.ToolTip("Rename Instrument"))
		self.btnclear = wx.BitmapButton(self.instrpanel, -1, wx.Bitmap(filepath("res/clear.png"), wx.BITMAP_TYPE_ANY), style=buttonstyle)
		self.btnclear.SetToolTip(wx.ToolTip("Remove Instrument"))
		self.btnadsr = wx.BitmapButton(self.instrpanel, -1, wx.Bitmap(filepath("res/adsr.png"), wx.BITMAP_TYPE_ANY), style=buttonstyle)
		self.btnadsr.SetToolTip(wx.ToolTip("Create ADSR Envelope"))
		self.btnfitloop = wx.BitmapButton(self.instrpanel, -1, wx.Bitmap(filepath("res/fitloop.png"), wx.BITMAP_TYPE_ANY), style=buttonstyle)
		self.btnfitloop.SetToolTip(wx.ToolTip("Fit Loop"))
		self.samplename = wx.StaticText(self.instrpanel, -1, label = "")
		self.volumeslider = wx.Slider(self.instrpanel, -1)
		self.volumeslider.SetRange(-4800, 2400)
		self.chkloop = wx.CheckBox(self.instrpanel, -1, label = "&Loop")
		self.edloopstart = wx.TextCtrl(self.instrpanel, -1, style=wx.TE_PROCESS_ENTER)
		self.edloopend = wx.TextCtrl(self.instrpanel, -1, style=wx.TE_PROCESS_ENTER)
		self.edsamplerate = wx.TextCtrl(self.instrpanel, -1, style=wx.TE_PROCESS_ENTER)
		self.chkpingpong = wx.CheckBox(self.instrpanel, -1, label = "&Ping Pong")
		self.cbmachine = wx.Choice(self.instrpanel, -1)
		self.cbenvelope = wx.Choice(self.instrpanel, -1)
		self.chkenable = wx.CheckBox(self.instrpanel, -1, label = "Active")
		self.envelope = EnvelopeView(self, self.instrpanel, -1)
		self.envelope.SetMinSize((-1,150))
		imglist = wx.ImageList(16,16)
		self.IMG_FOLDER = imglist.Add(wx.Bitmap(filepath("res/folder.png"), wx.BITMAP_TYPE_ANY))
		self.IMG_WAVE = imglist.Add(wx.Bitmap(filepath("res/wave.png"), wx.BITMAP_TYPE_ANY))
		self.filelist = wx.ListView(self.libpanel, -1, style=wx.SUNKEN_BORDER | wx.LC_REPORT)
		self.filelist.AssignImageList(imglist, wx.IMAGE_LIST_SMALL)
		self.filelist.InsertColumn(0, "Name", wx.LIST_FORMAT_LEFT, 250)
		self.filelist.InsertColumn(1, "Filesize", wx.LIST_FORMAT_RIGHT)
		self.filelist.InsertColumn(2, "Type")

		self.btnloadsample = wx.BitmapButton(self.libpanel, -1, wx.Bitmap(filepath("res/loadsample.png"), wx.BITMAP_TYPE_ANY), style=buttonstyle)
		self.btnloadsample.SetToolTip(wx.ToolTip("Add/Insert Instrument"))
		self.btnrefresh = wx.BitmapButton(self.libpanel, -1, wx.Bitmap(filepath("res/arrow_refresh.png"), wx.BITMAP_TYPE_ANY), style=buttonstyle)
		self.btnrefresh.SetToolTip(wx.ToolTip("Refresh Browser"))
		self.btnparent = wx.BitmapButton(self.libpanel, -1, wx.Bitmap(filepath("res/parent.png"), wx.BITMAP_TYPE_ANY), style=buttonstyle)
		self.btnparent.SetToolTip(wx.ToolTip("Go to Parent Folder"))
		self.stworkpath = wx.StaticText(self.libpanel, -1, "")

		self.__set_properties()
		self.__do_layout()
		self.update_all()
		wx.EVT_LIST_ITEM_SELECTED(self, self.samplelist.GetId(), self.on_samplelist_select)
		wx.EVT_LEFT_DCLICK(self.filelist, self.on_filelist_dclick)
		wx.EVT_LEFT_DCLICK(self.samplelist, self.on_samplelist_dclick)
		wx.EVT_BUTTON(self, self.btnparent.GetId(), self.on_parent_click)
		wx.EVT_BUTTON(self, self.btnloadsample.GetId(), self.on_load_sample)
		wx.EVT_BUTTON(self, self.btnstoresample.GetId(), self.on_save_sample)
		wx.EVT_BUTTON(self, self.btnplay.GetId(), self.on_play_wave)
		wx.EVT_BUTTON(self, self.btnstop.GetId(), self.on_stop_wave)
		wx.EVT_BUTTON(self, self.btnrefresh.GetId(), self.on_refresh)
		wx.EVT_BUTTON(self, self.btnclear.GetId(), self.on_clear)
		wx.EVT_BUTTON(self, self.btnadsr.GetId(), self.on_show_adsr)
		wx.EVT_BUTTON(self, self.btnfitloop.GetId(), self.on_fit_loop)
		wx.EVT_SCROLL(self.volumeslider, self.on_scroll_changed)
		wx.EVT_MOUSEWHEEL(self.volumeslider, self.on_mousewheel)
		wx.EVT_CHECKBOX(self, self.chkloop.GetId(), self.on_check_loop)
		wx.EVT_CHECKBOX(self, self.chkpingpong.GetId(), self.on_check_pingpong)
		wx.EVT_CHECKBOX(self, self.chkenable.GetId(), self.on_check_envdisabled)		
		wx.EVT_KILL_FOCUS(self.edloopstart, self.on_loop_start_apply)
		wx.EVT_KILL_FOCUS(self.edloopend, self.on_loop_end_apply)
		wx.EVT_KILL_FOCUS(self.edsamplerate, self.on_samplerate_apply)
		wx.EVT_TEXT_ENTER(self, self.edloopstart.GetId(), self.on_loop_start_apply)
		wx.EVT_TEXT_ENTER(self, self.edloopend.GetId(), self.on_loop_end_apply)
		wx.EVT_TEXT_ENTER(self, self.edsamplerate.GetId(), self.on_samplerate_apply)
		wx.EVT_KEY_DOWN(self.filelist, self.on_filelist_key_down)
		wx.EVT_KEY_DOWN(self.samplelist, self.on_samplelist_key_down)
		wx.EVT_CHAR(self.filelist, self.on_filelist_char)
		wx.EVT_SIZE(self, self.on_size)
		
	def on_show_adsr(self, event):
		"""
		Called when the ADSR button is clicked. Shows the ADSR Dialog.
		"""
		#self.adsrpanel.Show()
		self.adsrpanel.update_envelope()
		
	def on_size(self, event):
		"""
		Called when the panel is being resized.
		"""
		self.Layout()
		#event.Skip()
		x,y,w,h = self.filelist.GetClientRect()
		w -= 32
		self.filelist.SetColumnWidth(0, int(w * 0.5))
		self.filelist.SetColumnWidth(1, int(w * 0.25))
		self.filelist.SetColumnWidth(2, int(w * 0.25))
		x,y,w,h = self.samplelist.GetClientRect()
		w -= 32
		self.samplelist.SetColumnWidth(0, 48)
		self.samplelist.SetColumnWidth(1, w-48)
		
	def get_sample_selection(self):
		"""
		Returns a list with currently selected sample indices.
		"""
		index = self.samplelist.GetFirstSelected()
		if index == -1:
			return []
		results = [index]
		while True:
			index = self.samplelist.GetNextSelected(index)
			if index == -1:
				break
			results.append(index)
		return results
		
	def on_samplerate_apply(self, event):
		"""
		Callback that responds to changes in the sample rate edit field.
		
		@param event: CommandEvent event
		@type event: wx.CommandEvent
		"""
		try:
			v = min(max(int(self.edsamplerate.GetValue()),50),200000)
		except ValueError:
			return
		for i in self.get_sample_selection():
			w = player.get_wave(i)
			if w.get_level_count() >= 1:
				w.get_level(0).set_samples_per_second(v)
		self.update_sampleprops()
		#~ self.update_subsamplelist()
		
	def on_loop_start_apply(self, event):
		"""
		Callback that responds to changes in the loop-start edit field.
		
		@param event: CommandEvent event
		@type event: wx.CommandEvent
		"""
		try:
			v = int(self.edloopstart.GetValue())
		except ValueError:
			print "invalid value."
			return
		for i in self.get_sample_selection():
			w = player.get_wave(i)
			if w.get_level_count() >= 1:
				level = w.get_level(0)
				level.set_loop_start(min(max(v,0),level.get_loop_end()-1))
		self.update_sampleprops()
		#~ self.update_subsamplelist()

	def on_loop_end_apply(self, event):
		"""
		Callback that responds to changes in the loop-end edit field.
		
		@param event: CommandEvent event
		@type event: wx.CommandEvent
		"""
		try:
			v = int(self.edloopend.GetValue())
		except ValueError:
			print "invalid value."
			return
		for i in self.get_sample_selection():
			w = player.get_wave(i)
			if w.get_level_count() >= 1:
				level = w.get_level(0)
				level.set_loop_end(min(max(v,level.get_loop_start()+1),level.get_sample_count()))
		self.update_sampleprops()
		#~ self.update_subsamplelist()

	def on_check_pingpong(self, event):
		"""
		Callback of checkbox that enables or disables bidirectional looping for the selected sample.
		
		@param event: CommandEvent event
		@type event: wx.CommandEvent
		"""
		for i in self.get_sample_selection():
			w = player.get_wave(i)
			flags = w.get_flags()
			if (self.chkpingpong.GetValue()):
				flags = flags | zzub.zzub_wave_flag_pingpong
			else:
				flags = flags ^ (flags & zzub.zzub_wave_flag_pingpong)
			w.set_flags(flags)
		self.update_sampleprops()

	def on_check_loop(self, event):
		"""
		Callback of checkbox that enables or disables looping for the selected sample.
		
		@param event: CommandEvent event
		@type event: wx.CommandEvent
		"""
		for i in self.get_sample_selection():
			w = player.get_wave(i)
			flags = w.get_flags()
			if (self.chkloop.GetValue()):
				flags = flags | zzub.zzub_wave_flag_loop
			else:
				flags = flags ^ (flags & zzub.zzub_wave_flag_loop)
			w.set_flags(flags)
		self.update_sampleprops()
		
	def on_check_envdisabled(self, event):
		"""
		Callback of checkbox that enables or disables the envelope for the selected sample.
		
		@param event: CommandEvent event
		@type event: wx.CommandEvent
		"""
		for i in self.get_sample_selection():
			w = player.get_wave(i)
			if w.get_envelope_count():				
				env = w.get_envelope(0)
				enabled = self.chkenable.GetValue()
				env.enable(enabled)
				if enabled:
					w.set_flags(w.get_flags() | zzub.zzub_wave_flag_envelope)
		self.update_sampleprops()
		
	def on_scroll_changed(self, event):		
		"""
		Callback that responds to change in the wave volume slider.
		
		@param event: CommandEvent event
		@type event: wx.CommandEvent		
		"""
		vol = db2linear(self.volumeslider.GetValue() / 100.0)		
		for i in self.get_sample_selection():
			w = player.get_wave(i)
			w.set_volume(vol)
			
	def on_mousewheel(self, event):
		"""
		Sent when the mousewheel is used on the volume slider.

		@param event: A mouse event.
		@type event: wx.MouseEvent
		"""
		vol = self.volumeslider.GetValue()
		step = 100
		if event.m_wheelRotation > 0:
			vol += step
		else:
			vol -= step
		vol = min(max(self.volumeslider.GetMin(),vol), self.volumeslider.GetMax())
		self.volumeslider.SetValue(vol)
		self.on_scroll_changed(event)
		
	def on_clear(self, event):
		"""
		Callback of a button that clears the selected sample in the sample list.
		
		@param event: CommandEvent event
		@type event: wx.CommandEvent		
		"""
		for i in self.get_sample_selection():
			player.get_wave(i).clear()
		self.update_samplelist()
		self.update_sampleprops()
		#~ self.update_subsamplelist()
		self.rootwindow.patternframe.update_all()
		
	def on_refresh(self, event):
		"""
		Callback of a button that refreshes the file list.
		
		@param event: CommandEvent event
		@type event: wx.CommandEvent		
		"""
		self.working_directory = ''
		self.stworkpath.SetLabel(self.working_directory)
		self.update_filelist()
		
	def update_all(self):
		"""
		Updates all the components in the wave table.
		"""
		self.update_samplelist()
		self.update_filelist()
		self.update_sampleprops()
		#~ self.update_subsamplelist()
		self.envelope.update()
		
	def update_wave_amp(self):
		"""
		Updates the wave players current amplitude from the config.
		"""
		# master = player.get_plugin(0)
		# vol = -76.0 * (master.get_parameter_value(1, 0, 0) / 16384.0)
		vol = min(max(config.get_config().get_sample_preview_volume(),-76.0),0.0)
		amp = db2linear(vol,limit=-76.0)
		player.set_wave_amp(amp)
		
	def on_play_wave(self, event):
		"""
		Callback of a button that plays the currently selected sample in the sample list.
		
		@param event: CommandEvent event
		@type event: wx.CommandEvent		
		"""		
		selects = self.get_sample_selection()
		if selects:
			w = player.get_wave(selects[0])
			if w.get_level_count() >= 1:
				self.update_wave_amp()
				player.play_wave(w, 0, (4 << 4) + 1)
		
	def on_stop_wave(self, event):
		"""
		Callback of a button that stops playback of a wave file that is currently playing.
		
		@param event: CommandEvent event
		@type event: wx.CommandEvent		
		"""
		player.stop_wave()
		
	def on_save_sample(self, event):
		"""
		Callback that responds to clicking the save sample button.
		Saves a sample to disk.
		
		@param event: Command event
		@type event: wx.CommandEvent
		"""
		selects = self.get_sample_selection()
		for s in selects:
			w = player.get_wave(s)
			origpath = w.get_path().replace('/',os.sep).replace('\\',os.sep)
			if origpath:
				filename = os.path.splitext(os.path.basename(origpath))[0] + '.wav'
			else:
				filename = w.get_name() + '.wav'
			WAVE_WILDCARD = '|'.join([
				"Wave Files (*.wav)","*.wav",
			])
			dlg = wx.FileDialog(
				self, 
				message="Export Wave", 
				defaultFile = filename,
				wildcard = WAVE_WILDCARD,
				style=wx.SAVE | wx.OVERWRITE_PROMPT)
			dlg.SetFilterIndex(0)
			if dlg.ShowModal() == wx.ID_OK:
				filepath = dlg.GetPath()
				print w.save_sample(0, filepath)
			else:
				return
				
	def load_samples(self, samplepaths):
		"""
		Loads a list of samples into the sample list of the song.
		"""
		if not samplepaths:
			return
		selects = self.get_sample_selection()
		# if sample selection less than files, increase sample selection
		if len(selects) < len(samplepaths):
			diffcount = len(samplepaths) - len(selects)			
			selects += tuple(range(selects[-1]+1,selects[-1]+1+diffcount))			
		# if sample selection more than files, set sample selection equal to number files
		elif len(selects) > len(samplepaths):
			selects = selects[:len(samplepaths)]
		assert len(selects) == len(samplepaths)
		for source,target in zip(samplepaths, selects):
			print "loading %s => %s" % (source,target)
			try:
				player.lock()
				w = player.get_wave(target)
				w.clear()
				res = w.load_sample(0, source)
			except:
				import traceback
				traceback.print_exc()
			player.unlock()
			if res != 0:
				wx.MessageDialog(self, message="There was a problem loading '%s'." % source, caption = "Aldrin", style = wx.ICON_ERROR|wx.OK|wx.CENTER).ShowModal()
			else:
				w.set_name(os.path.splitext(os.path.basename(source))[0])
		self.update_samplelist()
		self.update_sampleprops()
		self.rootwindow.patternframe.update_all()
		self.notebook.SetSelection(0)
		self.samplelist.SetFocus()
		
	def on_load_sample(self, event):
		"""
		Callback that responds to clicking the load sample button. 
		Loads a sample from the file list into the sample list of the song.
		
		@param event: CommandEvent event
		@type event: wx.CommandEvent		
		"""
		samplepaths = [path for path in [os.path.join(self.working_directory, self.files[x][0]) for x in self.get_filelist_selection()] if os.path.isfile(path)]
		self.load_samples(samplepaths)
		
	def get_wavetable_paths(self):
		"""
		Returns a list of wavetable paths
		"""
		cfg = config.get_config()
		return cfg.get_wavetable_paths() + [cfg.get_freesound_samples_folder()]
		
	def on_parent_click(self, event):
		"""
		Callback that responds to clicking the parent button. Moves up in the directory hierarchy.
		
		@param event: CommandEvent event
		@type event: wx.CommandEvent		
		"""
		if not self.working_directory:
			return
		abspath = os.path.abspath(self.working_directory)
		if abspath in self.get_wavetable_paths():
			self.working_directory = ''
		else:
			self.working_directory = os.path.abspath(os.path.join(self.working_directory,'..'))
		self.update_filelist()
		
	def on_samplelist_dclick(self, event):
		"""
		Callback that responds to double click in the sample list. Plays the selected file.
		
		@param event: MouseEvent event
		@type event: wx.MouseEvent
		"""
		self.on_play_wave(event)
		
	def preview_sample(self, path):
		"""
		Previews a sample from the filesystem.
		"""
		base,ext = os.path.splitext(path)
		if ext.lower() in self.allowed_extensions:
			player.lock()
			try:
				w = player.get_wave(-1) # get preview wave
				w.clear()
				res = w.load_sample(0, path)
			except:
				import traceback
				traceback.print_exc()
			player.unlock()
			if res != 0:
				wx.MessageDialog(self, message="There was a problem loading '%s'." % path, caption = "Aldrin", style = wx.ICON_ERROR|wx.OK|wx.CENTER).ShowModal()
			else:
				self.update_wave_amp()
				player.play_wave(w, 0, (4 << 4) + 1)
				
	def goto_subfolder(self):
		"""
		Enters subfolder, if selected.
		"""
		if self.files:
			filepaths = [os.path.join(self.working_directory, self.files[x][0]) for x in self.get_filelist_selection()]
			if filepaths:
				if os.path.isdir(filepaths[0]):
					self.working_directory = filepaths[0]
					self.update_filelist()
					return True
		return False
		
	def preview_filelist_sample(self):
		"""
		Plays a preview of the selected file in the file list.
		"""
		if self.files:
			filepaths = [os.path.join(self.working_directory, self.files[x][0]) for x in self.get_filelist_selection()]
			if filepaths:
				if os.path.isdir(filepaths[0]):
					self.working_directory = filepaths[0]
					self.update_filelist()
					return False
				else:
					self.preview_sample(filepaths[0])
					return True
			return True
		
	def on_filelist_dclick(self, event):
		"""
		Callback that responds to double click in the file list. Plays a preview of the selected file.
		
		@param event: MouseEvent event
		@type event: wx.MouseEvent
		"""
		self.preview_filelist_sample()
		
	def on_filelist_char(self, event):
		"""
		Callback that responds to key stroke in the file list.
		
		@param event: Key event
		@type event: wx.KeyEvent
		"""
		event.Skip()

	def on_samplelist_key_down(self, event):
		"""
		Callback that responds to key stroke in the sample list.
		
		@param event: Key event
		@type event: wx.KeyEvent
		"""
		k = event.GetKeyCode()
		sellist = list(self.get_sample_selection())
		sel = sellist and sellist[0] or 0
		if k == wx.WXK_SPACE:
			self.on_play_wave(event)
		elif k in (wx.WXK_BACK, wx.WXK_RETURN):
			if event.ShiftDown():
				self.notebook.SetSelection(2)
				self.fspanel.edsearch.SetFocus()
			else:
				self.notebook.SetSelection(1)
				self.filelist.SetFocus()
		else:
			event.Skip()

	def on_filelist_key_down(self, event):
		"""
		Callback that responds to key stroke in the file list.
		
		@param event: Key event
		@type event: wx.KeyEvent
		"""
		k = event.GetKeyCode()
		sellist = list(self.get_filelist_selection())
		sel = sellist and sellist[0] or 0
		if k in (wx.WXK_RIGHT,wx.WXK_NUMPAD_RIGHT):
			if not self.preview_filelist_sample():
				sel = 0
			elif event.ShiftDown():
				sel += 1
		elif k == wx.WXK_SPACE:
			if not self.preview_filelist_sample():
				sel = 0
			else:
				sel += 1
		elif k in (wx.WXK_BACK, wx.WXK_RETURN):
			if not self.goto_subfolder():
				self.on_load_sample(None)
		elif k == wx.WXK_ESCAPE:
			self.notebook.SetSelection(0)
			self.samplelist.SetFocus()
		elif k in (wx.WXK_LEFT,wx.WXK_NUMPAD_LEFT):
			self.on_parent_click(None)
			sel = 0
		elif k in (wx.WXK_UP,wx.WXK_NUMPAD_UP):
			sel -= 1
		elif k in (wx.WXK_DOWN,wx.WXK_NUMPAD_DOWN):
			sel += 1
		elif k in (wx.WXK_PRIOR,wx.WXK_NUMPAD_PRIOR):
			sel -= 8
		elif k in (wx.WXK_NEXT,wx.WXK_NUMPAD_NEXT):
			sel += 8
		else:
			event.Skip()
		for osel in self.get_filelist_selection():
			self.filelist.SetItemState(osel, 0, wx.LIST_STATE_SELECTED | wx.LIST_STATE_FOCUSED)
		sel = min(max(sel, 0), self.filelist.GetItemCount()-1)
		self.filelist.SetItemState(sel, wx.LIST_STATE_SELECTED | wx.LIST_STATE_FOCUSED, wx.LIST_STATE_SELECTED | wx.LIST_STATE_FOCUSED)
		self.filelist.EnsureVisible(sel)
		
	def get_filelist_selection(self):
		"""
		yields the filelist selection.
		
		@return: a generator
		"""
		item = -1
		while True:
			item = self.filelist.GetNextItem(item, wx.LIST_NEXT_ALL, wx.LIST_STATE_SELECTED)
			if item == -1:
				break
			yield item
		
	def update_filelist(self):
		"""
		Updates the file list to display the files in the current working directory.
		"""
		self.filelist.DeleteAllItems()
		self.files = []
		dirs = []
		files = []
		def cmp_nocase(a,b):
			a = a[0]
			b = b[0]
			if a.lower() == b.lower():
				return 0
			if a.lower() < b.lower():
				return -1
			return 1
		filelist = []
		if not self.get_wavetable_paths():
			self.filelist.InsertStringItem(self.filelist.GetItemCount(), "Go to Preferences/Wavetable, set the wave dirs and click 'Refresh'")
			return
		if self.working_directory:
			filelist = os.listdir(self.working_directory)
		else:
			filelist = self.get_wavetable_paths()
		for filename in filelist:
			if not filename.startswith('.'):
				fullpath = os.path.join(self.working_directory,filename)
				if os.path.isdir(fullpath):
					dirs.append((filename,0,filename,'Folder',-1))
				else:
					base,ext = os.path.splitext(filename)
					if ext.lower() in self.allowed_extensions:
						files.append((filename,1,base,ext[1:].upper(),os.stat(fullpath)[stat.ST_SIZE]))
		self.files = sorted(dirs,cmp_nocase) + sorted(files,cmp_nocase)
		if self.files:
			for name,ftype,friendlyname,ext,fsize in self.files:
				index = self.filelist.InsertStringItem(self.filelist.GetItemCount(), prepstr(friendlyname))
				if ftype == 0:
					self.filelist.SetItemImage(index, self.IMG_FOLDER)
				else:
					self.filelist.SetItemImage(index, self.IMG_WAVE)
				if fsize != -1:
					self.filelist.SetStringItem(index, 1, format_filesize(fsize))
				self.filelist.SetStringItem(index, 2, ext)
			self.filelist.SetItemState(0, wx.LIST_STATE_SELECTED | wx.LIST_STATE_FOCUSED, wx.LIST_STATE_SELECTED | wx.LIST_STATE_FOCUSED)
			self.filelist.SetFocus()
		self.stworkpath.SetLabel(self.working_directory)
				
	def update_subsamplelist(self):
		"""
		Updates the subsample list, containing specifics about the wave length, rate and loop range.
		"""
		self.subsamplelist.ClearAll()
		w,h = self.subsamplelist.GetClientSize()
		cw = w / 5.0
		self.subsamplelist.InsertColumn(0, "Root", width=cw)
		self.subsamplelist.InsertColumn(1, "Length", width=cw)
		self.subsamplelist.InsertColumn(2, "Rate", width=cw)
		self.subsamplelist.InsertColumn(3, "Loop Begin", width=cw)
		self.subsamplelist.InsertColumn(4, "Loop End", width=cw)
		sel = self.get_sample_selection()
		if not sel:
			return
		w = player.get_wave(sel[0])
		for i in range(w.get_level_count()):
			level = w.get_level(i)
			self.subsamplelist.InsertStringItem(i, prepstr(note2str(None, level.get_root_note())))
			self.subsamplelist.SetStringItem(i, 1, prepstr("%i" % level.get_sample_count()), -1)
			self.subsamplelist.SetStringItem(i, 2, prepstr("%i" % level.get_samples_per_second()), -1)
			self.subsamplelist.SetStringItem(i, 3, prepstr("%i" % level.get_loop_start()), -1)
			self.subsamplelist.SetStringItem(i, 4, prepstr("%i" % level.get_loop_end()), -1)
				
	def update_sampleprops(self):
		"""
		Updates the sample property checkboxes and sample editing fields.
		Includes volume slider and looping properties.
		"""
		sel = self.get_sample_selection()
		if not sel:
			sel = -1
		else:
			sel = sel[0]
		if sel == -1:
			iswave = False
		else:
			w = player.get_wave(sel)
			iswave = w.get_level_count() >= 1
		self.volumeslider.Enable(iswave)
		self.samplename.SetLabel("")
		self.chkloop.Enable(iswave)
		self.edloopstart.Enable(iswave)
		self.edloopend.Enable(iswave)
		self.edsamplerate.Enable(iswave)
		self.chkpingpong.Enable(iswave)
		self.cbmachine.Enable(iswave)
		self.cbenvelope.Enable(iswave)
		self.chkenable.Enable(iswave)
		self.btnadsr.Enable(iswave)
		self.btnfitloop.Enable(iswave)
		self.btnclear.Enable(iswave)
		self.btnstoresample.Enable(iswave)
		self.btnrename.Enable(iswave)
		self.btnplay.Enable(iswave)
		self.btnstop.Enable(iswave)
		self.envelope.Enable(iswave)
		self.adsrpanel.update()
		if not iswave:
			return
		level = w.get_level(0)
		self.samplename.SetLabel(w.get_path())
		v = int(linear2db(w.get_volume()) * 100)
		self.volumeslider.SetValue(v)
		f = w.get_flags()
		isloop = bool(f & zzub.zzub_wave_flag_loop)
		ispingpong = bool(f & zzub.zzub_wave_flag_pingpong)
		self.chkloop.SetValue(isloop)
		self.edloopstart.Enable(isloop)		
		self.edloopstart.SetValue(str(level.get_loop_start()))
		self.edloopend.Enable(isloop)
		self.edloopend.SetValue(str(level.get_loop_end()))
		self.edsamplerate.SetValue(str(level.get_samples_per_second()))		
		self.chkpingpong.SetValue(ispingpong)
		self.chkpingpong.Enable(isloop)
		if w.get_envelope_count():
			env = w.get_envelope(0)
			self.chkenable.SetValue(env.is_enabled())
			self.envelope.Enable(env.is_enabled())
		else:
			self.envelope.Enable(False)
	
	def on_samplelist_select(self, event):
		"""
		Callback that responds to left click on sample list. 
		Updates the sample properties, the subsample list and the envelope.
		
		@param event: Command event
		@type event: wx.CommandEvent
		"""
		self.update_sampleprops()
		#~ self.update_subsamplelist()
		self.envelope.update()
		
	def update_samplelist(self):
		"""
		Updates the sample list that displays all the samples loaded in the file.
		"""
		# preserve selections across updates
		selected = self.get_sample_selection()
		if len(selected) == 0:
			selected = [0]
		focused = self.samplelist.GetFocusedItem()
		# update sample list
		self.samplelist.DeleteAllItems()
		for i in range(player.get_wave_count()):
			w = player.get_wave(i)
			index = self.samplelist.GetItemCount()
			self.samplelist.InsertStringItem(index, "%02X." % (i+1))
			self.samplelist.SetStringItem(index, 1, prepstr(w.get_name()))
			if w.get_level_count() >= 1:
				self.samplelist.SetItemImage(index, self.IMG_SAMPLE_WAVE)
				
		# restore selections
		for x in selected:
			self.samplelist.Select(x, True)
		self.samplelist.Focus(focused)
		
	def __set_properties(self):
		"""
		Sets properties during initialization.
		"""
		
	def on_fit_loop(self, event):
		"""
		Fits the current samplerate so that the sample fits
		the loop.
		"""
		import math
		bpm = player.get_bpm()
		for sel in self.samplelist.GetSelections():
			w = player.get_wave(sel)
			for i in range(w.get_level_count()):
				level = w.get_level(i)
				sps = level.get_samples_per_second()
				# get sample length
				ls = float(level.get_sample_count()) / float(sps)
				# samples per beat
				spb = 60.0/bpm
				# get exponent
				f = math.log(ls/spb) / math.log(2.0)
				# new samplerate
				sps = sps * 2**(f-int(f+0.5))
				level.set_samples_per_second(int(sps+0.5))
		self.update_sampleprops()
		#~ self.update_subsamplelist()

	def __do_layout(self):
		"""
		Arranges children components during initialization.
		"""
		samplebuttons = wx.BoxSizer(wx.HORIZONTAL)
		samplebuttons.Add(self.btnstoresample, 0, 0, 0)
		samplebuttons.Add(self.btnrename, 0, 0, 0)
		samplebuttons.Add(self.btnclear, 0, 0, 0)
		samplesel = wx.BoxSizer(wx.VERTICAL)
		samplesel.Add(samplebuttons, 0, wx.EXPAND, 0)
		samplesel.Add(self.samplelist, 1, wx.EXPAND, 0)
		loopprops = wx.BoxSizer(wx.HORIZONTAL)
		loopprops.Add(self.chkloop, 0, wx.ALL | wx.ALIGN_CENTER_VERTICAL, 5)
		loopprops.Add(self.edloopstart, 0, wx.ALL | wx.ALIGN_CENTER_VERTICAL, 5)
		loopprops.Add(self.edloopend, 0, wx.ALL | wx.ALIGN_CENTER_VERTICAL, 5)
		loopprops.Add(self.chkpingpong, 0, wx.ALL | wx.ALIGN_CENTER_VERTICAL, 5)
		loopprops.Add(self.edsamplerate, 0, wx.ALL | wx.ALIGN_CENTER_VERTICAL, 5)
		loopprops.Add(self.btnfitloop, 0, wx.ALL | wx.ALIGN_CENTER_VERTICAL, 5)
		envprops = wx.BoxSizer(wx.HORIZONTAL)		
		envprops.Add(self.btnadsr, 0, wx.ALL, 5)
		envprops.Add(self.cbmachine, 0, wx.ALL, 5)
		envprops.Add(self.cbenvelope, 0, wx.ALL, 5)
		envprops.Add(self.chkenable, 0, wx.ALL, 5)
		sampleprops = wx.BoxSizer(wx.VERTICAL)
		sampleplayer = wx.BoxSizer(wx.HORIZONTAL)
		sampleplayer.Add(self.btnplay, 0, wx.RIGHT, 1)
		sampleplayer.Add(self.btnstop, 0, 0, 0)
		sampleplayer.Add(self.samplename, 1, wx.ALIGN_CENTER_VERTICAL | wx.LEFT, 5)
		sampleprops.Add(sampleplayer, 0, wx.EXPAND, 0)
		sampleprops.Add(self.volumeslider, 0, wx.EXPAND, 0)
		sampleprops.Add(loopprops, 0, 0, 0)
		sampleprops.Add(envprops, 0, 0, 0)
		sampleprops.Add(self.envelope, 0, wx.EXPAND, 0)
		sampleprops.Add(self.adsrpanel, 0, wx.EXPAND, 0)
		samplesection = wx.BoxSizer(wx.HORIZONTAL)
		samplesection.Add(samplesel, 1, wx.EXPAND|wx.RIGHT, 5)
		#samplesection.Add(self.subsamplelist, 1, wx.EXPAND)
		samplesection.Add(sampleprops, 2, wx.EXPAND)
		self.instrpanel.SetAutoLayout(True)
		self.instrpanel.SetSizer(samplesection)
		librarybuttons = wx.BoxSizer(wx.HORIZONTAL)
		librarybuttons.Add(self.btnparent, 0, wx.RIGHT, 1)
		librarybuttons.Add(self.btnrefresh, 0, wx.RIGHT, 5)
		librarybuttons.Add(self.stworkpath, 1, wx.ALIGN_CENTER_VERTICAL|wx.LEFT|wx.RIGHT, 5)
		librarybuttons.Add(self.btnloadsample)
		librarysection = wx.BoxSizer(wx.VERTICAL)
		librarysection.Add(librarybuttons, 0, wx.EXPAND|wx.BOTTOM, 1)
		librarysection.Add(self.filelist, 1, wx.EXPAND)
		self.libpanel.SetAutoLayout(True)
		self.libpanel.SetSizer(librarysection)
		topsizer = wx.BoxSizer(wx.VERTICAL)
		topsizer.Add(self.notebook, 1, wx.EXPAND)
		self.SetAutoLayout(True)
		self.SetSizer(topsizer)
		self.Layout()

__all__ = [
	'EnvelopeView',
	'WavetablePanel',
]

if __name__ == '__main__':
	import sys, utils
	from main import run
	run(sys.argv)
