

CMachineParameter const paraRouting = 
{ 
	pt_byte,				// Type
	"Routing--",				// Name
	"Routing/Algorithm",	// description
	1,						// MinValue	
	15,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	1  					// Default
};



/**********************************************************
*                Osc4: Waveform                           *
*          Range : 1 to 99 (byte) Default : 1             *
**********************************************************/
CMachineParameter const paraOsc4Waveform = 
{ 
	pt_byte,				// Type
	"Osc4---Wave",				// Name
	"Osc4: Waveform",	// description
	1,						// MinValue	
	16,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	1  					// Default
};

/**********************************************************
*                Osc4: Frequency                          *
***********************************************************
*                                                         *
*          Range : 0 to 32 (byte) Default : 1             *
**********************************************************/
CMachineParameter const paraOsc4Frequency = 
{ 
	pt_byte,				// Type
	"        |--Freq",				// Name
	"Osc4: Frequency",	// description
	0,						// MinValue	
	32,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	1  					// Default
};

/**********************************************************
*                Osc4: Finetune                           *
***********************************************************
*     In steps of +0.004                                  *
*          Range : 0 to 254 (byte) Default : 0            *
**********************************************************/
CMachineParameter const paraOsc4Finetune = 
{ 
	pt_byte,				// Type
	"        |--Fine",				// Name
	"Osc4: Finetune",	// description
	0,						// MinValue	
	0xFE,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	0  					// Default
};

/**********************************************************
*                Osc4: Volume                             *
***********************************************************
*     64 = max vol = 4x rock                              *
*     -8 = /2                                             *
*     0  = 1/64 rock                                      *
*          Range : 0 to 64 (byte) Default : 32            *
**********************************************************/
CMachineParameter const paraOsc4Volume = 
{ 
	pt_byte,				// Type
	"        '--Volume",				// Name
	"Osc4: Volume",	// description
	0,						// MinValue	
	64,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	32  					// Default
};

/**********************************************************
*                Osc4: Attack                             *
***********************************************************
**********************************************************/
CMachineParameter const paraOsc4Attack = 
{ 
	pt_byte,				// Type
	"            |--A",				// Name
	"Osc4: Attack",	// description
	0,						// MinValue	
	32,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	32  					// Default
};

/**********************************************************
*                Osc4: Decay                              *
***********************************************************
**********************************************************/
CMachineParameter const paraOsc4Decay = 
{ 
	pt_byte,				// Type
	"            |--D",				// Name
	"Osc4: Decay",	// description
	0,						// MinValue	
	32,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	8  					// Default
};

/**********************************************************
*                Osc4: Sustain                            *
***********************************************************
**********************************************************/
CMachineParameter const paraOsc4Sustain = 
{ 
	pt_byte,				// Type
	"            |--S",				// Name
	"Osc4: Sustain",	// description
	0,						// MinValue	
	63,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	31  					// Default
};

/**********************************************************
*                Osc4: Release                            *
***********************************************************
**********************************************************/
CMachineParameter const paraOsc4Release = 
{ 
	pt_byte,				// Type
	"            '--R",				// Name
	"Osc4: Release",	// description
	0,						// MinValue	
	32,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	16  					// Default
};


/**********************************************************
*                Osc3: Waveform                           *
*          Range : 1 to 99 (byte) Default : 1             *
**********************************************************/
CMachineParameter const paraOsc3Waveform = 
{ 
	pt_byte,				// Type
	"Osc3---Wave",				// Name
	"Osc3: Waveform",	// description
	1,						// MinValue	
	16,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	1  					// Default
};

/**********************************************************
*                Osc3: Frequency                          *
***********************************************************
*                                                         *
*          Range : 0 to 32 (byte) Default : 1             *
**********************************************************/
CMachineParameter const paraOsc3Frequency = 
{ 
	pt_byte,				// Type
	"        |--Freq",				// Name
	"Osc3: Frequency",	// description
	0,						// MinValue	
	32,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	1  					// Default
};

/**********************************************************
*                Osc3: Finetune                           *
***********************************************************
*     In steps of +0.004                                  *
*          Range : 0 to 254 (byte) Default : 0            *
**********************************************************/
CMachineParameter const paraOsc3Finetune = 
{ 
	pt_byte,				// Type
	"        |--Fine",				// Name
	"Osc3: Finetune",	// description
	0,						// MinValue	
	0xFE,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	0  					// Default
};

