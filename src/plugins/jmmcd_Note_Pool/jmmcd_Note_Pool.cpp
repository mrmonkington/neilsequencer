// This is mostly done, but needs cleaning up. All the volumes,
// deviations, probabilities could be floats.

// problem: how do we do per-track controllers? eg add a track to
// control an extra machine. that's how it worked in buzz.

// thanks to btdsys, 7900, and usr for answering questions, but especially btdsys
// for making peerlib, without which this wouldn't be possible.
#include <cstdio>
#define MACHINE_NAME "Peer Note-Pool"
#define AUTHOR "jmmcd"
#define FULL_NAME AUTHOR " " MACHINE_NAME
#define MACHINE_VERSION "2.0"

#define ABOUT_TEXT FULL_NAME " " MACHINE_VERSION \
	"\nby James McDermott" \
	"\njamesmichaelmcdermott@gmail.com" \
	"\nwww.skynet.ie/~jmmcd" \
"\njmmcd on buzzmachines.com and buzzmusic.wipe-records.org and #buzz and aldrin-users and #aldrin"

#define PARA_VOLUME_MAX 0x80
#define PARA_VOLUME_DEF 0x60
#define MAX_TRACKS 128

#include <zzub/signature.h>
#include <zzub/plugin.h>

#include <float.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

char *pitch_to_string(int pitch);
char *oct_pitch_to_string(int octave, int pitch);


typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;
typedef enum {c, c_sh, d, d_sh, e, f, f_sh, g, g_sh, a, a_sh, b, off, numNotes} pitch_type;

char _name[numNotes][32];
char _desc[numNotes][64];


const zzub::parameter *paraCNote = 0;
const zzub::parameter *paraCVolume = 0;
const zzub::parameter *paraNote = 0;
const zzub::parameter *paraVolume = 0;
const zzub::parameter *paraProb = 0;
const zzub::parameter *paraCentre = 0;
const zzub::parameter *paraOctDev = 0;
const zzub::parameter *paraVolDev = 0;
const zzub::parameter *paraDotProb = 0;
const zzub::parameter *paraOn = 0;
const zzub::parameter *paraNoteProb[numNotes] = {0};

class cvals
{
public:
  byte note;
  byte volume;
};

#define numNote numNotes
#define numVolume (numNotes + 1)
#define numProb (numNotes + 2)
#define numCentre (numNotes + 3)
#define numOctDev (numNotes + 4)
#define numVolDev (numNotes + 5)
#define numDotProb (numNotes + 6)
#define numOn (numNotes + 7)



class gvals
{
public:
  byte note_prob[numNotes];
};

class tvals
{
public:
  byte note;
  byte volume;
  byte prob;
  byte centre_note;
  byte oct_dev;
  byte vol_dev;
  byte dot_prob;
  byte on;
};

class avals
{
public:
};


class CTrack {
public:
  int note;
  float volume;
  int prob;
  int centre_note, oct_dev;
  float vol_dev;
  float dot_prob;
  int on;

  int send_notep;
  int note_to_send;
  int vol_to_send;
  
  void Init() {
    on = false;
    note = zzub::note_value_none;
    volume = 80.0f / PARA_VOLUME_MAX;
    vol_dev = 0.0f;
    prob = 100;
    centre_note = 60;
  }
  void Stop() {
    on = false;
  }
};



class note_pool: public zzub::plugin
{
public:
  note_pool();
  virtual ~note_pool();
  virtual void process_events();
  virtual void init(zzub::archive *);
  virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode) {}
  virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
  virtual void command(int i);
  virtual void load(zzub::archive *arc) {}
  virtual void save(zzub::archive *) { }
  virtual const char * describe_value(int param, int value);
  virtual void OutputModeChanged(bool stereo) { }
  virtual void process_controller_events();
  virtual void destroy() { delete this; }
  virtual void stop();
  virtual void attributes_changed() {}
  virtual void set_track_count(int n);
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
  virtual void add_input(const char*) {}
  virtual void delete_input(const char*) {}
  virtual void rename_input(const char*, const char*) {}
  virtual void input(float**, int, float) {}
  virtual void midi_control_change(int, int, int) {}
  virtual bool handle_input(int, int, int) { return false; }
  // virtual void process_midi_events(zzub::midi_message*, int) {};
  // virtual void get_midi_output_names(zzub::outstream*) {};
  // virtual void set_stream_source(const char*) {};
  // virtual const char* get_stream_source() {};


