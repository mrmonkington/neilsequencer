/*
 *		Plucked String plug-in for Buzz
 *
 *			Written by George Nicolaidis aka Geonik
 */

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#include <zzub/signature.h>
#include <zzub/plugin.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <float.h>

typedef unsigned long  ulong;
typedef unsigned short uword;
typedef unsigned char  ubyte;
typedef unsigned char byte;

double const PI = 3.14159265358979323846;

#include "../DspClasses/DspClasses.h"
#include "../DspClasses/Delay.h"
#include "../Common/About.h"
//#include "../Common/Shared.h"

#define c_strName			"Plucked String"
#define c_strShortName		"Pluck String"

#define MaxTracks		16
#define MaxDynTracks	64
#define BufferSize		6144
#define MaxAmp			32768

int				dspcSampleRate;


/*
 *		Declarations
 */

class geonik_pluckedstring;

//void	DspAdd(float *pout, float const *pin, dword const n);


/*
 *		Tables
 */

static double const NoteFreqs[12] = { 130.8127, 138.5913, 146.8324, 155.5635, 164.8138, 174.6141, 184.9972, 195.9977, 207.6523, 220, 233.0819, 246.9417 };
static double const OctaveMul[10] = { 1.0 / 16, 1.0 / 8, 1.0 / 4, 1.0 / 2, 1.0, 1.0 * 2, 1.0 * 4, 1.0 * 8, 1.0 * 16, 1.0 * 32 };


/*
 *		Parameters
 */
const zzub::parameter *mpNote = 0;
const zzub::parameter *mpVolume = 0;
const zzub::parameter *mpSlide = 0;
const zzub::parameter *mpDamper = 0;

enum					 mpValues	  { mpvNote,mpvVolume,mpvSlide,mpvDamper };

#pragma pack(1)

struct GlobalParameters { };
struct TrackParameters {
    byte Note;
    byte Volume;
    byte Slide;
    byte Damper;
};

#pragma pack()


/*
 *		Attributes
 */
const zzub::attribute *maMaxDyn	= 0;
const zzub::attribute *maDefVol	= 0;
const zzub::attribute *maDynRange	= 0;

struct Attributes {
    int	 DynRange;
    int	 DefVol;
    int	 MaxDyn;
};

#pragma pack()


/*
 *		Track class
 */

struct CTrack {
    void		 Tick(int);
    void		 Reset();
    void		 Init();
    void		 Free();
    bool		 CheckIfPlaying();
    void		 Work(float *psamples, int numsamples);
    void		 WorkAdd(float *psamples, int numsamples);
    void		 Stop();
    void		 NoteOn(byte,bool);

    geonik_pluckedstring	*pMachine;
    CTrack		*LastTrack;

    CDelay		 psWave;

    bool		 Playing;
    double		 Amplitude;
    double		 Dampening;
    double		 LastSample;
    double		 SignalSum;
    double		 RmsQ;

};



class geonik_pluckedstring: public zzub::plugin
{
public:
    geonik_pluckedstring();
    virtual ~geonik_pluckedstring();
    virtual void process_events();
    virtual void init(zzub::archive *);
    virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
    virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) {
        return false;
    }
    virtual void command(int i);
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
    virtual bool is_track_muted(int) const {
        return false;
    }
    virtual void midi_note(int, int, int) {}
    virtual void event(unsigned int) {}
    virtual const zzub::envelope_info** get_envelope_infos() {
        return 0;
    }
    virtual bool play_wave(int, int, float) {
        return false;
    }
    virtual void stop_wave() {}
    virtual int get_wave_envelope_play_position(int) {
        return -1;
    }
    virtual const char* describe_param(int) {
        return 0;
    }
    virtual bool set_instrument(const char*) {
        return false;
    }
    virtual void get_sub_menu(int, zzub::outstream*) {}
    virtual void add_input(const char*) {}
    virtual void delete_input(const char*) {}
    virtual void rename_input(const char*, const char*) {}
    virtual void input(float**, int, float) {}
    virtual void midi_control_change(int, int, int) {}
    virtual bool handle_input(int, int, int) {
        return false;
    }

