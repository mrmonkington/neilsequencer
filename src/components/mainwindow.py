#encoding: latin-1

# Neil
# Modular Sequencer
# Copyright (C) 2006,2007,2008 The Neil Development Team
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

import os
import gtk
from neil.utils import format_time, ticks_to_time, prepstr, linear2db, db2linear, filepath, \
        is_debug, question, error, add_scrollbars, file_filter, new_stock_image_toggle_button, \
        new_stock_image_button, message, refresh_gui, show_manual
import zzub
import gobject
import config
import neil.errordlg as errordlg

import neil.common as common
MARGIN = common.MARGIN
MARGIN2 = common.MARGIN2
MARGIN3 = common.MARGIN3
MARGIN0 = common.MARGIN0

from neil.utils import make_submenu_item, make_stock_menu_item, make_stock_tool_item, make_stock_toggle_item, \
        make_stock_radio_item, make_menu_item, make_check_item, make_radio_item, new_theme_image,  \
        hicoloriconpath, Menu

import preferences
show_preferences = preferences.show_preferences

from neil.utils import CancelException

import neil.com as com

def cmp_view(a,b):
  a_order = (hasattr(a, '__view__') and a.__view__.get('order',0)) or 0
  b_order = (hasattr(b, '__view__') and b.__view__.get('order',0)) or 0
  return cmp(a_order, b_order)

class FramePanel(gtk.Notebook):
  __neil__ = dict(
          id = 'neil.core.framepanel',
          singleton = True,
          categories = [
          ],
  )

  def __init__(self):
    gtk.Notebook.__init__(self)
    self.set_tab_pos(gtk.POS_TOP)
    self.set_show_border(True)
    self.set_border_width(1)
    self.set_show_tabs(True)
    com.get("neil.core.icons") # make sure theme icons are loaded
    defaultpanel = None
    pages = sorted(com.get_from_category('neil.viewpanel'), cmp=cmp_view)
    for index, panel in enumerate(pages):
      if not hasattr(panel, '__view__'):
        print "panel",panel,"misses attribute __view__"
        continue
      options = panel.__view__
      stockid = options['stockid']
      label = options['label']
      key = options.get('shortcut', '')
      if options.get('default'):
        defaultpanel = panel
      panel.show_all()
      header = gtk.HBox()
      labelwidget = gtk.Label(label)
      #labelwidget.set_angle(90)
      header.pack_start(new_theme_image(stockid, gtk.ICON_SIZE_MENU))
      header.pack_start(labelwidget)
      header.show_all()
      if key:
        header.set_tooltip_text("%s (%s)" % (label, key))
      else:
        header.set_tooltip_text(label)
      self.append_page(panel, header)
    if defaultpanel:
      self.select_viewpanel(defaultpanel)
    self.show_all()

  def select_viewpanel(self, panel):
    for index in xrange(self.get_n_pages()):
      if self.get_nth_page(index) == panel:
        self.set_current_page(index)
        if hasattr(panel, 'handle_focus'):
          panel.handle_focus()
        return

class Accelerators(gtk.AccelGroup):

  __neil__ = dict(
          id = 'neil.core.accelerators',
          singleton = True,
          categories = [
          ],
  )

  def __init__(self):
    gtk.AccelGroup.__init__(self)

  def add_accelerator(self, shortcut, widget, signal="activate"):
    key, modifier = gtk.accelerator_parse(shortcut)
    return widget.add_accelerator(signal, self,  key,  modifier,
                                  gtk.ACCEL_VISIBLE)

