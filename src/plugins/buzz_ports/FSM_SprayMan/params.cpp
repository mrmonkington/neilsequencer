CMachineParameter const paraDryOut = 
{ 
	pt_byte,										// type
	"Dry out",
	"Dry out [dB]",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags
	240
};

CMachineParameter const paraFeedback = 
{ 
	pt_byte,									// type
	"Feedback",
	"Feedback",// description
	0,												// MinValue	
	99,											// MaxValue
	255,										  // NoValue
	MPF_STATE,								// Flags
	0,                       // default
};

CMachineParameter const paraOctaviation = 
{ 
	pt_byte,									// type
	"Fullness",
	"Fullness",// description
	0,												// MinValue	
	100,											// MaxValue
	255,										  // NoValue
	MPF_STATE,								// Flags
	0,                       // default
};

CMachineParameter const paraRichness = 
{ 
	pt_byte,									// type
	"Richness",
	"Richness", 		// description
	4,												// MinValue	
	MAX_GRANULES,										  // MaxValue
	255,										  // NoValue
	MPF_STATE,								// Flags
	12,                       // default
};

CMachineParameter const paraDensity = 
{ 
	pt_byte,									// type
	"Density",
	"Density",// description
	0,												// MinValue	
	64,											// MaxValue
	255,										  // NoValue
	MPF_STATE,								// Flags
	48,                       // default
};

CMachineParameter const paraSpaceyness = 
{ 
	pt_byte,									// type
	"Scattering",
	"Scattering",// description
	0,												// MinValue	
	64,											// MaxValue
	255,										  // NoValue
	MPF_STATE,								// Flags
	48,                       // default
};

CMachineParameter const paraAttack = 
{ 
	pt_byte,									// type
	"Attack",
	"Attack",// description
	0,												// MinValue	
	240,											// MaxValue
	255,										  // NoValue
	MPF_STATE,								// Flags
	60,                       // default
};

CMachineParameter const paraSustain = 
{ 
	pt_byte,									// type
	"Sustain",
	"Sustain",// description
	0,												// MinValue	
	240,											// MaxValue
	255,										  // NoValue
	MPF_STATE,								// Flags
	120,                       // default
};

CMachineParameter const paraRelease = 
{ 
	pt_byte,									// type
	"Release",
	"Release",// description
	0,												// MinValue	
	240,											// MaxValue
	255,										  // NoValue
	MPF_STATE,								// Flags
	32,                       // default
};

CMachineParameter const paraFatness = 
{ 
	pt_byte,									// type
	"Fatness",
	"Fatness",// description
	0,												// MinValue	
	64,											// MaxValue
	255,										  // NoValue
	MPF_STATE,								// Flags
	16,                       // default
};

CMachineParameter const paraWetOut = 
{ 
	pt_byte,										// type
	"Wet out",
	"Wet out [dB]",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags
	240
};

CMachineParameter const paraPan = 
{ 
	pt_byte,										// type
	"Pan",
	"Pan Position",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags
	120
};

CMachineParameter const paraSpread = 
{ 
	pt_byte,										// type
	"Spread",
	"Stereo Spread",					// description
	0,												  // MinValue	
	100,												  // MaxValue
	255,										// NoValue
	MPF_STATE,										// Flags
	50
};

CMachineParameter const paraDummy = 
{ 
	pt_byte,										// type
	"Dummy",
	"Dummy",					// description
	0,												  // MinValue	
	240,												  // MaxValue
	255,										// NoValue
	0,										// Flags
	240
};

CMachineParameter const *pParameters[] = 
{ 
	&paraDryOut,
	&paraFeedback,
	&paraOctaviation,
	&paraRichness,
	&paraDensity,
	&paraSpaceyness,
	&paraFatness,
	&paraAttack,
	&paraSustain,
	&paraRelease,
	&paraWetOut,
	&paraPan,
	&paraSpread,
};

/*
CMachineAttribute const attrMaxDelay = 
{
	"Max Delay (ms)",
	1,
	100000,
	1000	
};

CMachineAttribute const *pAttributes[] = 
{
	&attrMaxDelay
};
*/