public:

    CTrack				 Track[MaxDynTracks];
    int					 numTracks;
    int					 numDynTracks;

    double				 SilentEnough;
    double				 RmsC1;
    double				 RmsC2;

    GlobalParameters	 Param;
    TrackParameters		 TrackParam[MaxDynTracks];
    Attributes			 Attr;

};



/*
 *		Misc
 */

inline double frand()
{
    static long stat = 0x16BA2118;
    stat = stat * 1103515245 + 12345;
    return (double)stat * (1.0 / 0x80000000);
}

geonik_pluckedstring::geonik_pluckedstring()
{
    global_values   = &Param;
    track_values    = TrackParam;
    attributes = (int *)&Attr;
}


geonik_pluckedstring::~geonik_pluckedstring()
{
    for (int c = 0; c < MaxDynTracks; c++) {
        Track[c].Free();
    }
}

void geonik_pluckedstring::destroy()
{
    delete this;
}

void geonik_pluckedstring::init(zzub::archive *arc)
{
    dspcSampleRate	= _master_info->samples_per_second;
    numTracks = 0;
    numDynTracks = 0;
    for (int c = 0; c < MaxDynTracks; c++)	{
        Track[c].pMachine = this;
    }
    double b	= 2.0 - cos(10 * 2 * PI / (double)_master_info->samples_per_tick);
    RmsC2		= b - sqrt(b * b - 1.0);
    RmsC1		= 1.0 - RmsC2;
}


void geonik_pluckedstring::set_track_count(int const n)
{
    if(numDynTracks < n) {
        for(int c = numDynTracks; c < n; c++)
            Track[c].Init();
    }
    numTracks = n;
    numDynTracks = __max(numTracks,numDynTracks);
}


char const *geonik_pluckedstring::describe_value(int const ParamNum, int const Value)
{
    static char TxtBuffer[16];
    switch(ParamNum) {
    case mpvVolume:
        sprintf(TxtBuffer, "%.1f%%", (double)Value * (100.0 / 128.0));
        break;
    case mpvDamper:
        sprintf(TxtBuffer, "%.5f%", 1.0 - ((double)Value/256.0)*((double)Value/256.0));
        break;
    default:
        return NULL;
    }
    return TxtBuffer;
}


void geonik_pluckedstring::process_events()
{
    int c;
    for(c = 0; c < numDynTracks; c++) {
        Track[c].CheckIfPlaying();
    }

    for(c = 0; c < numTracks; c++)
        Track[c].Tick(c);
}


void geonik_pluckedstring::attributes_changed()
{

    if(numDynTracks > Attr.MaxDyn) {
        for(int i=Attr.MaxDyn; i<numDynTracks; i++) {
            Track[i].Reset();
        }
        numDynTracks = __max(numTracks,Attr.MaxDyn);
    }

    SilentEnough = pow(2.0,-(double)Attr.DynRange/3.0);
}


bool geonik_pluckedstring::process_stereo(float **pin, float **pout, int numsamples, int mode)
{

    if (mode != zzub::process_mode_write)
        return false;

    bool GotSomething = false;

    for (int c = 0; c < numDynTracks; c++)	{
        if(Track[c].Playing) {
            if(!GotSomething) {
                Track[c].Work(pout[0],numsamples);
                GotSomething = true;
            } else {
                Track[c].WorkAdd(pout[0],numsamples);
            }
        }
    }

    for (int i = 0; i < numsamples; i++) {
        pout[1][i] = pout[0][i];
    }
    return GotSomething;
}


void geonik_pluckedstring::stop()
{
    for (int c = 0; c < numDynTracks; c++) Track[c].Stop();
}


void geonik_pluckedstring::command(int const i)
{
    switch(i) {
    case 0:
        About();
        break;
    }
}





/*
 *		Track members		-	-	-	-	-	-	-	-	-	-
 */

void CTrack::Reset()
{
    Playing		= false;
    LastSample	= 0;
    Amplitude	= MaxAmp;
    Dampening	= 0.995/2.0;
    RmsQ		= 0;
    LastTrack	= this;
}


