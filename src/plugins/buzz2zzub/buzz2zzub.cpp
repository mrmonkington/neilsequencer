// buzz2zzub plugin adapter
// Copyright (C) 2006 Leonard Ritter (contact@leonard-ritter.com)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


// zzub2buzz allows running buzzmachines as zzub plugins
// please note that this library will only build correctly
// with msvc since gcc does not support thiscalls.

// however for debugging reasons, this file is also set up
// so it can be included on linux to fix compiler errors

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <map>
#define __BUZZ2ZZUB__
#include <zzub/signature.h>
#include "zzub/plugin.h"
#include "MachineInterface.h"
#include "mdk.h"
#include "mdkimpl.h"
#include "dsplib.h"
#include "../../libzzub/synchronization.h"  // needed by metaplugin
#include "../../libzzub/metaplugin.h"       // needs to set some internal Buzz-only stuff 
#include "../../libzzub/sequencer.h"        // needs to set some internal Buzz-only stuff 
#include "../../libzzub/player.h"           // needs to set some internal Buzz-only stuff 
#include "unhack.h"

void CopyM2S(float *pout, float *pin, int numsamples, float amp);


namespace buzz2zzub
{

using namespace zzub;



void CopyStereoToMono(float *pout, float *pin, int numsamples, float amp)
{
	do
	{
		*pout++ = (pin[0] + pin[1]) * amp;
		pin += 2;
	} while(--numsamples);
}

void Amp(float *pout, int numsamples, float amp) {
	for (int i=0; i<numsamples; i++) {
		pout[i]*=amp;
	}
}


inline bool my_isnan(double x) {
    return x != x;
}

 // TODO: finn bevis på at vi trenger sjekk for både 0 og isnan
bool isBufferZero(float* buf, int numsamples) {
	// sjekk om det bare er nuller i inputen
	bool zeroInput=true;
	for (int i=0; i<numsamples; i++) {
//		if (fabs(buf[i])!=0.0) {
		if (abs(buf[i])>=0.00001 && !my_isnan(buf[i])) {
			zeroInput=false;
			break;
		}
	}
	return zeroInput;
}

class CMachineDataInputWrap : public CMachineDataInput
{
public:
	instream* pi;

	CMachineDataInputWrap(instream *pi)
	{
		this->pi = pi;
	}

	virtual void Read(void *pbuf, int const numbytes)
	{
        if (pi->position()+numbytes <= pi->size())
		    pi->read(pbuf, numbytes);
	}
};

class CMachineDataOutputWrap : public CMachineDataOutput
{
public:
	outstream* po;

	CMachineDataOutputWrap(outstream *po)
	{
		this->po = po;
	}

	virtual void Write(void *pbuf, int const numbytes)
	{
		po->write(pbuf, numbytes);
	}
};
	

class outstreamwrap : public outstream
{
public:
	CMachineDataOutput* po;

	outstreamwrap(CMachineDataOutput *po)
	{
		this->po = po;
	}

	virtual int write(void *buffer, int size)
	{
		po->Write(buffer, size);
		return size;
	}

	virtual long position() {
		assert(false);
		return 0;
	}

	virtual void seek(long, int) {
		assert(false);
	}
};


void s2i(float *i, float **s, int numsamples) {
	float *p[] = {s[0],s[1]};
	while (numsamples--)
	{
		*i++ = *p[0]++;
		*i++ = *p[1]++;
	};
}

void i2s(float **s, float *i, int numsamples) {
	if (!numsamples)
		return;
	float *p[] = {s[0],s[1]};
	while (numsamples--)
	{
		*p[0]++ = *i++;
		*p[1]++ = *i++;
	};
}

struct plugin : zzub::plugin, CMICallbacks, zzub::event_handler {
	
	CMachineInterface* machine;
	CMachineInterfaceEx* machine2;
	CMDKImplementation* implementation;
    const info* machineInfo;

    int channels;
	bool isLdmixer;
	bool isUtrk;
	bool isVocoderXp;
	