/**********************************************************
*                Osc3: Volume                             *
***********************************************************
*     64 = max vol = 4x rock                              *
*     -8 = /2                                             *
*     0  = 1/64 rock                                      *
*          Range : 0 to 64 (byte) Default : 32            *
**********************************************************/
CMachineParameter const paraOsc3Volume = 
{ 
	pt_byte,				// Type
	"        '--Volume",				// Name
	"Osc3: Volume",	// description
	0,						// MinValue	
	64,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	32  					// Default
};

/**********************************************************
*                Osc3: Attack                             *
***********************************************************
**********************************************************/
CMachineParameter const paraOsc3Attack = 
{ 
	pt_byte,				// Type
	"            |--A",				// Name
	"Osc3: Attack",	// description
	0,						// MinValue	
	32,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	32  					// Default
};

/**********************************************************
*                Osc3: Decay                              *
***********************************************************
**********************************************************/
CMachineParameter const paraOsc3Decay = 
{ 
	pt_byte,				// Type
	"            |--D",				// Name
	"Osc3: Decay",	// description
	0,						// MinValue	
	32,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	8  					// Default
};

/**********************************************************
*                Osc3: Sustain                            *
***********************************************************
**********************************************************/
CMachineParameter const paraOsc3Sustain = 
{ 
	pt_byte,				// Type
	"            |--S",				// Name
	"Osc3: Sustain",	// description
	0,						// MinValue	
	63,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	31  					// Default
};

/**********************************************************
*                Osc3: Release                            *
***********************************************************
**********************************************************/
CMachineParameter const paraOsc3Release = 
{ 
	pt_byte,				// Type
	"            '--R",				// Name
	"Osc3: Release",	// description
	0,						// MinValue	
	32,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	16  					// Default
};


/**********************************************************
*                Osc2: Waveform                           *
*          Range : 1 to 99 (byte) Default : 1             *
**********************************************************/
CMachineParameter const paraOsc2Waveform = 
{ 
	pt_byte,				// Type
	"Osc2---Wave",				// Name
	"Osc2: Waveform",	// description
	1,						// MinValue	
	16,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	1  					// Default
};

/**********************************************************
*                Osc2: Frequency                          *
***********************************************************
*                                                         *
*          Range : 0 to 32 (byte) Default : 1             *
**********************************************************/
CMachineParameter const paraOsc2Frequency = 
{ 
	pt_byte,				// Type
	"        |--Freq",				// Name
	"Osc2: Frequency",	// description
	0,						// MinValue	
	32,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	1  					// Default
};

/**********************************************************
*                Osc2: Finetune                           *
***********************************************************
*     In steps of +0.004                                  *
*          Range : 0 to 254 (byte) Default : 0            *
**********************************************************/
CMachineParameter const paraOsc2Finetune = 
{ 
	pt_byte,				// Type
	"        |--Fine",				// Name
	"Osc2: Finetune",	// description
	0,						// MinValue	
	0xFE,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	0  					// Default
};

/**********************************************************
*                Osc2: Volume                             *
***********************************************************
*     64 = max vol = 4x rock                              *
*     -8 = /2                                             *
*     0  = 1/64 rock                                      *
*          Range : 0 to 64 (byte) Default : 32            *
**********************************************************/
CMachineParameter const paraOsc2Volume = 
{ 
	pt_byte,				// Type
	"        '--Volume",				// Name
	"Osc2: Volume",	// description
	0,						// MinValue	
	64,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	32  					// Default
};

/**********************************************************
*                Osc2: Attack                             *
***********************************************************
**********************************************************/
CMachineParameter const paraOsc2Attack = 
{ 
	pt_byte,				// Type
	"            |--A",				// Name
	"Osc2: Attack",	// description
	0,						// MinValue	
	32,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	32  					// Default
};

/**********************************************************
*                Osc2: Decay                              *
***********************************************************
**********************************************************/
CMachineParameter const paraOsc2Decay = 
{ 
	pt_byte,				// Type
	"            |--D",				// Name
	"Osc2: Decay",	// description
	0,						// MinValue	
	32,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	8  					// Default
};

/**********************************************************
*                Osc2: Sustain                            *
***********************************************************
**********************************************************/
CMachineParameter const paraOsc2Sustain = 
{ 
	pt_byte,				// Type
	"            |--S",				// Name
	"Osc2: Sustain",	// description
	0,						// MinValue	
	63,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	31  					// Default
};

/**********************************************************
*                Osc2: Release                            *
***********************************************************
**********************************************************/
CMachineParameter const paraOsc2Release = 
{ 
	pt_byte,				// Type
	"            '--R",				// Name
	"Osc2: Release",	// description
	0,						// MinValue	
	32,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	16  					// Default
};

