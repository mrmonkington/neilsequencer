/*
[22:45] <WhiteNoiz> inline float mi::Filter(float input)
[22:45] <WhiteNoiz> {
[22:45] <WhiteNoiz>   if (Cutoff>0.999f) Cutoff=0.999f;		// necessary?
[22:45] <WhiteNoiz>   float fa = float(1.0 - Cutoff); 
[22:45] <WhiteNoiz>   float fb = float(Reso * (1.0 + (1.0/fa)));
[22:45] <WhiteNoiz>   buf0 = fa * buf0 + Cutoff * (input + fb * (buf0 - buf1)); 
[22:45] <WhiteNoiz>   buf1 = fa * buf1 + Cutoff * buf0;
[22:45] <WhiteNoiz>   return buf1;
[22:45] <WhiteNoiz> }
[
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>

#if defined(_WIN32)
#include <windowsx.h>
#include <windows.h>
#include "Resource.h"
#endif

#include <zzub/signature.h>

#include "Tracker.h"
#include "../SurfsDSPLib/SRF_DSP.h"

#pragma optimize ("a", on)

#define TRACKER_LOCK(pCB) zzub::scopelock _slock(pCB)

#if defined(_WIN32)
HINSTANCE dllInstance;
#endif

const zzub::parameter *CMatildeTrackerMachine::m_paraNote = 0;
const zzub::parameter *CMatildeTrackerMachine::m_paraInstrument = 0;
const zzub::parameter *CMatildeTrackerMachine::m_paraVolume = 0;
const zzub::parameter *CMatildeTrackerMachine::m_paraEffect1 = 0;
const zzub::parameter *CMatildeTrackerMachine::m_paraArgument1 = 0;
const zzub::parameter *CMatildeTrackerMachine::m_paraEffect2 = 0;
const zzub::parameter *CMatildeTrackerMachine::m_paraArgument2 = 0;

const zzub::attribute *CMatildeTrackerMachine::m_attrVolumeRamp = 0;
const zzub::attribute *CMatildeTrackerMachine::m_attrVolumeEnvelope = 0;
const zzub::attribute *CMatildeTrackerMachine::m_attrMIDIChannel = 0;
const zzub::attribute *CMatildeTrackerMachine::m_attrMIDIVelocitySensitivity = 0;
const zzub::attribute *CMatildeTrackerMachine::m_attrMIDIWave = 0;
const zzub::attribute *CMatildeTrackerMachine::m_attrMIDIUsesFreeTracks = 0;
const zzub::attribute *CMatildeTrackerMachine::m_attrFilterMode = 0;
const zzub::attribute *CMatildeTrackerMachine::m_attrPitchEnvelopeDepth = 0;
const zzub::attribute *CMatildeTrackerMachine::m_attrVirtualChannels = 0;

zzub::plugin * create_plugin(const zzub::info *)
{
	return new CMatildeTrackerMachine();
}

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }

CMatildeTrackerMachine::info	CMatildeTrackerMachine::m_MachineInfo;

const zzub::envelope_info	CMatildeTrackerMachine::m_VolumeEnvelope=
{
	"Volume",
	zzub::envelope_flag_sustain|zzub::envelope_flag_loop
};

const zzub::envelope_info	CMatildeTrackerMachine::m_PanningEnvelope=
{
	"Panning",
	zzub::envelope_flag_sustain|zzub::envelope_flag_loop
};

const zzub::envelope_info	CMatildeTrackerMachine::m_PitchEnvelope=
{
	"Pitch",
	zzub::envelope_flag_sustain|zzub::envelope_flag_loop
};

const zzub::envelope_info	*	CMatildeTrackerMachine::m_Envelopes[4]=
{
	&m_VolumeEnvelope,
	&m_PanningEnvelope,
	&m_PitchEnvelope,
	0
};

struct matildeplugincollection : zzub::plugincollection {
	virtual void initialize(zzub::pluginfactory *factory) {
		factory->register_info(&CMatildeTrackerMachine::m_MachineInfo);
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
	return new matildeplugincollection();
}

CMatildeTrackerMachine::CMatildeTrackerMachine()
{
	global_values = 0;
	track_values = m_TrackValues;
	attributes = (int *)&m_Attributes;
	numTracks=0;
	m_iNextMIDITrack=0;
	m_iWaveTrack=-1;
	m_oSustainAllReleases=false;
	m_Wavetable.SetTracker( this );
}

CMatildeTrackerMachine::~CMatildeTrackerMachine()
{
}

void CMatildeTrackerMachine::init(zzub::archive *pi)
{
	TRACKER_LOCK(_host);

	int	c;
	for( c=0; c<MAX_TRACKS; c++ )
	{
		m_Tracks[c].m_pMachine=this;
		m_Tracks[c].m_pChannel=0;
		m_Tracks[c].Reset();
	}

	for( c=0; c<MAX_CHANNELS; c++ )
	{
		m_Channels[c].m_pMachine=this;
		m_Channels[c].m_pOwner=0;
		m_Channels[c].m_oFree=true;
	}

	for( c=0; c<MAX_TRACKS; c+=1 )
		m_Tracks[c].Stop();

	m_iWaveTrack=-1;
	m_oSustainAllReleases=false;

	m_Wavetable.Stop();
	m_oVirtualChannels=false;
#if defined(_WIN32)
	m_hDlg=0;
#endif
	m_oDoTick=false;

	//if( pi )
	//	pi->Read( &m_Attributes, sizeof(m_Attributes) );
}

CChannel	*	CMatildeTrackerMachine::AllocChannel()
{
	int	i;
	for( i=0; i<MAX_CHANNELS; i+=1 )
	{
		if( m_Channels[i].m_oFree )
		{
			m_Channels[i].m_oFree=false;
			return &m_Channels[i];
		}
	}

	i=(m_iNextFreeChannel++)&(MAX_CHANNELS-1);
	m_Channels[i].m_oFree=false;

	return &m_Channels[i];
}

void CMatildeTrackerMachine::process_events()
{
	m_oDoTick=true;
}

void CMatildeTrackerMachine::stop()
{
	TRACKER_LOCK(_host);

	for( int c=0; c<MAX_TRACKS; c+=1 )
		m_Tracks[c].Stop();

	m_iWaveTrack=-1;
	m_oSustainAllReleases=false;

	m_Wavetable.Stop();
}

bool CMatildeTrackerMachine::process_stereo(float **pin, float **pout, int numsamples, int mode)
{
	TRACKER_LOCK(_host);

	if( mode!=zzub::process_mode_write )
		return false;

	if( m_oDoTick )
	{
		m_oDoTick=false;
		for (int c = 0; c < numTracks; c++)
			m_Tracks[c].Tick(m_TrackValues[c]);
	}
	bool	gotsomething=false;

	SurfDSPLib::ZeroFloat( pout[0], numsamples );
	SurfDSPLib::ZeroFloat( pout[1], numsamples );

	int	usedchannels=0;

	for( int c=0; c<MAX_CHANNELS; c++ )
	{
		float	*p[] = {pout[0], pout[1]};
		bool	newgotsomething=gotsomething;

		if( !m_Channels[c].m_oFree )
			usedchannels+=1;
		
		if( m_Channels[c].m_pOwner )
		{
			CTrack	*pTrack=m_Channels[c].m_pOwner;

			if( _master_info->tick_position==0 )
			{
				pTrack->m_iLastTick=0;
				pTrack->m_iLastSample=0;
			}

			int	lastsample=pTrack->m_iLastSample+numsamples;

			while( pTrack->m_iLastSample<lastsample )
			{
				int nextticksample;
				nextticksample=(pTrack->m_iLastTick+1)*_master_info->samples_per_tick/pTrack->m_iSubDivide;

				if( (nextticksample>=pTrack->m_iLastSample) && (nextticksample<lastsample) )
				{
					if( nextticksample>pTrack->m_iLastSample )
					{
						if( !gotsomething )
							newgotsomething=m_Channels[c].Generate_Move( p, nextticksample-pTrack->m_iLastSample );
						else
							m_Channels[c].Generate_Add( p, nextticksample-pTrack->m_iLastSample );
						p[0]+=(nextticksample-pTrack->m_iLastSample);
						p[1]+=(nextticksample-pTrack->m_iLastSample);
					}

					pTrack->m_iLastTick+=1;
					pTrack->Process( pTrack->m_iLastTick );
					pTrack->m_iLastSample=nextticksample;
				}
				else
				{
					if( !gotsomething )
						newgotsomething|=m_Channels[c].Generate_Move( p, lastsample-pTrack->m_iLastSample );
					else
						m_Channels[c].Generate_Add( p, lastsample-pTrack->m_iLastSample );

					pTrack->m_iLastSample=lastsample;
				}
			}
		}
		else
		{
			if( !gotsomething )
				newgotsomething|=m_Channels[c].Generate_Move( p, numsamples );
			else
				m_Channels[c].Generate_Add( p, numsamples );
		}

		gotsomething=newgotsomething;
	}

#if defined(_WIN32)
	if( m_hDlg )
	{
		char	temp[80];
		sprintf( temp, "%d", usedchannels ); 	
		Static_SetText( GetDlgItem(m_hDlg,IDC_CHANNELCOUNT), temp );
		sprintf( temp, "%d", m_Wavetable.GetUsedSamples() ); 	
		Static_SetText( GetDlgItem(m_hDlg,IDC_SAMPLECOUNT), temp );
	}
#endif

	return gotsomething;
}

void CMatildeTrackerMachine::save(zzub::archive * po)
{
	TRACKER_LOCK(_host);

	//if( po )
	//	po->Write( &m_Attributes, sizeof(m_Attributes) );
}

void CMatildeTrackerMachine::attributes_changed()
{
	m_oVirtualChannels=m_Attributes.oVirtualChannels?true:false;
}

#if defined(_WIN32)
BOOL CALLBACK AboutDialog( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
		case WM_INITDIALOG:
		{
			((CMatildeTrackerMachine *)lParam)->m_hDlg=hDlg;
#ifdef	MONO
			Static_SetText( hDlg, "About Matilde Tracker (Mono) 1.5" );
#else
			Static_SetText( hDlg, "About Matilde Tracker 1.5" );
#endif
			Edit_SetText( GetDlgItem(hDlg,IDC_EDIT1),
"Matilde Tracker v1.5 is a tracker machine for Buzz which behaves more like "
"Protracker than Jeskola Tracker.\015\012\015\012"
"All Protracker effects that make sense in Buzz are implemented and "
"behave in a similar fashion to their Protracker cousins, so you'll feel right at home.\015\012\015\012"
"Implemented effects:\015\012\015\012"
"00/xy - Arpeggio\015\012\015\012"
"01/xx - Slide up\015\012\015\012"
"02/xx - Slide down\015\012\015\012"
"03/xx - Tone portamento\015\012"
"  If <xx> is zero, keep portamento'ing.\015\012\015\012"
"04/xy - Vibrato\015\012"
"  x=speed, y=depth. If zero, use previous value\015\012\015\012"
"05/xy - Slide panning\015\012"
"  x=amount to slide panning left\015\012"
"  y=amount to slide panning right\015\012\015\012"
"06/xy - Autopan\015\012"
"  x=speed, y=depth. If zero, use previous value\015\012\015\012"
"07/xy - Tremolo\015\012"
"  x=speed, y=depth. If zero, use previous value\015\012\015\012"
"08/xx - Set panning position\015\012"
"  0=left, 80=middle, FF=right\015\012\015\012"
"09/xx - Sample offset\015\012"
"  xx=offset into sample. Unlike Protracker this is not an absolute offset"
" but scales to the whole length of the sample, ie a value of 80 will"
" start from the middle of the sample. If there's no argument, the sample"
" offset will be set right at the end, useful for E8/01.\015\012\015\012"
"0A/xy - Volume slide\015\012"
"  x=amount to slide volume up\015\012"
"  y=amount to slide volume down\015\012\015\012"
"0F/xx - Subdivide amount\015\012"
"  Subdivide amount. This is the same as the Protracker Fxx command, except"
" it doesn't actually change the speed of the song, only the speed of the"
" effects. If the subdivide amount is higher, effects will be updated more"
" often, making them run faster. The default value is 6.\015\012\015\012"
"10/xx - Probability\015\012"
"  Probability for sample being played. 01=will almost certainly not be played,"
" 80=50%, FF=almost certain\015\012\015\012"
"11/xx - Loop fit\015\012"
"  Number of ticks the waveform's should take to complete. Changes the frequency of the waveform.\015\012\015\012"
"12/xy - Loop fit with tracking\015\012"
"  Same as 11 but tracks the speed of the song\015\012\015\012"
"13/xy - Auto shuffle\015\012"
"  x=Ticks to shuffle. 2 shuffles every other step.\015\012  y=Shuffle amount. 0=none, F=a full tick\015\012\015\012"
"14/xx - Randomize volume\015\012"
"  xx=Maximum amount the volume will be randomized\015\012\015\012"
"15/xx - Random delay\015\012"
"  xx=Maximum number of subdivision steps the note will be delayed\015\012\015\012"
"16/xx - Randomize pitch\015\012"
"  xx=Maximum number of notches the pitch will be randomized\015\012\015\012"
"17/xx - Harmonic play\015\012"
"  xx=The base frequency will be multiplied by xx\015\012\015\012"
"18/xy - Combined note delay and cut\015\012"
"  x=The subdivision step to trigger the note\015\012"
"  y=The subdivision step to release the note\015\012\015\012"
"19/xy - Sustain pedal\015\012"
"  y=The subdivision step to trigger the command\015\012"
"  x=1 - Depress sustain pedal\015\012"
"  x=2 - Release sustain pedal\015\012\015\012"
"DC/xx - Note release\015\012"
"  x=subdivision count at which sample is released\015\012\015\012"
"E1/xx - Fine slide up\015\012\015\012"
"E2/xx - Fine slide down\015\012\015\012"
"E4/0x - Set vibrato type\015\012"
"  x=0 - sine, retrig waveform at samplestart\015\012"
"  x=1 - saw, retrig waveform at samplestart\015\012"
"  x=2 - square, retrig waveform at samplestart\015\012"
"  x=4 - sine, don't retrig waveform at samplestart\015\012"
"  x=5 - saw, don't retrig waveform at samplestart\015\012"
"  x=6 - square, don't retrig waveform at samplestart\015\012\015\012"
"E5/xx - Set finetune\015\012"
"  00 = -1/2 halfnote, 80 = 0, FF = ~+1/2 halfnote\015\012\015\012"
"E6/0x - Set panning type\015\012"
"  see E4x for parameter\015\012\015\012"
"E7/0x - Set tremolo type\015\012"
"  see E4x for parameter\015\012\015\012"
"E8/01 - Reverse direction of sample being played\015\012\015\012"
"E9/xx - Retrig sample\015\012"
"  x=subdivision count at which sample is retriggered\015\012\015\012"
"EA/xx - Fine volume slide up\015\012\015\012"
"EB/xx - Fine volume slide down\015\012\015\012"
"EC/xx - Note cutoff\015\012"
"  x=subdivision count at which sample is cut\015\012\015\012"
"ED/xx - Note delay\015\012"
"  Delay samplestart for <x> subdivision steps\015\012\015\012"
"EE/xx - Fine panning slide left\015\012\015\012"
"EF/xx - Fine panning slide right\015\012\015\012"
 );
			return( TRUE );
		}
        case WM_COMMAND:
		{
            switch( LOWORD( wParam ))
			{
				case IDOK:
				case IDCANCEL:
					EndDialog( hDlg, 0 );
					return( TRUE );
					break;
			}
            break;
		}
		default:
		{
			return( FALSE );
        }
	}

	return( FALSE );
}

#endif

void CMatildeTrackerMachine::command(int i)
{
	if( i==0 )
	{
#if defined(_WIN32)
		DialogBoxParam( dllInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, (DLGPROC)AboutDialog, (LPARAM)this );
		m_hDlg=NULL;
#endif
	}
}

void CMatildeTrackerMachine::mute_track(int index)
{
}

bool CMatildeTrackerMachine::is_track_muted(int index) const
{
	return false;
}
void CMatildeTrackerMachine::midi_note(int channel, int value, int velocity)
{
	if( m_Attributes.iMIDIChannel==0 || channel!=m_Attributes.iMIDIChannel-1 )
		return;

	int v2;
	v2=value-24;	// + aval.MIDITranspose-24;

	if( v2/12>9 )
		return;

	int n=((v2/12)<<4) | ((v2%12)+1);
	if( velocity>0 )
	{
		if( m_iNextMIDITrack>=MAX_TRACKS )
			m_iNextMIDITrack=m_Attributes.iMIDIUsesFreeTracks?numTracks:0;

		if( m_Attributes.iMIDIUsesFreeTracks && m_iNextMIDITrack<numTracks )
			m_iNextMIDITrack=numTracks;

		if( m_iNextMIDITrack>=MAX_TRACKS )
			return;

		if( m_Tracks[m_iNextMIDITrack].m_oAvailableForMIDI ) 
		{
			CTrackVals	tv;

			tv.note=n;
			tv.instrument=m_Attributes.iMIDIWave;
			tv.effects[0].command=0;
			tv.effects[0].argument=0;
			tv.effects[1].command=0;
			tv.effects[1].argument=0;

			tv.volume = ((velocity*m_Attributes.iMIDIVelocity)>>8)+((256-m_Attributes.iMIDIVelocity)>>1);
			m_Tracks[m_iNextMIDITrack].Tick( tv );
			m_Tracks[m_iNextMIDITrack].m_oAvailableForMIDI=false;
			m_iNextMIDITrack+=1;
		}
	}
	else
	{
		for( int c=m_Attributes.iMIDIUsesFreeTracks?numTracks:0; c<MAX_TRACKS; c++ )
		{
			if( m_Tracks[c].m_iBaseNote==n )
			{
				CTrackVals	tv;

				tv.note=zzub::note_value_off;
				tv.instrument=zzub::wavetable_index_value_none;
				tv.volume=0xFF;
				tv.effects[0].command=0;
				tv.effects[0].argument=0;
				tv.effects[1].command=0;
				tv.effects[1].argument=0;

				m_Tracks[c].Tick( tv );
				m_Tracks[c].m_oAvailableForMIDI=true;
				//return;
			}
		}
	}
}

void CMatildeTrackerMachine::event(unsigned int data)
{
}

const char * CMatildeTrackerMachine::describe_value(int param, int value)
{
	if( (param==3) || (param==5) )
	{
		switch( value )
		{
			case	0:
				return "Arpeggio";
				break;
			case	1:
				return "Slide up";
				break;
			case	2:
				return "Slide down";
				break;
			case	3:
				return "Portamento";
				break;
			case	4:
				return "Vibrato";
				break;
			case	5:
				return "Slide panning";
				break;
			case	6:
				return "Autopanning";
				break;
			case	7:
				return "Tremolo";
				break;
			case	8:
				return "Panning";
				break;
			case	9:
				return "Offset";
				break;
			case	0xA:
				return "Volume slide";
				break;
			case	0x0F:
				return "Subdivide";
				break;
			case	0x10:
				return "Probability";
				break;
			case	0x11:
				return "Loop fit";
				break;
			case	0x12:
				return "Loop fit w/tracking";
				break;
			case	0x13:
				return "Auto shuffle";
				break;
			case	0x14:
				return "Randomize volume";
				break;
			case	0x15:
				return "Random delay";
				break;
			case	0x16:
				return "Randomize pitch";
				break;
			case	0x17:
				return "Harmonic";
				break;
			case	0x18:
				return "Note delay and cut";
				break;
			case	0x19:
				return "Sustain pedal";
				break;
			case	0x20:
				return "Set filter cutoff";
				break;
			case	0x21:
				return "Slide cutoff up";
				break;
			case	0x22:
				return "Slide cutoff down";
				break;
			case	0x23:
				return "Set cutoff LFO";
				break;
			case	0x24:
				return "Cutoff LFO";
				break;
			case	0x25:
				return "Fine slide cutoff up";
				break;
			case	0x26:
				return "Fine slide cutoff down";
				break;
			case	0x28:
				return "Set filter resonance";
				break;
			case	0x29:
				return "Slide resonance up";
				break;
			case	0x2A:
				return "Slide resonance down";
				break;
			case	0x2B:
				return "Set resonance LFO";
				break;
			case	0x2C:
				return "Resonance LFO";
				break;
			case	0x2D:
				return "Fine slide rez up";
				break;
			case	0x2E:
				return "Fine slide rez down";
				break;
			case	0x39:
				return "Set cue point";
				break;
			case	0xDC:
				return "Note release";
				break;
			case	0xE0:
				return "Set filter type";
				break;
			case	0xE1:
				return "Fine slide up";
				break;
			case	0xE2:
				return "Fine slide down";
				break;
			case	0xE4:
				return "Vibrato type";
				break;
			case	0xE5:
				return "Finetune";
				break;
			case	0xE6:
				return "Panning type";
				break;
			case	0xE7:
				return "Tremolo type";
				break;
			case	0xE8:
				return "Sample direction";
				break;
			case	0xE9:
				return "Retrig";
				break;
			case	0xEA:
				return "Fine volume up";
				break;
			case	0xEB:
				return "Fine volume down";
				break;
			case	0xEC:
				return "Note cut";
				break;
			case	0xED:
				return "Note delay";
				break;
			case	0xEE:
				return "Fine panning left";
				break;
			case	0xEF:
				return "Fine panning right";
				break;
		}
	}
	return NULL;
}

const zzub::envelope_info ** CMatildeTrackerMachine::get_envelope_infos()
{
	return &m_Envelopes[0];
}

bool CMatildeTrackerMachine::play_wave(int wave, int note, float volume)
{
	if( m_iNextMIDITrack>=MAX_TRACKS )
		m_iNextMIDITrack=m_Attributes.iMIDIUsesFreeTracks?numTracks:0;

	if( m_Attributes.iMIDIUsesFreeTracks && m_iNextMIDITrack<numTracks )
		m_iNextMIDITrack=numTracks;

	if( m_iNextMIDITrack>=MAX_TRACKS )
		return false;

	if( m_Tracks[m_iNextMIDITrack].m_oAvailableForMIDI ) 
	{
		CTrackVals	tv;

		tv.note=note;
		tv.instrument=wave;
		tv.effects[0].command=0;
		tv.effects[0].argument=0;
		tv.effects[1].command=0;
		tv.effects[1].argument=0;

		tv.volume = int(volume*128.0f);
		m_Tracks[m_iNextMIDITrack].Tick( tv );
		m_Tracks[m_iNextMIDITrack].m_oAvailableForMIDI=false;

		m_iWaveTrack=m_iNextMIDITrack;

		m_iNextMIDITrack+=1;

		return true;
	}
	return false;
}

void CMatildeTrackerMachine::stop_wave()
{
	if( m_iWaveTrack!=-1 )
	{
		CTrackVals	tv;

		tv.note=zzub::note_value_off;
		tv.instrument=zzub::wavetable_index_value_none;
		tv.volume=0;
		tv.effects[0].command=0;
		tv.effects[0].argument=0;
		tv.effects[1].command=0;
		tv.effects[1].argument=0;

		m_Tracks[m_iWaveTrack].Tick( tv );
		m_Tracks[m_iWaveTrack].m_oAvailableForMIDI=true;

		m_iWaveTrack=-1;
	}
}

int CMatildeTrackerMachine::get_wave_envelope_play_position(int env)
{
	if( m_iWaveTrack!=-1 )
	{
		return m_Tracks[m_iWaveTrack].GetWaveEnvPlayPos( env );
	}
	return -1;
}

void CMatildeTrackerMachine::set_track_count(int n)
{
	TRACKER_LOCK(_host);

	if( n>numTracks )
	{
		int i;
		for( i=numTracks; i<n; i+=1 )
		{
			m_Tracks[i].Reset();
		}
	}
	numTracks = n;
}

#if defined(_WIN32)

BOOL WINAPI DllMain( HANDLE hModule, DWORD fdwreason, LPVOID lpReserved )
{
	if( fdwreason==DLL_PROCESS_ATTACH )
		dllInstance=(HINSTANCE)hModule;

	return TRUE;
}

#endif
