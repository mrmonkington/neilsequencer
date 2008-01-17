CMachineParameter const paraLeftLength =
{
            pt_word,                        // Parameter data type
            "Left Length",                        // Parameter name as its shown in the parameter
                                                // window
            "L-Length",            // Parameter description as its shown in
                                                //the pattern view's statusbar
            1,                                    // Minimum value
            0xFFFE,                        // Maximum value
            0xFFFF,                        // Novalue, this value means "nothing
                                                // happened" in the mi::Tick procedure
            MPF_STATE,                        // Parameter options, MPF_STATE makes it 
                                                // appears as a slider
            0x10                        // the default slider value
};

CMachineParameter const paraLeftOffset =
{
            pt_word,                        // Parameter data type
            "Left Offset",                        // Parameter name as its shown in the parameter
                                                // window
            "L-Offset",            // Parameter description as its shown in
                                                //the pattern view's statusbar
            1,                                    // Minimum value
            0xFFFE,                        // Maximum value
            0xFFFF,                        // Novalue, this value means "nothing
                                                // happened" in the mi::Tick procedure
            MPF_STATE,                        // Parameter options, MPF_STATE makes it 
                                                // appears as a slider
            0x10                        // the default slider value
};

CMachineParameter const paraRightLength =
{
            pt_word,                        // Parameter data type
            "Right Length",                        // Parameter name as its shown in the parameter
                                                // window
            "R-Length",            // Parameter description as its shown in
                                                //the pattern view's statusbar
            1,                                    // Minimum value
            0xFFFE,                        // Maximum value
            0xFFFF,                        // Novalue, this value means "nothing
                                                // happened" in the mi::Tick procedure
            MPF_STATE,                        // Parameter options, MPF_STATE makes it 
                                                // appears as a slider
            0x10                        // the default slider value
};

CMachineParameter const paraRightOffset =
{
            pt_word,                        // Parameter data type
            "Right Offset",                        // Parameter name as its shown in the parameter
                                                // window
            "R-Offset",            // Parameter description as its shown in
                                                //the pattern view's statusbar
            1,                                    // Minimum value
            0xFFFE,                        // Maximum value
            0xFFFF,                        // Novalue, this value means "nothing
                                                // happened" in the mi::Tick procedure
            MPF_STATE,                        // Parameter options, MPF_STATE makes it 
                                                // appears as a slider
            0x10                        // the default slider value
};

CMachineParameter const paraSlaveLengths =
{
            pt_byte,                        // Parameter data type
            "Slave Lengths",                        // Parameter name as its shown in the parameter
                                                // window
            "SlaveLengths",            // Parameter description as its shown in
                                                //the pattern view's statusbar
            0,                                    // Minimum value
            1,                        // Maximum value
            0xFF,                        // Novalue, this value means "nothing
                                                // happened" in the mi::Tick procedure
            MPF_STATE,                        // Parameter options, MPF_STATE makes it 
                                                // appears as a slider
            0                        // the default slider value
};

CMachineParameter const paraSlaveOffsets =
{
            pt_byte,                        // Parameter data type
            "Slave Offsets",                        // Parameter name as its shown in the parameter
                                                // window
            "SlaveOffsets",            // Parameter description as its shown in
                                                //the pattern view's statusbar
            0,                                    // Minimum value
            1,                        // Maximum value
            0xFF,                        // Novalue, this value means "nothing
                                                // happened" in the mi::Tick procedure
            MPF_STATE,                        // Parameter options, MPF_STATE makes it 
                                                // appears as a slider
            0                        // the default slider value
};

CMachineParameter const paraDirection =
{
            pt_byte,                        // Parameter data type
            "Direction",                        // Parameter name as its shown in the parameter
                                                // window
            "Direction",            // Parameter description as its shown in
                                                //the pattern view's statusbar
            0,                                    // Minimum value
            2,                        // Maximum value
            0xFF,                        // Novalue, this value means "nothing
                                                // happened" in the mi::Tick procedure
            MPF_STATE,                        // Parameter options, MPF_STATE makes it 
                                                // appears as a slider
            0                        // the default slider value
};


CMachineParameter const paraMix =
{
            pt_word,                        // Parameter data type
            "Mix",                        // Parameter name as its shown in the parameter
                                                // window
            "Mix",            // Parameter description as its shown in
                                                //the pattern view's statusbar
            0,                                    // Minimum value
            0xFFFE,                        // Maximum value
            0xFFFF,                        // Novalue, this value means "nothing
                                                // happened" in the mi::Tick procedure
            MPF_STATE,                        // Parameter options, MPF_STATE makes it 
                                                // appears as a slider
            0xFFFE                        // the default slider value
};



CMachineParameter const paraResetBuffer =
{
            pt_byte,                        // Parameter data type
            "ResetBuffer",                        // Parameter name as its shown in the parameter
                                                // window
            "ResetBuffer",            // Parameter description as its shown in
                                                //the pattern view's statusbar
            0,                                    // Minimum value
            1,                        // Maximum value
            0xFF,                        // Novalue, this value means "nothing
                                                // happened" in the mi::Tick procedure
            0,                        // Parameter options, MPF_STATE makes it 
                                                // appears as a slider
            0                        // the default slider value
};






  // I don't know what's going on here: had to add these functions
  // without the MDK-prefix which just call the versions with the
  // prefix.

  virtual void Init(CMachineDataInput * const pi) {
    MDKInit(pi);
  }
  virtual bool Work(float *psamples, int numSamples, int const mode) {
    return MDKWork(psamples, numSamples, mode);
  }
  virtual bool WorkStereo(float *psamples, int numSamples, int const mode) {
    return MDKWorkStereo(psamples, numSamples, mode);
  }
  virtual void Save(CMachineDataOutput * const po) {
    MDKSave(po);
  }



