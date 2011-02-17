#encoding: latin-1

# converts param const blocks into zzub format, you need to paste the blocks manually.

def strip_comments(s):
	# strip any commentary
	o = ""
	for l in s.replace('\r','').split('\n'):
		l = l.split('//')[0]
		o += l + '\n'
	return o

order = """
  &paraWaveformA,
	&paraPWMRateA,
	&paraPWMRangeA,
	&paraPWOffsetA,
  &paraWaveformB,
	&paraPWMRateB,
	&paraPWMRangeB,
	&paraPWOffsetB,
  &paraTranspose,
  &paraDetune,
	&paraOscMix,        // 10
	&paraSubOscWave,
	&paraSubOscvolume,
  &paraGlide,

  &paraFilterType,  
  &paraFilterCutoff,
  &paraFilterResonance,
  &paraFilterModulation,
  &paraFilterAttack, 
  &paraFilterDecay, 
	&paraFilterSustain, // 20
	&paraFilterRelease,
  &paraFilterShape,
  &paraFilterInertia,
  &paraFilterTrack,

  &paraLFORate,
  &paraLFOAmount1, 
  &paraLFOAmount2,
  &paraLFOShape,

  &paraLFO2Rate,
  &paraLFO2Amount1, 
  &paraLFO2Amount2,
  &paraLFO2Shape,

  &paraAmpAttack,
  &paraAmpDecay,    
	&paraAmpSustain,
	&paraAmpRelease,

	&paraLFOMode,

  &paraNote,       
  &paraVelocity,     
  &paraLength,
	&paraCommand1,
	&paraArgument1,
	&paraCommand2,
	&paraArgument2,
"""

order = strip_comments(order)

import re
c = re.compile(r"[\n\s]*\&([^,]+)[,][\n\s]*")

varnames = []

while order:
	m = c.search(order)
	assert m
	assert m.start(0) == 0, m.start(0)
	assert m.start(0) != m.end(0)
	order = order[m.end(0):]
	varnames.append(m.group(1))
	print "const zzub::parameter *%s = 0;" % m.group(1)
	
print "// %i variables" % len(varnames)

for n in varnames:
	print "extern const zzub::parameter *%s;" % n

print "// %i variables" % len(varnames)

all_param_varnames = varnames

