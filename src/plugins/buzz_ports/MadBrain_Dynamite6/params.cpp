/**********************************************************
*                       Coarse tune                       *
***********************************************************
*          Range : 1 to ff (unsigned char) Default :80h            *
**********************************************************/
const zzub::parameter *paraCoarseTune = new zzub::parameter
  ( 
	zzub::parameter_type_byte,				// Type
	"Coarse tune",				// Name
	"Coarse tune",	// description
	1,						// value_min	
	0xff,					// value_max
	0x0,						// value_none
	zzub::parameter_flag_state,				// flags
	0x80  					// Default
  );

/**********************************************************
*                       Fine tune                         *
***********************************************************
*          Range : 1 to ff (unsigned char) Default :80h            *
**********************************************************/
const zzub::parameter *paraFineTune = new zzub::parameter
  ( 
	zzub::parameter_type_byte,				// Type
	"Fine tune",				// Name
	"Fine tune",	// description
	1,						// value_min	
	0xff,					// value_max
	0x0,						// value_none
	zzub::parameter_flag_state,				// flags
	0x80  					// Default
    );

/**********************************************************
*                     Amplification                       *
***********************************************************
*          Range : 1 to ff (unsigned char) Default :80h            *
**********************************************************/
const zzub::parameter *paraAmplification = new zzub::parameter
  ( 
	zzub::parameter_type_byte,				// Type
	"Amplification",				// Name
	"Amplification",	// description
	1,						// value_min	
	0xff,					// value_max
	0x0,						// value_none
	zzub::parameter_flag_state,				// flags
	0x20  					// Default
    );

/**********************************************************
*                      Env Attack                         *
***********************************************************
*          Range : 1 to 255 (unsigned char) Default :80h           *
**********************************************************/
const zzub::parameter *paraEnvAttack = new zzub::parameter
  ( 
	zzub::parameter_type_byte,				// Type
	"Env Attack",				// Name
	"Env Attack",	// description
	0,						// value_min	
	0xfe,					// value_max
	0xff,						// value_none
	zzub::parameter_flag_state,				// flags
	0x4  					// Default
    );

/**********************************************************
*                      Env Decay                          *
***********************************************************
*          Range : 1 to 255 (unsigned char) Default :80h           *
**********************************************************/
const zzub::parameter *paraEnvDecay = new zzub::parameter
  ( 
	zzub::parameter_type_byte,				// Type
	"Env Decay",				// Name
	"Env Decay",	// description
	1,						// value_min	
	0xff,					// value_max
	0,						// value_none
	zzub::parameter_flag_state,				// flags
	0xff  					// Default
    );


/**********************************************************
*                        Routing                          *
***********************************************************
*          Range : 1 to 255 (unsigned char) Default :80h           *
     -1-2-3-4-5-6-

          .-3-.
     -1-2-|-4-|-
          |-5-|
          '-6-'

   _,-1-¬,-3-4-5-6-
    `-2-´

          .-3-.
   _,-1-¬_|-4-|_
    `-2-´ |-5-|
          '-6-'

   _,---1-2---¬_
    `-3-4-5-6-´

     .-1-2-.
     |--3--|
    -|--4--|-
     |--5--|
     '--6--'

    .----1----.
  --|----2----|--
    '-3-4-5-6-'


    .-1-.
    |-2-|
   _|-3-|_
    |-4-|
    |-5-|
    '-6-'

**********************************************************/
const zzub::parameter *paraRouting = new zzub::parameter
  ( 
	zzub::parameter_type_byte,				// Type
	"Routing",				// Name
	"Routing",	// description
	0,						// value_min	
	0xa,					// value_max
	0xff,						// value_none
	zzub::parameter_flag_state,				// flags
	0  					// Default
    );

/**********************************************************
*                       Release                           *
***********************************************************
*          Range : 1 to 255 (unsigned char) Default :80h           *
**********************************************************/
const zzub::parameter *paraRelease = new zzub::parameter
  ( 
	zzub::parameter_type_word,				// Type
	"Release_____",				// Name
	"Release",	// description
	1,						// value_min	
	0xffff,					// value_max
	0,						// value_none
	zzub::parameter_flag_state,				// flags
	0xf000  					// Default
    );

/**********************************************************
*                    Pipe1 Length                         *
***********************************************************
*          Range : 1 to 3ff (unsigned char) Default :100h          *
**********************************************************/
const zzub::parameter *paraPipe1Length = new zzub::parameter
  ( 
	zzub::parameter_type_word,				// Type
	"Pipe1 Length",				// Name
	"Pipe1 Length",	// description
	1,						// value_min	
	0x3ff,					// value_max
	0x0,						// value_none
	zzub::parameter_flag_state,				// flags
	0xfe  					// Default
    );

/**********************************************************
*                    Pipe1 Feedback                       *
***********************************************************
*          Range : 1 to ffff (unsigned char) Default :f000h        *
**********************************************************/
const zzub::parameter *paraPipe1Feedback = new zzub::parameter
  ( 
	zzub::parameter_type_word,				// Type
	"          FBack",				// Name
	"Pipe1 Feedback",	// description
	1,						// value_min	
	0xffff,					// value_max
	0x0,						// value_none
	zzub::parameter_flag_state,				// flags
	0xf000  					// Default
    );