	plugin(CMachineInterface* machine, const info* mi)
	{
		this->implementation=0;
		this->machine2=0;
		this->machine = machine;
		this->global_values = this->machine->GlobalVals;
		this->track_values = this->machine->TrackVals;
		this->attributes = this->machine->AttrVals;
        this->machineInfo=mi;
        channels=1;
	}
	~plugin()
	{
		delete this->machine;
	}
	
	// zzub::plugin implementations
	
	virtual void destroy() { delete this; }
	virtual void init(archive *arc)
	{ 
		machine->pCB = this;
		machine->pMasterInfo = reinterpret_cast<CMasterInfo*>(_master_info);
        metaplugin* thisplugin=_host->get_metaplugin();
        thisplugin->_internal_machine=machine;
        thisplugin->_internal_machine_ex=machine2;
		if (arc)
			machine->Init(&CMachineDataInputWrap(arc->get_instream(""))); 
		else
			machine->Init(0); 

        isLdmixer = strcmp(machineInfo->uri, "@zzub.org/buzz2zzub/ld+mixer") == 0;
        isVocoderXp = strcmp(machineInfo->uri, "@zzub.org/buzz2zzub/ld+vocoder+xp") == 0;
        isUtrk = strcmp(machineInfo->uri, "@zzub.org/buzz2zzub/Fuzzpilz+UnwieldyTracker") == 0 || strcmp(machineInfo->uri, "@zzub.org/buzz2zzub/UnwieldyTracker") == 0;
		
		// ld mixer and utrk need event handlers
		if (isLdmixer || isVocoderXp || isUtrk)
			_host->set_event_handler(reinterpret_cast<zzub::metaplugin*>(thisplugin), this);
	}
	virtual void process_controller_events() {}
	virtual void process_events()
	{
		unhack::hackTick(_master_info->beats_per_minute, _host->get_song_begin_loop(), 
			_host->get_song_end_loop(), _host->get_song_end(), _host->get_play_position());

		// support hacked jumping from ticks
		int last_play_position = _host->get_play_position();

		machine->Tick();

		if (last_play_position != unhack::hackseq->songPos)
			_host->set_play_position(unhack::hackseq->songPos);

	}
/*	virtual bool process_audio(float *psamples, int numsamples, const int mode)
	{
		return machine->Work(psamples, numsamples, mode);
	}*/
	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode)
	{
        bool ret=false;
        int mode2=mode;

		float pini[256*2*2];
		float pouti[256*2*2];
		if (mode2&zzub_process_mode_read)
			s2i(pini, pin, numsamples); // stereo to interleaved
		if (mode2&zzub_process_mode_write)
			s2i(pouti, pout, numsamples);

        if (channels==1) {
            float buffer[256*2*2];
            memset(buffer, 0, 256*2*2*sizeof(float));
			if (mode2&zzub_process_mode_read) {
				if (machineInfo->flags&zzub_plugin_flag_mono_to_stereo)
					CopyStereoToMono(buffer, pini, numsamples, 0x8000); else
					CopyStereoToMono(buffer, pini, numsamples, 0x4000); // halve output volume for mono processing since mono->stereo makes a louder mix
			}
            if (machineInfo->flags&zzub_plugin_flag_mono_to_stereo) {
                if (mode2&zzub_process_mode_read)
                    Amp(pouti, numsamples*2, 0x8000);
                ret=machine->WorkMonoToStereo(buffer, pouti, numsamples, mode2);
            } else {
    		    ret=machine->Work(buffer, numsamples, mode2);
                if (ret)
                    CopyM2S(pouti, buffer, numsamples, 1.0f);
            }
        } else {
            if (mode2&zzub_process_mode_read) {
                Amp(pini, numsamples*2, 0x8000);
                Amp(pouti, numsamples*2, 0x8000);
            }
		    ret=machine->WorkMonoToStereo(pini, pouti, numsamples, mode2);
        }
        if (ret) {
            Amp(pouti, numsamples*2, 1.0f / 0x8000);
			i2s(pout, pouti, numsamples); // interleaved to stereo
		}
        return ret;
	}
	virtual void stop()
	{
		machine->Stop();
	}
	virtual void save(archive *arc)
	{
		machine->Save(&CMachineDataOutputWrap(arc->get_outstream("")));
	}
	virtual void attributes_changed()
	{
		machine->AttributesChanged();
	}
	virtual void command(int index)
	{
		machine->Command(index);
	}
	virtual void set_track_count(int count)
	{
		machine->SetNumTracks(count);
	}
	virtual void mute_track(int index)
	{
		machine->MuteTrack(index);
	}
	virtual bool is_track_muted(int index) const
	{ 
		return machine->IsTrackMuted(index); 
	}
	virtual void midi_note(int channel, int value, int velocity)
	{
		// support hacked jumping on midi notes, such as BTDSys LiveJumpHACK
		int last_play_position = unhack::hackseq->songPos;
		machine->MidiNote(channel, value, velocity);
		if (last_play_position != unhack::hackseq->songPos)
			_host->set_play_position(unhack::hackseq->songPos);
	}
	virtual void event(unsigned int data)
	{
		machine->Event(data);
	}
	virtual const char * describe_value(int param, int value)
	{
		return machine->DescribeValue(param, value); 
	}
	virtual const envelope_info ** get_envelope_infos()
	{
		return reinterpret_cast<const envelope_info**>(machine->GetEnvelopeInfos());
	}
	virtual bool play_wave(int wave, int note, float volume)
	{ 
		return machine->PlayWave(wave, note, volume); 
	}
	virtual void stop_wave()
	{
		machine->StopWave();
	}
	virtual int get_wave_envelope_play_position(int env)
	{ 
		return machine->GetWaveEnvPlayPos(env);
	}
	