decl = """
const zzub::parameter paraWaveformA = 
{ 
	zzub::parameter_type_byte,										// type
	"OSC1 Wave",
	"OSC1 Waveform",					// description
	0,												  // value_min	
	22,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	3
};

const zzub::parameter paraPWMRateA = 
{ 
	zzub::parameter_type_byte,										// type
	" - PWM Rate",
	"OSC1 Pulse Width Modulation Rate",					// description
	0,												  // value_min	
	239,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	80
};

const zzub::parameter paraPWMRangeA = 
{ 
	zzub::parameter_type_byte,										// type
	" - PWM Depth",
	"OSC1 Pulse Width Modulation Range",					// description
	0,												  // value_min	
	239,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	100
};

const zzub::parameter paraPWOffsetA = 
{ 
	zzub::parameter_type_byte,										// type
	" - PW Offset",
	"OSC1 Pulse Width Modulation Offset",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	180
};

const zzub::parameter paraWaveformB = 
{ 
	zzub::parameter_type_byte,										// type
	"OSC2 Wave",
	"OSC2 Waveform",					// description
	0,												  // value_min	
	22,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	3
};

const zzub::parameter paraPWMRateB = 
{ 
	zzub::parameter_type_byte,										// type
	" - PWM Rate",
	"OSC1 Pulse Width Modulation Rate",					// description
	0,												  // value_min	
	239,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	80
};

const zzub::parameter paraPWMRangeB = 
{ 
	zzub::parameter_type_byte,										// type
	" - PWM Depth",
	"OSC2 Pulse Width Modulation Range",					// description
	0,												  // value_min	
	239,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	180
};

const zzub::parameter paraPWOffsetB = 
{ 
	zzub::parameter_type_byte,										// type
	" - PW Offset",
	"OSC2 Pulse Width Modulation Offset",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	120
};

const zzub::parameter paraTranspose = 
{ 
	zzub::parameter_type_byte,										// type
	" - Transpose",
	"OSC2 Transpose",					// description
	0,												  // value_min	
	72,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	36+12
};

const zzub::parameter paraDetune = 
{ 
	zzub::parameter_type_byte,										// type
	" - Detune",
	"OSC Detune",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	8
};

const zzub::parameter paraOscMix = 
{ 
	zzub::parameter_type_byte,										// type
	"OSC Mix",
	"OSC Mix",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	60
};

const zzub::parameter paraSubOscWave = 
{ 
	zzub::parameter_type_byte,										// type
	"SubOsc Wave",
	"Sub Oscillator Wave",					// description
	0,												  // value_min	
	20,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	1
};

const zzub::parameter paraSubOscvolume = 
{ 
	zzub::parameter_type_byte,										// type
	"SubOsc Vol",
	"Sub Oscillator volume",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	240
};

const zzub::parameter paraGlide = 
{ 
	zzub::parameter_type_byte,										// type
	"Glide",
	"Glide",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	0
};

const zzub::parameter paraFilterType = 
{ 
	zzub::parameter_type_byte,										// type
	"Flt Type",
	"Filter Type",					// description
	0,												  // value_min	
	17,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	1
};

const zzub::parameter paraFilterCutoff = 
{ 
	zzub::parameter_type_byte,										// type
	" - Cutoff",
	"Filter Cutoff",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	60
};

const zzub::parameter paraFilterResonance = 
{ 
	zzub::parameter_type_byte,										// type
	" - Reso",
	"Filter Resonance",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	0
};

const zzub::parameter paraFilterModulation = 
{ 
	zzub::parameter_type_byte,										// type
	" - EnvMod",
	"Filter Modulation",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	200
};

const zzub::parameter paraFilterAttack = 
{ 
	zzub::parameter_type_byte,										// type
	" - Attack",
	"Filter Attack",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	50
};

const zzub::parameter paraFilterDecay = 
{ 
	zzub::parameter_type_byte,										// type
	" - Decay",
	"Filter Decay",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	80
};

const zzub::parameter paraFilterSustain = 
{ 
	zzub::parameter_type_byte,										// type
	" - Sustain",
	"Filter Sustain",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	40
};

const zzub::parameter paraFilterRelease = 
{ 
	zzub::parameter_type_byte,										// type
	" - Release",
	"Filter Release",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	10
};

const zzub::parameter paraFilterShape = 
{ 
	zzub::parameter_type_byte,										// type
	" - Mod Shp",
	"Filter Modulation Shape",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	100
};

const zzub::parameter paraFilterTrack = 
{ 
	zzub::parameter_type_byte,										// type
	" - KTrack",
	"Filter Key Tracking",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	180
};

const zzub::parameter paraLFORate = 
{ 
	zzub::parameter_type_byte,										// type
	"LFO1 Rate",
	"LFO1 Rate",					// description
	0,												  // value_min	
	254,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	80
};

const zzub::parameter paraLFOAmount1 = 
{ 
	zzub::parameter_type_byte,										// type
	" - To Cutoff",
	"LFO1->Cutoff",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	140
};

const zzub::parameter paraLFOAmount2 = 
{ 
	zzub::parameter_type_byte,										// type
	" - To Env",
	"LFO1->EnvMod",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	120
};

const zzub::parameter paraLFOShape = 
{ 
	zzub::parameter_type_byte,										// type
	" - Shape",
	"LFO1 Shape",					// description
	0,												  // value_min	
	16,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	0
};

const zzub::parameter paraLFO2Rate = 
{ 
	zzub::parameter_type_byte,										// type
	"LFO2 Rate",
	"LFO2 Rate",					// description
	0,												  // value_min	
	254,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	80
};

const zzub::parameter paraLFO2Amount1 = 
{ 
	zzub::parameter_type_byte,										// type
	" - To Cutoff",
	"LFO2->Cutoff",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	120
};

const zzub::parameter paraLFO2Amount2 = 
{ 
	zzub::parameter_type_byte,										// type
	" - To Res",
	"LFO2->Res",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	120
};

const zzub::parameter paraLFO2Shape = 
{ 
	zzub::parameter_type_byte,										// type
	" - Shape",
	"LFO2 Shape",					// description
	0,												  // value_min	
	16,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	0
};

const zzub::parameter paraAmpAttack = 
{ 
	zzub::parameter_type_byte,										// type
	"Amp Attack",
	"Amplitude Attack",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	40
};

const zzub::parameter paraAmpDecay = 
{ 
	zzub::parameter_type_byte,										// type
	" - Decay",
	"Amplitude Decay",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	40
};

const zzub::parameter paraAmpSustain = 
{ 
	zzub::parameter_type_byte,										// type
	" - Sustain",
	"Amplitude Sustain",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	180
};

const zzub::parameter paraAmpRelease = 
{ 
	zzub::parameter_type_byte,										// type
	" - Release",
	"Amplitude Rel",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	20
};

const zzub::parameter paraFilterInertia = 
{ 
	zzub::parameter_type_byte,										// type
	" - Inertia",
	"Filter Intertia",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	90
};

const zzub::parameter paraLFOMode = 
{ 
	zzub::parameter_type_byte,										// type
	"Mode flags",
	"Mode flags",					  // description
	0,											// value_min	
	127,										  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	0
};

const zzub::parameter paraLFOPhase = 
{ 
	zzub::parameter_type_byte,										// type
	"LFO Phase",
	"LFO Phase",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	0,										// flags // zzub::parameter_flag_state
	0
};

const zzub::parameter paraNote = 
{ 
	zzub::parameter_type_note,										// type
	"Note",
	"Note",					// description
	0,												  // value_min	
	240,												  // value_max
	0,										// value_none
	0,										// flags // zzub::parameter_flag_state
	0
};

const zzub::parameter paraVelocity = 
{ 
	zzub::parameter_type_byte,										// type
	"Velocity",
	"Velocity",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	224
};

const zzub::parameter paraLength = 
{ 
	zzub::parameter_type_byte,										// type
	"Length",
	"Length",					// description
	0,												  // value_min	
	240,												  // value_max
	255,										// value_none
	zzub::parameter_flag_state,										// flags // zzub::parameter_flag_state
	40
};

const zzub::parameter paraCommand1 = 
{ 
	zzub::parameter_type_byte,										// type
	"Command 1",
	"Command 1",					// description
	0,												  // value_min	
	255,												  // value_max
	255,										// value_none
	0,										// flags // zzub::parameter_flag_state
	0
};

const zzub::parameter paraArgument1 = 
{ 
	zzub::parameter_type_word,										// type
	"Argument 1",
	"Argument 1",					// description
	0,												  // value_min	
	65535,												  // value_max
	0,										// value_none
	0,										// flags // zzub::parameter_flag_state
	0
};

const zzub::parameter paraCommand2 = 
{ 
	zzub::parameter_type_byte,										// type
	"Command 2",
	"Command 2",					// description
	0,												  // value_min	
	255,												  // value_max
	255,										// value_none
	0,										// flags // zzub::parameter_flag_state
	0
};

const zzub::parameter paraArgument2 = 
{ 
	zzub::parameter_type_word,										// type
	"Argument 2",
	"Argument 2",					// description
	0,												  // value_min	
	65535,												  // value_max
	0,										// value_none
	0,										// flags // zzub::parameter_flag_state
	0
};

const zzub::parameter paraVibrato2 = 
{ 
	zzub::parameter_type_byte,										// type
	"Osc 2 Vib",
	"Osc 2 Vibrato",					// description
	0,												  // value_min	
	239,												  // value_max
	255,										// value_none
	0,										// flags // zzub::parameter_flag_state
	0
};
"""

