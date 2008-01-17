CMachineParameter const paraSpeed = 
{ 
	pt_word,										// type
	"Speed",
	"Oscillation Speed",							// description
	0,												// MinValue	
	65534,											// MaxValue
	65535,											// NoValue
	MPF_STATE,										// Flags
	1000											// Default
};



CMachineParameter const paraUnit =
{ 
	pt_byte, 										// type
	"Speed Unit",
	"Speed unit (0 = mHz, 1 = ms, 2 = tick, 3 = 256th of tick)",		
	0,												// MinValue	
	3,												// MaxValue
	0xff,											// NoValue
	MPF_STATE,										// Flags
	UNIT_MHZ										// Default
};



CMachineParameter const paraWave =
{ 
	pt_byte, 										// type
	"Wave Type",
	"Wave Type (0 = sin, 1 = square, 2 = triangle 3 = saw, 4 = inv. saw)",
	0,												// MinValue	
	4,												// MaxValue
	0xff,											// NoValue
	MPF_STATE,										// Flags
	WAVE_SIN,										// Default
};

CMachineParameter const paraWavePow =
{ 
	pt_byte, 										// type
	"Wave Power",
	"Wave Power (1 = 1, 2 = ^2, 3 = ^3 etc)",
	1,												// MinValue	
	13,												// MaxValue
	0xff,											// NoValue
	MPF_STATE,										// Flags
	1,												// Default
};

CMachineParameter const paraFloor =
{ 
	pt_byte, 										// type
	"Floor",
	"Floor 00 - fe",
	0,												// MinValue	
	0xfe,											// MaxValue
	0xff,											// NoValue
	MPF_STATE,										// Flags
	0												// Default
};

CMachineParameter const paraSpread =
{ 
	pt_word, 										// type
	"Phase",
	"Phase x0000 - x0800",
	0x0000,											// MinValue	
	0x0800,											// MaxValue
	0xffff,											// NoValue
	MPF_STATE,										// Flags
	0x0800,											// Default
};

CMachineParameter const paraSlur =
{ 
	pt_byte, 										// type
	"Slur",
	"Slur x00 - xfe",
	0x00,											// MinValue	
	0xfe,											// MaxValue
	0xff,											// NoValue
	MPF_STATE,										// Flags
	0xf2,											// Default
};

CMachineParameter const paraGain =
{ 
	pt_byte, 										// type
	"Gain",
	"Gain x00 - xfe",
	0x00,											// MinValue	
	0xf0,											// MaxValue
	0xff,											// NoValue
	MPF_STATE,										// Flags
	0x12,											// Default
};

CMachineParameter const paraReset =
{ 
	pt_switch, 										// type
	"Reset",
	"Reset 1 = Restart Oscillator",
	0x01,											// MinValue	
	0x01,											// MaxValue
	0xff,											// NoValue
	MPF_TICK_ON_EDIT,								// Flags
	0xff,											// Default
};

CMachineParameter const *pParameters[] = 
{ 
	&paraSpeed,
	&paraUnit,
	&paraWave,
	&paraWavePow,
	&paraFloor,
	&paraSpread,
	&paraSlur,
	&paraGain,
	&paraReset,
};

