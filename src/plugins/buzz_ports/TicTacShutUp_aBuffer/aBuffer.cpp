#include <zzub/signature.h>
#include <zzub/plugin.h>

#include "mdk.h"

//#include <windows.h>
#pragma optimize ("awy", on) 

class TicTacShutUp_abuffer;

const zzub::parameter *paraLeftLength = 0;
const zzub::parameter *paraLeftOffset = 0;
const zzub::parameter *paraRightLength = 0;
const zzub::parameter *paraRightOffset = 0;
const zzub::parameter *paraSlaveLengths = 0;
const zzub::parameter *paraSlaveOffsets = 0;
const zzub::parameter *paraDirection = 0;
const zzub::parameter *paraMix = 0;
const zzub::parameter *paraResetBuffer = 0;


// These are for interfacing with the Buzz host: unused by the zzub host.
CMachineParameter const *pParameters[] = {NULL};
CMachineAttribute const *pAttributes[] = {NULL};

#pragma pack(1)                        

class gvals
{
public:
			word leftLength;
			word leftOffset;
			word rightLength;
			word rightOffset;
			byte slaveLengths;
			byte slaveOffsets;
			byte direction;
			word mix;
			byte resetBuffer;
};

#pragma pack()

CMachineInfo const MacInfo = 
{
            MT_EFFECT,                        // Machine type
            MI_VERSION,                        // Machine interface version
            MIF_DOES_INPUT_MIXING,            // Machine flags
            0,                                    // min tracks
            0,                                    // max tracks
            9,                                    // numGlobalParameters
            0,                                    // numTrackParameters
            pParameters,            // pointer to parameter stuff
            0,                                    // numAttributes
            pAttributes,            // pointer to attribute stuff
            "Tic-Tac aBuffer",            // Full Machine Name
            "aBuffer",                        // Short name
            "Tic-Tac",            // Author name
            "&Glitch..."                        // Right click menu commands
};


//class miex : public CMDKMachineInterfaceEx { };

class mi : public CMachineInterface
{
public:
  mi();
  virtual ~mi();
  virtual void Tick();
  virtual void Init(CMachineDataInput * const pi);
  virtual bool Work(float *psamples, int numSamples, int const mode);
  virtual bool WorkStereo(float *psamples, int numSamples, int const mode);
  virtual void Command(int const i);
  virtual void Save(CMachineDataOutput * const po);
  virtual char const *DescribeValue(int const param, int const value);
  //virtual CMachineInterfaceEx *GetEx() { return &ex; }
  virtual void OutputModeChanged(bool stereo) {}


public:
  //miex ex;

public:
            //float dryOutAmt;
			float *pLeftBuffer; // left buffer
			float *pRightBuffer; // right buffer

			int leftBufferLength,rightBufferLength; // left and right buffer lengths

			int leftReadHead,rightReadHead; // left and right read head counter

			int leftWriteHead,rightWriteHead; // left and right write head counter

			int leftOffset,rightOffset; // left and right offset

			int leftLength,rightLength; // left and right lengths of the played loop

			int direction;

			bool leftReadDirection;
			bool rightReadDirection;
	
			int slaveLengths;
			int slaveOffsets;

			float mix; // dry/wet mix
			
			int resetBuffer;

            gvals gval;

			/* ???
			delete pbufl;
			delete pbufr;
			pbufl = new float[blenl];
			pbufr = new float[blenr];
			for(int i=0;i<blenl;i++)pbufl[i]=0;
			for(i=0;i<blenr;i++)pbufr[i]=0;
			rcl=rcr=wcl=wcr=0;
			*/
  TicTacShutUp_abuffer *pz;
};

mi::mi() { }
mi::~mi() 
{ 
	delete pLeftBuffer;
	delete pRightBuffer;
}

void mi::Init(CMachineDataInput * const pi)
{
  //g_samplerate = pz->_master_info->samples_per_second; FIXME: replace 441000 with samplerate * 10.

  //SetOutputMode(true); // No mono sounds
	    
			pLeftBuffer = new float[441000]; // left buffer
			pRightBuffer = new float[441000]; // right buffer

			leftBufferLength = rightBufferLength = 441000; // left and right buffer lengths

			leftReadHead = rightReadHead = 0; // left and right read head counter

			leftWriteHead = rightWriteHead = 0; // left and right write head counter

			leftOffset = rightOffset = 0; // left and right offset

			leftLength = rightLength = 0; // left and right lengths of the played loop

			direction = 0;

			leftReadDirection = false;
			rightReadDirection = false;

			slaveLengths = 0;

			mix = 0; // dry/wet mix

			resetBuffer = 0;
			
}