decl = strip_comments(decl)

import re

s = r"[\n\s]*const[\n\s]+(zzub\:\:)?parameter[\n\s]+([^:]+\:\:)?([^\n\s=]+)[\n\s]*[=][\n\s]*\{"
s += r"[\n\s]*(zzub\:\:)?parameter_type_([\w]+)[\n\s]*[,]"
s += r"[\n\s]*[\"]([^\"]*)[\"][\n\s]*[,]"
s += r"[\n\s]*[\"]([^\"]*)[\"][\n\s]*[,]"
s += r"[\n\s]*([^,]+)[\n\s]*[,]"
s += r"[\n\s]*([^,]+)[\n\s]*[,]"
s += r"[\n\s]*([^,]+)[\n\s]*[,]"
s += r"[\n\s]*([^,]+)[\n\s]*[,]"
s += r"[\n\s]*([^,}]+)[\n\s]*[,]?"
s += r"[\n\s]*}[\n\s]*;[\n\s]*"

c = re.compile(s)

decls = {}

while decl:
	m = c.search(decl)
	assert m
	assert m.start(0) == 0, "%i: %r..." % (m.start(0), decl[m.start(0):m.start(0)+100])
	assert m.start(0) != m.end(0)
	decl = decl[m.end(0):]

	param_classname = m.group(2)
	param_varname = m.group(3)
	param_type = m.group(5)
	param_name = m.group(6)
	param_description = m.group(7)
	param_min = m.group(8)
	param_max = m.group(9)
	param_none = m.group(10)
	param_flags = m.group(11)
	param_default = m.group(12)

	if param_classname:
		o = "%s%s = &add_global_parameter()\n" % (param_classname, param_varname)
	else:
		o = "%s = &add_global_parameter()\n" % param_varname
	o += "\t.set_%s()\n" % param_type
	o += '\t.set_name("%s")\n' % param_name
	o += '\t.set_description("%s")\n' % param_description
	o += '\t.set_value_min(%s)\n' % param_min
	o += '\t.set_value_max(%s)\n' % param_max
	o += '\t.set_value_none(%s)\n' % param_none
	o += '\t.set_flags(%s)\n' % param_flags
	o += '\t.set_value_default(%s);\n' % param_default
	o += '\n'
	decls[param_varname] = o
	
