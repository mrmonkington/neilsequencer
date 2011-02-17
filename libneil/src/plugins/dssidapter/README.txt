DSSIdaptor allows you to load DSSI plugins as zzub plugins.


TODO/Bad news:
-------------

DSSI guis talk directly to the plugin, so when you change parameters
via the gui, the self.plugin.set_parameter_value_direct() call in
rack.py is never triggered. The player doesn't know the user has
changed a value, so DSSI plugins' parameter values are not saved with
the song. I've added a hook, set_parameter_value_dssi, but I can't
figure out what to put in there to keep the compiler happy. Help? For
now, work around this by saving a preset in the DSSI gui, or by
pasting the parameter values you want into a pattern, or something...

We should add an LV2 adaptor.

We might want to look again at LADSPA, DSSI (and LV2) and allow
control-outs to work as peer connections, and we might want some
heuristics which guess (eg in the case of the LADSPA vocoder) that the
plugin does_input_mixing and should be given its audio inputs
separately.

There is a common problem with liblo which causes a delay when opening
a DSSI GUI. Upgrading to liblo 0.24 solves this problem. Otherwise,
check that your networking is correct. Make sure your loopback
interface (lo) is up. Make sure localhost is in the /etc/hosts file.
Try running jack-dssi-host to confirm that the problem is in the DSSI
networking rather than in Aldrin itself.

http://sourceforge.net/mailarchive/forum.php?forum_name=dssi-devel

Closing Aldrin doesn't call the plugins' destructor OR the destroy()
function. (I don't know why: is this a memory leak?) In the case of
DSSI it means the GUI stays open after Aldrin has closed. Deleting a
plugin *does* call these functions, and the DSSI GUI gets closed as
necessary.

The less_trivial_synth GUI splats warnings about unhandled OSC
commands on your terminal, but works fine.



Good news:
---------

Fluidsynth-DSSI had been the major stumbling block -- it was clicking
in a really annoying way. When I slowed down the TPB and BPM, I found
out that this was happening every tick, and then I realised that it
was happening precisely when process_stereo() was being called with a
non-power-of-two value for numsamples. Aldrin does this each tick
because the number of samples per tick is not a multiple of a power of
two. I found the problem in fluidsynth-dssi.c, and I made a patch and
sent it to the DSSI dev list. Sean Bolton (Fluidsynth-DSSI author)
made a better patch, which is attached in the Armstrong bug tracker on
bitbucket.org. The result is that Fluidsynth-DSSI is working for me
now.

I've added program changes (presets) and configure keys, and the
run_multiple_synths() method required by Fluidsynth and Hexter.
Plugins with audio inputs (eg ll-scope) are now allowed also. Plugins
with control-outs (eg my xy-gravity) are allowed (ie no longer
*crash*), but those control-outs aren't used for anything (yet).

Parameter changes in the GUI are transmitted to the plugin, and those
in the pattern editor are reflected in the GUI. Multiple tracks,
note-offs, the stop button, changing the number of tracks: should
work. The test buttons on some DSSI GUIs should work. MIDI-in should
work -- there's an attribute to set the channel.


These plugins work for me:

Trivial_Synth
Less_Trivial_Synth
Simple Mono Sampler
Simple Stereo Sampler
Hexter
Fluidsynth
WSynth
XSynth
Oscilloscope
WhySynth
Nekobee
Calf Filter
Calf Flanger
Calf Reverb
Calf Vintage Delay
Calf Rotary Speaker
Calf Phaser
Calf MultiChorus





-- jmmcd