/**********************************************************
*                Osc1: Waveform                           *
*          Range : 1 to 99 (byte) Default : 1             *
**********************************************************/
CMachineParameter const paraOsc1Waveform = 
{ 
	pt_byte,				// Type
	"Osc1---Wave",				// Name
	"Osc1: Waveform",	// description
	1,						// MinValue	
	16,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	1  					// Default
};

/**********************************************************
*                Osc1: Frequency                          *
***********************************************************
*                                                         *
*          Range : 0 to 32 (byte) Default : 1             *
**********************************************************/
CMachineParameter const paraOsc1Frequency = 
{ 
	pt_byte,				// Type
	"        |--Freq",				// Name
	"Osc1: Frequency",	// description
	0,						// MinValue	
	32,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	1  					// Default
};

/**********************************************************
*                Osc1: Finetune                           *
***********************************************************
*     In steps of +0.004                                  *
*          Range : 0 to 254 (byte) Default : 0            *
**********************************************************/
CMachineParameter const paraOsc1Finetune = 
{ 
	pt_byte,				// Type
	"        |--Fine",				// Name
	"Osc1: Finetune",	// description
	0,						// MinValue	
	0xFE,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	0  					// Default
};

/**********************************************************
*                Osc1: Volume                             *
***********************************************************
*     64 = max vol = 4x rock                              *
*     -8 = /2                                             *
*     0  = 1/64 rock                                      *
*          Range : 0 to 64 (byte) Default : 64            *
**********************************************************/
CMachineParameter const paraOsc1Volume = 
{ 
	pt_byte,				// Type
	"        '--Volume",				// Name
	"Osc1: Volume",	// description
	0,						// MinValue	
	64,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	56  					// Default
};

/**********************************************************
*                Osc1: Attack                             *
***********************************************************
**********************************************************/
CMachineParameter const paraOsc1Attack = 
{ 
	pt_byte,				// Type
	"            |--A",				// Name
	"Osc1: Attack",	// description
	0,						// MinValue	
	32,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	32  					// Default
};

/**********************************************************
*                Osc1: Decay                              *
***********************************************************
**********************************************************/
CMachineParameter const paraOsc1Decay = 
{ 
	pt_byte,				// Type
	"            |--D",				// Name
	"Osc1: Decay",	// description
	0,						// MinValue	
	32,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	8  					// Default
};

/**********************************************************
*                Osc1: Sustain                            *
***********************************************************
**********************************************************/
CMachineParameter const paraOsc1Sustain = 
{ 
	pt_byte,				// Type
	"            |--S",				// Name
	"Osc1: Sustain",	// description
	0,						// MinValue	
	63,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	31  					// Default
};

/**********************************************************
*                Osc1: Release                            *
***********************************************************
**********************************************************/
CMachineParameter const paraOsc1Release = 
{ 
	pt_byte,				// Type
	"            '--R",				// Name
	"Osc1: Release",	// description
	0,						// MinValue	
	32,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	16  					// Default
};

/**********************************************************
*                Lpf: Cutoff                              *
*          Range : 0 to 64 (byte) Default : 20            *
**********************************************************/
CMachineParameter const paraLpfCutoff = 
{ 
	pt_byte,				// Type
	"Lpf---Cutoff",				// Name
	"Lpf: Cutoff",	// description
	0,						// MinValue	
	0x80,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	0x6c  					// Default
};

/**********************************************************
*                Lpf: Resonance                           *
***********************************************************
**********************************************************/
CMachineParameter const paraLpfResonance = 
{ 
	pt_byte,				// Type
	"        |--Reso",				// Name
	"Lpf: Resonance",	// description
	0,						// MinValue	
	0x80,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	0x0  					// Default
};

/**********************************************************
*                Lpf: Key follow                          *
***********************************************************
**********************************************************/
CMachineParameter const paraLpfKeyFollow = 
{ 
	pt_byte,				// Type
	"        |--KF",				// Name
	"Lpf: Key Follow",	// description
	0,						// MinValue	
	0x80,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	0  					// Default
};

/**********************************************************
*                Lpf: Envelope                           *
***********************************************************
*     64 = max vol = 4x rock                              *
*     -8 = /2                                             *
*     0  = 1/64 rock                                      *
*          Range : 0 to 64 (byte) Default : 64            *
**********************************************************/
CMachineParameter const paraLpfEnvelope = 
{ 
	pt_byte,				// Type
	"        '--Env",				// Name
	"Lpf: Envelope",	// description
	0,						// MinValue	
	0x80,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	0  					// Default
};

/**********************************************************
*                Lpf: Attack                             *
***********************************************************
**********************************************************/
CMachineParameter const paraLpfAttack = 
{ 
	pt_byte,				// Type
	"            |--A",				// Name
	"Lpf: Attack",	// description
	0,						// MinValue	
	32,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	32  					// Default
};

