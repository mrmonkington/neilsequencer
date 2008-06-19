#ifndef	CTRACKER_H__
#define	CTRACKER_H__

#include	"Track.h"
#include	"Channel.h"
#include	"WavetableManager.h"
#if defined(_WIN32)
#include	<windows.h>
#endif

#define MAX_TRACKS		16
#define	MAX_CHANNELS	64

class	CAttrVals
{
public:
	int	iVolumeRamp;
	int	iVolumeEnvelopeTicks;
	int	iMIDIChannel;
	int	iMIDIVelocity;
	int	iMIDIWave;
	int	iMIDIUsesFreeTracks;
	int	iFilterMode;
	int	iPitchEnvelopeDepth;
	int	oVirtualChannels;
};

zzub::plugin * create_plugin(const zzub::info *);

class	CMatildeTrackerMachine : public zzub::plugin
{
public:
										CMatildeTrackerMachine();
	virtual								~CMatildeTrackerMachine();

	virtual void destroy() { delete this; }
	virtual void init(zzub::archive *pi);
	virtual void load(zzub::archive *arc) {}
	virtual void process_controller_events() {}
	virtual void process_events();
#ifdef	MONO
	virtual bool process_audio(float *psamples, int numsamples, int mode);
#endif
	virtual void set_track_count(int count);
	virtual void stop();
#ifndef MONO
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode);
#endif
	virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }
	virtual void save(zzub::archive *po);
	virtual void attributes_changed();
	virtual void command(int index);
	virtual void mute_track(int index);
	virtual bool is_track_muted(int index) const;
	virtual void midi_note(int channel, int value, int velocity);
	virtual void event(unsigned int data);
	virtual const char * describe_value(int param, int value);
	virtual const zzub::envelope_info ** get_envelope_infos();
	virtual bool play_wave(int wave, int note, float volume);
	virtual void stop_wave();
	virtual int get_wave_envelope_play_position(int env);
	
	// ::zzub::plugin methods
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
	CWavetableManager					m_Wavetable;

	int									numTracks;
	CTrack								m_Tracks[MAX_TRACKS];
	CChannel							m_Channels[MAX_CHANNELS];

	CChannel						*	AllocChannel();

	CTrackVals							m_TrackValues[MAX_TRACKS];
	CAttrVals							m_Attributes;
	int									m_iNextMIDITrack;
	int									m_iWaveTrack;
	int									m_iNextFreeChannel;

	static struct info : zzub::info {
		info() {
            this->flags = zzub::plugin_flag_plays_waves | zzub::plugin_flag_has_audio_output;
			this->min_tracks = 1;
			this->max_tracks = MAX_TRACKS;
			this->name = "Matilde Tracker";
			this->short_name = "MTrk";
			this->author = "Carsten Sørensen";
			this->uri = "@rift.dk/generator/Matilde+Tracker;1.5";
			
			CMatildeTrackerMachine::m_paraNote = &add_track_parameter()
				.set_note()
				.set_name("Note")
				.set_description("Note")
				.set_value_min(zzub::note_value_min)
				.set_value_max(zzub::note_value_max)
				.set_value_none(zzub::note_value_none)
				.set_flags(0)
				.set_value_default(0);


			CMatildeTrackerMachine::m_paraInstrument = &add_track_parameter()
				.set_byte()
				.set_name("Wave")
				.set_description("Wave to use (01-C8)")
				.set_value_min(zzub::wavetable_index_value_min)
				.set_value_max(zzub::wavetable_index_value_max)
				.set_value_none(zzub::wavetable_index_value_none)
				.set_flags(zzub::parameter_flag_wavetable_index|zzub::parameter_flag_state)
				.set_value_default(0);


			CMatildeTrackerMachine::m_paraVolume = &add_track_parameter()
				.set_byte()
				.set_name("Volume")
				.set_description("Volume (00-FE)")
				.set_value_min(0)
				.set_value_max(0xFE)
				.set_value_none(0xFF)
				.set_flags(0)
				.set_value_default(0);


			CMatildeTrackerMachine::m_paraEffect1 = &add_track_parameter()
				.set_byte()
				.set_name("Effect1")
				.set_description("Effect #1 (00-FE)")
				.set_value_min(0)
				.set_value_max(0xFE)
				.set_value_none(0xFF)
				.set_flags(0)
				.set_value_default(0);


			CMatildeTrackerMachine::m_paraArgument1 = &add_track_parameter()
				.set_byte()
				.set_name("Argument1")
				.set_description("Argument #1 (00-FF)")
				.set_value_min(0)
				.set_value_max(0xFF)
				.set_value_none(0)
				.set_flags(0)
				.set_value_default(0);


			CMatildeTrackerMachine::m_paraEffect2 = &add_track_parameter()
				.set_byte()
				.set_name("Effect2")
				.set_description("Effect #2 (00-FE)")
				.set_value_min(0)
				.set_value_max(0xFE)
				.set_value_none(0xFF)
				.set_flags(0)
				.set_value_default(0);


			CMatildeTrackerMachine::m_paraArgument2 = &add_track_parameter()
				.set_byte()
				.set_name("Argument2")
				.set_description("Argument #2 (00-FF)")
				.set_value_min(0)
				.set_value_max(0xFF)
				.set_value_none(0)
				.set_flags(0)
				.set_value_default(0);
				
			CMatildeTrackerMachine::m_attrVolumeRamp = &add_attribute()
				.set_name("Volume Ramp (ms)")
				.set_value_min(0)
				.set_value_max(5000)
				.set_value_default(1);


			CMatildeTrackerMachine::m_attrVolumeEnvelope = &add_attribute()
				.set_name("Volume Envelope Span (ticks)")
				.set_value_min(1)
				.set_value_max(1024)
				.set_value_default(64);


			CMatildeTrackerMachine::m_attrMIDIChannel = &add_attribute()
				.set_name("MIDI Channel")
				.set_value_min(0)
				.set_value_max(16)
				.set_value_default(0);


			CMatildeTrackerMachine::m_attrMIDIVelocitySensitivity = &add_attribute()
				.set_name("MIDI Velocity Sensitivity")
				.set_value_min(0)
				.set_value_max(256)
				.set_value_default(0);


			CMatildeTrackerMachine::m_attrMIDIWave = &add_attribute()
				.set_name("MIDI Wave")
				.set_value_min(zzub::wavetable_index_value_none)
				.set_value_max(zzub::wavetable_index_value_max)
				.set_value_default(zzub::wavetable_index_value_none);


			CMatildeTrackerMachine::m_attrMIDIUsesFreeTracks = &add_attribute()
				.set_name("MIDI Uses Free Tracks")
				.set_value_min(0)
				.set_value_max(1)
				.set_value_default(0);


			CMatildeTrackerMachine::m_attrFilterMode = &add_attribute()
				.set_name("Filter Mode")
				.set_value_min(0)
				.set_value_max(2)
				.set_value_default(1);


			CMatildeTrackerMachine::m_attrPitchEnvelopeDepth = &add_attribute()
				.set_name("Pitch Envelope Depth (semitones)")
				.set_value_min(0)
				.set_value_max(24)
				.set_value_default(2);


			CMatildeTrackerMachine::m_attrVirtualChannels = &add_attribute()
				.set_name("Enable Virtual Channels")
				.set_value_min(0)
				.set_value_max(1)
				.set_value_default(0);
		}
		
		virtual zzub::plugin* create_plugin() const { return new CMatildeTrackerMachine(); }
		virtual bool store_info(zzub::archive *data) const { return false; }

	} m_MachineInfo;


	static const zzub::envelope_info			m_PanningEnvelope;
	static const zzub::envelope_info			m_PitchEnvelope;
	static const zzub::envelope_info			m_VolumeEnvelope;
	static const zzub::envelope_info		*	m_Envelopes[4];

	static const zzub::attribute		*m_attrVolumeRamp;
	static const zzub::attribute		*m_attrVolumeEnvelope;
	static const zzub::attribute		*m_attrMIDIChannel;
	static const zzub::attribute		*m_attrMIDIVelocitySensitivity;
	static const zzub::attribute		*m_attrMIDIWave;
	static const zzub::attribute		*m_attrMIDIUsesFreeTracks;
	static const zzub::attribute		*m_attrFilterMode;
	static const zzub::attribute		*m_attrPitchEnvelopeDepth;
	static const zzub::attribute		*m_attrVirtualChannels;

	static const zzub::parameter		*m_paraNote;
	static const zzub::parameter		*m_paraInstrument;
	static const zzub::parameter		*m_paraVolume;
	static const zzub::parameter		*m_paraEffect1;
	static const zzub::parameter		*m_paraArgument1;
	static const zzub::parameter		*m_paraEffect2;
	static const zzub::parameter		*m_paraArgument2;

	bool								m_oSustainAllReleases;
	bool								m_oVirtualChannels;

#if defined(_WIN32)
	HWND								m_hDlg;
#endif
	bool								m_oDoTick;
};



#endif	//	CTRACKER_H__