	// CMICallbacks implementations

	virtual CWaveInfo const *GetWave(const int i)
	{
		return reinterpret_cast<CWaveInfo const*>(_host->get_wave(i));
	}
	virtual CWaveLevel const *GetWaveLevel(const int i, const int level)
	{
		return reinterpret_cast<CWaveLevel const*>(_host->get_wave_level(i, level));
	}
	virtual void MessageBox(char const *txt)
	{
		_host->message(txt);
	}
	virtual void Lock() { _host->lock(); }
	virtual void Unlock() { _host->unlock(); }
	virtual int GetWritePos() { return _host->get_write_position(); }
	virtual int GetPlayPos() { return _host->get_play_position(); }
	virtual float *GetAuxBuffer() { return _host->get_auxiliary_buffer()[0]; }
	virtual void ClearAuxBuffer() { _host->clear_auxiliary_buffer(); }
	virtual int GetFreeWave() { return _host->get_next_free_wave_index(); }
	virtual bool AllocateWave(int const i, int const size, char const *name) { return _host->allocate_wave(i,0,size,wave_buffer_type_si16,false,name); }
	virtual void ScheduleEvent(int const time, dword const data) { _host->schedule_event(time,data); }
	virtual void MidiOut(int const dev, dword const data) { _host->midi_out(dev, data); }
	virtual short const *GetOscillatorTable(int const waveform) { return _host->get_oscillator_table(waveform);}

// incredibly odd - raverb and some other jeskola machines requires this to run =)
// we do not keep the value though, it may haunt us later. both raverb and the host keep their own static copies of this value
// the value seems to be combined from getlocaltime, getsystemtime, gettimezoneinfo and more.
/*
	from buzz.exe disassembly of GetEnvSize implementation:
	00425028 69 C0 93 B1 39 3E imul        eax,eax,3E39B193h 
	0042502E 05 3B 30 00 00   add         eax,303Bh 
	00425033 25 FF FF FF 7F   and         eax,7FFFFFFFh 
	00425038 A3 F0 26 4D 00   mov         dword ptr ds:[004D26F0h],eax 
*/