class ViewMenu(Menu):
  __neil__ = dict(
          id = 'neil.core.viewmenu',
          singleton = True,
          categories = [
          ],
  )

  def on_check_item(self, menuitem, view):
    if menuitem.get_active():
      view.show_all()
    else:
      view.hide_all()

  def on_activate_item(self, menuitem, view):
    if 'neil.viewpanel' in view.__neil__.get('categories',[]):
      framepanel = com.get('neil.core.framepanel')
      framepanel.select_viewpanel(view)
    else:
      view.hide_all()

  def on_activate(self, widget, item, view):
    item.set_active(view.get_property('visible'))

  def __init__(self):
    Menu.__init__(self)
    views = sorted(com.get_from_category('view'), cmp=cmp_view)
    com.get("neil.core.icons") # make sure theme icons are loaded
    accel = com.get('neil.core.accelerators')
    for view in views:
      if not hasattr(view, '__view__'):
        print "view",view,"misses attribute __view__"
        continue
      options = view.__view__
      label = options['label']
      stockid = options.get('stockid', None)
      shortcut = options.get('shortcut', None)
      if options.get('toggle'):
        item = self.add_check_item(label, False, self.on_check_item, view)
        self.connect('show', self.on_activate, item, view)
      elif stockid:
        item = self.add_image_item(label, new_theme_image(stockid,
                                                          gtk.ICON_SIZE_MENU),
                                   self.on_activate_item, view)
      else:
        item = self.add_item(label, self.on_activate_item)
      if shortcut:
        accel.add_accelerator(shortcut, item)
    if 0:
      # TODO: themes
      tempsubmenu = gtk.Menu()
      defaultitem = gtk.RadioMenuItem(label="Default")
      tempsubmenu.append(defaultitem)
      self.thememenu = tempsubmenu
      cfg = config.get_config()
      if not cfg.get_active_theme():
        defaultitem.set_active(True)
      defaultitem.connect('toggled', self.on_select_theme, None)
      for name in sorted(cfg.get_theme_names()):
        item = gtk.RadioMenuItem(label=prepstr(name), group=defaultitem)
        if name == cfg.get_active_theme():
          item.set_active(True)
        item.connect('toggled', self.on_select_theme, name)
        tempsubmenu.append(item)
      self.append(make_submenu_item(tempsubmenu, "Themes"))

#~class NeilToolbar(gtk.Toolbar):
#~ __neil__ = dict(
#~        id = 'neil.core.toolbar',
#~        singleton = True,
#~        categories = [
#~                'view',
#~        ],
#~ )
#~
#~  __view__ = dict(
#~                label = "Toolbar",
#~                order = 0,
#~                toggle = True,
#~  )

# class NeilStatusbar(gtk.Statusbar):
#   __neil__ = dict(
#           id = 'neil.core.statusbar',
#           singleton = True,
#           categories = [
#                   'view',
#           ],
#   )

#   __view__ = dict(
#                   label = "Statusbar",
#                   order = 0,
#                   toggle = True,
#   )

#   def __init__(self):
#     gtk.Statusbar.__init__(self)
#     self.push(0, "Ready to rok again")

