#!/usr/bin/python
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
Converts a buzz machine source file to a zzub plugin source file.
"""

import os,sys,re

if len(sys.argv) != 2:
	print >> sys.stderr, "syntax: zzubify.py <buzz source>"
	raise SystemExit

scriptbasename = os.path.basename(os.path.dirname(os.path.abspath(sys.argv[1])))

mainclassname = scriptbasename.replace(' ','_').lower()

default_export = """
zzub::plugin * create_plugin(const zzub::info *)
{
	return new $MAINCLASSNAME();
}

const zzub::info *g_infos[] = 
{
	&$MAINCLASSNAME_info,
	0,
};

const zzub::info ** zzub_get_infos() {
	return g_infos;
}
""".replace('$MAINCLASSNAME', mainclassname)

replaces = [
	(r'#include\s+["<][.\\/A-Za-z0-9]*[Mm][Dd][Kk]\.[Hh][>"]', "#include <zzub/plugin.h>"),
	(r'#include\s+["<][.\\/]*[Mm]achine[Ii]nterface\.[Hh][>"]', "#include <zzub/plugin.h>"),
	(r'#include\s+["<][Ww]indows\.[Hh][>"]', ""),
	(r'#include\s+["<]stdlib\.[Hh][>"]', "#include <stdlib.h>\n#include <stdio.h>\n#include <string.h>"),
	
	('class [A-Za-z0-9_]+ : public CMDKMachineInterfaceEx { };', ''),
	
	(r'mi::mi', mainclassname + '::' + mainclassname),
	(r'mi::~mi', mainclassname + '::~' + mainclassname),
	(r'mi::', mainclassname + '::'),
	(r'mi\(', mainclassname + '('),
	(r'class mi', 'class ' + mainclassname),
	(r'([^\w]+)mi([^\w]+)', lambda m: m.group(1) + mainclassname + m.group(2)),
	
	('MacInfo', mainclassname + '_info'),
	
	(r'pi[-][>]Read', 'pi->read'),
	(r'po[-][>]Write', 'po->write'),

	('(virtual\s+)?void Tick', lambda m: (m.group(1) or '') + 'void process_events'),
	(r'void (\w+)::Tick', lambda m: 'void ' + m.group(1) + '::process_events'),
	('virtual void MDKInit', 'virtual void init'),
	(r'void (\w+)::MDKInit', lambda m: 'void ' + m.group(1) + '::init'),
	('(virtual\s+)?void Init([^\w]+)', lambda m: (m.group(1) or '') + 'void init' + m.group(2)),
	(r'void (\w+)::Init', lambda m: 'void ' + m.group(1) + '::init'),
	('virtual void MDKSave', 'virtual void save'),
	(r'void (\w+)::MDKSave', lambda m: 'void ' + m.group(1) + '::save'),
	('virtual bool MDKWorkStereo', 'virtual bool process_stereo'),
	(r'bool (\w+)::MDKWorkStereo', lambda m: 'bool ' + m.group(1) + '::process_stereo'),
	('virtual bool WorkMonoToStereo', 'virtual bool process_stereo'),
	(r'bool (\w+)::WorkMonoToStereo', lambda m: 'bool ' + m.group(1) + '::process_stereo'),
	('virtual void Command', 'virtual void command'),
	(r'void (\w+)::Command', lambda m: 'void ' + m.group(1) + '::command'),
	('(virtual )?char const\s*\*\s*DescribeValue', 'virtual const char * describe_value'),
	(r'char const\s*\*\s*(\w+)::DescribeValue', lambda m: 'const char * ' + m.group(1) + '::describe_value'),
	('virtual CMDKMachineInterfaceEx \*\s*GetEx', '//'),
	(r'virtual CMDKMachineInterfaceEx \*\s*(\w+)::GetEx', '//'),
	('miex ex;', ''),
	
	('SetOutputMode\(', '//'),
	(r'GetOscillatorTable', 'get_oscillator_table'),
	(r'GetAuxBuffer', 'get_auxiliary_buffer'),
	(r'ControlChange', 'control_change'),
	(r'GetNearestWaveLevel', 'get_nearest_wave_level'),
	(r'GetOscTblOffset', 'zzub::get_oscillator_table_offset'),
	(r'GetThisMachine', 'get_metaplugin'),
	(r'([^\w]+)GetWave([^\w]+)',lambda m: m.group(1) + 'get_wave' + m.group(2)),

	(r'DescribeValue', 'describe_value'),
	(r'([^\w]+)Tick([^\w]+)',lambda m: m.group(1) + 'process_events' + m.group(2)),
	(r'([^\w]+)Init([^\w]+)',lambda m: m.group(1) + 'init' + m.group(2)),

	(r'CMachineParameter\s+const\s+','const zzub::parameter '),
	(r'CMachineParameter','zzub::parameter'),
	(r'CMachineAttribute\s+const\s+','const zzub::attribute '),
	(r'CMachineAttribute','zzub::attribute'),
	(r'CMachineInfo\s+const\s+','const zzub::info '),
	(r'CMachineInfo','zzub::info'),
	
	(r'CMachineInterface', 'zzub::plugin'),
	(r'CMDKMachineInterfaceEx', 'zzub::plugin2'),
	(r'CMDKMachineInterface', 'zzub::plugin'),
	(r'CMachineDataInput\s+const\s+','const zzub::instream '),
	(r'CMachineDataInput', 'zzub::instream'),
	(r'CMachineDataOutput\s+const\s+','const zzub::outstream '),
	(r'CMachineDataOutput', 'zzub::outstream'),
	
	(r'CWaveLevel', 'zzub::wave_level'),
	(r'CWaveInfo', 'zzub::wave_info'),
	
	(r'Flags', 'flags'),
	(r'Volume', 'volume'),
	(r'RootNote', 'root_note'),
	(r'LoopStart', 'loop_start'),
	(r'LoopEnd', 'loop_end'),
	(r'pSamples', 'samples'),
	(r'numSamples', 'sample_count'),
	
	(r'GlobalVals', 'global_values'),
	(r'TrackVals', 'track_values'),
	(r'AttrVals', 'attributes'),
	(r'pMasterInfo', '_master_info'),
	(r'pCB', '_host'),
	
	('SamplesPerSec', 'samples_per_second'),
	('SamplesPerTick', 'samples_per_tick'),
		
	(r'pt_byte','zzub::parameter_type_byte'),
	(r'pt_word','zzub::parameter_type_word'),
	(r'pt_note','zzub::parameter_type_note'),
	(r'pt_switch','zzub::parameter_type_switch'),
	(r'([^\w]+)byte([^\w]+)',lambda m: m.group(1) + 'unsigned char' + m.group(2)),
	(r'([^\w]+)word([^\w]+)',lambda m: m.group(1) + 'unsigned short int' + m.group(2)),
	(r'([^\w]+)dword([^\w]+)',lambda m: m.group(1) + 'unsigned int' + m.group(2)),
	
	(r'([^\w]+)CMachine([^\w]+)',lambda m: m.group(1) + 'zzub::metaplugin' + m.group(2)),
		
	(r'MACHINE_LOCK', 'ZZUB_PLUGIN_LOCK'),
		
	(r'DefValue', 'value_default'),
	(r'NoValue', 'value_none'),
	(r'MaxValue', 'value_max'),
	(r'MinValue', 'value_min'),
	
	(r'MI_VERSION', 'zzub::version'),
	(r'MAX_BUFFER_LENGTH', 'zzub::buffer_size'),
	
	(r'MIF_DOES_INPUT_MIXING', 'zzub::plugin_flag_does_input_mixing'),
	(r'MIF_MONO_TO_STEREO', 'zzub::plugin_flag_mono_to_stereo'),
	(r'MIF_PLAYS_WAVES', 'zzub::plugin_flag_plays_waves'),
	
	(r'MT_EFFECT', 'zzub::plugin_type_effect'),
	(r'MT_GENERATOR', 'zzub::plugin_type_generator'),
	(r'MT_MASTER', 'zzub::plugin_type_master'),
	
	(r'MPF_STATE', 'zzub::parameter_flag_state'),
	(r'MPF_WAVE', 'zzub::parameter_flag_wavetable_index'),

	(r'WF_LOOP', 'zzub::wave_flag_loop'),

	(r'WM_READWRITE', 'zzub::process_mode_read_write'),
	(r'WM_WRITE', 'zzub::process_mode_write'),
	(r'WM_READ', 'zzub::process_mode_read'),
	(r'WM_NOIO', 'zzub::process_mode_no_io'),
	
	(r'NOTE_OFF', 'zzub::note_value_off'),
	(r'NOTE_MIN', 'zzub::note_value_min'),
	(r'NOTE_MAX', 'zzub::note_value_max'),
	(r'NOTE_NO', 'zzub::note_value_none'),
	
	(r'SWITCH_NO', 'zzub::switch_value_none'),
	(r'SWITCH_OFF', 'zzub::switch_value_off'),
	(r'SWITCH_ON', 'zzub::switch_value_on'),
	
	(r'OWF_SINE', 'zzub::oscillator_type_sine'),
	(r'OWF_NOISE', 'zzub::oscillator_type_noise'),
	
	('__min', 'std::min'),
	('__max', 'std::max'),
	
	('DLL_EXPORTS', default_export),
	
]

compilers = []

for k,v in replaces:
	compilers.append((re.compile(k),v))

for line in file(sys.argv[1],'r'):
	for c,v in compilers:
		line = c.sub(v,line)
	sys.stdout.write(line)