    virtual int GetEnvSize(int const wave, int const env) { 
		if (wave<0) {
			return ((wave*0x3E39B193) + 0x303b ) & 0x7FFFFFFF;
		}
		return _host->get_envelope_size(wave,env); 
	}
	virtual bool GetEnvPoint(int const wave, int const env, int const i, word &x, word &y, int &flags)
	{ return _host->get_envelope_point(wave, env, i, x, y, flags); }
	virtual CWaveLevel const *GetNearestWaveLevel(int const i, int const note)
	{
		if (i==-1 && note==-1) {
			return (CWaveLevel*)(implementation=new CMDKImplementation());
		}

		return reinterpret_cast<CWaveLevel const*>(_host->get_nearest_wave_level(i,note));
	}
	virtual void SetNumberOfTracks(int const n) { _host->set_track_count(n); }
	virtual CPattern *CreatePattern(char const *name, int const length)
	{ return reinterpret_cast<CPattern*>(_host->create_pattern(name, length)); }
	virtual CPattern *GetPattern(int const index) { return reinterpret_cast<CPattern*>(_host->get_pattern(index)); }
	virtual char const *GetPatternName(CPattern *ppat) { return _host->get_pattern_name(reinterpret_cast<zzub::pattern*>(ppat)); }
	virtual void RenamePattern(char const *oldname, char const *newname)
	{ _host->rename_pattern(oldname, newname); }
	virtual void DeletePattern(CPattern *ppat)
	{ _host->delete_pattern(reinterpret_cast<zzub::pattern*>(ppat)); }
	virtual int GetPatternData(CPattern *ppat, int const row, int const group, int const track, int const field)
	{ return _host->get_pattern_data(reinterpret_cast<zzub::pattern*>(ppat), row, group, track, field); }
	virtual void SetPatternData(CPattern *ppat, int const row, int const group, int const track, int const field, int const value)
	{ _host->set_pattern_data(reinterpret_cast<zzub::pattern*>(ppat), row, group, track, field, value); }
	virtual CSequence *CreateSequence() { return reinterpret_cast<CSequence*>(_host->create_sequence()); }
	virtual void DeleteSequence(CSequence *pseq) { _host->delete_sequence(reinterpret_cast<zzub::sequence*>(pseq)); }
	virtual CPattern *GetSequenceData(int const row) { return reinterpret_cast<CPattern*>(_host->get_sequence_data(row)); }
	virtual void SetSequenceData(int const row, CPattern *ppat) { _host->set_sequence_data(row, reinterpret_cast<zzub::pattern*>(ppat)); }
	virtual void SetMachineInterfaceEx(CMachineInterfaceEx *pex) { 
		printf("SetMachineInterfaceEx\n"); /* TODO */ 
		this->machine2=pex;
		//_host->set_plugin2(this);
	}
	virtual void ControlChange__obsolete__(int group, int track, int param, int value)
	{ _host->_legacy_control_change(group, track, param, value); }
	virtual int ADGetnumChannels(bool input) {
		MessageBox("ADGetnumChannels not implemented");
		return 0;
		//return _host->audio_driver_get_channel_count(input); 
	}
	virtual void ADWrite(int channel, float *psamples, int numsamples) {
		MessageBox("ADWrite not implemented");
		//_host->audio_driver_write(channel, psamples, numsamples); }
	}
	virtual void ADRead(int channel, float *psamples, int numsamples) {
		MessageBox("ADRead not implemented");
		//_host->audio_driver_read(channel, psamples, numsamples); 
	}

	virtual CMachine *GetThisMachine() { return reinterpret_cast<CMachine*>(_host->get_metaplugin()); }
	 // set value of parameter (group & 16 == don't record)
	virtual void ControlChange(CMachine *pmac, int group, int track, int param, int value) {
		bool record = true;
		bool immediate = false;
		if ((group & 0x10) == 0x10) {
			record = false;
			group ^= (group & 0x10);
		}
		_host->control_change(reinterpret_cast<zzub::metaplugin*>(pmac), group, track, param, value, record, immediate);
	}
	virtual CSequence *GetPlayingSequence(CMachine *pmac)
	{ return reinterpret_cast<CSequence*>(_host->get_playing_sequence(reinterpret_cast<zzub::metaplugin*>(pmac))); }
	virtual void *GetPlayingRow(CSequence *pseq, int group, int track)
	{ return _host->get_playing_row(reinterpret_cast<zzub::sequence*>(pseq), group, track); }

	virtual int GetStateFlags() { return _host->get_state_flags(); }

	virtual void SetnumOutputChannels(CMachine *pmac, int n) { 
		if (implementation) {
			CMDKMachineInterface* mdki=(CMDKMachineInterface*)machine;
			mdki->SetOutputMode(n==2);
			//implementation->SetOutputMode(n==2);
		}
        //assert(reinterpret_cast<zzub::metaplugin*>(pmac) == machine);
        this->channels=n;
		//_host->set_output_channel_count(reinterpret_cast<zzub::metaplugin*>(pmac), n); 
	}