/**********************************************************
*                    Pipe1 Filter                         *
***********************************************************
*          Range : 1 to ffff (unsigned char) Default :f000h        *
**********************************************************/
const zzub::parameter *paraPipe1Filter = new zzub::parameter
  ( 
	zzub::parameter_type_word,				// Type
	"_____Filter___",				// Name
	"Pipe1 Filter",	// description
	1,						// value_min	
	0xffff,					// value_max
	0x0,						// value_none
	zzub::parameter_flag_state,				// flags
	0x4000  					// Default
    );

/**********************************************************
*                    Pipe2 Length                         *
***********************************************************
*          Range : 1 to 3ff (unsigned char) Default :100h          *
**********************************************************/
const zzub::parameter *paraPipe2Length = new zzub::parameter
  ( 
	zzub::parameter_type_word,				// Type
	"Pipe2 Length",				// Name
	"Pipe2 Length",	// description
	1,						// value_min	
	0x3ff,					// value_max
	0x0,						// value_none
	zzub::parameter_flag_state,				// flags
	0xff  					// Default
    );

/**********************************************************
*                    Pipe2 Feedback                       *
***********************************************************
*          Range : 1 to ffff (unsigned char) Default :f000h        *
**********************************************************/
const zzub::parameter *paraPipe2Feedback = new zzub::parameter
  ( 
	zzub::parameter_type_word,				// Type
	"          FBack",				// Name
	"Pipe2 Feedback",	// description
	1,						// value_min	
	0xffff,					// value_max
	0x0,						// value_none
	zzub::parameter_flag_state,				// flags
	0xf000  					// Default
    );

/**********************************************************
*                    Pipe2 Filter                         *
***********************************************************
*          Range : 1 to ffff (unsigned char) Default :f000h        *
**********************************************************/
const zzub::parameter *paraPipe2Filter = new zzub::parameter
  ( 
	zzub::parameter_type_word,				// Type
	"_____Filter___",				// Name
	"Pipe2 Filter",	// description
	1,						// value_min	
	0xffff,					// value_max
	0x0,						// value_none
	zzub::parameter_flag_state,				// flags
	0x4000  					// Default
    );


/**********************************************************
*                    Pipe3 Length                         *
***********************************************************
*          Range : 1 to 3ff (unsigned char) Default :100h          *
**********************************************************/
const zzub::parameter *paraPipe3Length = new zzub::parameter
  ( 
	zzub::parameter_type_word,				// Type
	"Pipe3 Length",				// Name
	"Pipe3 Length",	// description
	1,						// value_min	
	0x3ff,					// value_max
	0x0,						// value_none
	zzub::parameter_flag_state,				// flags
	0x100  					// Default
    );

/**********************************************************
*                    Pipe3 Feedback                       *
***********************************************************
*          Range : 1 to ffff (unsigned char) Default :f000h        *
**********************************************************/
const zzub::parameter *paraPipe3Feedback = new zzub::parameter
  ( 
	zzub::parameter_type_word,				// Type
	"          FBack",				// Name
	"Pipe3 Feedback",	// description
	1,						// value_min	
	0xffff,					// value_max
	0x0,						// value_none
	zzub::parameter_flag_state,				// flags
	0xf000  					// Default
    );

/**********************************************************
*                    Pipe3 Filter                         *
***********************************************************
*          Range : 1 to ffff (unsigned char) Default :f000h        *
**********************************************************/
const zzub::parameter *paraPipe3Filter = new zzub::parameter
  ( 
	zzub::parameter_type_word,				// Type
	"_____Filter___",				// Name
	"Pipe3 Filter",	// description
	1,						// value_min	
	0xffff,					// value_max
	0x0,						// value_none
	zzub::parameter_flag_state,				// flags
	0x4000  					// Default
    );

/**********************************************************
*                    Pipe4 Length                         *
***********************************************************
*          Range : 1 to 3ff (unsigned char) Default :100h          *
**********************************************************/
const zzub::parameter *paraPipe4Length = new zzub::parameter
  ( 
	zzub::parameter_type_word,				// Type
	"Pipe4 Length",				// Name
	"Pipe4 Length",	// description
	1,						// value_min	
	0x3ff,					// value_max
	0x0,						// value_none
	zzub::parameter_flag_state,				// flags
	0x101  					// Default
    );

/**********************************************************
*                    Pipe4 Feedback                       *
***********************************************************
*          Range : 1 to ffff (unsigned char) Default :f000h        *
**********************************************************/
const zzub::parameter *paraPipe4Feedback = new zzub::parameter
  ( 
	zzub::parameter_type_word,				// Type
	"          FBack",				// Name
	"Pipe4 Feedback",	// description
	1,						// value_min	
	0xffff,					// value_max
	0x0,						// value_none
	zzub::parameter_flag_state,				// flags
	0xf000  					// Default
    );