void mi::Save(CMachineDataOutput * const po) { }

void mi::Tick() 
{
	//"if"s for checking if the values are slaved, if so, left controls both
	if(gval.slaveLengths != 0xFF) slaveLengths = (int)gval.slaveLengths;

	if(gval.slaveOffsets!= 0xFF) slaveOffsets = (int)gval.slaveOffsets;

	if(slaveLengths == 0)
	{
		if (gval.leftLength != 0xFFFF) leftLength = (int)gval.leftLength;
		if (gval.rightLength != 0xFFFF) rightLength = (int)gval.rightLength;
	}
	if(slaveLengths == 1)
	{
		if (gval.leftLength != 0xFFFF) leftLength = (int)gval.leftLength;
		if (gval.leftLength != 0xFFFF) rightLength = (int)gval.leftLength;
	}


	if(slaveOffsets == 0)
	{
		if (gval.leftOffset != 0xFFFF) leftOffset = (int)gval.leftOffset;
		if (gval.rightOffset != 0xFFFF) rightOffset = (int)gval.rightOffset;
	}
		
	if(slaveOffsets == 1)
	{
		if (gval.leftOffset != 0xFFFF) leftOffset = (int)gval.leftOffset;
		if (gval.leftOffset != 0xFFFF) rightOffset = (int)gval.leftOffset;
	}

	if (gval.direction != 0xFF) direction = (int)gval.direction;
	if (gval.mix != 0xFFFF) mix = (float)gval.mix / 65534;	

	if (gval.resetBuffer != 0xFF) resetBuffer = (int)gval.resetBuffer;

}

bool mi::Work(float *psamples, int numSamples, int const mode)
{
            return false;
}

bool mi::WorkStereo(float *psamples, int numSamples, const int mode)
{
    float inL,inR,outL,outR;
    int i;


    if(mode == WM_NOIO) return false;

	//check if resetBuffer has been triggered
	if(resetBuffer == 1)
	{
		leftWriteHead = 0;
		rightWriteHead = 0;
	}
	//make sure it gets set back to zero so it doesn't trigger every time
	resetBuffer = 0;

	for(i=0;i<numSamples;i++)
	{
		if(mode&WM_READ)
		{
		    inL = psamples[2 * i];
		    inR = psamples[2 * i + 1];
		}
		else inL = inR=0;
			pLeftBuffer[leftWriteHead] = inL;
			pRightBuffer[rightWriteHead] = inR;

				
		if(mode&WM_WRITE)
		{					  
			  outL = pLeftBuffer[(leftOffset + leftReadHead) % leftBufferLength];
			  outR = pRightBuffer[(rightOffset + rightReadHead) % rightBufferLength];
			  psamples[2 * i] = mix * outL + (1 - mix) * inL;
			  psamples[2 * i + 1] = mix * outR + (1 - mix) * inR;
		}

		leftWriteHead++;

		if(leftWriteHead >= leftBufferLength)
			leftWriteHead=0;

		rightWriteHead++;

		if(rightWriteHead >= rightBufferLength)
			rightWriteHead=0;

		if(direction == 0)
		{
			leftReadHead++;
					
			if(leftReadHead >= leftLength)
				leftReadHead = 0;

			rightReadHead++;
					
			if(rightReadHead >= rightLength)
				rightReadHead = 0;
		}
		if(direction == 1)
		{	
			leftReadHead--;
					
			if(leftReadHead <= leftOffset)
				leftReadHead = leftOffset + leftLength;

			rightReadHead--;
					
			if(rightReadHead <= rightOffset)
				rightReadHead = rightOffset + rightLength;
		}
		//ping pong
		if(direction == 2)
		{
			//if it hasn't already reached the end of ping:
			if(leftReadDirection == false)
			{
				leftReadHead++;
						
				if(leftReadHead >= leftLength)
				{	
					leftReadDirection = true;
				}
			}
			if(rightReadDirection == false)
			{
				rightReadHead++;
						
				if(rightReadHead >= rightLength)
				{
					rightReadDirection = true;
				}
					
			}
			//if it has already reached the end of ping:
			if(leftReadDirection == true)
			{
				leftReadHead--;

				if(leftReadHead <= 0)
				{	
					leftReadHead = 0;
					leftReadDirection = false;
				}
			}

			if(rightReadDirection == true)
			{
				rightReadHead--;

				if(rightReadHead <= 0)
				{	
					rightReadHead = 0;
					rightReadDirection = false;
				}
			}
		}//end of if(direction == 2) for
	}//end of numsaples for



	return true;
}