    struct event_wrap {
        BEventType et;
        EVENT_HANDLER_PTR p;
        void* param;
    };

    std::vector<event_wrap> events;

    virtual bool invoke(zzub_event_data_t& data) {
		switch (data.type) {
			case event_type_pre_set_tracks:
				pre_set_tracks_event();
				break;
			case event_type_set_tracks:
				post_set_tracks_event();
				break;
			case event_type_pre_connect:
				pre_add_input_event();
				break;
			case event_type_connect:
				post_add_input_event();
				break;
			case event_type_pre_disconnect:
				pre_delete_input_event();
				break;
			case event_type_disconnect:
				post_delete_input_event();
				break;
		}

		for (size_t i=0; i<events.size(); i++) {
            if (events[i].et==data.type) {
                EVENT_HANDLER_PTR evptr=events[i].p;
                return (machine->*evptr)(events[i].param);
            }
        }
        return false;
    }

	virtual void SetEventHandler(CMachine *pmac, BEventType et, EVENT_HANDLER_PTR p, void *param)
	{
		if (events.size() == 0)
			_host->set_event_handler(reinterpret_cast<zzub::metaplugin*>(pmac), this);
        event_wrap ew={et, p, param};
        events.push_back(ew);
		//real_event_handler = p;
//		_host->set_event_handler(reinterpret_cast<zzub::metaplugin*>(pmac), (zzub::event_type)et, (zzub::event_handler_method)&buzz2zzub::plugin::on_event, param);
	}

	virtual char const *GetWaveName(int const i) { return _host->get_wave_name(i); }

	virtual void SetInternalWaveName(CMachine *pmac, int const i, char const *name)
	{ _host->set_internal_wave_name(reinterpret_cast<zzub::metaplugin*>(pmac), i, name); }

	virtual void GetMachineNames(CMachineDataOutput *pout)
	{ _host->get_plugin_names(&outstreamwrap(pout)); }
	
	virtual CMachine *GetMachine(char const *name) { return reinterpret_cast<CMachine*>(_host->get_metaplugin(name)); }

	std::list<CMachineInfo *> machineinfos;
	
	virtual CMachineInfo const *GetMachineInfo(CMachine *pmac) {		
		const zzub::info *_info = _host->get_info(reinterpret_cast<zzub::metaplugin*>(pmac));
		if (!_info) return 0;	// could happen after deleting a peer controlled machine
		
		CMachineInfo *buzzinfo = new CMachineInfo();
		machineinfos.push_back(buzzinfo);
		buzzinfo->Type = _info->type;
		buzzinfo->Version = _info->version;
		buzzinfo->Flags = _info->flags;
		buzzinfo->minTracks = _info->min_tracks;
		buzzinfo->maxTracks = _info->max_tracks;
		buzzinfo->numGlobalParameters = _info->global_parameters.size();
		buzzinfo->numTrackParameters = _info->track_parameters.size();
		const CMachineParameter **param = new const CMachineParameter *[buzzinfo->numGlobalParameters+buzzinfo->numTrackParameters];
		buzzinfo->Parameters = param;
		for (int i=0; i < buzzinfo->numGlobalParameters; ++i) {
			*param = (const CMachineParameter *)_info->global_parameters[i];
			param++;
		}
		for (int i=0; i < buzzinfo->numTrackParameters; ++i) {
			*param = (const CMachineParameter *)_info->track_parameters[i];
			param++;
		}
		buzzinfo->numAttributes = _info->attributes.size();
		buzzinfo->Attributes = (const CMachineAttribute **)&_info->attributes[0];
		buzzinfo->Name = _info->name;
		buzzinfo->ShortName = _info->short_name;
		buzzinfo->Author = _info->author;
		buzzinfo->Commands = _info->commands;
		buzzinfo->pLI = (CLibInterface*)_info->plugin_lib;

		return buzzinfo;
	}
	virtual char const *GetMachineName(CMachine *pmac)
	{ return _host->get_name(reinterpret_cast<zzub::metaplugin*>(pmac)); }

