// buzz2zzub plugin adapter
// Copyright (C) 2006 Leonard Ritter (contact@leonard-ritter.com)
// Copyright (C) 2006-2007 Anders Ervik <calvin@countzero.no>
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
#include <cassert>
#include <vector>
#include <iostream>
#include <list>
#include <algorithm>
#include <map>
#include <sstream>
#include <fstream>
#include <ctime>
#include <string>

static const int WM_CUSTOMVIEW_CREATE = WM_USER+4;
static const int WM_CUSTOMVIEW_SET_CHILD = WM_USER+5;
static const int WM_CUSTOMVIEW_GET = WM_USER+6;
static const int WM_CUSTOMVIEW_FOCUS = WM_USER+7;
static const int WM_GET_THEME = WM_USER+8;

// WM_CUSTOMVIEW_CREATE
//	wParam zzub_plugin_t*
//	lParam label of view to open.
// if a view with the specified label exists, the method fails
// WM_CUSTOMVIEW_GET
//	wParam zzub_plugin_t*
//	lParam label of view to open.
// return the view with the specified label or NULL
// WM_CUSTOMVIEW_FOCUS
//	wParam zzub_plugin_t*
//	lParam label of view to open.
// return the view with the specified label or NULL
// WM_CUSTOMVIEW_SET_CHILD
//	wParam hCustomView
//	lParam hPluginWnd
// return the view with the specified label or NULL

#ifdef min
	#undef min
#endif

#ifdef max
	#undef max
#endif

#define _USE_MATH_DEFINES
#include <cmath>
#include "inifile.h"

#define __BUZZ2ZZUB__
#include <zzub/signature.h>
#include "zzub/plugin.h"
#include "MachineInterface.h"
#include "mdk.h"
#include "mdkimpl.h"
#include "dsplib.h"
#include "unhack.h"

#define PLUGIN_FLAGS_MASK (zzub_plugin_flag_is_root|zzub_plugin_flag_has_audio_input|zzub_plugin_flag_has_audio_output|zzub_plugin_flag_has_event_output)
#define ROOT_PLUGIN_FLAGS (zzub_plugin_flag_is_root|zzub_plugin_flag_has_audio_input)
#define GENERATOR_PLUGIN_FLAGS (zzub_plugin_flag_has_audio_output)
#define EFFECT_PLUGIN_FLAGS (zzub_plugin_flag_has_audio_input|zzub_plugin_flag_has_audio_output)
#define CONTROLLER_PLUGIN_FLAGS (zzub_plugin_flag_has_event_output)

using std::cout;
using std::cerr;
using std::endl;
using std::map;
using std::string;
using std::vector;

using namespace zzub;

void CopyM2S(float *pout, float *pin, int numsamples, float amp);

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


struct plugintools {
	static HMODULE hModule;  // set this in DLL_PROCESS_ATTACH

	static std::string getPluginPath() {
		if (!hModule) return "";

		char modulename[MAX_PATH];
		GetModuleFileName(hModule, modulename, MAX_PATH);
		std::string result = modulename;
		size_t ls = result.find_last_of("\\/");
		return result.substr(0, ls + 1);
	}
};

void conformParameter(zzub::parameter& param) {
	int in = std::min(param.value_min, param.value_max);
	int ax = std::max(param.value_min, param.value_max);
	param.value_min = in;
	param.value_max = ax;

	if (param.type == zzub::parameter_type_switch) {
		param.value_min = zzub::switch_value_off;
		param.value_max = zzub::switch_value_on;
		param.value_none = zzub::switch_value_none;
	} else
	if (param.type == zzub::parameter_type_note) {
		param.value_min = zzub::note_value_min;
		param.value_max = zzub::note_value_max;
		param.value_none = zzub::note_value_none;
	}
}

class CMachine {
public:
	// Jeskola Buzz compatible CMachine header.
	// Some machines look up these by reading directly from zzub::metaplugin memory.

	char _placeholder[16];
	char* _internal_name;					// 0x14: polac's VST reads this string, set to 0
	char _placeholder2[52];
	void* _internal_machine;				// pointer to CMachine*, scanned for by some plugins
	void* _internal_machine_ex;				// 0x50: same as above, but is not scanned for
	char _placeholder3[20];
	char* _internal_global_state;			// 0x68: copy of machines global state
	char* _internal_track_state;			// 0x6C: copy of machines track state
	char _placeholder4[120];
	int _internal_seqCommand;				// 0xE8: used by mooter, 0 = --, 1 = mute, 2 = thru
	char _placeholder6[17];
	bool hardMuted;							// 0xFD: true when muted by user, used by mooter

	// End of Buzz compatible header

	zzub_plugin_t* plugin;
	CMachineInfo* buzzinfo;

	CMachine() {
		plugin = 0;
		buzzinfo = 0;
	}
	virtual ~CMachine() { }

	static bool checkBuzzCompatibility() {
		// check offsets that may be used for known hacks
		int nameofs = offsetof(CMachine, _internal_name);			// 0x14 / 0x18 (+/- vtbl)
		int exofs = offsetof(CMachine, _internal_machine_ex);		// 0x50
		int gstateofs = offsetof(CMachine, _internal_global_state);	// 0x68
		int tstateofs = offsetof(CMachine, _internal_track_state);	// 0x6c
		//int xofs = offsetof(CMachine, x);							// 0xa8
		//int yofs = offsetof(CMachine, y);							// 0xac
		int seqcmdofs = offsetof(CMachine, _internal_seqCommand); // 0xe8
		int hardmuteofs = offsetof(CMachine, hardMuted);			// 0xfd

		if (exofs != 0x50) return false;
		if (gstateofs != 0x68) return false;
		if (tstateofs != 0x6c) return false;
		
		if (seqcmdofs != 0xe8) return false;
		if (hardmuteofs != 0xfd) return false;
		return true;
	}
};