void CTrack::Init()
{
    psWave.Alloc(BufferSize);
    psWave.Clear();
    Reset();
}


void CTrack::Free()
{
    Playing		= false;
    RmsQ		= 0;
}


bool CTrack::CheckIfPlaying()
{
    double const c1 = pMachine->RmsC1;
    double const c2 = pMachine->RmsC2;
    double		 q  = RmsQ;
    float		*inb= psWave.pBuffer;

    if(Playing) {
        int ns = psWave.iLength;
        do {
            double v = *inb++;
            q = c1 * v * v + c2 * q;
        } while(--ns);
        RmsQ = q;
        if(q < pMachine->SilentEnough) {
            Playing = false;
            RmsQ = 0;
        }
    }
    return Playing;
}


void CTrack::NoteOn(byte Note, bool Slide)
{
    int		note	= (Note & 15) - 1;
    int		oct		= Note >> 4;

    double	freq	= NoteFreqs[note] * OctaveMul[oct];

    psWave.SetDelay((ulong)floor((double)pMachine->_master_info->samples_per_second / freq));

    LastSample		= 0;
    Amplitude		= (double)pMachine->Attr.DefVol * (MaxAmp / 128.0);
    Playing			= true;
    RmsQ			= MaxAmp^2;

    if(!Slide) {
        int i;
        for(i=0; i<psWave.iLength; i++) {
            psWave.pBuffer[i] = (float)frand();
        }
        LastSample = psWave.pBuffer[i-1];
    }
}


void CTrack::Tick(int ThisTrack)
{

    TrackParameters &tp = pMachine->TrackParam[ThisTrack];

    if(tp.Note == zzub::note_value_off) {
        LastTrack->Amplitude /= 2;
    }

    else if(tp.Note != zzub::note_value_none) {
        if(tp.Slide == zzub::switch_value_on) LastTrack->NoteOn(tp.Note,true);
        else {
            double m = MaxAmp^20;
            int t;
            for(int c=0; c < __max(pMachine->numTracks,pMachine->Attr.MaxDyn); c++) {
                if(c <  pMachine->numTracks && c != ThisTrack) continue;
                if(c >= pMachine->numDynTracks) {
                    pMachine->Track[c].Init();
                    pMachine->numDynTracks++;
                }
                if(pMachine->Track[c].RmsQ < m) {
                    m = pMachine->Track[c].RmsQ;
                    t = c;
                }
                if(m < pMachine->SilentEnough) break;
            }
            pMachine->Track[t].NoteOn(tp.Note,false);
            pMachine->Track[t].Dampening = pMachine->Track[ThisTrack].Dampening;
            LastTrack = &(pMachine->Track[t]);
        }
    }

    if(tp.Damper != mpDamper->value_none) {
        double a = ((double)tp.Damper/256.0);
        a = (1.0 - a*a)/2.0;
        pMachine->Track[ThisTrack].Dampening = a;
        LastTrack->Dampening = a;
    }

    if(tp.Volume != mpVolume->value_none) {
        LastTrack->Amplitude = (double)(tp.Volume * (MaxAmp / 128));
    }
}


void CTrack::Stop() { }


/*
 *		Worker functions
 */

void CTrack::Work(float *Dest, int numsamples)
{

    float downscale = 1.0f/32768.0f;
    double const d=Dampening;
    double const a=Amplitude;

    float *dp = psWave.pBuffer + psWave.iPos;
    double lv = LastSample;

    while(numsamples > 0) {
        int c = __min(numsamples,(psWave.pBuffer + psWave.iLength) - dp);
        numsamples -= c;
        do {
            double v = *dp;
            *dp++ = (float)(d * (v + lv));
            lv = v;
            *Dest++ = (float)(downscale * v * a);
        } while(--c);
        if(dp == psWave.pBuffer + psWave.iLength) dp = psWave.pBuffer;
    }
    psWave.iPos = dp - psWave.pBuffer;
    LastSample = lv;
}


