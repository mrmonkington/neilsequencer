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

"""
Contains all classes and functions needed to render the wavetable editor and
the envelope viewer.
"""

if __name__ == '__main__':
    import os
    os.system('../../bin/neil-combrowser neil.core.wavetablepanel')
    raise SystemExit

import gtk
import gobject
import os, sys, stat
from neil.utils import prepstr, db2linear, linear2db, note2str, filepath, \
     new_listview, new_image_button, add_scrollbars, file_filter, question, \
     format_filesize, error, new_stock_image_button, \
     ObjectHandlerGroup, imagepath
import neil.utils as utils
import zzub
import config
from neil.envelope import EnvelopeView
from neil.waveedit import WaveEditPanel
import neil.common as common
from neil.common import MARGIN, MARGIN2, MARGIN3
import neil.com as com
from neil.utils import Menu

class WavetablePanel(gtk.VBox):
    """
    Wavetable editor.

    Contains a list of samples loaded in the song and a file list showing wave files in the file system.
    It contains controls to transfer files between the song and the file system, and components that facilitate
    sample editing for example loops and envelopes.
    """
    __neil__ = dict(
            id='neil.core.wavetablepanel',
            singleton=True,
            categories=[
                    'neil.viewpanel',
                    'view',
            ]
    )

    __view__ = dict(
                    label="Wavetable",
                    stockid="neil_samplebank",
                    shortcut='F9',
                    order=9,
    )

    def __init__(self):
        """
        Initialization.
        """
        self.ohg = ObjectHandlerGroup()
        self.working_directory = ''
        self.files = []
        self.needfocus = True
        gtk.VBox.__init__(self)
        self.instrpanel = gtk.HPaned()
        self.instrpanel.set_border_width(MARGIN2)
        self.libpanel = \
            gtk.FileChooserDialog(title="Open Sample",
                                  action=gtk.FILE_CHOOSER_ACTION_OPEN,
                                  buttons=(gtk.STOCK_CANCEL,
                                           gtk.RESPONSE_CANCEL,
                                           gtk.STOCK_OPEN,
                                           gtk.RESPONSE_OK))
        preview = gtk.VBox(False, MARGIN)
        #preview.set_size_request(100,-1)

        #btnopen = new_stock_image_button(gtk.STOCK_ADD, "Add/Insert Instrument", self.tooltips)
        #self.ohg.connect(btnopen,'clicked', self.on_load_sample)
        #btndeletefile = new_stock_image_button(gtk.STOCK_DELETE, "Delete File", self.tooltips)
        #self.ohg.connect(btndeletefile, 'clicked', self.on_delete_file)
        #btnrenamefile = new_stock_image_button(gtk.STOCK_BOLD, "Rename File", self.tooltips)
        #self.ohg.connect(btnrenamefile,'clicked', self.on_rename_file)
        #btneditfile= gtk.Button("Edit")
        #self.tooltips.set_tip(btneditfile, "Open Sample in External Editor")
        #self.ohg.connect(btneditfile,'clicked', self.on_edit_file)

        chkautoplay = gtk.CheckButton("_Automatic Preview")
        chkautoplay.set_active(False)
        self.chkautoplay = chkautoplay

        #hbox = gtk.HBox(False, MARGIN)
        #hbox.pack_end(btneditfile, expand=False)
        #hbox.pack_end(btnrenamefile, expand=False)
        #hbox.pack_end(btndeletefile, expand=False)
        #hbox.pack_end(btnopen, expand=False)
        #hbox.pack_end(chkautoplay, expand=False)

        #self.libpanel.set_extra_widget(hbox)
        self.previewdesc = gtk.Label()
        self.previewdesc.set_alignment(0, 0)
        btnpreviewplay = new_stock_image_button(gtk.STOCK_MEDIA_PLAY, "Preview Sample")
        self.ohg.connect(btnpreviewplay, 'clicked', self.on_play_filelist_wave)
        btnpreviewstop = new_stock_image_button(gtk.STOCK_MEDIA_STOP, "Stop Preview")
        self.ohg.connect(btnpreviewstop, 'clicked', self.on_stop_wave)

        hbox = gtk.HBox(False, MARGIN)
        hbox.pack_start(btnpreviewplay, expand=False)
        hbox.pack_start(btnpreviewstop, expand=False)
        preview.pack_start(hbox, expand=False)
        preview.pack_start(self.previewdesc, expand=False)
        preview.pack_start(chkautoplay, expand=False)
        preview.show_all()

        self.libpanel.set_preview_widget(preview)
        #self.libpanel.set_border_width(MARGIN2)
        #self.libpanel.add_shortcut_folder(config.get_config().get_freesound_samples_folder())
        self.libpanel.add_filter(file_filter('All Supported Formats', '*.wav', '*.aif',
                                             '*.aifc', '*.aiff', '*.flac', '*.xi',
                                             '*.au', '*.paf', '*.snd', '*.voc',
                                             '*.smp', '*.iff', '*.8svx', '*.16svx',
                                             '*.w64', '*.mat4', '*.mat5', '*.pvf',
                                             '*.htk', '*.caf', '*.sd2', '*.raw'))
        self.libpanel.add_filter(file_filter('All Files', '*'))
        self.libpanel.add_filter(file_filter('Wave Audio File Format (*.wav)', '*.wav'))
        self.libpanel.add_filter(file_filter('Audio Interchange File Format (*.aif, *.aiff, *.aifc)', '*.aif', '*.aiff', '*.aifc'))
        self.libpanel.add_filter(file_filter('Free Lossless Audio Codec (*.flac)', '*.flac'))
        self.libpanel.add_filter(file_filter('Fasttracker 2 Extended Instrument File (*.xi)', '*.xi'))
        self.libpanel.add_filter(file_filter('Sun Microsystems Audio File Format (*.au, *.snd)', '*.au', '*.snd'))
        self.libpanel.add_filter(file_filter('Ensoniq PARIS Audio Format (*.paf)', '*.paf'))
        self.libpanel.add_filter(file_filter('Creative Labs Audio File (*.voc)', '*.voc'))
        self.libpanel.add_filter(file_filter('SampleVision Audio Sample Format (*.smp)', '*.smp'))
        self.libpanel.add_filter(file_filter('Interchange File Format (*.iff, *.8svx, *.16svx)', '*.iff', '*.8svx', '*.16svx'))
        self.libpanel.add_filter(file_filter('Sony Wave64 Audio Format (*.w64)', '*.w64'))
        self.libpanel.add_filter(file_filter('Matlab Audio Format (*.mat4, *.mat5)', '*.mat4', '*.mat5'))
        self.libpanel.add_filter(file_filter('Portable Voice Format (*.pvf)', '*.pvf'))
        self.libpanel.add_filter(file_filter('Hidden Markov Model Toolkit Format (*.htk)', '*.htk'))
        self.libpanel.add_filter(file_filter('Core Audio Format (*.caf)', '*.caf'))
        self.libpanel.add_filter(file_filter('Sound Designer II File (*.sd2)', '*.sd2'))
        self.libpanel.add_filter(file_filter('Raw Data Audio Format (*.raw)', '*.raw'))
        self.libpanel.set_local_only(True)
        self.libpanel.set_select_multiple(True)
        #self.append_page(self.instrpanel, gtk.Label("Instruments"))
        #self.append_page(self.libpanel, gtk.Label("Library"))
        #self.set_current_page(0)
        self.pack_start(self.instrpanel, expand=True)
        self.samplelist, self.samplestore, columns = new_listview([
                ('#', str),
                ('Name', str),
                (None, gobject.TYPE_PYOBJECT),
        ])
        self.samplelist.get_selection().select_path(0)
        # XXX: TODO
        #~ imglist = wx.ImageList(16,16)
        #~ self.IMG_SAMPLE_WAVE = imglist.Add(wx.Bitmap(filepath("res/wave.png"), wx.BITMAP_TYPE_ANY))
        #~ self.samplelist.AssignImageList(imglist, wx.IMAGE_LIST_SMALL)
        self.btnloadsample = new_stock_image_button(gtk.STOCK_OPEN, "Load Sample")
        self.btnstoresample = new_stock_image_button(gtk.STOCK_SAVE_AS, "Save Sample")
        self.btnstop = new_stock_image_button(gtk.STOCK_MEDIA_STOP, "Stop Preview")
        self.btnplay = new_stock_image_button(gtk.STOCK_MEDIA_PLAY, "Preview Sample")
        self.btnrename = new_stock_image_button(gtk.STOCK_BOLD, "Rename Instrument")
        self.btnclear = new_stock_image_button(gtk.STOCK_REMOVE, "Remove Instrument")
        #self.btnfitloop = new_image_button(imagepath("fitloop.png"), "Fit Loop", self.tooltips)
        #self.btnstrloop = new_image_button(imagepath("fitloop.png"), "Stretch Loop", self.tooltips)
        self.samplename = gtk.Label("")
        self.samplename.set_alignment(0, 0.5)
        self.chkloop = gtk.CheckButton("_Loop")
        self.edloopstart = gtk.Entry()
        self.edloopstart.set_size_request(50, -1)
        self.edloopend = gtk.Entry()
        self.edloopend.set_size_request(50, -1)
        self.edsamplerate = gtk.Entry()
        self.edsamplerate.set_size_request(50, -1)
        self.chkpingpong = gtk.CheckButton("_Ping Pong")
        #self.cbmachine = gtk.combo_box_new_text()
        #self.cbenvelope = gtk.combo_box_new_text()
        self.chkenable = gtk.CheckButton("Use Envelope")
        self.envelope = EnvelopeView(self)
        self.waveedit = WaveEditPanel(self)

        # Buttons to find zero crossings for looping samples.
        #self.btn_start_prev = gtk.Button("<")
        #self.btn_start_next = gtk.Button(">")
        #self.btn_end_prev = gtk.Button("<")
        #self.btn_end_next = gtk.Button(">")

        samplebuttons = gtk.HBox(False, MARGIN)
        samplebuttons.pack_start(self.btnloadsample, expand=False)
        samplebuttons.pack_start(self.btnstoresample, expand=False)
        samplebuttons.pack_start(self.btnrename, expand=False)
        samplebuttons.pack_start(self.btnclear, expand=False)
        samplesel = gtk.VBox(False, MARGIN)
        samplesel.pack_start(samplebuttons, expand=False)
        samplesel.pack_end(add_scrollbars(self.samplelist))
        loopprops = gtk.HBox(False, MARGIN)
        loopprops.pack_start(self.btnplay, expand=False)
        loopprops.pack_start(self.btnstop, expand=False)
        loopprops.pack_start(self.chkloop, expand=False)
        #loopprops.pack_start(self.btn_start_prev, expand=False)
        #loopprops.pack_start(self.btn_start_next, expand=False)
        loopprops.pack_start(self.edloopstart, expand=False)
        loopprops.pack_start(self.edloopend, expand=False)
        #loopprops.pack_start(self.btn_end_prev, expand=False)
        #loopprops.pack_start(self.btn_end_next, expand=False)
        loopprops.pack_start(self.chkpingpong, expand=False)
        loopprops.pack_start(self.edsamplerate, expand=False)
        #loopprops.pack_start(self.btnfitloop, expand=False)
        #loopprops.pack_start(self.btnstrloop, expand=False)
        envprops = gtk.HBox(False, MARGIN)
        #envprops.pack_start(self.cbmachine, expand=False)
        #envprops.pack_start(self.cbenvelope, expand=False)
        envprops.pack_start(self.chkenable, expand=False)
        sampleprops = gtk.VBox(False, MARGIN)
        sampleprops.pack_start(loopprops, expand=False)
        nbsampleprops = gtk.VPaned()
        envsection = gtk.VBox(False, MARGIN)
        envsection.set_border_width(MARGIN)
        envsection.pack_start(envprops, expand=False)
        self.envscrollwin = add_scrollbars(self.envelope)
        envsection.pack_start(self.envscrollwin)
        self.waveedit.set_size_request(-1, 300)
        nbsampleprops.add1(self.waveedit)
        nbsampleprops.add2(envsection)
        sampleprops.pack_start(nbsampleprops)
        self.instrpanel.add1(samplesel)
        self.instrpanel.add2(sampleprops)
        self.instrpanel.set_position(250)

        #self.connect("expose_event", self.expose)

        self.ohg.connect(self.samplelist.get_selection(), 'changed', self.on_samplelist_select)
        self.ohg.connect(self.samplelist, 'button-press-event', self.on_samplelist_click)
        self.ohg.connect(self.samplelist, 'key-press-event', self.on_samplelist_key_down)

        self.ohg.connect(self.btnloadsample, 'clicked', self.on_load_sample)
        self.ohg.connect(self.btnstoresample, 'clicked', self.on_save_sample)
        self.ohg.connect(self.btnplay, 'clicked', self.on_play_wave)
        self.ohg.connect(self.btnstop, 'clicked', self.on_stop_wave)
        self.ohg.connect(self.btnclear, 'clicked', self.on_clear)
        self.ohg.connect(self.btnrename, 'clicked', self.on_rename_instrument)
        #self.ohg.connect(self.btnfitloop,'clicked', self.on_fit_loop)
        #self.ohg.connect(self.btnstrloop,'clicked', self.on_stretch_loop)
        self.ohg.connect(self.chkloop, 'clicked', self.on_check_loop)
        self.ohg.connect(self.chkpingpong, 'clicked', self.on_check_pingpong)
        self.ohg.connect(self.chkenable, 'clicked', self.on_check_envdisabled)

        self.ohg.connect(self.edloopstart, 'focus-out-event', self.on_loop_start_apply)
        self.ohg.connect(self.edloopstart, 'activate', self.on_loop_start_apply)
        self.ohg.connect(self.edloopend, 'focus-out-event', self.on_loop_end_apply)
        self.ohg.connect(self.edloopend, 'activate', self.on_loop_end_apply)
        self.ohg.connect(self.edsamplerate, 'focus-out-event', self.on_samplerate_apply)
        self.ohg.connect(self.edsamplerate, 'activate', self.on_samplerate_apply)

        self.ohg.connect(self.libpanel, 'key-press-event', self.on_filelist_key_down)
        #self.ohg.connect(self.libpanel,'file-activated', self.on_load_sample)
        self.ohg.connect(self.libpanel, 'selection-changed', self.on_libpanel_selection_changed)

        currentpath = config.get_config().get_default('SampleBrowserPath')

        if currentpath:
            try:
                self.libpanel.set_current_folder(currentpath)
            except:
                print "couldn't set current sample browser path: '%s'." % currentpath

        eventbus = com.get('neil.core.eventbus')
        eventbus.zzub_wave_allocated += self.update_samplelist
        eventbus.zzub_wave_allocated += self.update_sampleprops
        eventbus.zzub_wave_allocated += self.envelope.update
        eventbus.zzub_wave_allocated += self.waveedit.update
        eventbus.zzub_delete_wave += self.update_samplelist
        eventbus.zzub_delete_wave += self.update_sampleprops
        eventbus.zzub_delete_wave += self.envelope.update
        eventbus.zzub_delete_wave += self.waveedit.update
        eventbus.zzub_wave_changed += self.update_samplelist
        eventbus.zzub_wave_changed += self.update_sampleprops
        eventbus.zzub_wave_changed += self.envelope.update
        eventbus.zzub_wave_changed += self.waveedit.update
        eventbus.active_waves_changed += self.update_samplelist
        eventbus.active_waves_changed += self.update_sampleprops
        eventbus.active_waves_changed += self.envelope.update
        eventbus.active_waves_changed += self.waveedit.update
        eventbus.document_loaded += self.update_samplelist
        eventbus.document_loaded += self.update_sampleprops
        eventbus.document_loaded += self.envelope.update
        eventbus.document_loaded += self.waveedit.update

        self.update_samplelist()
        self.update_sampleprops()
        self.envelope.update()
        self.waveedit.update()

    def expose(self, widget, *args):
        if self.needfocus:
            self.samplelist.grab_focus()
            self.needfocus = False

    def handle_focus(self):
        self.samplelist.grab_focus()
        player = com.get('neil.core.player')
        if player.active_waves == []:
            self.samplelist.get_selection().select_path(0)
        else:
            self.samplelist.get_selection().\
                select_path(player.active_waves[0].get_index())
        self.needfocus = True

    def on_libpanel_selection_changed(self, widget):
        """
        Called when the current file browser selection changes.
        """
        config.get_config().set_default('SampleBrowserPath', self.libpanel.get_current_folder())
        filename = self.libpanel.get_preview_filename()
        if filename and os.path.isfile(filename):
            self.previewpath = filename
            text = "Size: %s" % format_filesize(os.stat(filename)[stat.ST_SIZE])
            self.previewdesc.set_markup(text)
            self.libpanel.set_preview_widget_active(True)
            if self.chkautoplay.get_active():
                self.preview_sample(self.previewpath)
        else:
            self.libpanel.set_preview_widget_active(False)

    def on_size(self, event):
        """
        Called when the panel is being resized.
        """
        self.Layout()
        #event.Skip()
        x, y, w, h = self.filelist.GetClientRect()
        w -= 32
        self.filelist.SetColumnWidth(0, int(w * 0.5))
        self.filelist.SetColumnWidth(1, int(w * 0.25))
        self.filelist.SetColumnWidth(2, int(w * 0.25))
        x, y, w, h = self.samplelist.GetClientRect()
        w -= 32
        self.samplelist.SetColumnWidth(0, 48)
        self.samplelist.SetColumnWidth(1, w - 48)

    def get_sample_selection(self):
        """
        Returns a list with currently selected sample indices.
        """
        player = com.get('neil.core.player')
        return [w.get_index() for w in player.active_waves]

    def get_samplelist_selection(self):
        """
        Returns a list with currently selected sample indices.
        """
        model, rows = self.samplelist.get_selection().get_selected_rows()
        sel = []
        for row in rows:
            it = model.get_iter(row)
            sel.append(model.get_value(it, 2))
        return sel

    def on_delete_file(self, widget):
        """
        Deletes currently selected file
        """
        files = [path for path in self.libpanel.get_filenames() if os.path.isfile(path)]
        if not files:
            return
        sel = self.get_sample_selection()
        if question(self, '<b><big>Really delete selected files?</big></b>', False) != gtk.RESPONSE_YES:
            return
        for file in files:
            os.remove(file)
        self.libpanel.set_current_folder(self.libpanel.get_current_folder())

    def on_rename_file(self, widget):
        """
        Renames currently selected file
        """
        files = [path for path in self.libpanel.get_filenames() if os.path.isfile(path)]
        if not(files) or len(files) > 1:
            return
        data_entry = DataEntry(self, "Rename File", "New Name:")
        if data_entry.run() == gtk.RESPONSE_OK:
            try:
                value = data_entry.edit.get_text()
                filename = os.path.basename(files[0])
                if filename[filename.rfind("."):] != value[value.rfind("."):]:
                    value = value + filename[filename.rfind("."):]
                os.rename(files[0], os.path.join(os.path.dirname(files[0]), value))
            except:
                import traceback
                traceback.print_exc()
        data_entry.destroy()
        self.libpanel.set_current_folder(self.libpanel.get_current_folder())

    def on_swap_instrument(self, widget):
        """
        Swap instrument with another
        """
        data_entry = DataEntry(self, "Swap Instruments", "With #:")
        if data_entry.run() == gtk.RESPONSE_OK:
            try:
                selects = self.get_sample_selection()
                player = com.get('neil.core.player')

                sourceIdx = self.get_sample_selection()[0]
                targetIdx = int("0x" + data_entry.edit.get_text(), 16) - 1

                # swap rows
                self.samplestore.swap(self.samplestore.get_iter(sourceIdx), self.samplestore.get_iter(targetIdx))

                tmp = self.samplestore[sourceIdx][0]
                sourceWaveIdx = self.samplestore[sourceIdx][2].get_index()
                targetWaveIdx = self.samplestore[targetIdx][2].get_index()

                # swap #s
                self.samplestore[sourceIdx][0] = self.samplestore[targetIdx][0]
                self.samplestore[targetIdx][0] = tmp

                # swap wave indices
                self.samplestore[targetIdx][2].set_index(targetWaveIdx)
                self.samplestore[sourceIdx][2].set_index(targetWaveIdx)

                player.history_commit("swap instruments")
            except:
                import traceback
                traceback.print_exc()
        data_entry.destroy()


    def on_rename_instrument(self, widget):
        """
        Renames currently selected sample.
        """
        player = com.get('neil.core.player')
        selects = self.get_sample_selection()
        if not(selects) or len(selects) > 1:
            return
        data_entry = DataEntry(self, "Rename Instrument")
        if data_entry.run() == gtk.RESPONSE_OK:
            try:
                value = data_entry.edit.get_text()
                target = self.get_sample_selection()[0]
                w = player.get_wave(target)
                w.set_name(value)
                player.history_commit("rename instrument")
            except:
                import traceback
                traceback.print_exc()
        data_entry.destroy()

    def on_samplerate_apply(self, widget, *args):
        """
        Callback that responds to changes in the sample rate edit field.

        @param event: CommandEvent event
        @type event: wx.CommandEvent
        """
        try:
            v = min(max(int(self.edsamplerate.get_text()), 50), 200000)
        except ValueError:
            return
        player = com.get('neil.core.player')
        for i in self.get_sample_selection():
            w = player.get_wave(i)
            if w.get_level_count() >= 1:
                w.get_level(0).set_samples_per_second(v)
        player.history_commit("change samplerate")
        self.update_sampleprops()
        #~ self.update_subsamplelist()

    def on_loop_start_apply(self, widget, *args):
        """
        Callback that responds to changes in the loop-start edit field.

        @param event: CommandEvent event
        @type event: wx.CommandEvent
        """
        try:
            v = int(self.edloopstart.get_text())
        except ValueError:
            print "invalid value."
            return
        player = com.get('neil.core.player')
        for i in self.get_sample_selection():
            w = player.get_wave(i)
            if w.get_level_count() >= 1:
                level = w.get_level(0)
                level.set_loop_start(min(max(v, 0), level.get_loop_end() - 1))
        player.history_commit("change loop start")
        self.update_sampleprops()
        #~ self.update_subsamplelist()

    def on_loop_end_apply(self, widget, *args):
        """
        Callback that responds to changes in the loop-end edit field.

        @param event: CommandEvent event
        @type event: wx.CommandEvent
        """
        try:
            v = int(self.edloopend.get_text())
        except ValueError:
            print "invalid value."
            return
        player = com.get('neil.core.player')
        for i in self.get_sample_selection():
            w = player.get_wave(i)
            if w.get_level_count() >= 1:
                level = w.get_level(0)
                level.set_loop_end(min(max(v, level.get_loop_start() + 1), level.get_sample_count()))
        player.history_commit("change loop end")
        self.update_sampleprops()
        #~ self.update_subsamplelist()

    def on_check_pingpong(self, widget):
        """
        Callback of checkbox that enables or disables bidirectional looping for the selected sample.
        """
        player = com.get('neil.core.player')
        for i in self.get_sample_selection():
            w = player.get_wave(i)
            flags = w.get_flags()
            if (self.chkpingpong.get_active()):
                flags = flags | zzub.zzub_wave_flag_pingpong
            else:
                flags = flags ^ (flags & zzub.zzub_wave_flag_pingpong)
            w.set_flags(flags)
        player.history_commit("pingpong option")
        self.update_sampleprops()

    def on_check_loop(self, widget):
        """
        Callback of checkbox that enables or disables looping for the selected sample.
        """
        player = com.get('neil.core.player')
        for i in self.get_sample_selection():
            w = player.get_wave(i)
            flags = w.get_flags()
            if (self.chkloop.get_active()):
                flags = flags | zzub.zzub_wave_flag_loop
            else:
                flags = flags ^ (flags & zzub.zzub_wave_flag_loop)
            w.set_flags(flags)
        player.history_commit("loop option")
        self.update_sampleprops()

    def on_check_envdisabled(self, widget):
        """
        Callback of checkbox that enables or disables the envelope for the selected sample.
        """
        player = com.get('neil.core.player')
        for i in self.get_sample_selection():
            w = player.get_wave(i)
            if w.get_envelope_count() == 0:
                w.set_envelope_count(1)
            env = w.get_envelope(0)
            enabled = self.chkenable.get_active()
            env.enable(enabled)
            if enabled:
                w.set_flags(w.get_flags() | zzub.zzub_wave_flag_envelope)
        player.history_commit("envelope option")
        self.update_sampleprops()

    def on_scroll_changed(self, range, scroll, value):
        """
        Callback that responds to change in the wave volume slider.

        @param event: CommandEvent event
        @type event: wx.CommandEvent
        """
        player = com.get('neil.core.player')
        vol = db2linear(int(value) / 100.0)
        for i in self.get_sample_selection():
            w = player.get_wave(i)
            w.set_volume(vol)
        player.history_commit("change wave volume")

    def on_mousewheel(self, widget, event):
        """
        Sent when the mousewheel is used on the volume slider.

        @param event: A mouse event.
        @type event: wx.MouseEvent
        """
        pass
        #vol = int(self.volumeslider.get_value())
        #step = 100
        #if event.direction == gtk.gdk.SCROLL_UP:
        #    vol += step
        #elif event.direction == gtk.gdk.SCROLL_DOWN:
        #    vol -= step
        #vol = min(max(-4800,vol), 2400)
        #self.volumeslider.set_value(vol)
        #self.on_scroll_changed(widget, gtk.SCROLL_NONE, vol)

    def on_clear(self, widget):
        """
        Callback of a button that clears the selected sample in the sample list.
        """
        sel = self.get_sample_selection()
        if len(sel) > 1:
            if question(self, '<b><big>Really delete %s instruments?</big></b>' % len(sel), False) != gtk.RESPONSE_YES:
                return
        elif question(self, '<b><big>Really delete instrument?</big></b>', False) != gtk.RESPONSE_YES:
            return
        player = com.get('neil.core.player')
        for i in sel:
            player.get_wave(i).clear()
        if len(sel) > 1:
            desc = "delete instruments"
        else:
            desc = "delete instrument"
        player.history_commit(desc)

    def on_refresh(self, event):
        """
        Callback of a button that refreshes the file list.

        @param event: CommandEvent event
        @type event: wx.CommandEvent
        """
        self.working_directory = ''
        self.stworkpath.SetLabel(self.working_directory)

    def update_wave_amp(self):
        """
        Updates the wave players current amplitude from the config.
        """
        # master = player.get_plugin(0)
        # vol = -76.0 * (master.get_parameter_value(1, 0, 0) / 16384.0)
        player = com.get('neil.core.player')
        vol = min(max(config.get_config().get_sample_preview_volume(), -76.0), 0.0)
        amp = db2linear(vol, limit= -76.0)
        #player.set_wave_amp(amp)

    def on_play_filelist_wave(self, widget):
        """
        Callback of a button that plays the currently selected preview sample.
        """
        self.preview_sample(self.previewpath)

    def on_play_wave(self, widget):
        """
        Callback of a button that plays the currently selected sample in the sample list.
        """
        player = com.get('neil.core.player')
        selects = self.get_sample_selection()
        if selects:
            w = player.get_wave(selects[0])
            if w.get_level_count() >= 1:
                self.update_wave_amp()
                player.preview_wave(w)

    def on_stop_wave(self, event):
        """
        Callback of a button that stops playback of a wave file that is currently playing.
        """
        player = com.get('neil.core.player')
        player.stop_preview()

    def on_save_sample(self, widget):
        """
        Callback that responds to clicking the save sample button.
        Saves a sample to disk.

        @param event: Command event
        @type event: wx.CommandEvent
        """
        player = com.get('neil.core.player')
        selects = self.get_sample_selection()
        for s in selects:
            w = player.get_wave(s)
            origpath = w.get_path().replace('/', os.sep).replace('\\', os.sep)
            if origpath:
                filename = os.path.splitext(os.path.basename(origpath))[0] + '.wav'
            else:
                filename = w.get_name() + '.wav'
            dlg = gtk.FileChooserDialog(title="Export Sample", parent=self.get_toplevel(), action=gtk.FILE_CHOOSER_ACTION_SAVE,
                    buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_SAVE, gtk.RESPONSE_OK))
            dlg.set_current_name(filename)
            dlg.set_do_overwrite_confirmation(True)
            dlg.add_filter(file_filter('Wave Files (*.wav)', '*.wav'))
            response = dlg.run()
            filepath = dlg.get_filename()
            dlg.destroy()
            if response == gtk.RESPONSE_OK:
                player.save_wave(w, filepath)
            else:
                return

    def load_samples(self, samplepaths):
        """
        Loads a list of samples into the sample list of the song.
        """
        if not samplepaths:
            return
        selects = self.get_sample_selection()
        if not selects:
            error(self, "<b><big>Can't load sample.</big></b>\n\nNo target instrument slot is selected.")
            return
        # if sample selection less than files, increase sample selection
        if len(selects) < len(samplepaths):
            diffcount = len(samplepaths) - len(selects)
            selects += tuple(range(selects[-1] + 1, selects[-1] + 1 + diffcount))
        # if sample selection more than files, set sample selection equal to number files
        elif len(selects) > len(samplepaths):
            selects = selects[:len(samplepaths)]
        assert len(selects) == len(samplepaths)
        player = com.get('neil.core.player')
        for source, target in zip(samplepaths, selects):
            w = player.get_wave(target)
            w.clear()
            w.set_volume(1.0)
            if not player.load_wave(w, source):
                player.history_flush()
                error(self, "<b><big>Unable to load <i>%s</i>.</big></b>\n\nThe file may be corrupt or the file type is not supported." % source)
                return
            else:
                w.set_name(os.path.splitext(os.path.basename(source))[0])
                w.set_path(source)
        player.history_commit("load instrument")
        #self.set_current_page(0)
        self.samplelist.grab_focus()
        self.samplelist.set_cursor(selects[0])

    def on_load_sample(self, widget):
        """
        Callback that responds to clicking the load sample button.
        Loads a sample from the file list into the sample list of the song.
        """
        response = self.libpanel.run()
        if response == gtk.RESPONSE_OK:
            filenames = self.libpanel.get_filenames()
            samplepaths = [path for path in filenames if os.path.isfile(path)]
            self.load_samples(samplepaths)
        self.libpanel.hide()

    def get_wavetable_paths(self):
        """
        Returns a list of wavetable paths
        """
        cfg = config.get_config()
        return cfg.get_wavetable_paths()

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
            self.working_directory = os.path.abspath(os.path.join(self.working_directory, '..'))

    def on_samplelist_click(self, widget, event):
        """
        Callback that responds to clicks in the sample list.

        @param event: MouseEvent event
        @type event: wx.MouseEvent
        """
        #  Plays the selected file
        if (event.button == 1) and (event.type == gtk.gdk._2BUTTON_PRESS):
            # double click
            self.on_play_wave(event)
            #I think this makes much more sense..
            #self.on_load_sample(widget)
        #  Open context menu
        elif (event.button == 3):
            menu = Menu()
            menu.add_item("Load Sample", self.on_load_sample)
            menu.add_item("Save Sample", self.on_save_sample)
            menu.add_item("Remove Instrument", self.on_delete_file)
            menu.add_item("Rename Instrument", self.on_rename_instrument)
            menu.add_item("Swap Instrument", self.on_swap_instrument)
            menu.show_all()
            menu.attach_to_widget(self, None)
            menu.popup(self, event)

    def preview_sample(self, path):
        """
        Previews a sample from the filesystem.
        """
        base, ext = os.path.splitext(path)
        player = com.get('neil.core.player')
        # (4 << 4) + 1 ?
        if not player.preview_file(path):
            error(self, "<b><big>Unable to preview <i>%s</i>.</big></b>\n\nThe file may be corrupt or the file type is not supported." % path)
            return

    def goto_subfolder(self):
        """
        Enters subfolder, if selected.
        """
        if self.files:
            filepaths = [os.path.join(self.working_directory, self.files[x][0]) for x in self.get_filelist_selection()]
            if filepaths:
                if os.path.isdir(filepaths[0]):
                    self.working_directory = filepaths[0]
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
                    return False
                else:
                    self.preview_sample(filepaths[0])
                    return True
            return True

    def on_filelist_dclick(self, widget, event):
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

    def on_samplelist_key_down(self, widget, event):
        """
        Callback that responds to key stroke in the sample list.

        @param event: Key event
        @type event: wx.KeyEvent
        """
        # Read the name of the key pressed.
        k = gtk.gdk.keyval_name(event.keyval)
        # When space button is pressed play the wave.
        if k == 'Space':
            self.on_play_wave(event)
        elif k in ('BackSpace', 'Return'):
            # If backspace or return are pressed go to file browser.
            #self.set_current_page(1)
            #if self.filetreeview:
            #    self.filetreeview.grab_focus()
            #else:
            #    self.libpanel.grab_focus()
            self.on_load_sample(widget)
        else:
            return False
        return True


    def on_filelist_key_down(self, widget, event):
        """
        Callback that responds to key stroke in the file list.

        @param event: Key event
        @type event: wx.KeyEvent
        """
        k = gtk.gdk.keyval_name(event.keyval)
        mask = event.state
        kv = event.keyval
        print k, kv
        if k == 'Escape':
            #this doesn't seem to do anything, and set_current_page
            #doesn't exist!
            #self.set_current_page(0)
            self.libpanel.hide()
            self.samplelist.needfocus = True
        else:
            return False
        return True

    def update_subsamplelist(self):
        """
        Updates the subsample list, containing specifics about the wave length, rate and loop range.
        """
        self.subsamplelist.ClearAll()
        w, h = self.subsamplelist.GetClientSize()
        cw = w / 5.0
        self.subsamplelist.InsertColumn(0, "Root", width=cw)
        self.subsamplelist.InsertColumn(1, "Length", width=cw)
        self.subsamplelist.InsertColumn(2, "Rate", width=cw)
        self.subsamplelist.InsertColumn(3, "Loop Begin", width=cw)
        self.subsamplelist.InsertColumn(4, "Loop End", width=cw)
        sel = self.get_sample_selection()
        if not sel:
            return
        player = com.get('neil.core.player')
        w = player.get_wave(sel[0])
        for i in range(w.get_level_count()):
            level = w.get_level(i)
            self.subsamplelist.InsertStringItem(i, prepstr(note2str(None, level.get_root_note())))
            self.subsamplelist.SetStringItem(i, 1, prepstr("%i" % level.get_sample_count()), -1)
            self.subsamplelist.SetStringItem(i, 2, prepstr("%i" % level.get_samples_per_second()), -1)
            self.subsamplelist.SetStringItem(i, 3, prepstr("%i" % level.get_loop_start()), -1)
            self.subsamplelist.SetStringItem(i, 4, prepstr("%i" % level.get_loop_end()), -1)

    def update_sampleprops(self, *args):
        """
        Updates the sample property checkboxes and sample editing fields.
        Includes volume slider and looping properties.
        """
        block = self.ohg.autoblock()
        player = com.get('neil.core.player')
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
        #self.volumeslider.set_sensitive(iswave)
        self.samplename.set_label("")
        self.chkloop.set_sensitive(iswave)
        self.edloopstart.set_sensitive(iswave)
        self.edloopend.set_sensitive(iswave)
        self.edsamplerate.set_sensitive(iswave)
        self.chkpingpong.set_sensitive(iswave)
        #self.cbmachine.set_sensitive(iswave)
        #self.cbenvelope.set_sensitive(iswave)
        self.chkenable.set_sensitive(iswave)
        #self.btnfitloop.set_sensitive(iswave)
        #self.btnstrloop.set_sensitive(iswave)
        self.btnclear.set_sensitive(iswave)
        self.btnstoresample.set_sensitive(iswave)
        self.btnrename.set_sensitive(iswave)
        self.btnplay.set_sensitive(iswave)
        self.btnstop.set_sensitive(iswave)
        self.envelope.set_sensitive(iswave)
        self.waveedit.set_sensitive(iswave)
        if not iswave:
            return
        level = w.get_level(0)
        self.samplename.set_label(w.get_path())
        #self.samplename.set_label("Sample Name: "+w.get_name())
        v = int(linear2db(w.get_volume()) * 100)
        #self.volumeslider.set_value(v)

        f = w.get_flags()

        isloop = bool(f & zzub.zzub_wave_flag_loop)
        ispingpong = bool(f & zzub.zzub_wave_flag_pingpong)

        self.chkloop.set_active(isloop)
        self.edloopstart.set_sensitive(isloop)
        self.edloopstart.set_text(str(level.get_loop_start()))
        self.edloopend.set_sensitive(isloop)
        self.edloopend.set_text(str(level.get_loop_end()))
        self.edsamplerate.set_text(str(level.get_samples_per_second()))
        self.chkpingpong.set_active(ispingpong)
        self.chkpingpong.set_sensitive(isloop)
        if w.get_envelope_count() != 0:
            env = w.get_envelope(0)
            self.chkenable.set_active(env.is_enabled())
            self.envelope.set_sensitive(env.is_enabled())
        else:
            self.chkenable.set_active(False)
            self.envelope.set_sensitive(False)
            #if env.is_enabled():
            #    self.envscrollwin.show_all()
            #else:
            #    self.envscrollwin.hide_all()
        #else:
        #    self.envelope.set_sensitive(False)
        #    self.envscrollwin.hide_all()

    def on_samplelist_select(self, selection):
        """
        Callback that responds to left click on sample list.
        Updates the sample properties, the subsample list and the envelope.

        @param event: Command event
        @type event: wx.CommandEvent
        """
        player = com.get('neil.core.player')
        player.active_waves = self.get_samplelist_selection()

    def update_samplelist(self, *args):
        """
        Updates the sample list that displays all the samples loaded in the file.
        """
        # update sample list
        block = self.ohg.autoblock()
        player = com.get('neil.core.player')
        selection = self.samplelist.get_selection()
        iter = self.samplestore.get_iter_first()
        if not iter: # empty
            for i in range(player.get_wave_count()):
                w = player.get_wave(i)
                it = self.samplestore.append(["%02X." % (i + 1), prepstr(w.get_name()), w])
                if w in player.active_waves:
                    selection.select_iter(it)
        else: # update, it's a bit faster than rebuilding the list.
            while iter:
                w = self.samplestore.get_value(iter, 2)
                self.samplestore.set_value(iter, 1, prepstr(w.get_name()))
                if w in player.active_waves:
                    selection.select_iter(iter)
                iter = self.samplestore.iter_next(iter)

    def __set_properties(self):
        """
        Sets properties during initialization.
        """

    def on_stretch_loop(self, event):
        """
        Stretches the sample so it fits the loop
        """
        import math
        player = com.get('neil.core.player')
        bpm = player.get_bpm()
        for sel in self.get_sample_selection():
            w = player.get_wave(sel)
            for i in range(w.get_level_count()):
                level = w.get_level(i)
                sps = level.get_samples_per_second()
                # get sample length
                ls = float(level.get_sample_count()) / float(sps)
                # samples per beat
                spb = 60.0 / bpm
                # get exponent
                f = math.log(ls / spb) / math.log(2.0)
                # new samplerate
                newsps = sps * 2 ** (f - int(f + 0.5))
                # new size
                newsize = int(((level.get_sample_count() * sps) / newsps) + 0.5)
                level.stretch_range(0, level.get_sample_count(), newsize)
                self.waveedit.update()
                #level.set_samples_per_second(int(sps+0.5))
        player.history_commit("stretch loop")

    def on_fit_loop(self, event):
        """
        Fits the current samplerate so that the sample fits
        the loop.
        """
        import math
        player = com.get('neil.core.player')
        bpm = player.get_bpm()
        for sel in self.get_sample_selection():
            w = player.get_wave(sel)
            for i in range(w.get_level_count()):
                level = w.get_level(i)
                sps = level.get_samples_per_second()
                # get sample length
                ls = float(level.get_sample_count()) / float(sps)
                # samples per beat
                spb = 60.0 / bpm
                # get exponent
                f = math.log(ls / spb) / math.log(2.0)
                # new samplerate
                sps = sps * 2 ** (f - int(f + 0.5))
                level.set_samples_per_second(int(sps + 0.5))
        player.history_commit("fit loop")


class DataEntry(gtk.Dialog):
    """
    A data entry control for renaming files
    """
    def __init__(self, parent, title, label):
        """
        Initializer.
        """
        gtk.Dialog.__init__(self,
                title,
                parent.get_toplevel(),
                gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
                (gtk.STOCK_OK, gtk.RESPONSE_OK, gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL),
        )
        self.label = gtk.Label(label)
        self.edit = gtk.Entry()
        s = gtk.HBox()
        s.pack_start(self.label, expand=False)
        s.pack_start(self.edit)
        self.vbox.pack_start(s, expand=False)
        self.edit.grab_focus()
        self.edit.select_region(1, -1)
        self.show_all()
        self.edit.connect('activate', self.on_text_enter)

    def on_text_enter(self, widget):
        self.response(gtk.RESPONSE_OK)


__all__ = [
        'EnvelopeView',
        'WavetablePanel',
]

__neil__ = dict(
        classes=[
                EnvelopeView,
                WavetablePanel,
        ],
)