	virtual bool GetInput(int index, float *psamples, int numsamples, bool stereo, float *extrabuffer)
	{ return _host->get_input(index, psamples, numsamples, stereo, extrabuffer); }


	// plugin2
	virtual const char* describe_param(int param) { 
		if (!machine2) return 0;
		return machine2->DescribeParam(param); 
	}
	virtual bool set_instrument(const char *name) { 
		if (!machine2) return false;
		machine2->SetInstrument(name);
		return false; 
	}
	virtual void get_sub_menu(int index, outstream *os) {
		if (!machine2) return ;
		CMachineDataOutputWrap mdow(os);
		machine2->GetSubMenu(index, &mdow);
	}
	virtual void add_input(const char *name) {
		if (!machine2) return;
		machine2->AddInput(name, true);
	}
	virtual void delete_input(const char *name) {
		if (!machine2) return ;
		machine2->DeleteInput(name);
	}
	virtual void rename_input(const char *oldname, const char *newname) { 
		if (!machine2) return;
		machine2->RenameInput(oldname, newname);
	}
	virtual void input(float **samples, int size, float amp) {
		if (!machine2) return ;
        // always stereo input
        if (samples!=0) {
            float buffer[256*2*2];
			s2i(buffer,samples,size);
            Amp(buffer, size*2, 0x8000);
    		machine2->Input(buffer, size, amp);
        } else
            machine2->Input(0,0,0);
	}
	virtual void midi_control_change(int ctrl, int channel, int value) {
		if (!machine2) return ;
		machine2->MidiControlChange(ctrl, channel, value);
	}/*
	virtual void set_input_channels(char const *macname, bool stereo) {
		if (!machine2)
			return zzub::plugin::set_input_channels(macname, stereo);
		machine2->SetInputChannels(macname, stereo);
	}*/
	virtual bool handle_input(int index, int amp, int pan) { 
		if (!machine2) return false;
		return machine2->HandleInput(index, amp, pan);
	}

	void evil_lock() {

		metaplugin* thisplugin = _host->get_metaplugin();
		player* thisplayer = thisplugin->player;

		thisplayer->playerLock.lock();

		// setting thisplayer->workStarted to false tells the player to execute editing operations on the current thread
		thisplayer->workStarted = false;
	}

	void evil_unlock() {

		metaplugin* thisplugin = _host->get_metaplugin();
		player* thisplayer = thisplugin->player;

		thisplayer->workStarted = true;
		thisplayer->playerLock.unlock();
	}

	void pre_add_input_event() {
		if (isLdmixer || isVocoderXp) evil_lock();
	}

	void post_add_input_event() {
		if (isLdmixer || isVocoderXp) evil_unlock();
	}

	void pre_delete_input_event() { }
	void post_delete_input_event() { }

	void pre_set_tracks_event() {
		if (isUtrk) evil_lock();
	}

	void post_set_tracks_event() {
		if (isUtrk) evil_unlock();
	}
};


}

typedef CMachineInfo const *(__cdecl *GET_INFO)();
typedef CMachineInterface *(__cdecl *CREATE_MACHINE)();


struct buzzmachineinfo : zzub::info {
	buzzmachineinfo(const CMachineInfo *buzzinfo) {
	}
	
	virtual zzub::plugin* create_plugin() const {
		return 0;
	}
	virtual bool store_info(zzub::archive *arc) const { return false; }
};

struct libwrap : public zzub::lib {
	CLibInterface* blib;

	libwrap(CLibInterface* mlib) {
		blib=mlib;
	}

	virtual void get_instrument_list(zzub::outstream* os)  {
		if (!blib) return ;

		buzz2zzub::CMachineDataOutputWrap mdow(os);
		blib->GetInstrumentList(&mdow);
	}
};

struct buzzplugininfo : zzub::info
{
	std::string m_uri;
	std::string m_name;
	std::string m_path;
	HINSTANCE hDllInstance;
	GET_INFO GetInfo;
	CREATE_MACHINE CreateMachine;
	
	buzzplugininfo()
	{
		hDllInstance = 0;
		GetInfo = 0;
		CreateMachine = 0;
	}
	