void CTrack::WorkAdd(float *Dest, int numsamples)
{

    float downscale = 1.0f/32768.0f;
    double const d=Dampening;
    double const a=Amplitude;

    float *dp = psWave.pBuffer + psWave.iPos;
    double lv = LastSample;

    while(numsamples > 0) {
        int c = __min(numsamples,(psWave.pBuffer + psWave.iLength) - dp);
        numsamples -= c;
        do {
            double v = *dp;
            *dp++ = (float)(d * (v + lv));
            lv = v;
            *Dest++ += (float)(downscale * v * a);
        } while(--c);
        if(dp == psWave.pBuffer + psWave.iLength) dp = psWave.pBuffer;
    }
    psWave.iPos = dp - psWave.pBuffer;
    LastSample = lv;
}






const char *zzub_get_signature()
{
    return ZZUB_SIGNATURE;
}


struct geonik_pluckedstring_plugin_info : zzub::info {
    geonik_pluckedstring_plugin_info() {
        this->flags = zzub::plugin_flag_has_audio_output;
        this->min_tracks = 1;
        this->max_tracks = MaxTracks;
        this->name = "Geonik PluckedString";
        this->short_name = "PluckedString";
        this->author = "Geonik";
        this->uri = "jamesmichaelmcdermott@gmail.com/generator/pluckedstring;1";

        mpNote = &add_track_parameter()
                 .set_note()
                 .set_name("Note")
                 .set_description("Note")
                 .set_value_min(zzub::note_value_min)
                 .set_value_max(zzub::note_value_max)
                 .set_value_none(zzub::note_value_none)
                 .set_flags(zzub::parameter_flag_event_on_edit)
                 .set_value_default(0);
        mpVolume = &add_track_parameter()
                   .set_byte()
                   .set_name("Volume")
                   .set_description("Volume (0=0%, 80=100%, FE=198%)")
                   .set_value_min(0)
                   .set_value_max(0xFE)
                   .set_value_none(0xFF)
                   .set_flags(0)
                   .set_value_default(80);
        mpSlide = &add_track_parameter()
                  .set_switch()
                  .set_name("Slide")
                  .set_description("Slide to note")
                  .set_value_min(-1)
                  .set_value_max(-1)
                  .set_value_none(zzub::switch_value_none)
                  .set_flags(0)
                  .set_value_default(zzub::switch_value_off);
        mpDamper = &add_track_parameter()
                   .set_byte()
                   .set_name("Damper")
                   .set_description("Dampening factor (Default=18)")
                   .set_value_min(0)
                   .set_value_max(0x80)
                   .set_value_none(0xFF)
                   .set_flags(zzub::parameter_flag_state)
                   .set_value_default(18);

        maDynRange = &add_attribute()
                     .set_name("Dynamic Range (dB)")
                     .set_value_min(30)
                     .set_value_max(120)
                     .set_value_default(60);
        maDefVol = &add_attribute()
                   .set_name("Default Volume")
                   .set_value_min(0)
                   .set_value_max(128)
                   .set_value_default(128);
        maMaxDyn = &add_attribute()
                   .set_name("Dynamic Channels")
                   .set_value_min(0)
                   .set_value_max(MaxDynTracks)
                   .set_value_default(8);


    }
    virtual zzub::plugin* create_plugin() const {
        return new geonik_pluckedstring();
    }
    virtual bool store_info(zzub::archive *data) const {
        return false;
    }
} geonik_pluckedstring_info;

struct pluckedstringplugincollection : zzub::plugincollection {
    virtual void initialize(zzub::pluginfactory *factory) {
        factory->register_info(&geonik_pluckedstring_info);
    }

    virtual const zzub::info *get_info(const char *uri, zzub::archive *data) {
        return 0;
    }
    virtual void destroy() {
        delete this;
    }
    // Returns the uri of the collection to be identified,
    // return zero for no uri. Collections without uri can not be
    // configured.
    virtual const char *get_uri() {
        return 0;
    }

    // Called by the host to set specific configuration options,
    // usually related to paths.
    virtual void configure(const char *key, const char *value) {}
};

zzub::plugincollection *zzub_get_plugincollection()
{
    return new pluckedstringplugincollection();
}