void mi::Command(int const i)
{
            switch (i)
            {
            case 0:
	      //MessageBox(NULL,"Tic-Tac Shut Up aBuffer v1.0->gayfarmer@hotmail.com","aBuffer == sweet;",MB_OK|MB_SYSTEMMODAL);
                        break;
            default:
                        break;
            }
}
char const *mi::DescribeValue(int const param, int const value)
{
            static char txt[16];
            switch(param)
            {
            case 0:
            case 1:
            case 2:
                        sprintf(txt,"%.1f", (float)value);
                        return txt;
                        break;
            case 3:
                        sprintf(txt,"%.1f", (float)value);
                        return txt;
                        break;			
			case 4:		
						if(value == 0)
						{
							sprintf(txt,"Off",(float)value);
						}
						
						if(value == 1)
						{
							sprintf(txt,"On",(float)value);
						}
							
						return txt;
						break;
						
			case 5:
						if(value == 0)
						{
							sprintf(txt,"Off",(float)value);
							return txt;
							break;
						}
						
						if(value == 1)
						{
							sprintf(txt,"On",(float)value);
							return txt;
							break;
						}
						
			case 6:		
						if(value == 0)
						{
							sprintf(txt,"Forward",(float)value);
							return txt;
							break;
						}
						
						if(value == 1)
						{
							sprintf(txt,"Backward",(float)value);
							return txt;
							break;
						}
						

						if(value == 2)
						{
							sprintf(txt,"Ping Pong",(float)value);
							return txt;
							break;
						}
						
			case 7:
						sprintf(txt,"%.1f%% Wet",(float)value/655.34f);
                        return txt;
                        break;				
            default:
                        return NULL;
            }
}

#pragma optimize ("", on) 

//DLL_EXPORTS



class TicTacShutUp_abuffer: public zzub::plugin
{
public:
  TicTacShutUp_abuffer();
  virtual ~TicTacShutUp_abuffer();
  virtual void process_events();
  virtual void init(zzub::archive *);
  virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
  virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
  virtual void command(int i) {}
  virtual void load(zzub::archive *arc) {}
  virtual void save(zzub::archive *) { }
  virtual const char * describe_value(int param, int value);
  virtual void OutputModeChanged(bool stereo) { }
  
  // ::zzub::plugin methods
  virtual void process_controller_events() {}
  virtual void destroy();
  virtual void stop();
  virtual void attributes_changed();
  virtual void set_track_count(int);
  virtual void mute_track(int) {}
  virtual bool is_track_muted(int) const { return false; }
  virtual void midi_note(int, int, int) {}
  virtual void event(unsigned int) {}
  virtual const zzub::envelope_info** get_envelope_infos() { return 0; }
  virtual bool play_wave(int, int, float) { return false; }
  virtual void stop_wave() {}
  virtual int get_wave_envelope_play_position(int) { return -1; }
  virtual const char* describe_param(int) { return 0; }
  virtual bool set_instrument(const char*) { return false; }
  virtual void get_sub_menu(int, zzub::outstream*) {}
  virtual void add_input(const char*, zzub::connection_type) {}
  virtual void delete_input(const char*, zzub::connection_type) {}
  virtual void rename_input(const char*, const char*) {}
  virtual void input(float**, int, float) {}
  virtual void midi_control_change(int, int, int) {}
  virtual bool handle_input(int, int, int) { return false; }
  virtual void process_midi_events(zzub::midi_message* pin, int nummessages) {}
  virtual void get_midi_output_names(zzub::outstream *pout) {}
  virtual void set_stream_source(const char* resource) {}
  virtual const char* get_stream_source() { return 0; }
  
public:
  
  gvals gval;
private:
  mi *abuffer_cmachine;
};



TicTacShutUp_abuffer::TicTacShutUp_abuffer() {
  abuffer_cmachine = new mi;
  abuffer_cmachine->pz = this;


  global_values = &abuffer_cmachine->gval;
}
TicTacShutUp_abuffer::~TicTacShutUp_abuffer() {
}
void TicTacShutUp_abuffer::process_events() {
  abuffer_cmachine->Tick();
}
void TicTacShutUp_abuffer::init(zzub::archive *arc) {
  // The pointer passed in to this function is unused, so pass 0.
  abuffer_cmachine->Init(0);
}
bool TicTacShutUp_abuffer::process_stereo(float **pin, float **pout, int numsamples, int mode) {

  if (mode == zzub::process_mode_write || mode == zzub::process_mode_no_io)
    return false;
  if (mode == zzub::process_mode_read) // _thru_
    return true;

  // The MDKWorkStereo() function overwrites its input with its
  // output. Right? We don't do that in zzub, so copy the input data,
  // pass in the copy, then copy the overwritten copy to the output?
  
  // Also, MDKWorkStereo() expects *interleaved* channels. 

  // I hope 20000 is bigger than any buffer ever will be! FIXME. Can't
  // allocate in this real-time function.
  float tmp[20000];
  // Copy and interleave.
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < numsamples; j++) {
      tmp[2 * j + i] = pin[i][j];
    }
  }
  bool retval = abuffer_cmachine->WorkStereo(tmp, numsamples, mode);
  // Copy and un-interleave. 
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < numsamples; j++) {
      pout[i][j] = tmp[2 * j + i];
    }
  }
  return retval;
  
}