/**********************************************************
*                    Pipe4 Filter                         *
***********************************************************
*          Range : 1 to ffff (unsigned char) Default :f000h        *
**********************************************************/
const zzub::parameter *paraPipe4Filter = new zzub::parameter
  ( 
	zzub::parameter_type_word,				// Type
	"_____Filter___",				// Name
	"Pipe4 Filter",	// description
	1,						// value_min	
	0xffff,					// value_max
	0x0,						// value_none
	zzub::parameter_flag_state,				// flags
	0x4000  					// Default
    );



/**********************************************************
*                    Pipe5 Length                         *
***********************************************************
*          Range : 1 to 3ff (unsigned char) Default :100h          *
**********************************************************/
const zzub::parameter *paraPipe5Length = new zzub::parameter
  ( 
	zzub::parameter_type_word,				// Type
	"Pipe5 Length",				// Name
	"Pipe5 Length",	// description
	1,						// value_min	
	0x3ff,					// value_max
	0x0,						// value_none
	zzub::parameter_flag_state,				// flags
	0x102  					// Default
    );

/**********************************************************
*                    Pipe5 Feedback                       *
***********************************************************
*          Range : 1 to ffff (unsigned char) Default :f000h        *
**********************************************************/
const zzub::parameter *paraPipe5Feedback = new zzub::parameter
  ( 
	zzub::parameter_type_word,				// Type
	"          FBack",				// Name
	"Pipe5 Feedback",	// description
	1,						// value_min	
	0xffff,					// value_max
	0x0,						// value_none
	zzub::parameter_flag_state,				// flags
	0xf000  					// Default
    );

/**********************************************************
*                    Pipe5 Filter                         *
***********************************************************
*          Range : 1 to ffff (unsigned char) Default :f000h        *
**********************************************************/
const zzub::parameter *paraPipe5Filter = new zzub::parameter
  ( 
	zzub::parameter_type_word,				// Type
	"_____Filter___",				// Name
	"Pipe5 Filter",	// description
	1,						// value_min	
	0xffff,					// value_max
	0x0,						// value_none
	zzub::parameter_flag_state,				// flags
	0x4000  					// Default
    );



/**********************************************************
*                    Pipe6 Length                         *
***********************************************************
*          Range : 1 to 3ff (unsigned char) Default :100h          *
**********************************************************/
const zzub::parameter *paraPipe6Length = new zzub::parameter
  ( 
	zzub::parameter_type_word,				// Type
	"Pipe6 Length",				// Name
	"Pipe6 Length",	// description
	1,						// value_min	
	0x3ff,					// value_max
	0x0,						// value_none
	zzub::parameter_flag_state,				// flags
	0x100  					// Default
    );

/**********************************************************
*                    Pipe6 Feedback                       *
***********************************************************
*          Range : 1 to ffff (unsigned char) Default :f000h        *
**********************************************************/
const zzub::parameter *paraPipe6Feedback = new zzub::parameter
  ( 
	zzub::parameter_type_word,				// Type
	"          FBack",				// Name
	"Pipe6 Feedback",	// description
	1,						// value_min	
	0xffff,					// value_max
	0x0,						// value_none
	zzub::parameter_flag_state,				// flags
	0xf000  					// Default
    );

/**********************************************************
*                    Pipe6 Filter                         *
***********************************************************
*          Range : 1 to ffff (unsigned char) Default :f000h        *
**********************************************************/
const zzub::parameter *paraPipe6Filter = new zzub::parameter
  ( 
	zzub::parameter_type_word,				// Type
	"_____Filter___",				// Name
	"Pipe6 Filter",	// description
	1,						// value_min	
	0xffff,					// value_max
	0x0,						// value_none
	zzub::parameter_flag_state,				// flags
	0x4000  					// Default
    );



/**********************************************************
*                        Note                             *
***********************************************************
*                      Standard                           *
*   1/2 tones from A-4: (x/16)*12+(x%16)-70+detune-128    *
*             pitch in hz: 440*2^(fromA4/12)              *
*             Waveguide length: sr/hz                     *
**********************************************************/
const zzub::parameter *paraNote = new zzub::parameter
  ( 
	zzub::parameter_type_note,				// Type
	"Note",				// Name
	"Note",	// description
	zzub::note_value_min,						// value_min	
	zzub::note_value_max,					// value_max
	zzub::note_value_none,						// value_none
	0,				// flags
	0x80  					// Default
    );

/**********************************************************
*                     volume                              *
***********************************************************
* 0 = 0
* 80h = 100%
* FEh = ~200%
**********************************************************/
const zzub::parameter *paravolume = new zzub::parameter
  ( 
	zzub::parameter_type_byte,				// Type
	"volume",				// Name
	"volume, 80h = 100%, FEh = ~200%",		// description
	0,						// value_min	
	0xfe,					// value_max
	0xff,						// value_none
	zzub::parameter_flag_state,				// flags
	0x80  					// Default
    );
