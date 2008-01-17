Updates:
-------


2008 01 09: added Geonik DF Filter and MadBrain Dynamite6 and a couple
of help files; added Intoxicat ACloud (thanks to Intoxicat for sending
the code!).

2008 01 07: added a new empty function, process_offline(), to work
with latest zzub.

2008 01 03: added some new machines: Elenzil Modulator, Geonik
Omega-1, MadBrain 4fm2f, FSM ArpMan, FSM Sprayman.

2007 12 26: added Jeskola Noise Generator.

2007 12 22: realised that the aBuffer crash wasn't its fault: it
happens for every effect. So aBuffer is officially working.

2007 12 19: changes to SConscript, will it now build for everyone?
Added some new machines: Tic Tac Shut-Up aBuffer (don't use this yet)
Geonik Gapper, CyanPhase DTMF-1.

2007 12 18: fixed the crash when deleting the machines, made them
output stereo.

2007 12 17: initial release: Geonik's PluckedString and PrimiFun.


What is this?
------------

This directory contains Buzz machine code written by various authors,
checked into zzub svn by someone, and ported to zzub by me, jmmcd.
Copyright remains with the original authors; the license is assumed to
be GPL (in some cases it's stated explicitly). The porting consisted
of a few small edits plus the addition, to each machine, of a thin
layer of zzub which basically calls the underlying Buzz machine
functions. There's been very little testing so far. 

If you're a brave user, please have a go. But don't save any valuable
songs using these machines (yet) -- no guarantees they'll ever be
stable or part of zzub proper. If you have problems or if you'd like
to request some more porting or if you know where to find some more
Buzz machine source code please contact me. If you'd like to thank or
acknowledge the original authors, find their addresses in the source
code -- they deserve it!



To make this work:
-----------------

1. Copy the contents into your <plugins> directory (for me, <plugins>
is ~/dev/zzub/src/plugins and it contains other directories like
green_milk/ and buzz2zzub/):

$ cp -r Geonik/ CyanPhase_DTMF-1/ TicTacShutUp_aBuffer/
Jeskola_Noise_Generator/ MadBrain_4fm2f/ MadBrain_Dynamite6/
Elenzil_Modulator/ Geoffroy_Notefilter/ FSM_ArpMan/ FSM_SprayMan/
Intoxicat_ACloud/ <plugins>


2. As you did when installing zzub:

$ cd zzub
$ scons
$ sudo scons install


3. Add these lines to your /usr/<local>/share/aldrin/index.xml
(remembering to keep the generators and the effects in their proper places):

	<ref label="Geonik PrimiFun (Synthesizer)" uri="jamesmichaelmcdermott@gmail.com/generator/primifun;1"/>
	<ref label="Geonik PluckedString (Synthesizer)" uri="jamesmichaelmcdermott@gmail.com/generator/pluckedstring;1"/>
	<ref label="Geonik Omega-1 (Synthesizer) [Warning: Bugs?]" uri="jamesmichaelmcdermott@gmail.com/generator/omega1;1"/>
	<ref label="CyanPhase DTMF-1 (Telephone)" uri="jamesmichaelmcdermott@gmail.com/generator/DTMF_1;1"/>
	<ref label="Jeskola Noise Generator" uri="jamesmichaelmcdermott@gmail.com/generator/Noise;1"/>
	<ref label="MadBrain 4fm2f (FM Synth)" uri="jamesmichaelmcdermott@gmail.com/generator/4fm2f;1"/>
	<ref label="MadBrain Dynamite6 (Waveguide Synth)" uri="jamesmichaelmcdermott@gmail.com/generator/dynamite6;1"/>
	<ref label="FSM ArpMan (Arpeggiating Synth) [Warning: CPU]" uri="jamesmichaelmcdermott@gmail.com/generator/arpman;1"/>
	<ref label="Intoxicat ACloud (Wavetable Granulation)" uri="jamesmichaelmcdermott@gmail.com/generator/acloud;1"/>

	<ref label="Geonik Gapper (Modulator)" uri="jamesmichaelmcdermott@gmail.com/effect/gapper;1"/>
	<ref label="Geonik DF Filter (Non-linear filter)" uri="jamesmichaelmcdermott@gmail.com/effect/dffilter;1"/>
	<ref label="Elenzil Modulator" uri="jamesmichaelmcdermott@gmail.com/effect/modulator;1"/>
	<ref label="TicTac ShutUp aBuffer (Sample/Hold)" uri="jamesmichaelmcdermott@gmail.com/effect/abuffer;1"/>
	<ref label="Geoffroy NoteFilter" uri="jamesmichaelmcdermott@gmail.com/effect/notefilter;1"/>
	<ref label="FSM SprayMan (Granulation)" uri="jamesmichaelmcdermott@gmail.com/effect/sprayman;1"/>


4. Restart Aldrin.



TODO:
----

Geonik Omega-1 is mostly done, but I don't know how and where to make
Scons install the sound-file Mandpluk.wav. If you want to try it out,
change the value of "wav_filename" in Omega1.cpp, copy the file to
whatever path you specified, and recompile. I think there might be
other problems with it also...

MadBrain's Dynamite sounds *superb*, but buzzmachines.com reviews say:
amplitude parameter clicks on movement; sub-sonic frequencies, needs
dc-correction.

Elenzil Modulator clicks when the floor is low and the square or saw
waves are in use. Should be easily fixed.

My own Peer Note-Pool is in progress.

I wonder is it possible to port the presets, where they're available.




Notes:
-----

ArpMan uses an outrageous amount of CPU, but there is potential for
improving it. Have a look at the code for some hints.

Is the aBuffer right? The default settings seem strange to me.

The default setting for 4fm2f sounds like the machine is broken, but
there are some great sounds to be had with the right settings.

Noise Generator + Note Filter is a great combination.

Geonik DF Filter (aka Dobson-Fitch filter) isn't suitable for live use
-- it blows up very easily. But it can sound really cool. See the help
file for notes on parameter ranges.

ACloud uses a common hack -- parameters with odd ranges -- to produce
"dividers" in the machine control window, but I don't think it works
in Aldrin (or I can't get it to work). Just ignore the divider
parameters.


Thanks:
------

Thanks to #aldrin people for answering questions so far and James
Stone for converting the Mandpluk.sw file.

Many thanks to the original authors for making their source code
available.





Contact:
-------

James McDermott (jmmcd on aldrin-users and #aldrin)
jamesmichaelmcdermott@gmail.com
http://www.skynet.ie/~jmmcd

This archive is available from here, unless/until it gets checked into
zzub svn:

http://www.skynet.ie/~jmmcd/software/zzub_ports_jmmcd.tgz