	void detach()
	{
		if (hDllInstance)
		{
            unhack::freeLibrary(hDllInstance);
			hDllInstance = 0;
			GetInfo = 0;
			CreateMachine = 0;
		}		
	}
	
	virtual zzub::plugin* create_plugin() const {
		CMachineInterface* machine = CreateMachine();
		if (machine)
		{
			return new buzz2zzub::plugin(machine, this);
		}
		return 0;
	}
	
	virtual bool store_info(zzub::archive *arc) const {
		return false;
	}
	
	bool init()
	{
		assert (!hDllInstance);

        hDllInstance = unhack::loadLibrary(m_path.c_str());
        //hDllInstance = LoadLibrary(m_path.c_str());

		if (!hDllInstance) {
			printf("%s: LoadLibrary failed.\n", m_path.c_str());
			return false;
		}
        GetInfo = (GET_INFO)unhack::getProcAddress(hDllInstance, "GetInfo");
//		GetInfo = (GET_INFO)GetProcAddress(hDllInstance, "GetInfo");
		if (!GetInfo) {
			printf("%s: missing GetInfo.\n", m_path.c_str());
			return false;
		}
		
        CreateMachine = (CREATE_MACHINE)unhack::getProcAddress(hDllInstance, "CreateMachine");
//		CreateMachine = (CREATE_MACHINE)GetProcAddress(hDllInstance, "CreateMachine");
		if (!CreateMachine) {
			printf("%s: missing CreateMachine.\n", m_path.c_str());
			return false;
		}
		
		m_uri="@zzub.org/buzz2zzub/" + m_name;
		replace(m_uri.begin(), m_uri.end(), ' ', '+');
		uri = m_uri.c_str();
		
		const CMachineInfo *buzzinfo = GetInfo();

		type = buzzinfo->Type;
		version = buzzinfo->Version;
		flags = buzzinfo->Flags;
		min_tracks = buzzinfo->minTracks;
		max_tracks = buzzinfo->maxTracks;
		for (int i = 0; i < buzzinfo->numGlobalParameters; ++i) {
            zzub::parameter& param=add_global_parameter();
            param = *(const zzub::parameter *)buzzinfo->Parameters[i];
		}
		for (int i = 0; i < buzzinfo->numTrackParameters; ++i) {
            zzub::parameter& param=add_track_parameter();
            param = *(const zzub::parameter *)buzzinfo->Parameters[buzzinfo->numGlobalParameters+i];
		}
		for (int i = 0; i < buzzinfo->numAttributes; ++i) {
            zzub::attribute& attr=add_attribute();
            attr = *(const zzub::attribute *)buzzinfo->Attributes[i];
		}
		name = buzzinfo->Name;
		short_name = buzzinfo->ShortName;
		author = buzzinfo->Author;
		commands = buzzinfo->Commands;
		if (buzzinfo->pLI)
			plugin_lib = new libwrap(buzzinfo->pLI);

		return true;
	}
};

std::vector<buzzplugininfo *> g_buzzplugins;

struct buzzplugincollection : zzub::plugincollection {
	// Called by the host initially. The collection registers
	// plugins through the pluginfactory::register_info method.
	// The factory pointer remains valid and can be stored
	// for later reference.
	virtual void initialize(zzub::pluginfactory *factory) {
		for (std::vector<buzzplugininfo *>::iterator i = g_buzzplugins.begin(); i != g_buzzplugins.end(); ++i) {
			const zzub::info *_info = *i;
			factory->register_info(_info);
		}
	}

	// Called by the host upon song loading. If the collection
	// can not provide a plugin info based on the uri or
	// the metainfo passed, it should return a null pointer.
	virtual const zzub::info *get_info(const char *uri, zzub::archive *arc) { return 0; }
	
	// Called by the host upon destruction. You should
	// delete the instance in this function
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
	return new buzzplugincollection();
}

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }

HMODULE g_hModule;

