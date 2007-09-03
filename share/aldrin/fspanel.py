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
Contains all classes and functions needed to render the freesound panel.
"""

from gtkimport import gtk
import gobject
import os, sys, stat
from utils import prepstr, db2linear, linear2db, note2str, format_filesize, \
	filepath, new_listview, new_image_button, add_scrollbars, error, question, \
	run_function_with_progress
import utils
import zzub
import config
from envelope import EnvelopeView, ADSRPanel
import freesound
import common
from common import MARGIN, MARGIN2, MARGIN3
player = common.get_player()

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


def format_duration(secs):
	mins = int(secs / 60)
	secs = int((secs % 60) + 0.5)
	return "%i:%02i" % (mins,secs)
	
class FreesoundPanel(gtk.HBox):
	COL_NAME = 0
	COL_DURATION = 1
	COL_FILESIZE = 2
	COL_ADDEDBY = 3
	COL_ID = 4
	
	def __init__(self, wavetable):
		self.wavetable = wavetable
		gtk.HBox.__init__(self)
		self.splitter = gtk.HPaned()
		self.leftpanel = gtk.VBox(False, MARGIN)
		vsizer = self.leftpanel
		fsearch = gtk.Frame("Search")
		fdetails = gtk.Frame("Details")
		vsearch = gtk.VBox(False, MARGIN)
		vsearch.set_border_width(MARGIN)
		vdetails = gtk.VBox()
		vdetails.set_border_width(MARGIN)
		fsearch.add(vsearch)
		vdetailgrid = gtk.VBox()
		self.detailsitems = [fdetails]
		sg = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)
		def add_detail(title, value=None):
			label = gtk.Label()
			label.set_markup("<b>%s</b>" % title)
			label.set_alignment(1, 0)
			sg.add_widget(label)
			if not value:
				value = gtk.Label()
				value.set_alignment(0, 0)
				value.set_line_wrap(True)
				value.set_size_request(150,-1)
				value.set_justify(gtk.JUSTIFY_FILL)
				value.set_selectable(True)
			self.detailsitems.append(label)
			self.detailsitems.append(value)
			row = gtk.HBox(False, MARGIN)
			row.pack_start(label, expand=False)
			row.pack_start(value)
			vdetailgrid.pack_start(row, expand=False)
			return value
		self.dtname = add_detail("Name")
		self.dtuser = add_detail("Added by")
		self.dtdownloads = add_detail("Downloads")
		self.dtrating = add_detail("Rating")
		self.dttype = add_detail("Type")
		self.dtlength = add_detail("Duration")
		self.dtsize = add_detail("Filesize")
		self.dttags = add_detail("Tags")
		self.dtdesc = add_detail("Description")
		self.imgwave = gtk.Image()
		self.detailsitems.append(self.imgwave)
		vdetails.pack_start(self.imgwave, expand=False)
		vdetails.pack_start(vdetailgrid)
		searchgroup = gtk.HBox(False, MARGIN)
		self.edsearch = gtk.Entry()
		self.progress = gtk.ProgressBar()
		searchgroup.add(self.edsearch)
		self.btnsearchtags = gtk.CheckButton("Tags")
		self.btnsearchdesc = gtk.CheckButton("Descriptions")
		self.btnsearchfiles = gtk.CheckButton("Files")
		self.btnsearchusers = gtk.CheckButton("People")
		self.btncancel = gtk.Button(stock=gtk.STOCK_CANCEL)
		self.btnsearchtags.set_active(True)
		self.btnopen = new_image_button(filepath("res/loadsample.png"), "Add/Insert Instrument")
		self.btnopen.connect('clicked', self.on_load_sample)
		self.resultlist, self.resultstore, columns = new_listview([
			('Name', str),
			('Duration', str),
			('Filesize', str),
			('Added by', str),
			('#', str),
		])
		# XXX: TODO
		#~ imglist = wx.ImageList(16,16)
		#~ self.IMG_FOLDER = imglist.Add(wx.Bitmap(filepath("res/folder.png"), wx.BITMAP_TYPE_ANY))
		#~ self.IMG_WAVE = imglist.Add(wx.Bitmap(filepath("res/wave.png"), wx.BITMAP_TYPE_ANY))
		#~ self.resultlist.AssignImageList(imglist, wx.IMAGE_LIST_SMALL)
		vsearch.pack_start(searchgroup)
		progressgroup = gtk.HBox(False, MARGIN)
		progressgroup.pack_start(self.progress)
		progressgroup.pack_end(self.btncancel, expand=False)
		progressgroup.pack_end(self.btnopen, expand=False)
		row = gtk.HBox(False, MARGIN)
		row.pack_start(self.btnsearchtags, expand=False)
		row.pack_start(self.btnsearchdesc, expand=False)
		row.pack_start(self.btnsearchfiles, expand=False)
		row.pack_start(self.btnsearchusers, expand=False)
		vsearch.pack_start(row, expand=False)
		vsearch.pack_start(progressgroup, expand=False)
		logo = gtk.Image()
		logo.set_from_pixbuf(gtk.gdk.pixbuf_new_from_file(filepath("res/fsbanner.png")))
		lbox = gtk.HBox()
		lbox.pack_start(logo, expand=False)
		vsizer.pack_start(lbox, expand=False)
		vsizer.pack_start(fsearch, expand=False)
		scrollwin = add_scrollbars(vdetails)
		scrollwin.set_border_width(MARGIN)
		fdetails.add(scrollwin)
		vsizer.pack_start(fdetails)
		self.splitter.pack1(self.leftpanel, False, False)
		scrollwin = add_scrollbars(self.resultlist)
		self.splitter.pack2(scrollwin, True, False)
		self.add(self.splitter)
		self.cmds = []
		self.cancel = False
		
		gobject.timeout_add(100, self.on_populate)
		self.btncancel.connect('clicked', self.on_cancel)
		self.edsearch.connect('activate', self.on_search)
		self.resultlist.get_selection().connect('changed', self.on_select)
		self.resultlist.connect('button-press-event', self.on_resultlist_dclick)
		self.resultlist.connect('key-press-event', self.on_resultlist_key_down)
		self.client = None
		self.results = {}
		self.imgcache = {}
		self.wavecache = {}
		self.show_details()
		import thread
		thread.start_new_thread(self.preload_freesound, ())
		self.connect('realize', self.on_realize)
		
	def on_realize(self, widget):
		self.progress.hide()
		self.btncancel.hide()
		self.btnopen.hide()
		self.show_details()
		
	def on_resultlist_key_down(self, widget, event):
		"""
		Called when the key is pressed in the result list.
		"""
		k = gtk.gdk.keyval_name(event.keyval)
		mask = event.state
		kv = event.keyval
		if k in ('Right', 'KP_Right'):
			self.preview_current()
		elif kv == 32:
			self.preview_current()
			path, col = self.resultlist.get_cursor()
			self.resultlist.set_cursor((path[0]+1,))
		elif k in ('BackSpace', 'Return'):
			self.on_load_sample(None)
		elif k == 'Escape':
			self.wavetable.set_current_page(0)
			self.wavetable.samplelist.grab_focus()
		elif k in ('Left', 'KP_Left'):
			self.edsearch.select_region(0,-1)
			self.edsearch.grab_focus()
		else:
			return False
		return True
			
	def on_load_sample(self, *args):
		"""
		Called to load samples based on the current file list selection.
		
		Samples will first be downloaded to the local freesound folder,
		and then into the wavetable.
		"""
		import thread
		files = []
		store, rows = self.resultlist.get_selection().get_selected_rows()
		for row in rows:
			iter = store.get_iter(row)
			sampleid = store.get_value(iter, 4)
			if not sampleid:
				continue
			sampleid = int(sampleid)
			if sampleid > 0:
				sample = self.results[sampleid]
				orgfilename = extract_text(get_node(sample,'originalFilename'))
				ext = os.path.splitext(orgfilename)[1].lower()
				filename = str(sampleid) + ext
				fullpath = os.path.join(config.get_config().get_freesound_samples_folder(), filename)
				files.append((sampleid, fullpath))
		if files:
			run_function_with_progress(self, "Downloading samples from freesound...", True, self.download_samples, files)
			
	def import_samples(self, filenames):
		"""
		Imports a sequence of samples to the wavetable.
		"""
		self.wavetable.load_samples(filenames)
	
	def download_samples(self, dialog, files):
		outfiles = []
		import re
		rx = re.compile(r'inline[;]\s*filename[=]["]([^"]*)["]')
		try:
			fs = self.get_freesound(warn=True)
			assert fs
			filecount = len(files)
			for index, (sampleid, path) in enumerate(files):
				if dialog._response:
					break
				value = (float(index) / filecount)
				dialog.fraction = value
				dialog.markup = "Downloading sample #%s (%s of %s)..." % (sampleid,index+1,filecount)
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
		self.cmds.append((self.import_samples,(outfiles,)))
		return True
				
	def preview_current(self):
		"""
		Preview currently focused item.
		"""
		import thread
		path, col = self.resultlist.get_cursor()
		if path:
			sampleid = self.resultstore.get_value(self.resultstore.get_iter(path), 4)
			if sampleid:
				sampleid = int(sampleid)
				sample = self.results[sampleid]
				thread.start_new_thread(self.load_preview_wave, (extract_text(get_node(sample,'preview')),))

		
	def on_resultlist_dclick(self, widget, event):
		"""
		Called when a list entry is doubleclicked.
		"""
		if (event.button == 1) and (event.type == gtk.gdk._2BUTTON_PRESS):
			# double click
			self.preview_current()
		
	def preload_freesound(self):
		"""
		Loads freesound service and logs in.
		"""
		self.cmds.append((self.resultstore.append,(["Logging in...",'','','',''],)))
		self.cmds.append((self.edsearch.set_property, ("editable", False)))
		self.get_freesound()
		self.cmds.append((self.edsearch.set_property, ("editable", True)))
		self.cmds.append((self.resultstore.clear,()))
		
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
		self.imgwave.set_from_pixbuf(self.imgcache[url])
		self.imgwave.show()
		
	def load_wave_image(self, url):
		self.active_url = url
		if not url in self.imgcache:
			import StringIO
			bmpdata = self.get_freesound(warn=True).get(url)
			loader = gtk.gdk.PixbufLoader()
			loader.write(bmpdata)
			loader.close()
			img = loader.get_pixbuf()
			img = img.scale_simple(200, 48, gtk.gdk.INTERP_BILINEAR)
			self.imgcache[url] = img
		self.cmds.append((self.apply_image, (url,)))
		
	def show_details(self, sample=None):
		if sample:
			self.imgwave.clear()
			import thread
			thread.start_new_thread(self.load_wave_image, (extract_text(get_node(sample,'image')),))
			
			self.dtname.set_label(extract_text(get_node(sample,'originalFilename')))
			self.dtuser.set_label(extract_text(get_node(sample,'user/name')))
			self.dtdownloads.set_label(extract_text(get_node(sample,'statistics/downloads')))
			self.dtrating.set_label(extract_text(get_node(sample,'statistics/rating')))
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
			self.dttype.set_label(', '.join([x for x in [ext,sr,br,bd,chans] if x]))
			length = extract_text(get_node(sample,'duration'))
			if length:
				length = format_duration(float(length))
			self.dtlength.set_label(length)
			filesize = extract_text(get_node(sample,'filesize'))
			if filesize:
				filesize = format_filesize(int(filesize))
			self.dtsize.set_label(filesize)
			tags = []
			if get_node(sample,'tags'):
				for tag in get_node(sample,'tags').childNodes:
					if (tag.nodeType == tag.ELEMENT_NODE) and (tag.nodeName == 'tag'):
						tags.append(extract_text(tag))
			self.dttags.set_label(', '.join(tags))
			self.dtdesc.set_label(extract_text(get_node(sample,'descriptions/description/text')))
			for item in self.detailsitems:
				if item != self.imgwave:
					item.show()
		else:
			for item in self.detailsitems:
				item.hide()
		
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
		data = [
			prepstr(originalFilename),
			duration,
			filesize,
			addedby,
			itemid
		]
		self.resultstore.append(data)
		#~ self.resultlist.SetItemData(index, int(itemid))
		#~ self.resultlist.SetItemImage(index, self.IMG_WAVE)
			
	def on_select(self, selection):
		"""
		Called when the user selects an entry.
		"""
		store, rows = selection.get_selected_rows()
		if rows:
			sampleid = store.get_value(store.get_iter(rows[0]), 4)
			if sampleid:
				sampleid = int(sampleid)
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
				import traceback
				traceback.print_exc()
				self.cmds.append((utils.error,(self, "There was an error logging into freesound.")))
				return None
			self.client = client
			print "logged in."
		return self.client
		
	def on_populate(self):
		"""
		Populates the list control from results.
		"""
		while self.cmds:
			cmd,args = self.cmds[0]
			self.cmds = self.cmds[1:]
			try:
				#print cmd,args
				cmd(*args)
			except:
				import traceback
				traceback.print_exc()
		return True
				
	def show_error(self, message):
		self.cmds.append((error,(self,message)))
		
	def cancel_search(self):
		self.cmds.append((self.progress.hide,()))
		self.cmds.append((self.btncancel.hide,()))
		self.cmds.append((self.btnopen.hide, ()))
		
	def search_thread(self, args):
		self.cancel = False
		self.cmds.append((self.progress.set_fraction,(0.0,)))
		self.cmds.append((self.progress.show,()))
		self.cmds.append((self.btncancel.show,()))
		self.cmds.append((self.btnopen.show, ()))
		try:
			fs = self.get_freesound(warn=True)
			assert fs
			if args['search'].startswith('#') and args['search'][1:].isdigit():
				items = [args['search'][1:]]
			else:
				self.result = fs.search(**args)
				items = [item.getAttribute("id") for item in self.result.childNodes if item.nodeType == item.ELEMENT_NODE and item.nodeName == 'sample']
			self.cmds.append((self.resultstore.clear,()))
			self.results = {}
			if len(items) > 1000:
				self.cmds.append((self.resultstore.append,(["Too many hits (max. 1000). Please refine your search.",'','','',''],)))
				self.cancel_search()
				return
			elif not items:
				self.cmds.append((self.resultstore.append,(["No results matching your search terms.",'','','',''],)))
			else:
				self.cmds.append((self.resultstore.append,(["Querying %s result(s)..." % len(items),'','','',''],)))
			for index, itemid in enumerate(items):
				if self.cancel:
					break
				self.cmds.append((self.progress.set_fraction,(1.0/len(items) * index,)))
				#itemid = item.getAttribute("id")
				for i in range(3):
					try:
						sampleitem = fs.get_sample_info(itemid)
						if index == 0:
							self.cmds.append((self.resultstore.clear,()))
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
			self.cmds.append((self.resultstore.clear,()))
		self.cancel_search()
		
	def on_search(self, event):
		"""
		Called when user hits return in search field.
		"""
		self.resultstore.clear()
		self.resultstore.append(["Searching...",'','','',''])
		args = dict(
			search=self.edsearch.get_text(),
			searchDescriptions=int(self.btnsearchdesc.get_active()),
			searchTags=int(self.btnsearchtags.get_active()),
			searchFilenames=int(self.btnsearchfiles.get_active()),
			searchUsernames=int(self.btnsearchusers.get_active())
		)
		import thread
		thread.start_new_thread(self.search_thread, (args,))
		self.resultlist.grab_focus()

if __name__ == '__main__':
	import sys, utils
	from main import run
	run(sys.argv + [utils.filepath('demosongs/paniq-knark.ccm')])