private:

  int weighted_bool(int n);
  int oct_pitch_to_midi(int octave, int pitch);
  int midi_to_pitch(int midi_note);
  int midi_to_oct(int midi_note);
  int buzz_to_midi(int buzz_note);
  int midi_to_buzz(int midi_note);
  void process_note_regular(float *note);
  int note_pick(int centre, int oct_dev);
  int round(float in);
  float vol_rand(float vol, float dev);

  
public:
  
  int numTracks;
  
  CTrack tracks[MAX_TRACKS];
  
  int note_prob[numNotes];


  
private:
  gvals gval;
  tvals tval[MAX_TRACKS];
  cvals cval[MAX_TRACKS];
};




////////////////////////////////////////////////////////////////////////

// probability functions

int note_pool::weighted_bool(int n) {
  if (n > ((rand() % 100))) {
    return 1;
  } else {
    return 0;
  }
}

////////////////////////////////////////////////////////////////////////

// conversion functions

int note_pool::oct_pitch_to_midi(int octave, int pitch) {
  return octave * 12 + pitch;
}


int note_pool::midi_to_pitch(int midi_note) {
  return midi_note % 12;
}


int note_pool::midi_to_oct(int midi_note) {
  return midi_note / 12;
}


int note_pool::buzz_to_midi(int buzz_note) {
  if ((buzz_note % 16) > 11) {
    // it's not a real buzz note! hehe
    buzz_note += (16 - (buzz_note % 16)); // ie push it up to next C FIXME - not right!
  }
  return ((buzz_note >> 4) * 12 + (buzz_note & 0x0f) - 1);
}


int note_pool::midi_to_buzz(int midi_note) {
  return (16 * ((midi_note - (midi_note % 12)) / 12) + midi_note % 12 + 1);
}

char * pitch_to_string(int pitch) {
  static char txt[10];
  char letter;
  int sharp = 1;
  switch (pitch) {
  case c:
    sharp = 0;
  case c_sh:
    letter = 'C'; break;
  case d:
    sharp = 0;
  case d_sh:
    letter = 'D'; break;
  case e:
    sharp = 0;
    letter = 'E'; break;
  case f:
    sharp = 0;
  case f_sh:
    letter = 'F'; break;
  case g:
    sharp = 0;
  case g_sh:
    letter = 'G'; break;
  case a:
    sharp = 0;
  case a_sh:
    letter = 'A'; break;
  case b:
    sharp = 0;
    letter = 'B'; break;
  case off:
    // has to be dealt with specially
    break;
  default:
    break;
  }
  if (!sharp) {
    sprintf(txt, "%c", letter);
  } else {
    sprintf(txt, "%c%c", letter, '#');
  }
  if (pitch == off) {
    sprintf(txt, "Off");
  }
  return txt;
}


char * oct_pitch_to_string(int octave, int pitch) {
  static char txt[10];
  sprintf(txt, "%s%d", pitch_to_string(pitch), octave);
  if (pitch == off) {
    sprintf(txt, "Off");
  }
  
  return txt;
}

/////////////////////////////////////////////////////////////////////////


void note_pool::set_track_count(int n) {
  if (numTracks < n) {
    for (int i = numTracks; i < n; i++) {
      tracks[i].Init();
    }
  } else if (n < numTracks) {
    for (int i = n; i < numTracks; i++)
      tracks[i].Stop();
  }
  numTracks = n;
}