void load_plugins(const char *relpath) {
	char modulename[MAX_PATH];
	GetModuleFileName(g_hModule, modulename, MAX_PATH);
	*strrchr(modulename,'\\') = '\0';
	strcat(modulename, "\\");
	strcat(modulename, relpath);
	strcat(modulename, "\\");
	char searchpath[MAX_PATH];
	strcpy(searchpath, modulename);
	strcat(searchpath, "*.dll");
	
	printf("buzz2zzub: searching folder %s...\n", modulename);
	WIN32_FIND_DATA fd;
	HANDLE hFind=FindFirstFile(searchpath, &fd);

	while (hFind!=INVALID_HANDLE_VALUE) {
		
		if ( (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)==0) {
			char fullfilepath[MAX_PATH];
			strcpy(fullfilepath, modulename);
			strcat(fullfilepath, fd.cFileName);					
			char name[MAX_PATH];
			strcpy(name, fd.cFileName);
			buzzplugininfo *i = new buzzplugininfo();
			*strrchr(name, '.') = '\0';
			i->m_name = name;
			i->m_path = fullfilepath;
			printf("buzz2zzub: adding %s (%s)\n", name, fullfilepath);
			if (i->init())
				g_buzzplugins.push_back(i);
			else
			{
				i->detach();
				delete i;
			}
		}

		if (!FindNextFile(hFind, &fd)) break;
	}
	FindClose(hFind);

}

BOOL WINAPI DllMain( HMODULE hModule, DWORD fdwreason, LPVOID lpReserved )
{
	switch(fdwreason)
	{
		case DLL_PROCESS_ATTACH: {
            DSP_Init(44100);
			
			g_hModule = hModule;

            unhack::enablePatch("Polac VST 1.1", "seq");
            unhack::enablePatch("Polac VST 1.1", "bpm");
            unhack::enablePatch("Polac VSTi 1.1", "seq");
            unhack::enablePatch("Polac VSTi 1.1", "bpm");
            unhack::enablePatch("Polac Metronome", "seq");
            unhack::enablePatch("P. DooM's HACK Msync", "bpm");
            unhack::enablePatch("P. DooM's HACK Msync", "seq");
            unhack::enablePatch("P. DooM's HACK Jump", "seq");
            unhack::enablePatch("BuzzInAMovie", "seq");
            unhack::enablePatch("BuzzInAMovie", "bpm");
            unhack::enablePatch("2ndPLoopJumpHACK", "seq");
            unhack::enablePatch("ld mixer", "seq");
            unhack::enablePatch("ld mixer", "midi");
            unhack::enablePatch("Rebirth MIDI 2", "midi");
			unhack::enablePatch("BTDSys LiveJumpHACK", "seq");
			unhack::enablePatch("BTDSys LiveJumpHACK", "bpm");
            unhack::enablePatch("BTDSys PeerCtrl", "bpm");
            unhack::enablePatch("BTDSys PeerCtrl", "seq");
            unhack::enablePatch("BTDSys PeerCtrl 'Basic'", "bpm");
            unhack::enablePatch("BTDSys PeerCtrl 'Basic'", "seq");
            unhack::enablePatch("Geoffroy TapTempoHACK", "bpm");
            unhack::enablePatch("Geoffroy TapTempoHACK", "seq");
            unhack::enablePatch("Polac Metronome", "seq");
            unhack::enablePatch("Fuzzpilz Inp", "seq");
            unhack::enablePatch("rewirelink", "seq");

            unhack::enablePatch("BuzzRizer", "replace(80000000000000000F000000,80000000020000000F000000)");
            unhack::enablePatch("Geonik's 2p Filter", "replace(CEFFFFFF010000000C000000,CEFFFFFF020000000C000000)");
            unhack::enablePatch("Geonik's Expression 2", "replace(CEFFFFFF010000000C000000,CEFFFFFF020000000C000000)");
            unhack::enablePatch("HD Monster_Kick", "replace(020000000C000000,010000000C000000)");

			load_plugins("buzz");
			load_plugins("..\\generators");
			load_plugins("..\\effects");
		} break;
		case DLL_PROCESS_DETACH:
		{
			std::vector<buzzplugininfo *>::iterator i;
			for (i = g_buzzplugins.begin(); i != g_buzzplugins.end(); ++i)
			{
				(*i)->detach();
				delete *i;
			}
			g_buzzplugins.clear();
		} break;
		default:
			break;
	}

	return TRUE;
}