/**********************************************************
*                Lpf: Decay                              *
***********************************************************
**********************************************************/
CMachineParameter const paraLpfDecay = 
{ 
	pt_byte,				// Type
	"            |--D",				// Name
	"Lpf: Decay",	// description
	0,						// MinValue	
	32,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	8  					// Default
};

/**********************************************************
*                Lpf: Sustain                            *
***********************************************************
**********************************************************/
CMachineParameter const paraLpfSustain = 
{ 
	pt_byte,				// Type
	"            |--S",				// Name
	"Lpf: Sustain",	// description
	0,						// MinValue	
	63,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	31  					// Default
};

/**********************************************************
*                Lpf: Release                            *
***********************************************************
**********************************************************/
CMachineParameter const paraLpfRelease = 
{ 
	pt_byte,				// Type
	"            '--R",				// Name
	"Lpf: Release",	// description
	0,						// MinValue	
	32,					// MaxValue
	0xff,						// NoValue
	MPF_STATE,				// Flags
	16  					// Default
};


/**********************************************************
*                        Note                             *
***********************************************************
*                      Standard                           *
*   1/2 tones from A-4: (x/16)*12+(x%16)-70+detune-128    *
*             pitch in hz: 440*2^(fromA4/12)              *
*             Waveguide length: sr/hz                     *
**********************************************************/
CMachineParameter const paraNote = 
{ 
	pt_note,				// Type
	"Note",				// Name
	"Note",	// description
	NOTE_MIN,						// MinValue	
	NOTE_MAX,					// MaxValue
	NOTE_NO,						// NoValue
	0,				// Flags
	0x80  					// Default
};

/**********************************************************
*                     Effect 1                            *
***********************************************************
* 00-40 = Set volume
* 5x    Volume slide down (idem)
* 6x    Volume slide up (see slide table)
* 7x    Pitch slide down (idem)
* 8x    Pitch slide up (idem)
* 9x    Glissando (idem)
* Ax    Vibrato (see vibrato table)
* Bx    Note delay (x=delay in 16ths of a row)
* Cx    Note retrigger (x=retrigs per row)
* Dx    ..
* Ex    ..
* Fx    ..
*
* Slide table:
* 0: Continue
* 1: 1
* 2: 2
* 3: 3
* 4: 4
* 5: 6
* 6: 8
* 7: 12
* 8: 16
* 9: 24
* A: 32
* B: 48
* C: 64
* D: 96
* E: 128
* F: 192
* 
* Vibrato table:
*     Amount
* Rate  1   2   3   4   5
*    1  A1  A2  A3  A4  A5
*    2  A6  A7  A8  A9  AA
*    3  AB  AC  AD  AE  AF
* 
**********************************************************/
CMachineParameter const paraVolume = 
{ 
	pt_byte,				// Type
	"Volume",				// Name
	"00-40:Volume",
	0,						// MinValue	
	0x40,					// MaxValue
	0xff,						// NoValue
	0,				// Flags
	0x40  					// Default
};






// Parameter Declaration
CMachineParameter const *pParameters[] = { 
// global

	&paraRouting,

	&paraOsc4Waveform,
	&paraOsc4Frequency,
	&paraOsc4Finetune,
	&paraOsc4Volume,
	&paraOsc4Attack,
	&paraOsc4Decay,
	&paraOsc4Sustain,
	&paraOsc4Release,

	&paraOsc3Waveform,
	&paraOsc3Frequency,
	&paraOsc3Finetune,
	&paraOsc3Volume,
	&paraOsc3Attack,
	&paraOsc3Decay,
	&paraOsc3Sustain,
	&paraOsc3Release,

	&paraOsc2Waveform,
	&paraOsc2Frequency,
	&paraOsc2Finetune,
	&paraOsc2Volume,
	&paraOsc2Attack,
	&paraOsc2Decay,
	&paraOsc2Sustain,
	&paraOsc2Release,

	&paraOsc1Waveform,
	&paraOsc1Frequency,
	&paraOsc1Finetune,
	&paraOsc1Volume,
	&paraOsc1Attack,
	&paraOsc1Decay,
	&paraOsc1Sustain,
	&paraOsc1Release,

	&paraLpfCutoff,
	&paraLpfResonance,
	&paraLpfKeyFollow,
	&paraLpfEnvelope,
	&paraLpfAttack,
	&paraLpfDecay,
	&paraLpfSustain,
	&paraLpfRelease,

	// Track
	&paraNote,
	&paraVolume

};