void note_pool::process_events() {
  //printf("in process_events\n");
  for (int i = 0; i < numNotes; i++) {
    if (gval.note_prob[i] != paraNoteProb[i]->value_none) {
      note_prob[i] = gval.note_prob[i];
    }
  }
  for (int t = 0; t < numTracks; t++) {
    if (tval[t].centre_note != paraCentre->value_none) {
      tracks[t].centre_note = tval[t].centre_note;
    }
    if (tval[t].oct_dev != paraOctDev->value_none) {
      tracks[t].oct_dev = tval[t].oct_dev;
    }
    if (tval[t].vol_dev != paraVolDev->value_none) {
      tracks[t].vol_dev = tval[t].vol_dev / 100.0f;
    }
    if (tval[t].dot_prob != paraDotProb->value_none) {
      // paraDotProb.Max is 254: this squaring allows more fine-tuning
      // at the lower end of the scale.
      tracks[t].dot_prob = 100.0f 
	* tval[t].dot_prob * tval[t].dot_prob / (254.0f * 254.0f);
    }
    if (tval[t].on != paraOn->value_none) {
      tracks[t].on = tval[t].on;
    }
  }


  for (int t = 0; t < numTracks; t++) {
    if (tracks[t].on) {
      if (tval[t].note != paraNote->value_none && tval[t].prob == paraProb->value_none) { 
	// definitely play the given note
	tracks[t].send_notep = 1;
	tracks[t].note_to_send = tval[t].note;
      } else if (tval[t].note == paraNote->value_none && tval[t].prob != paraProb->value_none) { 
	// pick a random note and play with probability prob
	tracks[t].send_notep = weighted_bool(tval[t].prob);
	tracks[t].note_to_send = note_pick(tracks[t].centre_note, tracks[t].oct_dev);
      } else if (tval[t].note != paraNote->value_none && tval[t].prob != paraProb->value_none) { 
	// play the given note with probability prob
	tracks[t].send_notep = weighted_bool(tval[t].prob);
	tracks[t].note_to_send = tval[t].note;
      } else {
	// pick a random note and play with probability dot_prob
	tracks[t].send_notep = weighted_bool(tracks[t].dot_prob);
	tracks[t].note_to_send = note_pick(tracks[t].centre_note, tracks[t].oct_dev);
      }
    
      if (tracks[t].send_notep) {
	// Get the volume to be sent, as a proportion
	float vol_prop;
	if (tval[t].volume != paraVolume->value_none) {
	  tracks[t].volume = (float) tval[t].volume / PARA_VOLUME_MAX;
	  vol_prop = tracks[t].volume;
	} else {
	  vol_prop = vol_rand(tracks[t].volume, tracks[t].vol_dev);
	}
	// Scale the volume to the range of the target volume param.
	tracks[t].vol_to_send = paraCVolume->value_min + vol_prop * (paraCVolume->value_max - paraCVolume->value_min);
      }
    }
  }
}
 
void note_pool::process_controller_events() {
  //printf("in process_controller_events\n");
  for (int t = 0; t < numTracks; t++) {
    // 	just sending note and vol for now -- FIXME want to send slide etc as well.
    if (tracks[t].on) {
      if (tracks[t].send_notep) {
	cval[t].note = tracks[t].note_to_send;
	cval[t].volume = tracks[t].vol_to_send;
	printf("track %d sending note: %d; vol %d\n",
	       t, tracks[t].note_to_send, tracks[t].vol_to_send);
      }
    }
  }
}