class NeilFrame(gtk.Window):
  """
  The application main window class.
  """

  __neil__ = dict(
          id = 'neil.core.window.root',
          singleton = True,
          categories = [
                  'rootwindow',
          ],
  )

  OPEN_SONG_FILTER = [
          file_filter("CCM Songs (*.ccm)", "*.ccm"),
  ]

  SAVE_SONG_FILTER = [
          file_filter("CCM Songs (*.ccm)","*.ccm"),
  ]

  DEFAULT_EXTENSION = '.ccm'

  title = "Neil"
  filename = ""

  event_to_name = dict([(getattr(zzub,x),x) for x in dir(zzub) if \
                        x.startswith('zzub_event_type_')])

  def __init__(self):
    """
    Initializer.
    """

    gtk.Window.__init__(self, gtk.WINDOW_TOPLEVEL)
    errordlg.install(self)
    self.set_geometry_hints(self,600,400)
    self.set_position(gtk.WIN_POS_CENTER)

    self.open_dlg = gtk.FileChooserDialog(title="Open", parent=self, action=gtk.FILE_CHOOSER_ACTION_OPEN,
            buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_OPEN, gtk.RESPONSE_OK)
    )
    self.open_dlg.add_shortcut_folder(filepath('demosongs'))
    for filefilter in self.OPEN_SONG_FILTER:
      self.open_dlg.add_filter(filefilter)
    self.save_dlg = gtk.FileChooserDialog(title="Save", parent=self, action=gtk.FILE_CHOOSER_ACTION_SAVE,
            buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_SAVE, gtk.RESPONSE_OK)
    )
    self.save_dlg.set_do_overwrite_confirmation(True)
    for filefilter in self.SAVE_SONG_FILTER:
      self.save_dlg.add_filter(filefilter)

    vbox = gtk.VBox()
    self.add(vbox)

    self.accelerators = com.get('neil.core.accelerators')
    self.add_accel_group(self.accelerators)

    # Menu Bar
    self.neilframe_menubar = gtk.MenuBar()
    vbox.pack_start(self.neilframe_menubar, expand=False)
    self.filemenu = gtk.Menu()
    filemenuitem = make_submenu_item(self.filemenu, "_File")
    filemenuitem.connect('activate', self.update_filemenu)
    self.neilframe_menubar.append(filemenuitem)
    self.update_filemenu(None)

    self.editmenu = gtk.Menu()
    editmenuitem = make_submenu_item(self.editmenu, "_Edit")
    editmenuitem.connect('activate', self.update_editmenu)
    self.update_editmenu(None)

    self.neilframe_menubar.append(editmenuitem)
    tempmenu = com.get('neil.core.viewmenu')
    self.neilframe_menubar.append(make_submenu_item(tempmenu, "_View"))
    self.toolsmenu = gtk.Menu()
    item = make_submenu_item(self.toolsmenu, "_Tools")
    self.neilframe_menubar.append(item)
    toolitems = com.get_from_category('menuitem.tool', self.toolsmenu)
    if not toolitems:
      item.destroy()
    tempmenu = gtk.Menu()
    tempmenu.append(make_stock_menu_item(gtk.STOCK_HELP, self.on_help_contents))
    # Menu item that launches a pdf reader with a document explaining Neil shortcuts
    #shortcuts_menu_item = gtk.MenuItem("_Shortcuts")
    #shortcuts_menu_item.connect('activate', self.on_help_shortcuts)
    #tempmenu.append(shortcuts_menu_item)
    irc_menu_item = gtk.MenuItem("Ask on _IRC")
    irc_menu_item.connect('activate', self.on_irc)
    tempmenu.append(irc_menu_item)
    bugreport_menu_item = gtk.MenuItem("Report a _Bug")
    bugreport_menu_item.connect('activate', self.on_bug_report)
    tempmenu.append(bugreport_menu_item)
    # Separator
    tempmenu.append(gtk.SeparatorMenuItem())
    donate_menu_item = gtk.MenuItem("_Donate")
    donate_menu_item.connect('activate', self.on_donate)
    tempmenu.append(donate_menu_item)
    tempmenu.append(gtk.SeparatorMenuItem())
    # Menu item that launches the about box
    tempmenu.append(make_stock_menu_item(gtk.STOCK_ABOUT, self.on_about))
    self.neilframe_menubar.append(make_submenu_item(tempmenu, "_Help"))
    #~ # Menu Bar end

    # Tool Bar
    #self.neilframe_toolbar = com.get('neil.core.toolbar')
    #vbox.pack_start(self.neilframe_toolbar, expand=False)

    #self.neilframe_toolbar.insert(make_stock_tool_item(gtk.STOCK_NEW, self.new),-1)
    #self.neilframe_toolbar.insert(make_stock_tool_item(gtk.STOCK_OPEN, self.on_open),-1)
    #self.neilframe_toolbar.insert(make_stock_tool_item(gtk.STOCK_SAVE, self.on_save),-1)
    #extrasep = gtk.SeparatorToolItem()
    #self.neilframe_toolbar.insert(extrasep,-1)
    #if not com.get_from_category('menuitem.toolbar', self.neilframe_toolbar):
    #   extrasep.destroy()

    self.mastertoolbar = com.get('neil.core.panel.master')
    self.transport = com.get('neil.core.panel.transport')
    self.playback_info = com.get('neil.core.playback')
    self.framepanel = com.get('neil.core.framepanel')

    hbox = gtk.HBox()
    hbox.add(self.framepanel)
    #hbox.pack_end(self.mastertoolbar, expand=False)
    vbox.add(hbox)

    #self.neilframe_statusbar = com.get('neil.core.statusbar')

    vbox.pack_start(self.transport, expand=False)
    #vbox.pack_end(self.neilframe_statusbar, expand=False)

    self.update_title()
    gtk.window_set_default_icon_list(
            gtk.gdk.pixbuf_new_from_file(hicoloriconpath("48x48/apps/neil.png")),
            gtk.gdk.pixbuf_new_from_file(hicoloriconpath("32x32/apps/neil.png")),
            gtk.gdk.pixbuf_new_from_file(hicoloriconpath("24x24/apps/neil.png")),
            gtk.gdk.pixbuf_new_from_file(hicoloriconpath("22x22/apps/neil.png")),
            gtk.gdk.pixbuf_new_from_file(hicoloriconpath("16x16/apps/neil.png")))
    self.resize(750, 550)

    self.connect('key-press-event', self.on_key_down)
    self.connect('destroy', self.on_destroy)
    self.connect('delete-event', self.on_close)
    
    self.framepanel.connect('switch-page', self.page_select)

    gobject.timeout_add(500, self.update_title)
    self.activated=0

    self.show_all()
    self.load_view()

    eventbus = com.get('neil.core.eventbus')
    eventbus.document_path_changed += self.on_document_path_changed
    eventbus.print_mapping()

    options, args = com.get('neil.core.options').get_options_args()
    if len(args) > 1:
      self.open_file(args[1])
    for driver in com.get_from_category('driver'):
      if driver.init_failed:
        gobject.timeout_add(50, show_preferences, self, 1)
        break

  def on_undo(self, *args):
    """
    Called when an undo item is being called.
    """
    player = com.get('neil.core.player')
    player.set_callback_state(False)
    player.undo()
    player.set_callback_state(True)
    eventbus = com.get('neil.core.eventbus')
    eventbus.document_loaded()
    #self.print_history()

  def on_redo(self, *args):
    """
    Called when an undo item is being called.
    """
    player = com.get('neil.core.player')
    player.set_callback_state(False)
    player.redo()
    player.set_callback_state(True)
    eventbus = com.get('neil.core.eventbus')
    eventbus.document_loaded()
    #self.print_history()

  def print_history(self):
    """
    Dumps the current undo history to console.
    """
    player = com.get('neil.core.player')
    pos = player.history_get_position()
    historysize = player.history_get_size()
    if not historysize:
      print "no history."
      return
    print "----"
    for index in xrange(historysize):
      desc = str(player.history_get_description(index))
      s = '#%i: "%s"' % (index,desc)
      if pos == index:
        s += ' <-'
      print s
    print "----"

  def can_activate_undo(self, *args):
    """
    handler for can-activate-accel signal by Undo menuitem. Checks if undo can be executed.
    """
    player = com.get('neil.core.player')
    return player.can_undo()

  def can_activate_redo(self, *args):
    """
    handler for can-activate-accel signal by Redo menuitem. Checks if redo can be executed.
    """
    player = com.get('neil.core.player')
    return player.can_redo()

  def update_editmenu(self, *args):
    """
    Updates the edit menu, including the undo menu.
    """
    for item in self.editmenu:
      item.destroy()
    player = com.get('neil.core.player')

    pos = player.history_get_position()
    self.print_history()

    accel = com.get('neil.core.accelerators')
    item = make_menu_item("Undo", "", self.on_undo)
    accel.add_accelerator("<Control>Z", item)
    if player.can_undo():
      item.get_children()[0].set_label('Undo "%s"' % player.history_get_description(pos-1))
    else:
      item.set_sensitive(False)
    item.connect('can-activate-accel', self.can_activate_undo)
    self.editmenu.append(item)

    item = make_menu_item("Redo", "", self.on_redo)
    accel.add_accelerator("<Control>Y", item)
    if player.can_redo():
      item.get_children()[0].set_label('Redo "%s"' % player.history_get_description(pos))
    else:
      item.set_sensitive(False)
    item.connect('can-activate-accel', self.can_activate_redo)
    self.editmenu.append(item)

    self.editmenu.append(gtk.SeparatorMenuItem())
    self.editmenu.append(make_stock_menu_item(gtk.STOCK_CUT, self.on_cut))
    self.editmenu.append(make_stock_menu_item(gtk.STOCK_COPY, self.on_copy))
    self.editmenu.append(make_stock_menu_item(gtk.STOCK_PASTE, self.on_paste))
    self.editmenu.append(gtk.SeparatorMenuItem())
    self.editmenu.append(make_stock_menu_item(gtk.STOCK_PREFERENCES, self.on_preferences))
    self.editmenu.show_all()
  
  def page_select(self, notebook, page, page_num, *args):
    new_page = notebook.get_nth_page(page_num)
    #print new_page
    new_page.handle_focus()

  def update_filemenu(self, *args):
    """
    Updates the most recent files in the file menu.

    @param widget: the Menu item.
    @type widget: gtk.MenuItem
    """
    for item in self.filemenu:
      item.destroy()
    self.filemenu.append(make_stock_menu_item(gtk.STOCK_NEW, self.new, frame=self, shortcut="<Control>N"))
    self.filemenu.append(make_stock_menu_item(gtk.STOCK_OPEN, self.on_open, frame=self, shortcut="<Control>O"))
    self.filemenu.append(make_stock_menu_item(gtk.STOCK_SAVE, self.on_save, frame=self, shortcut="<Control>S"))
    self.filemenu.append(make_stock_menu_item(gtk.STOCK_SAVE_AS, self.on_save_as))
    recent_files = config.get_config().get_recent_files_config()
    if recent_files:
      self.filemenu.append(gtk.SeparatorMenuItem())
      for i,filename in enumerate(recent_files):
        filetitle=os.path.basename(filename).replace("_","__")
        self.filemenu.append(make_menu_item("_%i %s" % (i+1,filetitle), "", self.open_recent_file, filename))
    self.filemenu.append(gtk.SeparatorMenuItem())
    self.filemenu.append(make_stock_menu_item(gtk.STOCK_QUIT, self.on_exit))
    self.filemenu.show_all()

  def get_active_view(self):
    """
    Returns the active panel view.
    """
    for pindex,(ctrlid,(panel,menuitem)) in self.pages.iteritems():
      if panel.window and panel.window.is_visible() and hasattr(panel,'view'):
        return panel.view

  def on_copy(self, event):
    """
    Sent when the copy function is selected from the menu.

    @param event: Menu event.
    @type event: MenuEvent
    """
    view = self.get_active_view()
    if view and hasattr(view, 'on_copy'):
      view.on_copy(event)

  def on_cut(self, event):
    """
    Sent when the cut function is selected from the menu.

    @param event: Menu event.
    @type event: MenuEvent
    """
    view = self.get_active_view()
    if view and hasattr(view, 'on_cut'):
      view.on_cut(event)

  def on_paste(self, event):
    """
    Sent when the paste function is selected from the menu.

    @param event: Menu event.
    @type event: MenuEvent
    """
    view = self.get_active_view()
    if view and hasattr(view, 'on_paste'):
      view.on_paste(event)

  def load_view(self):
    """
    Called to load view settings from config
    """
    cfg = config.get_config()
    cfg.load_window_pos("MainFrameWindow", self)
    #~cfg.load_window_pos("Toolbar", self.neilframe_toolbar)
    #cfg.load_window_pos("MasterToolbar", self.mastertoolbar)
    cfg.load_window_pos("Transport", self.transport)
    cfg.load_window_pos("Playback", self.playback_info)
    #cfg.load_window_pos("StatusBar", self.neilframe_statusbar)

  def save_view(self):
    """
    Called to store view settings to config
    """
    cfg = config.get_config()
    cfg.save_window_pos("MainFrameWindow", self)
    #~cfg.save_window_pos("Toolbar", self.neilframe_toolbar)
    #cfg.save_window_pos("MasterToolbar", self.mastertoolbar)
    cfg.save_window_pos("Transport", self.transport)
    cfg.save_window_pos("Playback", self.playback_info)
    #cfg.save_window_pos("StatusBar", self.neilframe_statusbar)

  def on_help_contents(self, *args):
    """
    Event handler triggered by the help menu option.

    @param event: menu event.
    @type event: MenuEvent
    """
    import os
    show_manual()

  def on_irc(self, *args):
      import webbrowser
      webbrowser.open("http://webchat.freenode.net/?channels=neil-tracker&uio=d4")

  def on_bug_report(self, *args):
      import webbrowser
      webbrowser.open("https://bitbucket.org/bucket_brigade/neil/issues/new")

  def on_donate(self, *args):
      import webbrowser
      webbrowser.open("https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=TCNUSE2A74ZSG&lc=LT&item_name=Neil%20Modular%20Tracker&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donate_SM%2egif%3aNonHosted")

  def on_about(self, *args):
    """
    Event handler triggered by the "About" menu option.

    @param event: menu event.
    @type event: MenuEvent
    """
    com.get('neil.core.dialog.about', self).show()

  def on_preferences(self, *args):
    """
    Event handler triggered by the "Preferences" menu option.

    @param event: menu event.
    @type event: MenuEvent
    """
    show_preferences(self)

  def on_key_down(self, widget, event):
      """
      Event handler for key events.
      """
      k = gtk.gdk.keyval_name(event.keyval)
      player = com.get('neil.core.player')
      driver = com.get('neil.core.driver.audio')
      if k == 'F6':
          self.play_from_cursor(event)
      elif k == 'F5':
          player.play()
      elif k == 'F8':
          player.stop()
      else:
          return False
      return True

  def on_activate_page(self, widget, unused, page_num):
    self.select_page(page_num)

  def open_recent_file(self, widget, filename):
    """
    Event handler triggered by recent file menu options.

    @param event: menu event.
    @type event: MenuEvent
    """
    try:
      self.save_changes()
      self.open_file(filename)
    except CancelException:
      pass

  def update_title(self):
    """
    Updates the title to display the filename of the currently
    loaded document.
    """
    player = com.get('neil.core.player')
    filename = os.path.basename(player.document_path)
    if not filename:
      filename = 'Unsaved'
    if player.document_changed():
      filename = '*'+filename
    if filename:
      title = filename + ' - ' + self.title
    else:
      title = self.title
    self.set_title(title)
    return True

  def open_file(self, filename):
    """
    Loads a song from disk. The old document will be wiped, and
    the song will be added to the recent file list.

    @param filename: Path to song.
    @type filename: str
    """
    if not os.path.isfile(filename):
      return
    self.clear()
    player = com.get('neil.core.player')
    base,ext = os.path.splitext(filename)
    if ext.lower() in ('.ccm'):
      dlg = gtk.Dialog('Neil', parent=self, flags=gtk.DIALOG_MODAL)
      progBar = gtk.ProgressBar()
      progBar.set_text('Loading CCM Song...')
      progBar.set_size_request(300, 40)
      progBar.set_fraction(0)
      progBar.show()
      dlg.vbox.pack_start(progBar)
      dlg.show()
      done = False
      def progress_callback():
        progBar.pulse()
        return not done
      progBar.pulse()
      refresh_gui()
      gobject.timeout_add(50, progress_callback)
      player.load_ccm(filename)
      done = True
      # The following loads sequencer step size.
      try:
          seq = com.get('neil.core.sequencerpanel')
          index = seq.toolbar.steps.index(player.get_seqstep())
          seq.toolbar.stepselect.set_active(index)
      except ValueError:
          seq.toolbar.stepselect.set_active(5)
      refresh_gui()
      dlg.destroy()
    else:
      message(self, "'%s' is not a supported file format." % ext)
      return

  def on_document_path_changed(self, path):
    self.update_title()
    com.get('neil.core.config').add_recent_file_config(path)

  def save_file(self, filename):
    """
    Saves a song to disk. The document will also be added to the
    recent file list.

    @param filename: Path to song.
    @type filename: str
    """
    player = com.get('neil.core.player')
    try:
      if not os.path.splitext(filename)[1]:
        filename += self.DEFAULT_EXTENSION
      if os.path.isfile(filename):
        if config.get_config().get_incremental_saving():
          # rename incremental
          path,basename = os.path.split(filename)
          basename, ext = os.path.splitext(basename)
          i = 0
          while True:
            newpath = os.path.join(path,"%s%s.%03i.bak" % (basename, ext, i))
            if not os.path.isfile(newpath):
              break
            i += 1
          print '%s => %s' % (filename, newpath)
          os.rename(filename, newpath)
        else:
          # store one backup copy
          path,basename = os.path.split(filename)
          basename,ext = os.path.splitext(basename)
          newpath = os.path.join(path,"%s%s.bak" % (basename,ext))
          if os.path.isfile(newpath):
            os.remove(newpath)
          print '%s => %s' % (filename, newpath)
          os.rename(filename, newpath)
      base,ext = os.path.splitext(filename)
      result = player.save_ccm(filename)
      assert result == 0
      player.document_unchanged()
    except:
      import traceback
      text = traceback.format_exc()
      traceback.print_exc()
      error(self, "<b><big>Error saving file:</big></b>\n\n%s" % text)
    #~ progress.Update(100)
    #self.update_title()
    #com.get('neil.core.config').add_recent_file_config(filename)

  def on_open(self, *args):
    """
    Event handler triggered by the "Open File" menu option.

    @param event: menu event.
    @type event: MenuEvent
    """
    try:
      self.save_changes()
      self.open()
    except CancelException:
      pass

  def open(self):
    """
    Shows the open file dialog and if successful, loads the
    selected song from disk.
    """
    response = self.open_dlg.run()
    self.open_dlg.hide()
    if response == gtk.RESPONSE_OK:
      self.open_file(self.open_dlg.get_filename())

  def on_save(self, *args):
    """
    Event handler triggered by the "Save" menu option.
    """
    try:
      self.save()
    except CancelException:
      pass

  def save(self):
    """
    Shows a save file dialog if filename is unknown and saves the file.
    """
    player = com.get('neil.core.player')
    if not player.document_path:
      self.save_as()
    else:
      self.save_file(player.document_path)

  def save_as(self):
    """
    Shows a save file dialog and saves the file.
    """
    player = com.get('neil.core.player')
    self.save_dlg.set_filename(player.document_path)
    response = self.save_dlg.run()
    self.save_dlg.hide()
    if response == gtk.RESPONSE_OK:
      filepath = self.save_dlg.get_filename()
      self.save_file(filepath)
    else:
      raise CancelException

  def on_save_as(self, *args):
    """
    Event handler triggered by the "Save As" menu option.

    @param event: menu event.
    @type event: MenuEvent
    """
    try:
      self.save_as()
    except CancelException:
      pass

  def clear(self):
    """
    Clears the current document.
    """
    common.get_plugin_infos().reset()
    player = com.get('neil.core.player')
    player.clear()
    player.set_loop_start(0)
    player.set_loop_end(com.get('neil.core.sequencerpanel').view.step)
    player.get_plugin(0).set_parameter_value(1, 0, 1, config.get_config().get_default_int('BPM', 126), 1)
    player.get_plugin(0).set_parameter_value(1, 0, 2, config.get_config().get_default_int('TPB', 4), 1)
    player.history_flush_last()

  def play(self, *args):
    """
    Event handler triggered by the "Play" toolbar button.

    @param event: menu event.
    @type event: MenuEvent
    """
    player = com.get('neil.core.player')
    player.play()

  def play_from_cursor(self, *args):
    """
    Event handler triggered by the F6 key.

    @param event: menu event.
    @type event: MenuEvent
    """
    player = com.get('neil.core.player')
    player.set_position(max(com.get('neil.core.sequencerpanel').view.row,0))
    player.play()

  def on_select_theme(self, widget, data):
    """
    Event handler for theme radio menu items.

    @param event: menu event.
    @type event: MenuEvent
    """
    cfg = config.get_config()
    if not data:
      cfg.select_theme(None)
    else:
      cfg.select_theme(data)
    self.document_changed()

  def stop(self, *args):
    """
    Event handler triggered by the "Stop" toolbar button.

    @param event: menu event.
    @type event: MenuEvent
    """
    player = com.get('neil.core.player')
    player.stop()

  def save_changes(self):
    """
    Asks whether to save changes or not. Throws a {CancelException} if
    cancelled.
    """
    player = com.get('neil.core.player')
    if not player.document_changed():
      return
    if player.document_path:
      text = "<big><b>Save changes to <i>%s</i>?</b></big>" % os.path.basename(player.document_path)
    else:
      text = "<big><b>Save changes?</b></big>"
    response = question(self, text)
    if response == int(gtk.RESPONSE_CANCEL) or response == int(gtk.RESPONSE_DELETE_EVENT):
      raise CancelException
    elif response == int(gtk.RESPONSE_YES):
      self.save()

  def new(self, *args):
    """
    Event handler triggered by the "New" menu option.

    @param event: menu event.
    @type event: MenuEvent
    """
    try:
      self.save_changes()
      self.clear()
      self.update_title()
      com.get('neil.core.player').document_unchanged()
    except CancelException:
      pass

  def on_destroy(self, *args):
    """
    Event handler triggered when the window is being destroyed.
    """
    eventbus = com.get('neil.core.eventbus')
    eventbus.shutdown()

  def on_exit(self, *args):
    """
    Event handler triggered by the "Exit" menu option.

    @param event: menu event.
    @type event: MenuEvent
    """
    if not self.on_close(None, None):
      self.destroy()

  def on_close(self, *args):
    """
    Event handler triggered when the window is being closed.
    """
    self.save_view()
    try:
      self.save_changes()
      self.hide_all()
      return False
    except CancelException:
      return True



__neil__ = dict(
        classes = [
                FramePanel,
                ViewMenu,
                Accelerators,
                NeilFrame,
                #NeilStatusbar,
                #~NeilToolbar,
        ],
)

if __name__ == '__main__':
  pass