for vn in varnames:
	print decls[vn]

attrorder = """
	&attrMIDIChannel,
  &attrMIDIVelocity,
  &attrHighQuality,
  &attrCrispness,
  &attrTheviderness,
  &attrGlobalTuning,
  &attrClipTable,
"""

attrorder = strip_comments(attrorder)
print attrorder
import re
c = re.compile(r"[\n\s]*\&([^,\n\s]+)[,]?[\n\s]*")

varnames = []

while attrorder:
	m = c.search(attrorder)
	assert m
	assert m.start(0) == 0, m.start(0)
	assert m.start(0) != m.end(0)
	attrorder = attrorder[m.end(0):]
	varnames.append(m.group(1))
	print "const zzub::attribute *%s = 0;" % m.group(1)

print "// %i attributes" % len(varnames)

for n in varnames:
	print "extern const zzub::attribute *%s;" % n
	
print "// %i attributes" % len(varnames)

attrdecls = """
const zzub::attribute attrMIDIChannel = 
{
	"MIDI Channel (0=off)",
	0,
	16,
	0	
};

const zzub::attribute attrMIDIVelocity = 
{
	"MIDI Use Velocity",
	0,
	1,
	0	
};

const zzub::attribute attrHighQuality = 
{
	"High quality",
	0,
	3,
	1
};

const zzub::attribute attrCrispness = 
{
	"Crispness factor",
	0,
	3,
	0
};

const zzub::attribute attrTheviderness = 
{
	"Theviderness factor",
	0,
	50,
	20
};

const zzub::attribute attrGlobalTuning = 
{
	"Global tuning (cents)",
	-100,
	100,
	0
};

const zzub::attribute attrVirtualChannels = 
{
	"Fadeout vchannels",
	0,
	16,
	8
};

const zzub::attribute attrClipTable = 
{
	"Colour",
	0,
	3,
	0
};
"""

attrdecls = strip_comments(attrdecls)

import re

s = r"[\n\s]*const[\n\s]+(zzub\:\:)?attribute[\n\s]+([^:]+\:\:)?([^\n\s=]+)[\n\s]*[=][\n\s]*\{"
s += r"[\n\s]*[\"]([^\"]*)[\"][\n\s]*[,]"
s += r"[\n\s]*([^,]+)[\n\s]*[,]"
s += r"[\n\s]*([^,]+)[\n\s]*[,]"
s += r"[\n\s]*([^,}\n\s]+)[\n\s]*[,]?"
s += r"[\n\s]*}[\n\s]*;[\n\s]*"

c = re.compile(s)

decls = {}

while attrdecls:
	m = c.search(attrdecls)
	assert m
	assert m.start(0) == 0, m.start(0)
	assert m.start(0) != m.end(0)
	attrdecls = attrdecls[m.end(0):]

	param_class = m.group(2)
	param_varname = m.group(3)
	param_name = m.group(4)
	param_min = m.group(5)
	param_max = m.group(6)
	param_default = m.group(7)

	if param_class:
		o = "%s%s = &add_attribute()\n" % (param_class,param_varname)
	else:
		o = "%s = &add_attribute()\n" % param_varname
	o += '\t.set_name("%s")\n' % param_name
	o += '\t.set_value_min(%s)\n' % param_min
	o += '\t.set_value_max(%s)\n' % param_max
	o += '\t.set_value_default(%s);\n' % param_default
	o += '\n'
	decls[param_varname] = o
	
for vn in varnames:
	print decls[vn]

code = """

"""

if code.strip():
	for vn in all_param_varnames:
		code = code.replace(vn + ".", vn + "->")

	print code