// i'm using the words pitch and note interchangeably here
int note_pool::note_pick(int centre, int oct_dev) {
  int pitch, octave;
  float r = rand() / (float) RAND_MAX;
  float bin = 0.0f;
  int sigma = 0; // sum of relative probabilities
  
  for (int i = 0; i < numNotes; i++) {
    //printf("note_prob[%d] = %d\n", i, note_prob[i]);
    sigma += note_prob[i];
  }

  if (sigma == 0) {
    // All the note probabilities are zero.
    return zzub::note_value_off;
  }
  
  for (int i = 0; i < numNotes; i++) {
    bin += note_prob[i] / (float) sigma;
    if (r < bin) {
      pitch = i;
      break;
    }
  }
    
  // note-off!
  if (pitch == off) return zzub::note_value_off;
  
  int centre_note = midi_to_pitch(buzz_to_midi(centre));
  octave = midi_to_oct(buzz_to_midi(centre));
  
  // first, transform so we're as close to target note as possible.
  if (centre_note - pitch > 6) {
    octave++;
  } else if (pitch - centre_note > 6) {
    octave--;
  }
  
  if (oct_dev < 10) {
    // minus-mode:
    r = - (rand() / (float) RAND_MAX); // r is between -1 and 0
  } else if (oct_dev < 20) {
    // centred-mode:
    oct_dev -= 10;
    r = (2 * rand() / (float) RAND_MAX) - 1.0f; // r is between -1 and 1
  } else {
    // plus-mode:
    oct_dev -= 20;
    r = (rand() / (float) RAND_MAX); // r is between 0 and 1
  }
  
  octave += round(oct_dev * r * r * r);
  
  //printf("adding %d\n", round(oct_dev * r * r * r));
  if (octave > 9) octave = 9;
  if (octave < 0) octave = 0;
  
  // convert from octave-pitch to midi to buzz
  int retval = midi_to_buzz(oct_pitch_to_midi(octave, pitch));
  //printf("note_pick returning %d; octave = %d, pitch = %d\n", retval, octave, pitch);
  return retval;
}

int note_pool::round(float in) {
  //printf("round: in=%f; out=%d\n", in, (int) floor(in + 0.5f));
  return (int) (in + 0.5f);
}

float note_pool::vol_rand(float vol, float dev) {
  float r = (2.0f * rand() / (float) RAND_MAX) - 1.0f; // r is between -1 and 1
  vol += dev * r * r * r;
  if (vol > 1.0f) vol = 1.0f;
  if (vol < 0.0f) vol = 0.0f;
  return vol;
}



void note_pool::stop() {
  for (int m = 0; m < MAX_TRACKS; m++) {
    tracks[m].Stop();
  }
}


////////////////////////////////////////////////////////////////////////

char const *note_pool::describe_value(const int param, const int value)
{
  static char txt[16];
  //printf("in describe_value: param = %d\n", param);
	
  switch (param) {
  case numNote: // note
    sprintf(txt, "%s", oct_pitch_to_string(midi_to_oct(buzz_to_midi(value)),
					   midi_to_pitch(buzz_to_midi(value))));
    return txt;
  case numVolume: // volume
    sprintf(txt, "%i%%", (int) (value * 100.0f / PARA_VOLUME_MAX));
    return txt;
  case numProb: // prob
    sprintf(txt, "%i", value);
    return txt;
  case numCentre:
    sprintf(txt, "%s", oct_pitch_to_string(midi_to_oct(buzz_to_midi(value)),
					   midi_to_pitch(buzz_to_midi(value))));
    return txt;
  case numVolDev:
    sprintf(txt, "%d%%", value);
    return txt;
  case numOctDev:
    if (value < 10) {
      sprintf(txt, "- %i", value);
    } else if (value < 20) {
      sprintf(txt, "+/- %i", value - 10);
    } else {
      sprintf(txt, "+ %i", value - 20);
    }
    return txt;
  case numDotProb: // 
    sprintf(txt, "%.2f%%", 100.0f * value * value / (254.0f * 254.0f));
    return txt;
  case numOn:
    if (value) {
      sprintf(txt, "On");
    } else {
      sprintf(txt, "Off");
    }
    return txt;
  default: // includes note-probabilities
    sprintf(txt, "%d%%", value);
    return txt;
  }
}


















note_pool::note_pool() {
  global_values = &gval;
  track_values = &tval;
  controller_values = &cval;
  numTracks = 0;
  srand(time(NULL));
}

note_pool::~note_pool() {
  for (int m = 0; m < MAX_TRACKS; m++) {
  }
}

void note_pool::init(zzub::archive *)
{
  int m;
  
  for (m = 0; m < MAX_TRACKS; m++) {
    tracks[m].Init();
  }
  

}

void note_pool::command(int const i)
{
}

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }


struct note_pool_plugin_info : zzub::info {
  note_pool_plugin_info() {
    this->flags = zzub::plugin_flag_has_event_output;
    this->min_tracks = 1;
    this->max_tracks = MAX_TRACKS;
    this->name = "jmmcd Note_Pool";
    this->short_name = "Note_Pool";
    this->author = "jmmcd <jamesmichaelmcdermott@gmail.com>";
    this->uri = "jmmcd/Controllers/Note-Pool";

//     int def_prob[numNotes];
//     int p[] = {20, 0, 25, 0, 25, 15, 0, 10, 0, 30, 0, 10, 10};
//     for (int i = 0; i < numNotes; i++) {
//       def_prob[i] = p[i];
//     }
    int def_prob[] = {20, 0, 25, 0, 25, 15, 0, 10, 0, 30, 0, 10, 10};


    paraCNote = &add_controller_parameter()
      .set_note()
      .set_name("CNote")
      .set_description("Controller Note");
    paraCVolume = &add_controller_parameter()
      .set_byte()
      .set_name("CVolume")
      .set_description("Controller Volume")
      .set_value_min(0)
      .set_value_max(PARA_VOLUME_MAX)
      .set_value_none(PARA_VOLUME_MAX + 1)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(PARA_VOLUME_DEF);

    paraNote = &add_track_parameter()
      .set_note()
      .set_name("Note")
      .set_description("Note");
    paraVolume = &add_track_parameter()
      .set_byte()
      .set_name("Volume")
      .set_description("Volume")
      .set_value_min(0)
      .set_value_max(PARA_VOLUME_MAX)
      .set_value_none(PARA_VOLUME_MAX + 1)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(PARA_VOLUME_DEF);
    paraProb = &add_track_parameter()
      .set_byte()
      .set_name("Prob")
      .set_description("Probability a note will be played this tick")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(101)
      .set_flags(0)
      .set_value_default(100);
    paraCentre = &add_track_parameter()
      .set_note()
      .set_name("Centre")
      .set_description("Centre Note")
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(65);
    paraOctDev = &add_track_parameter()
      .set_byte()
      .set_name("OctDev")
      .set_description("Octave randomisation amount (up to the amount specified)")
      .set_value_min(0)
      .set_value_max(29)
      .set_value_none(30)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(10);
    paraVolDev = &add_track_parameter()
      .set_byte()
      .set_name("VolDev")
      .set_description("Volume randomisation amount")
      .set_value_min(0)
      .set_value_max(100)
      .set_value_none(101)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(0);
    paraDotProb = &add_track_parameter()
      .set_byte()
      .set_name("DotProb")
      .set_description("Probability that a note will be played on empty tick")
      .set_value_min(0)
      .set_value_max(0xFE)
      .set_value_none(0xFF)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(30);
    paraOn = &add_track_parameter()
      .set_switch()
      .set_name("On")
      .set_description("On/Off switch")
      .set_value_min(zzub::switch_value_off)
      .set_value_max(zzub::switch_value_on)
      .set_value_none(zzub::switch_value_none)
      .set_flags(zzub::parameter_flag_state)
      .set_value_default(zzub::switch_value_off);
    for (int i = 0; i < numNotes; i++) {
//       _name[i] = (char *)malloc(32 * sizeof(char));
//       _desc[i] = (char *)malloc(64 * sizeof(char));
      sprintf(_name[i], "Prob(%s)", pitch_to_string(i));
      sprintf(_desc[i], "RELATIVE Probability note %s will be played", pitch_to_string(i));

      paraNoteProb[i] = &add_global_parameter()
	.set_byte()
	.set_name(_name[i])
	.set_description(_desc[i])
	.set_value_min(0)
	.set_value_max(100)
	.set_value_none(101)
	.set_flags(zzub::parameter_flag_state)
	.set_value_default(def_prob[i]);
    }
  }


  virtual zzub::plugin* create_plugin() const { return new note_pool(); }
  virtual bool store_info(zzub::archive *data) const { return false; }
} note_pool_info;

struct note_poolplugincollection : zzub::plugincollection {
  virtual void initialize(zzub::pluginfactory *factory) {
    factory->register_info(&note_pool_info);
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
  return new note_poolplugincollection();
}
  
  