namespace buzz2zzub {

struct plugin;
struct buzzplugininfo;

typedef CMachineInfo const *(__cdecl *GET_INFO)();
typedef CMachineInterface *(__cdecl *CREATE_MACHINE)();

const int OSCTABSIZE = (2048+1024+512+256+128+64+32+16+8+4)*sizeof(short);

short oscTables[8][OSCTABSIZE];

double square(double v) {
		double sqmod=fmod(v, 2.0f*M_PI);
		return sqmod<M_PI?-1:1;
	}

double sawtooth(double v) {
	return (fmod(v, 2.0f*M_PI) / M_PI)-1;
}

double triangle(double v) {
	double sqmod=fmod(v, 2.0f*M_PI);

	if (sqmod<M_PI) {
		return sqmod/M_PI;
	} else
		return (M_PI-(sqmod-M_PI)) / M_PI;
}

void generate_oscillator_tables() {
	int tabSize = 2048;
	srand(static_cast<unsigned int>(time(0)));
	for (int tabLevel = 0; tabLevel < 11; tabLevel++) {
		int tabOfs = GetOscTblOffset(tabLevel);
		for (int i = 0; i < tabSize; i++) {
			double dx = (double)i/tabSize;
			oscTables[OWF_SINE][tabOfs+i] = (short)(sin(dx*2.0f*M_PI)*32000);
			oscTables[OWF_SAWTOOTH][tabOfs+i] = (short)(sawtooth(dx*2.0f*M_PI)*32000);
			oscTables[OWF_PULSE][tabOfs+i] = (short)(square(dx*2.0f*M_PI)*32000);
			oscTables[OWF_TRIANGLE][tabOfs+i] = (short)(triangle(dx*2.0f*M_PI)*32000);
			oscTables[OWF_NOISE][tabOfs+i] = (short) (((float)rand()/(float)RAND_MAX)*64000.f - 32000);
			oscTables[OWF_303_SAWTOOTH][tabOfs+i] = (short)(sawtooth(dx*2.0f*M_PI)*32000);
			oscTables[6][tabOfs+i] = (short)(sin(dx*2.0f*M_PI)*32000);
		}
		tabSize/=2;
	}
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

struct libwrap : public zzub::lib {
	CLibInterface* blib;
	buzzplugininfo* info;

	libwrap(CLibInterface* mlib, buzzplugininfo* _info) ;

	virtual void get_instrument_list(zzub::outstream* os);
};

class CWaveLevelImpl : public CWaveLevel {
};

class CWaveInfoImpl : public CWaveInfo {
	std::string name;
	std::vector<CWaveLevelImpl> wave_levels;
};

class CMachineManager {
public:
	std::vector<CWaveInfoImpl> buzz_waves;
	std::map<zzub_plugin_t*, CMachine*> plugin_to_machine;

	CMachine* get(zzub::host* host, zzub_plugin_t* metaplugin);
	CMachine* create(buzz2zzub::plugin* plugin);
	CMachine* create(zzub::plugin* plugin);
	void destroy(zzub_plugin_t* metaplugin);
};

struct buzzplugincollection;

struct buzzplugininfo : zzub::info
{
	buzzplugincollection* plugincollection;
	std::string m_uri;
	std::string m_name;
	std::string m_path;
	int origFlags;
	HINSTANCE hDllInstance;
	GET_INFO GetInfo;
	CREATE_MACHINE CreateMachine;
	bool lockAddInput, lockSetTracks, useSequencerHack;
	CMachineManager* machines;

	buzzplugininfo();
	
	bool attach();
	void detach();
	
	virtual zzub::plugin* create_plugin() const;
	
	virtual bool store_info(zzub::archive *arc) const;
	
	bool init();

};

struct buzzplugincollection : zzub::plugincollection {

	CMachineManager machines;
	std::vector<buzzplugininfo *> buzzplugins;

	buzzplugincollection() {
		DSP_Init(44100);

		load_config();

		load_plugins("buzz");
		load_plugins("..\\generators");
		load_plugins("..\\effects");
	}

	~buzzplugincollection() {
		std::vector<buzzplugininfo *>::iterator i;
		for (i = buzzplugins.begin(); i != buzzplugins.end(); ++i)
		{
			(*i)->detach();
			delete (*i)->plugin_lib;	// buzzplugininfo has no destructor so we free this here
			delete *i;
		}
		buzzplugins.clear();
	}

	// Called by the host initially. The collection registers
	// plugins through the pluginfactory::register_info method.
	// The factory pointer remains valid and can be stored
	// for later reference.
	virtual void initialize(zzub::pluginfactory *factory) {
		for (std::vector<buzzplugininfo *>::iterator i = buzzplugins.begin(); i != buzzplugins.end(); ++i) {
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

	void load_config() {
		std::string configPath = plugintools::getPluginPath() + "buzz2zzub.ini";
		ini::file inifile(configPath);
		int patchCount = inifile.section("Patches").get("Count", 0);
		for (int i = 0; i < patchCount; i++) {
			std::stringstream patchName;
			patchName << "Patch" << i;
			std::string patchString = inifile.section("Patches").get<std::string>(patchName.str(), ""); 
			if (patchString.empty()) continue;
			size_t fe = patchString.find_first_of(':');
			if (fe == std::string::npos) continue;
			std::string dllName = patchString.substr(0, fe);
			std::string patchCommand = patchString.substr(fe+1);
			unhack::enablePatch(dllName, patchCommand);
			cout << "Read patch from ini: " << dllName << " -> '" << patchCommand << "'" << endl;
		}
	}

	bool attach(const buzzplugininfo* self) {
		std::vector<buzzplugininfo*>::iterator i = find(buzzplugins.begin(), buzzplugins.end(), self);
		if (i == buzzplugins.end()) return false;
		return (*i)->attach();
	}

	map<std::string, buzzplugininfo*> plugin_cache;

	bool load_cache(std::string pluginPath) {

		std::string cacheFile = pluginPath + "buzz2zzub.dat";

		cerr << "buzz2zzub: loading cache " << cacheFile << endl;
		std::ifstream f;
		f.open(cacheFile.c_str());
		if (!f.good()) return false;
		
		std::string version;
		std::getline(f, version);

		if (version != "buzz2zzub-cache-version-1") return false;

		while (!f.eof() && !f.fail()) {
			std::string dll_name, name;
			std::string flagstring;
			int flags;
			std::string short_name;
			std::getline(f, dll_name);
			std::getline(f, name);
			std::getline(f, flagstring);
			flags = atoi(flagstring.c_str());
			std::getline(f, short_name);

			if (name.empty() || short_name.empty()) continue;

			cout << "Read cached plugin: " << name << ", flags=" << flags << ", short=" << short_name << endl;
			buzzplugininfo* i = new buzzplugininfo();
			i->plugincollection = this;
			i->machines = &machines;
			i->m_name = dll_name;
			i->m_path = pluginPath + dll_name + ".dll";
			i->init();

			i->name = name;
			i->flags = flags;
			i->short_name = short_name;

			if (flags & plugin_flag_uses_lib_interface)
				i->plugin_lib = new libwrap(0, i);
			
			plugin_cache[dll_name] = i;
		}

		f.close();
		return true;
	}

	void enumerate_plugins(std::string pluginPath, std::vector<buzzplugininfo*>& result_plugins) {
		std::string searchPath = pluginPath + "*.dll";

		cout << "buzz2zzub: searching folder " << pluginPath << "..." << endl;
		WIN32_FIND_DATA fd;
		HANDLE hFind = FindFirstFile(searchPath.c_str(), &fd);

		while (hFind != INVALID_HANDLE_VALUE) {
			
			if ( (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)==0) {
				std::string fullFilePath = pluginPath + fd.cFileName;
				std::string name = fd.cFileName;
				size_t ld = name.find_last_of('.');
				name = name.substr(0, ld);

				map<std::string, buzzplugininfo*>::iterator i;
				i = plugin_cache.find(name);
				if (i != plugin_cache.end()) {
					// we know this plugin from before
					buzzplugins.push_back(i->second);
					result_plugins.push_back(i->second);
				} else {
					// first time we see this plugin!
					buzzplugininfo *i = new buzzplugininfo();
					i->plugincollection = this;
					i->machines = &machines;
					i->m_name = name;
					i->m_path = fullFilePath;
					cout << "buzz2zzub: adding " << name << "(" << fullFilePath << ")" << endl;
					if (i->init()) {
						if (i->attach()) {
#if defined(USE_PLUGIN_CACHE)
							i->detach();
#endif
							buzzplugins.push_back(i);
							result_plugins.push_back(i);
						}
					} else {
						i->detach();
						delete i;
					}
				}
			}

			if (!FindNextFile(hFind, &fd)) break;
		}
		FindClose(hFind);
	}

	bool save_cache(std::string cachePath, const std::vector<buzzplugininfo*>& plugins) {
		std::ofstream f;
		f.open(cachePath.c_str());
		if (!f.good()) return false;

		f << "buzz2zzub-cache-version-1" << endl;

		std::vector<buzzplugininfo*>::const_iterator i;
		std::vector<char> bytes(1024);
		for (i = plugins.begin(); i != plugins.end(); ++i) {
			buzzplugininfo& info = **i;
			f << info.m_name << endl;
			f << info.name << endl;
			f << info.flags << endl;
			f << info.short_name << endl;
		}

		f.close();

		return true;
	}

	void load_plugins(const char *relpath) {
		std::string pluginPath = plugintools::getPluginPath() + relpath + "\\";

#if defined(USE_PLUGIN_CACHE)

		if (!load_cache(pluginPath)) {
			cerr << "buzz2zzub: could not load buzz2zzub.dat" << endl;
		}
#endif

		std::vector<buzzplugininfo*> result_plugins;
		enumerate_plugins(pluginPath, result_plugins);

#if defined(USE_PLUGIN_CACHE)

		save_cache(pluginPath + "buzz2zzub.dat", result_plugins);

#endif
	}
};

struct plugin : zzub::plugin, CMICallbacks, zzub::event_handler {

	struct master_events : zzub::event_handler {
		plugin* _plugin;

		virtual bool invoke(zzub_event_data_t& data) {
			switch (data.type) {
				case event_type_pre_set_tracks:
					if (data.set_tracks.plugin == _plugin->_host->get_metaplugin())
						_plugin->pre_set_tracks_event();
					break;
				case event_type_post_set_tracks:
					if (data.set_tracks.plugin == _plugin->_host->get_metaplugin())
						_plugin->post_set_tracks_event();
					break;
				case event_type_pre_connect:
					if (data.connect_plugin.to_plugin == _plugin->_host->get_metaplugin())
						_plugin->pre_add_input_event();
					break;
				case event_type_post_connect:
					if (data.connect_plugin.to_plugin == _plugin->_host->get_metaplugin())
						_plugin->post_add_input_event();
					break;
				case event_type_new_pattern:
					if (data.new_pattern.plugin == _plugin->_host->get_metaplugin())
						_plugin->on_new_pattern(data.new_pattern.index);
					break;
			}
			return false;
		}

	};

	CMachineInterface* machine;
	CMachineInterfaceEx* machine2;
	CMDKImplementation* implementation;
	const struct buzzplugininfo* machineInfo;
	master_events mevents;

	int channels;
	int track_count;
	HWND hWndPatternEditor;
	int pattern_editor_command;
	int first_pattern_editor_sub_command;
	int last_pattern_editor_sub_command;

	DWORD dwThreadID;

	// unhacking wavetable recording:
	struct allocated_wave_level {
		int wave, level;
		CWaveLevel data;
	};
	std::vector<allocated_wave_level> allocated_waves;
	std::vector<CWaveInfo*> local_wavetable;
	
	void parse_commands(string commands, vector<string>& result, int& sub_commands) {
		sub_commands = 0;
		string::iterator last = commands.begin();
		for (;;) {
			string::iterator i = find(last, commands.end(), '\n');
			if (i != commands.end()) {
				result.push_back(string(last, i));
				if (result.back().length() && result.back()[0] == '/') sub_commands++;
				last = i;
				last++;
			} else {
				result.push_back(string(last, commands.end()));
				if (result.back().length() && result.back()[0] == '/') sub_commands++;
				break;
			}

		}
		
	}

	plugin(CMachineInterface* machine, const buzzplugininfo* mi)
	{
		this->implementation = 0;
		this->machine2 = 0;
		this->machine = machine;
		this->global_values = this->machine->GlobalVals;
		this->track_values = this->machine->TrackVals;
		this->attributes = this->machine->AttrVals;
		this->machineInfo = mi;
		channels =1;
		if (mi->origFlags & MIF_PATTERN_EDITOR) {
			int num_sub_commands = 0;
			vector<string> parsed_commands;
			parse_commands(machineInfo->commands, parsed_commands, num_sub_commands);
			pattern_editor_command = parsed_commands.size() - 1;
			first_pattern_editor_sub_command = (num_sub_commands) * 256;
			last_pattern_editor_sub_command = ((num_sub_commands + 1) * 256) - 1;
		} else 
			pattern_editor_command = -1;

		hWndPatternEditor = 0;

		track_count = machineInfo->min_tracks;
		mevents._plugin = this;
		local_wavetable.resize(200);
	}
	~plugin()
	{
		machineInfo->machines->destroy(_host->get_metaplugin());
		if (machineInfo->lockAddInput || machineInfo->lockSetTracks || (machineInfo->origFlags & MIF_PATTERN_EDITOR))
			_host->remove_event_handler(_host->get_metaplugin("Master"), &mevents);	// listen to event_type_pre_xxx-messages sent to the master

		delete this->machine;
	}
	
	// zzub::plugin implementations
	
	virtual void destroy() { delete this; }
	virtual void init(archive *arc)
	{ 
		dwThreadID = GetCurrentThreadId();
		machineInfo->machines->create(this);

		machine->pCB = this;
		machine->pMasterInfo = reinterpret_cast<CMasterInfo*>(_master_info);
		
		zzub_plugin_t* thisplugin = _host->get_metaplugin();

		if (arc)
			machine->Init(&CMachineDataInputWrap(arc->get_instream(""))); 
		else
			machine->Init(0);

		// need event handlers
		if (machineInfo->lockAddInput || machineInfo->lockSetTracks || (machineInfo->origFlags & MIF_PATTERN_EDITOR))
			_host->set_event_handler(_host->get_metaplugin("Master"), &mevents);	// listen to event_type_pre_xxx-messages sent to the master
		c = 0;
	}
	virtual void process_controller_events() {}

	void transfer_hacked_plugin_states() {
		// TODO: transfer current parameter values from state_last to hacked tstate and gstate
		CMachine* m = GetThisMachine();
		
		transfer_track(1, 0, m->_internal_global_state, machineInfo->global_parameters);
		
		char* param_ptr = m->_internal_track_state;
		for (int i = 0; i < track_count; i++) {
			int size = transfer_track(2, i, param_ptr, machineInfo->track_parameters);
			param_ptr += size;

		}
	}
	
	int transfer_track(int group, int track, char* param_ptr, const std::vector<const zzub::parameter*>& params) {
		int size = 0;
		for (int i = 0; i < params.size(); i++) {
			int v = _host->get_parameter(_host->get_metaplugin(), group, track, i);
			int param_size = params[i]->get_bytesize();
			memcpy(param_ptr, &v, param_size);
			param_ptr += param_size;
			size += param_size;
		}
		return size;
	}

	virtual void process_events()
	{
		int last_play_position;
		if (machineInfo->useSequencerHack) {
			unhack::hackTick(_master_info->beats_per_minute, _host->get_song_begin_loop(), 
				_host->get_song_end_loop(), _host->get_song_end(), _host->get_play_position());

			// support hacked jumping from ticks
			last_play_position = _host->get_play_position();
		}

		transfer_hacked_plugin_states();

		machine->Tick();

		// check for hacked jumps
		if (machineInfo->useSequencerHack) {
			if (last_play_position != unhack::hackseq->songPos)
				_host->set_play_position(unhack::hackseq->songPos);
		}

	}
	
	virtual bool process_offline(float **pin, float **pout, int *numsamples, int *channels, int *samplerate) { return false; }

	double c;

	virtual bool process_stereo(float **pin, float **pout, int numsamples, int mode)
	{
		int last_play_position;
		if (machineInfo->useSequencerHack) {
			// support hacked jumping from work()
			unhack::hackTick(_master_info->beats_per_minute, _host->get_song_begin_loop(), 
				_host->get_song_end_loop(), _host->get_song_end(), _host->get_play_position());
			last_play_position = _host->get_play_position();
		}

		bool ret = false;
		int mode2 = mode;

		float pini[256*2*2];
		float pouti[256*2*2];
		if (mode2&zzub_process_mode_read)
			s2i(pini, pin, numsamples); // stereo to interleaved
		if (mode2&zzub_process_mode_write)
			s2i(pouti, pout, numsamples);

		if (channels == 1) {
			float buffer[256*2*2];
			memset(buffer, 0, 256*2*2*sizeof(float));
			if (mode2&zzub_process_mode_read) {
				if (machineInfo->origFlags & MIF_MONO_TO_STEREO)
					CopyStereoToMono(buffer, pini, numsamples, 0x8000); else
					CopyStereoToMono(buffer, pini, numsamples, 0x4000); // halve output volume for mono processing since mono->stereo makes a louder mix
			}
			if (machineInfo->origFlags & MIF_MONO_TO_STEREO) {
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

		// check for hacked jumps
		if (machineInfo->useSequencerHack) {
			if (last_play_position != unhack::hackseq->songPos)
				_host->set_play_position(unhack::hackseq->songPos);
		}
		return ret;
	}
	virtual void stop()
	{
		machine->Stop();
	}
	virtual void load(archive *arc) {}
	virtual void save(archive *arc)
	{
		machine->Save(&CMachineDataOutputWrap(arc->get_outstream("")));
	}
	virtual void attributes_changed()
	{
		machine->AttributesChanged();
	}

	void open_pattern_editor(int index) {
		cout << "Open pattern editor!" << endl;
		host_info* hi = _host->get_host_info();
		if (hi->id == 42 && hi->version == 0x0503) {
			// plugin sier: åpne view med navn "X"
			cout << "hello buzé!" << endl;
			std::string name = _host->get_name(_host->get_metaplugin())
				+ (std::string)" Custom Pattern Editor";

			HWND hCustomView = (HWND)SendMessage((HWND)hi->host_ptr, 
				WM_CUSTOMVIEW_GET, (WPARAM)_host->get_metaplugin(),
				(LPARAM)name.c_str());

			if (!hCustomView) {
				hCustomView = (HWND)SendMessage((HWND)hi->host_ptr, 
					WM_CUSTOMVIEW_CREATE, (WPARAM)_host->get_metaplugin(),
					(LPARAM)name.c_str());

				if (!hCustomView) {
					cerr << "buzz2zzub: could not open custom view in buze" << endl;
					return ;
				}
				
				// only create pattern editor once
				if (!hWndPatternEditor) {
					hWndPatternEditor = (HWND)machine2->CreatePatternEditor(hCustomView);
					cout << "got " << std::hex << (unsigned int)hWndPatternEditor << endl;
				}

				SendMessage((HWND)hi->host_ptr, WM_CUSTOMVIEW_SET_CHILD, 
					(WPARAM)hCustomView, (LPARAM)hWndPatternEditor);
			} else {
				// set focus to existing pattern editor
				SendMessage((HWND)hi->host_ptr, WM_CUSTOMVIEW_FOCUS,
					(WPARAM)_host->get_metaplugin(), (LPARAM)name.c_str());
			}

			machine2->SetEditorPattern((CPattern*)_host->get_pattern(index));
		}
	}

	virtual void command(int index)
	{
		if ((machineInfo->origFlags & MIF_PATTERN_EDITOR) && index >= first_pattern_editor_sub_command && index <= last_pattern_editor_sub_command) {
			int pattern_index = index - first_pattern_editor_sub_command;
			open_pattern_editor(pattern_index);
		} else {
			machine->Command(index);
		}
	}
	virtual void set_track_count(int count)
	{
		track_count = count;
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

	void play_pattern(int index) {
		if (machineInfo->origFlags & MIF_PATTERN_EDITOR) {
			machine2->PlayPattern((CPattern*)_host->get_pattern(index));
		}
	}
	
	// CMICallbacks implementations

	virtual CWaveInfo const *GetWave(const int i)
	{
		CWaveInfo const* w = reinterpret_cast<CWaveInfo const*>(_host->get_wave(i));
		if (w) {
		}
		return w;
	}
	virtual CWaveLevel const *GetWaveLevel(const int i, const int level)
	{
		CWaveLevel const* l = reinterpret_cast<CWaveLevel const*>(_host->get_wave_level(i, level));
		if (l) {
		}
		return l;
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
	virtual bool AllocateWave(int const i, int const size, char const *name) { 
		// TODO: should we use GetThreadId() to determine whether we are in the user thread 
		// or the audio thread...? (allocate_wave_direct vs allocate_wave)
		// TODO: we need to return a fake wave to the plugin and allocate a proper wave
		// in zzub after the plugin messes with the const datas.
		return _host->allocate_wave_direct(i,0,size,wave_buffer_type_si16,false,name); 
	}
	virtual void ScheduleEvent(int const time, dword const data) { 
		MessageBox("ScheduleEvent not implemented");
		//_host->schedule_event(time,data); 
	}
	virtual void MidiOut(int const dev, dword const data) { _host->midi_out(dev, data); }
	virtual short const *GetOscillatorTable(int const waveform) { return oscTables[waveform]; /*return _host->get_oscillator_table(waveform);*/ }

// incredibly odd - raverb and some other jeskola machines require this to run =)
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
		} else
		if (wave == 0 || wave > 200)	// jeskola tracker asks for GetEnvSize(0)
			return 0;
		return _host->get_envelope_size(wave,env); 
	}
	virtual bool GetEnvPoint(int const wave, int const env, int const i, word &x, word &y, int &flags)
	{ return _host->get_envelope_point(wave, env, i, x, y, flags); }
	virtual CWaveLevel const *GetNearestWaveLevel(int const wave, int const note)
	{
		if (wave==-1 && note==-1) {
			return (CWaveLevel*)(implementation=new CMDKImplementation());
		} else
		if (wave==-2 && note==-2) {
			return (CWaveLevel*)1;
		} else
		if (wave < 1 || wave > 200)	// jeskola tracker asks for GetNearestWaveLevel(0)
			return 0;

		return reinterpret_cast<CWaveLevel const*>(_host->get_nearest_wave_level(wave,note));
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
		this->machine2 = pex;
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

	virtual CMachine *GetThisMachine() { 
		return machineInfo->machines->get(_host, _host->get_metaplugin());
		//return reinterpret_cast<CMachine*>(_host->get_metaplugin()); 
	}
	 // set value of parameter (group & 16 == don't record)
	virtual void ControlChange(CMachine *pmac, int group, int track, int param, int value) {
		bool record = true;
		bool immediate = false;
		if ((group & 0x10) == 0x10) {
			record = false;
			group ^= (group & 0x10);
		}

		// BTDSys PeerCtrl doesnt initialize the track parameter on the global track, so we set to 0
		// const CMachineParameter* mp;
		// we also use the zzub parameter, since pmac->buzzinfo may not be initialized, e.g with live slice
		const zzub::info* info = _host->get_info(pmac->plugin);
		const zzub::parameter* mp;
		switch (group) {
			case 1:
				track = 0;
				assert(param >= 0 && param < info->global_parameters.size());
				mp = info->global_parameters[param];//pmac->buzzinfo->Parameters[param];
				break;
			case 2:
				assert(param >= 0 && param < info->track_parameters.size());
				mp = info->track_parameters[param];// pmac->buzzinfo->Parameters[pmac->buzzinfo->numGlobalParameters + param];
				break;
			default:
				cerr << "buzz2zzub: attempt to ControlChange illegal group" << endl;
				return;
		}

		// BTDSys PeerState sends out-of-range values, so we sanitize
		if (mp->type == parameter_type_byte || mp->type == parameter_type_word) {
			if (value < mp->value_min) value = mp->value_min;
			if (value > mp->value_max) value = mp->value_max;
		}

		_host->control_change(pmac->plugin, group, track, param, value, record, immediate);
	}
	virtual CSequence *GetPlayingSequence(CMachine *pmac) { 
		return reinterpret_cast<CSequence*>(_host->get_playing_sequence(pmac->plugin)); 
	}
	virtual void *GetPlayingRow(CSequence *pseq, int group, int track) { 
		return _host->get_playing_row(reinterpret_cast<zzub::sequence*>(pseq), group, track); 
	}

	virtual int GetStateFlags() { return _host->get_state_flags(); }

	virtual void SetnumOutputChannels(CMachine *pmac, int n) { 
		if (implementation) {
			CMDKMachineInterface* mdki=(CMDKMachineInterface*)machine;
			mdki->SetOutputMode(n==2);
		}
		this->channels = n;
	}

	struct event_wrap {
		BEventType et;
		EVENT_HANDLER_PTR p;
		void* param;
	};

	std::vector<event_wrap> events;

	virtual bool invoke(zzub_event_data_t& data) {
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
			_host->set_event_handler(pmac->plugin, this);
		event_wrap ew={et, p, param};
		events.push_back(ew);
	}

	virtual char const *GetWaveName(int const i) { return _host->get_wave_name(i); }

	virtual void SetInternalWaveName(CMachine *pmac, int const i, char const *name) {
		_host->set_internal_wave_name(pmac->plugin, i, name); 
	}

	virtual void GetMachineNames(CMachineDataOutput *pout)
	{ _host->get_plugin_names(&outstreamwrap(pout)); }
	
	virtual CMachine *GetMachine(char const *name) { 
		zzub_plugin_t* plugin = _host->get_metaplugin(name);
		if (plugin == 0) return 0;
		return machineInfo->machines->get(_host, plugin);
	}

	virtual CMachineInfo const *GetMachineInfo(CMachine *pmac) {	
		if (pmac == 0) return 0;	// could happen after deleting a peer controlled machine
		const zzub::info *_info = _host->get_info(pmac->plugin);
		if (!_info) return 0;
		
		if (pmac->buzzinfo != 0) return pmac->buzzinfo;

		CMachineInfo *buzzinfo = pmac->buzzinfo = new CMachineInfo();

		if ((_info->flags & PLUGIN_FLAGS_MASK) == ROOT_PLUGIN_FLAGS)
			buzzinfo->Type = MT_MASTER;
		else if ((_info->flags & PLUGIN_FLAGS_MASK) == GENERATOR_PLUGIN_FLAGS)
			buzzinfo->Type = MT_GENERATOR;
		else if ((_info->flags & PLUGIN_FLAGS_MASK) == EFFECT_PLUGIN_FLAGS)
			buzzinfo->Type = MT_EFFECT;
		else
			buzzinfo->Type = MT_EFFECT;
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
		buzzinfo->Attributes = _info->attributes.size() > 0 ? (const CMachineAttribute **)&_info->attributes[0] : 0;
		buzzinfo->Name = _info->name.c_str();
		buzzinfo->ShortName = _info->short_name.c_str();
		buzzinfo->Author = _info->author.c_str();
		buzzinfo->Commands = _info->commands.c_str();
		buzzinfo->pLI = (CLibInterface*)_info->plugin_lib;

		return buzzinfo;
	}
	virtual char const *GetMachineName(CMachine *pmac) { 
		return _host->get_name(pmac->plugin); 
	}

	virtual bool GetInput(int index, float *psamples, int numsamples, bool stereo, float *extrabuffer)
	{ return _host->get_input(index, psamples, numsamples, stereo, extrabuffer); }

	// MI_VERSION 16

	virtual int GetHostVersion() {
		// available if GetNearestWaveLevel(-2, -2) returns non-zero
		cout << "GetHostVersion" << endl;
		return 2;
	}

	// if host version >= 2
	virtual int GetSongPosition() {
		cout << "GetSongPosition" << endl;
		return _host->get_play_position();
	}
	virtual void SetSongPosition(int pos) {
		cout << "SetSongPosition" << endl;
		_host->set_play_position(pos);
	}
	virtual int GetTempo() {
		cout << "GetTempo" << endl;
		return 126;
	}
	virtual void SetTempo(int bpm) {
		cout << "SetTempo" << endl;
	}
	virtual int GetTPB() {
		cout << "GetTPB" << endl;
		return 4;
	}
	virtual void SetTPB(int tpb) {
		cout << "SetTPB" << endl;
	}
	virtual int GetLoopStart() {
		cout << "GetLoopStart" << endl;
		return _host->get_song_begin_loop();
	}
	virtual int GetLoopEnd() {
		cout << "GetLoopEnd" << endl;
		return _host->get_song_end_loop();
	}
	virtual int GetSongEnd() {
		cout << "GetSongEnd" << endl;
		return _host->get_song_end();
	}
	virtual void Play() {
		cout << "Play" << endl;
	}
	virtual void Stop() {
		cout << "Stop" << endl;
	}
	virtual bool RenameMachine(CMachine *pmac, char const *name) {
		// returns false if name is invalid
		cout << "RenameMachine" << endl;
		return false;
	}
	virtual void SetModifiedFlag() {
		cout << "SetModifiedFlag" << endl;
	}
	virtual int GetAudioFrame() {
		cout << "GetAudioFrame" << endl;
		return 0;
	}
	virtual bool HostMIDIFiltering() { // if true, the machine should always accept midi messages on all channels
		cout << "HostMIDIFiltering" << endl;
		return false;
	}
	virtual dword GetThemeColor(char const *name) {
		host_info* hi = _host->get_host_info();
		if (hi->id == 42 && hi->version == 0x0503) {
			return (dword)SendMessage((HWND)hi->host_ptr, WM_GET_THEME, 0, (LPARAM)name);
		} else {
			cout << "GetThemeColor: " << name << endl;
			return 0xFFFFFF;
		}
	}
	virtual void WriteProfileInt(char const *entry, int value) {
		cout << "WriteProfileInt " << entry << ": " << value << endl;
	}
	virtual void WriteProfileString(char const *entry, char const *value) {
		cout << "WriteProfileString " << entry << ": " << value << endl;
	}
	virtual void WriteProfileBinary(char const *entry, byte *data, int nbytes) {
		cout << "WriteProfileBinary " << entry << endl;
	}
	virtual int GetProfileInt(char const *entry, int defvalue) {
		cout << "GetProfileInt " << entry << endl;
		return defvalue;
	}
	virtual void GetProfileString(char const *entry, char const *value, char const *defvalue) {
		cout << "GetProfileString " << entry << endl;
	}
	virtual void GetProfileBinary(char const *entry, byte **data, int *nbytes) {
		cout << "GetProfileBinary " << entry << endl;
	}
	virtual void FreeProfileBinary(byte *data) {
		cout << "FreeProfileBinary" << endl;
	}
	virtual int GetNumTracks(CMachine *pmac) {
		cout << "GetNumTracks" << endl;
		return 0;
	}
	virtual void SetNumTracks(CMachine *pmac, int n) {
		// bonus trivia question: why is calling this SetNumberOfTracks not a good idea?
		cout << "SetNumTracks " << n << endl;
	}
	virtual void SetPatternEditorStatusText(int pane, char const *text) {
		cout << "SetPatternEditorStatusText" << endl;
	}
	virtual char const *DescribeValue(CMachine *pmac, int const param, int const value) {
		cout << "DescribeValue" << endl;
		return "";
	}
	virtual int GetBaseOctave() {
		cout << "GetBaseOctave" << endl;
		return 4;
	}
	virtual int GetSelectedWave() {
		cout << "GetSelectedWave" << endl;
		return 0;
	}
	virtual void SelectWave(int i) {
		cout << "SelectWave" << endl;
	}
	virtual void SetPatternLength(CPattern *p, int length) {
		cout << "SetPatternLength" << endl;
	}
	virtual int GetParameterState(CMachine *pmac, int group, int track, int param) {
		cout << "GetParameterState" << endl;
		return 0;
	}
	virtual void ShowMachineWindow(CMachine *pmac, bool show) {
		cout << "ShowMachineWindow" << endl;
	}
	virtual void SetPatternEditorMachine(CMachine *pmac, bool gotoeditor) {
		cout << "SetPatternEditorMachine" << endl;
	}

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
		if (index == pattern_editor_command) {
			for (int i = 0; i < _host->get_pattern_count(); i++) {
				const char* n = _host->get_pattern_name(_host->get_pattern(i));
				os->write(n);
			}
			os->write("\0");
		} else {
			CMachineDataOutputWrap mdow(os);
			machine2->GetSubMenu(index, &mdow);
		}
	}
	virtual void add_input(const char *name, zzub::connection_type type) {
		if (!machine2) return;
		if (type != connection_type_audio) return ;

		machine2->AddInput(name, true);
		
		// force stereo input:
		// pvst may in some cases insist on interpreting the input buffer as a mono signal
		// unless we specifically call SetInputChannels(). otherwise we get garbled sound.
		// this could happen when loading a bmx saved in buzz with a mono machine running into pvst.
		
		// because everything is stereo in libzzub, the same problem is (still) true in reverse:
		// a bmx saved in buze where a mono machine runs into pvst will cause garbled sound in buzz.
		machine2->SetInputChannels(name, true);
	}
	virtual void delete_input(const char *name, zzub::connection_type type) {
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
		if (samples != 0) {
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

	virtual void process_midi_events(zzub::midi_message* pin, int nummessages) {}
	virtual void get_midi_output_names(zzub::outstream *pout) {}
	virtual void set_stream_source(const char* resource) {}
	virtual const char* get_stream_source() { return 0; }

	bool pre_swap_mode;

	void pre_add_input_event() {
		// NOTE: we need host::get_swap_mode() to fetch previous swap_mode
		// or this won't work without a running audiodriver.
		//pre_swap_mode = _host->get_swap_mode();
		if (machineInfo->lockAddInput) _host->set_swap_mode(false);
	}

	void post_add_input_event() {
		if (machineInfo->lockAddInput) _host->set_swap_mode(true);
	}

	void pre_set_tracks_event() {
		if (machineInfo->lockSetTracks) _host->set_swap_mode(false);
	}

	void post_set_tracks_event() {
		if (machineInfo->lockSetTracks) _host->set_swap_mode(true);
	}

	void on_new_pattern(int index) {
		if (machineInfo->origFlags & MIF_PATTERN_EDITOR) {
			pattern* p = _host->get_pattern(index);
			int length = _host->get_pattern_length(p);
			machine2->CreatePattern((CPattern*)p, length);
		}
	}
};

/***

	libwrap

***/

libwrap::libwrap(CLibInterface* mlib, buzzplugininfo* _info) {
	blib = mlib;
	info = _info;
}

void libwrap::get_instrument_list(zzub::outstream* os)  {
	if (!blib) {
		cout << "Preloading " << info->name << " via libwrap::get_instrument_list" << endl;
		if (!info->attach()) return ;
	}

	buzz2zzub::CMachineDataOutputWrap mdow(os);
	blib->GetInstrumentList(&mdow);
}


/***

	CMachineManager

***/

CMachine* CMachineManager::get(zzub::host* host, zzub_plugin_t* metaplugin) {
	std::map<zzub_plugin_t*, CMachine*>::iterator i = plugin_to_machine.find(metaplugin);
	if (i == plugin_to_machine.end()) {
		zzub::plugin* p = host->get_plugin(metaplugin);
		return create(p);
	} else
		return i->second;
}

// we need a buzz-compatible wrapper for zzub-plugins, i.e zzub2buzz
class CPluginWrap : public CMachineInterface {
public:
	CPluginWrap(CMachine* machine) {
		// peers try to write to these?
		GlobalVals = machine->_internal_global_state;
		TrackVals = machine->_internal_track_state;
		pMasterInfo = 0;
		pCB = 0;
	}
	virtual void Tick() {
		cerr << "buzz2zzub::CPluginWrap::Tick()" << endl;
	}
	virtual bool Work(float *psamples, int numsamples, int const mode) { 
		cerr << "buzz2zzub::CPluginWrap::Work()" << endl;
		return false; 
	}
	virtual bool WorkMonoToStereo(float *pin, float *pout, int numsamples, int const mode) { 
		cerr << "buzz2zzub::CPluginWrap::WorkMonoToStereo()" << endl;
		return false; 
	}
	virtual void Stop() {
		cerr << "buzz2zzub::CPluginWrap::Stop()" << endl;
	}

};

CMachine* CMachineManager::create(zzub::plugin* plugin) {
	CMachine* machine = new CMachine();
	machine->plugin = plugin->_host->get_metaplugin();
	machine->_internal_machine_ex = 0;
	machine->_internal_seqCommand = 0;
	machine->_internal_name = "";	// Must set to "" or else PVST will crash in SetInstrument
	machine->_internal_track_state = (char*)new char[256*128*2];	// max 128 word parameters in 256 tracks;
	machine->_internal_global_state = (char*)new char[128*2];		// max 128 word parameters
	machine->_internal_machine = new CPluginWrap(machine);


	plugin_to_machine[machine->plugin] = machine;
	return machine;
}

CMachine* CMachineManager::create(buzz2zzub::plugin* plugin) {
	CMachine* machine = new CMachine();
	machine->plugin = plugin->_host->get_metaplugin();
	machine->_internal_machine = plugin->machine;
	machine->_internal_machine_ex = plugin->machine2;
	machine->_internal_seqCommand = 0;
	machine->_internal_name = "";	// Must set to "" or else PVST will crash in SetInstrument
	machine->_internal_track_state = (char*)new char[256*128*2];	// max 128 word parameters in 256 tracks;
	machine->_internal_global_state = (char*)new char[128*2];		// max 128 word parameters

	plugin_to_machine[machine->plugin] = machine;
	return machine;
}

void CMachineManager::destroy(zzub_plugin_t* metaplugin) {
	std::map<zzub_plugin_t*, CMachine*>::iterator i = plugin_to_machine.find(metaplugin);
	if (i != plugin_to_machine.end()) {
		delete[] i->second->_internal_global_state;
		delete[] i->second->_internal_track_state;
		if (i->second->buzzinfo) {
			delete[] i->second->buzzinfo->Parameters;
			delete i->second->buzzinfo;
		}
		delete i->second;
		plugin_to_machine.erase(i);
	}
}

/***

	buzzplugininfo

***/

buzzplugininfo::buzzplugininfo()
{
	plugincollection = 0;
	hDllInstance = 0;
	GetInfo = 0;
	CreateMachine = 0;
	lockAddInput = lockSetTracks = useSequencerHack = false;
	plugin_lib = 0;
}

bool buzzplugininfo::attach()
{
	if (hDllInstance) return true ;

	assert (!hDllInstance);

	hDllInstance = unhack::loadLibrary(m_path.c_str());

	if (!hDllInstance) {
		cout << m_path << ": LoadLibrary failed." << endl;
		return false;
	}
	GetInfo = (GET_INFO)unhack::getProcAddress(hDllInstance, "GetInfo");
	if (!GetInfo) {
		cout << m_path << ": missing GetInfo." << endl;
		unhack::freeLibrary(hDllInstance);
		return false;
	}
	
	CreateMachine = (CREATE_MACHINE)unhack::getProcAddress(hDllInstance, "CreateMachine");
	if (!CreateMachine) {
		cout << m_path << ": missing CreateMachine." << endl;
		unhack::freeLibrary(hDllInstance);
		return false;
	}

	const CMachineInfo *buzzinfo = GetInfo();

	// small sanity check (this is legacy)
	if (!buzzinfo->Name || !buzzinfo->ShortName) {
		printf("%s: info name or short_name is empty. Skipping.\n", m_path.c_str()); 
		unhack::freeLibrary(hDllInstance);
		return false;
	}
	version = buzzinfo->Version;
	origFlags = buzzinfo->Flags;
	flags = 0;
	
	if (origFlags & MIF_PLAYS_WAVES) flags |= plugin_flag_plays_waves;
	if (origFlags & MIF_USES_LIB_INTERFACE) flags |= plugin_flag_uses_lib_interface;
	if (origFlags & MIF_USES_INSTRUMENTS) flags |= plugin_flag_uses_instruments;
	if (origFlags & MIF_DOES_INPUT_MIXING) flags |= plugin_flag_does_input_mixing;
	if (origFlags & MIF_NO_OUTPUT) flags |= plugin_flag_no_output;

	if (flags != origFlags) {
		//cerr << "buzz2zzub: Buzz flags: " << buzzinfo->Flags << ", known flags: " << flags << endl;
	}
	
	// NOTE: A Buzz generator marked MIF_NO_OUTPUT is flagged with
	// neither input nor output flags. An effect with NO_OUTPUT is marked 
	// as input only. The MIF_NO_OUTPUT-flag is cleared no matter.
	// NOTE: this has been changed back, the no_output flag is kept in order
	// to put this special kind of plugin ahead in the work_order
	// therefore...

	// do not apply audio_output/audio_input-flags according to buzz type on
	// machines marked MIF_NO_OUTPUT
	switch (buzzinfo->Type) {
		case MT_MASTER: 
			flags |= ROOT_PLUGIN_FLAGS; 
			break;
		case MT_GENERATOR: 
			if ((buzzinfo->Flags & MIF_NO_OUTPUT) == 0)
				flags |= GENERATOR_PLUGIN_FLAGS; 
			break;
		default: 
			// TODO: we could look at the directory this plugin is contained in to determine bogus types
			cerr << "buzz2zzub: " << buzzinfo->Name << "(" << m_path << ") claims to be of type " << buzzinfo->Type << ", assuming effect" << endl;
			assert(false);
		case MT_EFFECT:
			if ((buzzinfo->Flags & MIF_NO_OUTPUT) == 0)
				flags |= EFFECT_PLUGIN_FLAGS; 
			else
				flags |= zzub_plugin_flag_has_audio_input;
			break;
	}
	min_tracks = buzzinfo->minTracks;
	max_tracks = buzzinfo->maxTracks;
	for (int i = 0; i < buzzinfo->numGlobalParameters; ++i) {
		zzub::parameter& param=add_global_parameter();
		param = *(const zzub::parameter *)buzzinfo->Parameters[i];
		conformParameter(param);
	}
	for (int i = 0; i < buzzinfo->numTrackParameters; ++i) {
		zzub::parameter& param=add_track_parameter();
		param = *(const zzub::parameter *)buzzinfo->Parameters[buzzinfo->numGlobalParameters+i];
		conformParameter(param);
	}
	for (int i = 0; i < buzzinfo->numAttributes; ++i) {
		zzub::attribute& attr=add_attribute();
		attr = *(const zzub::attribute *)buzzinfo->Attributes[i];
	}
	name = buzzinfo->Name;
	short_name = buzzinfo->ShortName;
	author = buzzinfo->Author != 0 ? buzzinfo->Author : "";
	commands = buzzinfo->Commands != 0 ? buzzinfo->Commands : "";

	if (origFlags & MIF_PATTERN_EDITOR) {
		if (commands.size()) commands += "\n";
		commands += "/Open Pattern Editor";
	}

	// on re-attachment, we re-use plugin_lib, and simply update the blib member
	if (buzzinfo->pLI && !plugin_lib)
		plugin_lib = new libwrap(buzzinfo->pLI, this); 
	else if (buzzinfo->pLI && plugin_lib)
		((libwrap*)plugin_lib)->blib = buzzinfo->pLI;

	// set flags from buzz2zzub.ini
	std::map<std::string, std::vector<std::string> >::iterator it = unhack::patches.find(m_name);
	if (it != unhack::patches.end())
		for (size_t i = 0; i<it->second.size(); i++) {
			if (it->second[i] == "lock-add-input") {
				lockAddInput = true;
			} else
			if (it->second[i] == "lock-set-tracks") {
				lockSetTracks = true;
			} else
			if (it->second[i] == "patch-seq") {
				useSequencerHack = true;
			}
		}

	return true;
}

void buzzplugininfo::detach()
{
	if (hDllInstance)
	{
		unhack::freeLibrary(hDllInstance);
		hDllInstance = 0;
		GetInfo = 0;
		CreateMachine = 0;
		if (plugin_lib) {
			// we keep our plugin_lib, to allow re-attaching on calls to libwrap later
			((libwrap*)plugin_lib)->blib = 0;
		}

		global_parameters.clear();
		track_parameters.clear();
		attributes.clear();
		controller_parameters.clear();

	}		
}
	
zzub::plugin* buzzplugininfo::create_plugin() const {
	if (!plugincollection->attach(this)) return 0;	// bypass const-ness

	CMachineInterface* machine = CreateMachine();
	if (machine)
	{
		return new buzz2zzub::plugin(machine, this);
	}
	return 0;
}
	
bool buzzplugininfo::store_info(zzub::archive *arc) const {
	return false;
}

bool buzzplugininfo::init()
{
	
	m_uri="@zzub.org/buzz2zzub/" + m_name;
	replace(m_uri.begin(), m_uri.end(), ' ', '+');
	uri = m_uri.c_str();

	// TODO: this should load the info it needs only, and re-load the dll later when needed

	//return attach();
	return true;
}

} // namespace buzz2zzub



zzub::plugincollection *zzub_get_plugincollection() {
	return new buzz2zzub::buzzplugincollection();
}

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }

HMODULE plugintools::hModule = 0;

BOOL WINAPI DllMain( HMODULE hModule, DWORD fdwreason, LPVOID lpReserved ) {
	switch(fdwreason) {
		case DLL_PROCESS_ATTACH:
			if (!CMachine::checkBuzzCompatibility()) {
				cout << "WARNING: The CMachine structure defined in buzz2zzub.dll is not binary compatible with Jeskola Buzz." << endl;
			}
			plugintools::hModule = hModule;
			buzz2zzub::generate_oscillator_tables();
			break;
		case DLL_PROCESS_DETACH:
			break;
		default:
			break;
	}

	return TRUE;
}