const char * TicTacShutUp_abuffer::describe_value(int param, int value) {
  return abuffer_cmachine->DescribeValue(param, value);
}


void TicTacShutUp_abuffer::set_track_count(int n) {
  abuffer_cmachine->SetNumTracks(n);
}
void TicTacShutUp_abuffer::stop() {
  abuffer_cmachine->Stop();
}

void TicTacShutUp_abuffer::destroy() { 
  delete abuffer_cmachine;
  delete this; 
}

void TicTacShutUp_abuffer::attributes_changed() {
  abuffer_cmachine->AttributesChanged();
}


const char *zzub_get_signature() { return ZZUB_SIGNATURE; }


struct TicTacShutUp_abuffer_plugin_info : zzub::info {
  TicTacShutUp_abuffer_plugin_info() {
    this->flags = zzub::plugin_flag_has_audio_input | zzub::plugin_flag_has_audio_output;
    this->min_tracks = 0;
    this->max_tracks = 0;
    this->name = "Tic-Tac Shut Up aBuffer";
    this->short_name = "aBuffer";
    this->author = "Tic-Tac Shut Up (ported by jmmcd <jamesmichaelmcdermott@gmail.com>)";
    this->uri = "jamesmichaelmcdermott@gmail.com/effect/abuffer;1";
    
    paraLeftLength = &add_global_parameter()
      .set_word()
      .set_name("Left Length")
      .set_description("L-Length")
      .set_value_min(1)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x10);
    paraLeftOffset = &add_global_parameter()
      .set_word()
      .set_name("Left Offset")
      .set_description("L-Offset")
      .set_value_min(1)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x10);
    paraRightLength = &add_global_parameter()
      .set_word()
      .set_name("Right Length")
      .set_description("R-Length")
      .set_value_min(1)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x10);
    paraRightOffset = &add_global_parameter()
      .set_word()
      .set_name("Right Offset")
      .set_description("R-Offset")
      .set_value_min(1)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0x10);
    paraSlaveLengths = &add_global_parameter()
      .set_byte()
      .set_name("Slave Lengths")
      .set_description("SlaveLengths")
      .set_value_min(0)
      .set_value_max(1)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraSlaveOffsets = &add_global_parameter()
      .set_byte()
      .set_name("Slave Offsets")
      .set_description("SlaveOffsets")
      .set_value_min(0)
      .set_value_max(1)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraDirection = &add_global_parameter()
      .set_byte()
      .set_name("Direction")
      .set_description("Direction")
      .set_value_min(0)
      .set_value_max(2)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraMix = &add_global_parameter()
      .set_word()
      .set_name("Mix")
      .set_description("Mix")
      .set_value_min(0)
      .set_value_max(0xFFFE)
      .set_value_none(0xFFFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0xFFFE);
    paraResetBuffer = &add_global_parameter()
      .set_byte()
      .set_name("ResetBuffer")
      .set_description("ResetBuffer")
      .set_value_min(0)
      .set_value_max(1)
      .set_value_none(0xFF)
      .set_flags(0)
      .set_value_default(0);


  } 
  virtual zzub::plugin* create_plugin() const { return new TicTacShutUp_abuffer(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} TicTacShutUp_abuffer_info;

struct abufferplugincollection : zzub::plugincollection {
  virtual void initialize(zzub::pluginfactory *factory) {
    factory->register_info(&TicTacShutUp_abuffer_info);
  }
  
  virtual const zzub::info *get_info(const char *uri, zzub::archive *data) { return 0; }
  virtual void destroy() { delete this; }
  // Returns the uri of the collection to be identified,
  // return zero for no uri. Collections without uri can not be 
  // configured.
  virtual const char *get_uri() { return 0; }
  
  // Called by the host to set specific configuration options,
  // usually related to paths.
  virtual void configure(const char *key, const char *value) {}
};

zzub::plugincollection *zzub_get_plugincollection() {
  return new abufferplugincollection();
}
  
