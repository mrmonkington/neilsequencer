/*
Copyright (C) 2006-2007 Leonard Ritter

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "common.h"
#include <locale.h>
#include <string>
#include <algorithm>
#include <iostream>
#include <sstream>
#include "../minizip/unzip.h"
#include "../minizip/zip.h"
#include "FLAC/all.h"

#include "ccm.h"
#include "bmxreader.h"
#include "archive.h"

#if defined(_MAX_PATH)
#define CCM_MAX_PATH MAX_PATH
#else
#define CCM_MAX_PATH 32768
#endif

using namespace pugi;

namespace {

// SampleEnumerator is a helper class for converting and rendering samples of any bits/channels
// This could be used in Buzes wave rendrer and wave player instead of the awful getSampleAt
// Alternately use some other resampler
class SampleEnumerator {
    unsigned char delta;
    char* buffer;
    unsigned long samples;
    unsigned long currentSample;
    int channels;
    int format;

    float multiplier;
public:
    // maxValue==0x7fff = retreive 16 bit sample values
    // maxValue==0x7fffff = retreive 24 bit sample values
    // maxValue==0x7fffffff = retreive 32 bit sample values
    // maxValue==1 = retreive float sample values
    SampleEnumerator(zzub::wave_info_ex& wave, int level, float maxValue=-1) { 
        currentSample=0;

        if (maxValue<0)
            maxValue=static_cast<float>(((1<<wave.get_bits_per_sample(level))>>1)-1);

        buffer=(char*)wave.get_sample_ptr(level);
        samples=wave.get_sample_count(level);
        channels=wave.get_stereo()?2:1;
		format=wave.get_wave_format(level);
        switch (format) {
		    case zzub::wave_buffer_type_si16:
			    delta=2;
                multiplier= maxValue / 0x7fff;
                break;
		    case zzub::wave_buffer_type_si24:
                delta=3;
                multiplier= maxValue / 0x7fffff;
                break;
		    case zzub::wave_buffer_type_si32:
                delta=4;
                multiplier= maxValue / 0x7fffffff;
                break;
		    case zzub::wave_buffer_type_f32:
                delta=4;
                multiplier= maxValue;
                break;
            default:
                throw "unknown wave format";
        }
    }

    int getInt(char channel) {
        return static_cast<int>(getFloat(channel));
    }

    float getFloat(char channel) {
        float temp;
        ptrdiff_t interleave=delta * channel;
        switch (format) {
		    case zzub::wave_buffer_type_si16:
			    return static_cast<float>(*(short*)(buffer + interleave)) * multiplier;
            case zzub::wave_buffer_type_si24:
        		temp=static_cast<float>(*(unsigned int*)(buffer + interleave));
                return temp * multiplier;
		    case zzub::wave_buffer_type_si32:
                return static_cast<float>(*(int*)(buffer + interleave)) * multiplier;
		    case zzub::wave_buffer_type_f32:
                return static_cast<float>(*(float*)(buffer + interleave)) * multiplier;
            default:
                throw "unknown wave format";
        }
        return 0;
    }

    inline bool next(int skipSamples=1) {
        int bytes=delta*skipSamples*channels;
        buffer+=bytes;
        currentSample+=skipSamples;
        if (skipSamples<0 || currentSample>=samples) return false;
        return true;
    }


};


}

/***

    ArchiveWriter

***/

bool ArchiveWriter::create(std::string fileName) {
    // truncate file to 0 bytes with a FileWriter:
	zzub::file_outstream reset;
    if (!reset.create(fileName.c_str())) return false;
    reset.close();

    // open zip and append at end of file:
    f=zipOpen(fileName.c_str(), APPEND_STATUS_CREATEAFTER);
    if (!f)
        return false;

    return true;
}

bool ArchiveWriter::open(std::string fileName) {
    f=zipOpen(fileName.c_str(), APPEND_STATUS_ADDINZIP);
    if (!f) {
        f=zipOpen(fileName.c_str(), APPEND_STATUS_CREATE);
        
        if (!f)
            return false;
    }

    return true;
}

void ArchiveWriter::close() {
    zipClose(f, 0);
}

int ArchiveWriter::write(void* v, int size) {
    if (ZIP_OK!=zipWriteInFileInZip(f, v, size)) {
        // no error reporting

    }
	return size;
}

bool ArchiveWriter::createFileInArchive(std::string fileName) {
    if (currentFileInArchive!="") {
        closeFileInArchive();
    }

    if (ZIP_OK!=zipOpenNewFileInZip(f, fileName.c_str(), 0, 0, 0, 0, 0, 0, Z_DEFLATED, Z_DEFAULT_COMPRESSION))
        return false;

    currentFileInArchive=fileName;

    return true;
}

void ArchiveWriter::closeFileInArchive() {
    if (currentFileInArchive=="") return ;
    zipCloseFileInZip(f);
    currentFileInArchive="";
}

void ArchiveWriter::writeLine(std::string line) {
	line += "\n";
	outstream::write(line.c_str());
}

/***

    ArchiveReader

***/

bool ArchiveReader::open(std::string fileName) {
    f=unzOpen(fileName.c_str());

    if (!f) return false;

    resetFileInArchive();
    return true;
}

void ArchiveReader::close() {
    unzClose(f);
}

void ArchiveReader::populateInfo(compressed_file_info* cfi, std::string fileName, unz_file_info* uzfi) {
    cfi->name=fileName;
    cfi->compressed_size=uzfi->compressed_size;
    cfi->uncompressed_size=uzfi->uncompressed_size;
}

bool ArchiveReader::findFirst(compressed_file_info* info) {
    if (UNZ_OK!=unzGoToFirstFile(f)) return false;

    unz_file_info file_info;
    char fileNameInZip[CCM_MAX_PATH];

    int err = unzGetCurrentFileInfo(f, &file_info, fileNameInZip, CCM_MAX_PATH, NULL, 0, NULL, 0);
    if (err!=UNZ_OK) return false;

    if (info) populateInfo(info, fileNameInZip, &file_info);
    return true;
}

bool ArchiveReader::findNext(compressed_file_info* info) {
    if (UNZ_OK!=unzGoToNextFile(f)) return false;

    unz_file_info file_info;
    char fileNameInZip[CCM_MAX_PATH];

    int err = unzGetCurrentFileInfo(f, &file_info, fileNameInZip, CCM_MAX_PATH, NULL, 0, NULL, 0);
    if (err!=UNZ_OK) return false;

    if (info) populateInfo(info, fileNameInZip, &file_info);
    return true;
}

#define CASESENSITIVITY (0)

bool ArchiveReader::openFileInArchive(std::string fileName, compressed_file_info* info) {
    if (unzLocateFile(f, fileName.c_str(), CASESENSITIVITY)!=UNZ_OK) return false;

    unz_file_info file_info;
    char fileNameInZip[32768];

    int err = unzGetCurrentFileInfo(f, &file_info, fileNameInZip, 32768, NULL, 0, NULL, 0);
    if (err!=UNZ_OK) return false;

    if (UNZ_OK!=unzOpenCurrentFile(f)) return false;

    populateInfo(&currentFileInfo, fileNameInZip, &file_info);

    if (info) *info=currentFileInfo;

    return true;
}

void ArchiveReader::closeFileInArchve() {
    if (currentFileInfo.name=="") return ;

    unzCloseCurrentFile(f);
    resetFileInArchive();
}

int ArchiveReader::read(void* buffer, int size) {
    if (currentFileInfo.name=="") return 0;

    char* charBuffer = (char*)buffer;
    size_t peekSize=0;
    if (hasPeeked && size>0) {
        charBuffer[0] = lastPeekByte;
        charBuffer++;
        size--;
        lastReadOfs++;
        hasPeeked=false;
        peekSize=1;
    }

    int err = unzReadCurrentFile(f, charBuffer, size);
    if (err<0) return 0;
    err += (int)peekSize;

    lastReadOfs+=err;

    return err;
}

void ArchiveReader::resetFileInArchive() {
    hasPeeked=false;
    currentFileInfo.name="";
    lastReadOfs=0;
}

namespace zzub {

/*! \struct CcmWriter
	\brief .CCM exporter
*/

/*! \struct CcmReader
	\brief .CCM importer
*/

bool encodeFLAC(zzub::outstream* writer, zzub::wave_info_ex& info, int level);

using namespace std;

xml_node CcmWriter::saveHead(xml_node &parent, zzub::song &player) {
	if (strlen(player.song_comment.c_str())) {
		// save song info
		if (arch.createFileInArchive("readme.txt")) {
			arch.write((void*)player.song_comment.c_str(), (int)strlen(player.song_comment.c_str()));
			arch.closeFileInArchive();
			xml_node commentmeta = addMeta(parent, "comment");
			commentmeta.append_attribute("src") = "readme.txt";
		} else {
			std::cerr << "unable to save comment in readme.txt" << std::endl;
		}
	}
	
	return parent;
}

xml_node CcmWriter::addMeta(xml_node &parent, const std::string &propname) {
	xml_node item = parent.append_child(node_element);
	item.set_name("meta");
	item.attribute("name").set_name(propname.c_str());
	return item;
}

xml_node CcmWriter::saveClasses(xml_node &parent, zzub::song &player) {
	xml_node item = parent.append_child(node_element);
	item.set_name("pluginclasses");
	
    std::vector<const zzub::info*> distinctLoaders;
    for (int i = 0; i<player.get_plugin_count(); i++) {
		std::vector<const zzub::info*>::iterator p = find<vector<const zzub::info*>::iterator >(distinctLoaders.begin(), distinctLoaders.end(), player.get_plugin(i).info);
        if (p == distinctLoaders.end()) distinctLoaders.push_back(player.get_plugin(i).info);
    }

    for (size_t i=0; i<distinctLoaders.size(); i++) {
        saveClass(item, *distinctLoaders[i]);
    }

	return item;
}

std::string connectiontype_to_string(int connectiontype) {
	switch (connectiontype) {
		case zzub::connection_type_audio: return "audio";
		case zzub::connection_type_event: return "event";
		case zzub::connection_type_midi: return "midi";
		default: assert(0);
	}
	return "";
}

std::string paramtype_to_string(int paramtype) {
	switch (paramtype) {
		case zzub::parameter_type_note: return "note16"; // buzz note with base 16
		case zzub::parameter_type_switch: return "switch";
		case zzub::parameter_type_byte: return "byte";
		case zzub::parameter_type_word: return "word";
		default: assert(0);
	}
	return "";
}


std::string id_from_ptr(const void *p) {
	char id[64];
	sprintf(id, "%x", p);
	return id;
}

xml_node CcmWriter::saveParameter(xml_node &parent, const zzub::parameter &p) {

	// we take the same format as for lunar manifests

	xml_node item = parent.append_child(node_element);
	item.set_name("parameter");
	
	item.append_attribute("id") = id_from_ptr(&p).c_str();
	item.append_attribute("name") = p.name;
	
	item.append_attribute("type") = paramtype_to_string(p.type).c_str();
	item.append_attribute("minvalue") = p.value_min;
	item.append_attribute("maxvalue") = p.value_max;
	item.append_attribute("novalue") = p.value_none;
	item.append_attribute("defvalue") = p.value_default;
	
	if (p.flags & zzub::parameter_flag_wavetable_index) {
		item.append_attribute("waveindex") = "true";
	}
	if (p.flags & zzub::parameter_flag_state) {
		item.append_attribute("state") = "true";
	}
	if (p.flags & zzub::parameter_flag_event_on_edit) {
		item.append_attribute("editevent") = "true";
	}
	
	return item;
}

xml_node CcmWriter::saveClass(xml_node &parent, const zzub::info &info) {

	// we take the same format as for lunar manifests

	xml_node item = parent.append_child(node_element);
	item.set_name("pluginclass");
	item.append_attribute("id") = info.uri.c_str();
	if (info.flags & zzub::plugin_flag_plays_waves)
		item.append_attribute("plays_waves") = true;
	if (info.flags & zzub::plugin_flag_uses_lib_interface)
		item.append_attribute("uses_lib_interface") = true;
	if (info.flags & zzub::plugin_flag_uses_instruments)
		item.append_attribute("uses_instruments") = true;
	if (info.flags & zzub::plugin_flag_does_input_mixing)
		item.append_attribute("does_input_mixing") = true;
	if (info.flags & zzub::plugin_flag_is_root)
		item.append_attribute("is_root") = true;
	if (info.flags & zzub::plugin_flag_has_audio_input)
		item.append_attribute("has_audio_input") = true;
	if (info.flags & zzub::plugin_flag_has_audio_output)
		item.append_attribute("has_audio_output") = true;
	if (info.flags & zzub::plugin_flag_has_event_input)
		item.append_attribute("has_event_input") = true;
	if (info.flags & zzub::plugin_flag_has_event_output)
		item.append_attribute("has_event_output") = true;
	
	mem_archive arc;
	info.store_info(&arc);
	saveArchive(item, info.uri, arc);
	
	xml_node params = item.append_child(node_element);
	params.set_name("parameters");
	
    if (info.global_parameters.size()>0) {
		xml_node global = params.append_child(node_element);
		global.set_name("global");
		
        for (size_t i=0; i < info.global_parameters.size(); i++) {
            const zzub::parameter* p = info.global_parameters[i];
			saveParameter(global, *p).append_attribute("index") = (int)i;
        }
    }

    if (info.track_parameters.size()>0) {
		xml_node track = params.append_child(node_element);
		track.set_name("track");
  
        for (size_t i=0; i<info.track_parameters.size(); i++) {
            const zzub::parameter* p = info.track_parameters[i];
			saveParameter(track, *p).append_attribute("index") = (int)i;
        }
    }

	return item;
}

double amp_to_double(int amp) {
	return double(amp) / 16384.0;
}

int double_to_amp(double v) {
	return int((v * 16384.0)+0.5);
}

double pan_to_double(int pan) {
	return (double(pan) / 16384.0) - 1.0;
}

int double_to_pan(double v) {
	return int(((v + 1.0) * 16384.0)+0.5);
}

xml_node CcmWriter::saveEventBinding(xml_node &parent, zzub::event_connection_binding &binding) {
	xml_node item = parent.append_child(node_element);
	item.set_name("binding");
	
	item.append_attribute("source_param_index") = binding.source_param_index;
	item.append_attribute("target_group_index") = binding.target_group_index;
	item.append_attribute("target_track_index") = binding.target_track_index;
	item.append_attribute("target_param_index") = binding.target_param_index;
	
	return item;
}

xml_node CcmWriter::saveEventBindings(xml_node &parent, std::vector<zzub::event_connection_binding> &bindings) {
	xml_node item = parent.append_child(node_element);
	item.set_name("bindings");
	
	for (size_t i=0; i<bindings.size(); i++) {
		saveEventBinding(item, bindings[i]);
	}

	return item;
}

xml_node CcmWriter::saveArchive(xml_node &parent, const std::string &pathbase, zzub::mem_archive &arc) {
	zzub::mem_archive::buffermap::iterator i;
	for (i = arc.buffers.begin(); i != arc.buffers.end(); ++i) {
		if (i->second.size()) {
			xml_node data = parent.append_child(node_element);
			data.set_name("data");
			std::string filename;
			data.append_attribute("type") = "raw";
			data.append_attribute("base") = i->first.c_str();
			if (i->first == "") {
				filename = pathbase + "/raw";
			} else {
				filename = pathbase + "/" + i->first;
			}
			arch.createFileInArchive(filename);
			arch.write(&i->second[0], i->second.size());
			arch.closeFileInArchive();
			data.append_attribute("src") = filename.c_str();
		}
	}
	return xml_node();
}

xml_node CcmWriter::saveInit(xml_node &parent, zzub::song &player, int plugin) {
	xml_node item = parent.append_child(node_element);
	item.set_name("init");
	
	mem_archive arc;
	metaplugin& m = *player.plugins[plugin];
	m.plugin->save(&arc);
	saveArchive(item, id_from_ptr((const void*)plugin), arc);

	if (m.info->global_parameters.size()) {
		xml_node global = item.append_child(node_element);
		global.set_name("global");
		
		for (size_t i = 0; i != m.info->global_parameters.size(); ++i) {
			xml_node n = global.append_child(node_element);
			n.set_name("n");
			n.append_attribute("ref") = id_from_ptr(m.info->global_parameters[i]).c_str();
			n.append_attribute("v") = player.plugin_get_parameter(plugin, 1, 0, i);
		}
	}

	if (m.tracks > 0) {
		xml_node tracks = item.append_child(node_element);
		tracks.set_name("tracks");
		
		for (int t = 0; t != m.tracks; ++t) {
			xml_node track = tracks.append_child(node_element);
			track.set_name("track");
			track.append_attribute("index") = t;
			
			for (size_t i = 0; i != m.info->track_parameters.size(); ++i) {
				xml_node n = track.append_child(node_element);
				n.set_name("n");
				n.append_attribute("ref") = id_from_ptr(m.info->track_parameters[i]).c_str();
				n.append_attribute("v") = player.plugin_get_parameter(plugin, 2, t, i);
			}
		}
	}
	
	
    if (m.plugin->attributes && m.info->attributes.size() > 0)
		saveAttributes(item, player, plugin);
	
	return item;
}

xml_node CcmWriter::saveAttributes(xml_node &parent, zzub::song &player, int plugin) {
	xml_node item = parent.append_child(node_element);
	item.set_name("attributes");
	
	metaplugin& m = *player.plugins[plugin];

    // save attributes
	for (size_t i = 0; i < m.info->attributes.size(); i++) {
		const attribute& attr = *m.info->attributes[i];
		xml_node n = item.append_child(node_element);
		n.set_name("n");
		n.append_attribute("name") = attr.name;
		n.append_attribute("v") = m.plugin->attributes[i];
	}

	return item;
}

xml_node CcmWriter::savePatternTrack(xml_node &parent, const std::string &colname, double fac, zzub::song &player, int plugin, zzub::pattern &p, int group, int track) {
	xml_node item = parent.append_child(node_element);
	item.set_name(colname.c_str());
	if (group == 2)
		item.append_attribute("index") = track;
	
	//zzub::patterntrack& t = *p.getPatternTrack(group, track);
    for (size_t i = 0; i < p.rows; ++i) {
        for (size_t j = 0; j < p.groups[group][track].size(); ++j) {
			const zzub::parameter *param = player.plugin_get_parameter_info(plugin, group, track, j);
			assert(param != 0);

			int value = p.groups[group][track][j][i];
			if (value != param->value_none) {
				xml_node e = item.append_child(node_element);
				e.set_name("e");
				e.append_attribute("t") = fac * double(i);
				if (group == 0) {
					e.append_attribute("ref") = id_from_ptr(player.plugin_get_input_connection(plugin, track)).c_str();
					// we save normals, not exposing implementation details
					if (j == 0) {
						e.append_attribute("amp") = amp_to_double(value);
					} else if (j == 1) {
						e.append_attribute("pan") = pan_to_double(value);
					} else {
						assert(0);
					}
				} else {
					e.append_attribute("ref") = id_from_ptr(param).c_str();
					e.append_attribute("v") = value;
				}
			}
        }
    }
	
	return item;
}

xml_node CcmWriter::savePattern(xml_node &parent, zzub::song &player, int plugin, zzub::pattern &p) {
	// we're going to save pattern data as events, since storing data
	// in a table is implementation detail. also, empty patterns will
	// take less space.
	
	// event positions and track length is stored in beats, not rows. this makes it
	// easier to change the tpb count manually, and have the loader adapt all
	// patterns on loading.
	
	double tpbfac = 1.0/double(player.plugin_get_parameter(0, 1, 0, 2));
	
	xml_node item = parent.append_child(node_element);
	item.set_name("events");
	item.append_attribute("id") = id_from_ptr(&p).c_str();
	item.append_attribute("name") = p.name.c_str();
	item.append_attribute("length") = double(p.rows) * tpbfac;

	// save connection columns	
	for (size_t j = 0; j < p.groups[0].size(); ++j) {
		savePatternTrack(item, "c", tpbfac, player, plugin, p, 0, j);
	}

	// save globals
	savePatternTrack(item, "g", tpbfac, player, plugin, p, 1, 0);

	// save tracks
	for (size_t j = 0; j < p.groups[2].size(); j++) {
		savePatternTrack(item, "t", tpbfac, player, plugin, p, 2, j);
	}

	return item;
}

xml_node CcmWriter::savePatterns(xml_node &parent, zzub::song &player, int plugin) {
	xml_node item = parent.append_child(node_element);
	item.set_name("eventtracks");

    for (size_t i = 0; i != player.plugins[plugin]->patterns.size(); i++) {
		savePattern(item, player, plugin, *player.plugins[plugin]->patterns[i]);
	}
	
	return item;
}

xml_node CcmWriter::saveSequence(xml_node &parent, double fac, zzub::song &player, int track) {
	xml_node item = parent.append_child(node_element);
	item.set_name("sequence");
	
	int plugin_id = player.sequencer_tracks[track].plugin_id;
	for (size_t i = 0; i < player.sequencer_tracks[track].events.size(); ++i) {
		sequencer_track::time_value &ev = player.sequencer_tracks[track].events[i];

		xml_node e = item.append_child(node_element);
		e.set_name("e");
		e.append_attribute("t") = fac * double(ev.first);
		if (ev.second == sequencer_event_type_mute) {
			e.append_attribute("mute") = true;
		} else if (ev.second == sequencer_event_type_break) {
			e.append_attribute("break") = true;
		} else if (ev.second == sequencer_event_type_thru) {
			e.append_attribute("thru") = true;
		} else if (ev.second >= 0x10) {
			zzub::pattern& p = *player.plugins[plugin_id]->patterns[ev.second - 0x10];
			e.append_attribute("ref") = id_from_ptr(&p).c_str();
		} else {
			assert(0);
		}
	}

	return item;
}

xml_node CcmWriter::saveSequences(xml_node &parent, zzub::song &player, int plugin) {
	
	// event positions and track length is stored in beats, not rows. this makes it
	// easier to change the tpb count manually, and have the loader adapt all
	// sequences on loading.
	double tpbfac = 1.0/double(player.plugin_get_parameter(0, 1, 0, 2));

	xml_node item = parent.append_child(node_element);
	item.set_name("sequences");

	for (int i = 0; i != player.sequencer_tracks.size(); i++) {
		if (player.sequencer_tracks[i].plugin_id == plugin) {
			saveSequence(item, tpbfac, player, i);
		}
	}

	return item;
}

xml_node CcmWriter::saveMidiMappings(xml_node &parent, zzub::song &player, int plugin) {
	xml_node item = parent.append_child(node_element);
	item.set_name("midi");
	
	for (size_t j = 0; j < player.midi_mappings.size(); j++) {
		midimapping& mm = player.midi_mappings[j];
		if (mm.plugin_id == plugin) {
			xml_node bind = item.append_child(node_element);
			bind.set_name("bind");
			
			const zzub::parameter *param = player.plugin_get_parameter_info(plugin, mm.group, mm.track, mm.column);
			assert(param);

			if (mm.group == 0) {
				bind.append_attribute("ref") = id_from_ptr(player.plugin_get_input_connection(plugin, mm.track)).c_str();
				if (mm.column == 0) {
					bind.append_attribute("target") = "amp";
				} else if (mm.column == 1) {
					bind.append_attribute("target") = "pan";
				} else {
					assert(0);
				}
				bind.append_attribute("track") = mm.track;
			} else if (mm.group == 1) {
				bind.append_attribute("ref") = id_from_ptr(param).c_str();
				bind.append_attribute("target") = "global";
			} else if (mm.group == 2) {
				bind.append_attribute("track") = mm.track;
				bind.append_attribute("ref") = id_from_ptr(param).c_str();
				bind.append_attribute("target") = "track";
			} else {
				assert(0);
			}
			
			bind.append_attribute("channel") = mm.channel;
			bind.append_attribute("controller") = mm.controller;
		}
	}
	return item;
}

xml_node CcmWriter::savePlugin(xml_node &parent, zzub::song &player, int plugin) {
	xml_node item = parent.append_child(node_element);
	item.set_name("plugin");
	item.append_attribute("id") = plugin;//id_from_ptr(plugin);
	item.append_attribute("name") = player.plugins[plugin]->name.c_str();
	item.append_attribute("ref") = player.plugins[plugin]->info->uri.c_str();
	
	// ui properties should be elsewhere
	xml_node position = item.append_child(node_element);
	position.set_name("position");
	position.append_attribute("x") = player.plugins[plugin]->x;
	position.append_attribute("y") = player.plugins[plugin]->y;

	if (player.plugin_get_input_connection_count(plugin) > 0) {
		xml_node connections = item.append_child(node_element);
		connections.set_name("connections");

		for (int i = 0; i < player.plugin_get_input_connection_count(plugin); i++) {
			xml_node item = connections.append_child(node_element);
			item.set_name("input");

			item.append_attribute("id") = id_from_ptr(player.plugin_get_input_connection(plugin, i)).c_str();
			item.append_attribute("ref") = player.plugin_get_input_connection_plugin(plugin, i);
			item.append_attribute("type") = connectiontype_to_string(player.plugin_get_input_connection_type(plugin, i)).c_str();
			
			switch (player.plugin_get_input_connection_type(plugin, i)) {
				case zzub::connection_type_audio:
					// we save normals, not exposing implementation details
					item.append_attribute("amplitude") = amp_to_double(player.plugin_get_parameter(plugin, 0, i, 0));
					item.append_attribute("panning") = pan_to_double(player.plugin_get_parameter(plugin, 0, i, 1));
					break;
				case zzub::connection_type_event: {
					zzub::event_connection &ac = *(zzub::event_connection*)player.plugin_get_input_connection(plugin, i);
					saveEventBindings(item, ac.bindings);
				} break;
				case zzub::connection_type_midi: {
					zzub::midi_connection &ac = *(zzub::midi_connection*)player.plugin_get_input_connection(plugin, i);
					item.append_attribute("device") = ac.device_name.c_str();
				} break;
				default:
					assert(0);
					break;
			}

		}
	}
	
	saveInit(item, player, plugin);
	
	saveMidiMappings(item, player, plugin);
	
	if (player.plugins[plugin]->patterns.size() > 0)
		savePatterns(item, player, plugin);

	saveSequences(item, player, plugin);

	return item;
}

xml_node CcmWriter::savePlugins(xml_node &parent, zzub::song &player) {
	xml_node item = parent.append_child(node_element);
	item.set_name("plugins");

    for (int i = 0; i < player.get_plugin_count(); i++) {
		savePlugin(item, player, player.get_plugin_id(i));
    }
	
	return item;
}

xml_node CcmWriter::saveEnvelope(xml_node &parent, zzub::envelope_entry& env) {
	xml_node item = parent.append_child(node_element);
	item.set_name("envelope");

	xml_node adsr = item.append_child(node_element);
	adsr.set_name("adsr");
	adsr.append_attribute("attack") = double(env.attack) / 65535.0;
	adsr.append_attribute("decay") = double(env.decay) / 65535.0;
	adsr.append_attribute("sustain") = double(env.sustain) / 65535.0;
	adsr.append_attribute("release") = double(env.release) / 65535.0;	
	adsr.append_attribute("precision") = double(env.subDivide) / 127.0;
	// no adsr flags yet?
	// please never add flags directly, instead set boolean values.
	
	xml_node points = item.append_child(node_element);
	points.set_name("points");
	for (size_t i = 0; i != env.points.size(); ++i) {
		envelope_point &pt = env.points[i];
		xml_node point = points.append_child(node_element);
		point.set_name("e");
		point.append_attribute("t") = double(pt.x) / 65535.0;
		point.append_attribute("v") = double(pt.y) / 65535.0;
		if (pt.flags & zzub::envelope_flag_sustain) {
			point.append_attribute("sustain") = true;
		}
		if (pt.flags & zzub::envelope_flag_loop) {
			point.append_attribute("loop") = true;
		}
	}
	
	return item;
}

xml_node CcmWriter::saveEnvelopes(xml_node &parent, zzub::wave_info_ex &info) {
	xml_node item = parent.append_child(node_element);
	item.set_name("envelopes");
	
	for (size_t i=0; i != info.envelopes.size(); ++i) {
		if (!info.envelopes[i].disabled) {
			xml_node envnode = saveEnvelope(item, info.envelopes[i]);
			envnode.append_attribute("index") = (int)i;
		}
	}
	
	return item;
}

int midi_to_buzz_note(int value) {
	return ((value / 12) << 4) + (value % 12) + 1;
}

int buzz_to_midi_note(int value) {
	return 12 * (value >> 4) + (value & 0xf) - 1;
}

xml_node CcmWriter::saveWave(xml_node &parent, zzub::wave_info_ex &info) {
	xml_node item = parent.append_child(node_element);
	item.set_name("instrument");
	
	item.append_attribute("id") = id_from_ptr(&info).c_str();
	item.append_attribute("name") = info.name.c_str();
	item.append_attribute("volume") = (double)info.volume;
	
	saveEnvelopes(item, info);
	
	xml_node waves = item.append_child(node_element);
	waves.set_name("waves");

	for (int j = 0; j < info.get_levels(); j++) {
		if (info.get_sample_count(j)) {
			wave_level_ex &level = info.levels[j];
			xml_node levelnode = waves.append_child(node_element);
			levelnode.set_name("wave");
			levelnode.append_attribute("id") = id_from_ptr(&level).c_str();
			levelnode.append_attribute("index") = j;
			levelnode.append_attribute("filename") = info.fileName.c_str();
			
			// store root notes as midinote values
			levelnode.append_attribute("rootnote") = buzz_to_midi_note(level.root_note);
			
			levelnode.append_attribute("loopstart") = level.loop_start;
			levelnode.append_attribute("loopend") = level.loop_end;
			
			// most flags are related to levels, not to the instrument
			// this is clearly wrong and has to be fixed once we support
			// more than one wave level in the application.
			if (info.flags & zzub::wave_flag_loop) {
				levelnode.append_attribute("loop") = true;
			}
			if (info.flags & zzub::wave_flag_pingpong) {
				levelnode.append_attribute("pingpong") = true;
			}
			if (info.flags & zzub::wave_flag_envelope) {
				levelnode.append_attribute("envelope") = true;
			}
			
			xml_node slices = levelnode.append_child(node_element);
			slices.set_name("slices");
			for (size_t k = 0; k < level.slices.size(); k++) {
				xml_node slice = slices.append_child(node_element);
				slice.set_name("slice");
				slice.append_attribute("value") = level.slices[k];
			}
			
			// flac can't store anything else than default samplerates,
			// and most possibly the same goes for ogg vorbis as well
			// so save the samplerate here instead.
			levelnode.append_attribute("samplerate") = (int)info.get_samples_per_sec(j);
			
			// flac is going to be the standard format. maybe we're
			// going to implement ogg vorbis once the need arises.
			std::string wavename = id_from_ptr(&level) + ".flac";
			levelnode.append_attribute("src") = wavename.c_str();
			
			arch.createFileInArchive(wavename);
			// TODO: floats should be saved in a different format
			encodeFLAC(&arch, info, j);
			arch.closeFileInArchive();
		}
		
	}

	return item;
}

xml_node CcmWriter::saveWaves(xml_node &parent, zzub::song &player) {
	xml_node item = parent.append_child(node_element);
	item.set_name("instruments");
	
    for (size_t i = 0; i < player.wavetable.waves.size(); i++) {

        // NOTE: getting reference, ~wave_info_ex frees the sample data!!1
        wave_info_ex& info = *player.wavetable.waves[i];

        if (info.get_levels()) {
			xml_node instr = saveWave(item, info);
			instr.append_attribute("index") = (int)i;
		}
    }
	
	return item;
}

bool CcmWriter::save(std::string fileName, zzub::player* player) {

	if (!arch.create(fileName)) return false;

	const char* loc = setlocale(LC_NUMERIC, "C");

	pugi::xml_document xml;

	xml_node xmldesc = xml.append_child(node_pi);
	xmldesc.set_name("xml");
	xmldesc.set_value("version=\"1.0\" encoding=\"utf8\"");
//	xmldesc.append_attribute("version") = "1.0";
//	xmldesc.append_attribute("encoding") = "utf-8";
	xml_node xmix = xml.append_child(node_element);
	xmix.set_name("xmix");
	xmix.append_attribute("xmlns:xmix") = "http://www.zzub.org/ccm/xmix";
	
	// save meta information
	saveHead(xmix, player->front);
	
	// save main sequencer settings
	{
		double tpbfac = 1.0/double(player->front.plugin_get_parameter(0, 1, 0, 2));	
		
		//sequencer &seq = player->song_sequencer;
		xml_node item = xmix.append_child(node_element);
		item.set_name("transport");
		item.append_attribute("bpm") = double(player->front.plugin_get_parameter(0, 1, 0, 1));
		item.append_attribute("tpb") = double(player->front.plugin_get_parameter(0, 1, 0, 2));
		item.append_attribute("loopstart") = double(player->front.song_loop_begin) * tpbfac;
		item.append_attribute("loopend") = double(player->front.song_loop_end) * tpbfac;
		item.append_attribute("start") = double(player->front.song_begin) * tpbfac;
		item.append_attribute("end") = double(player->front.song_end) * tpbfac;
	}
	
	// save classes
	saveClasses(xmix, player->front);
	
	// save plugins
	savePlugins(xmix, player->front);
	
	// save waves
	saveWaves(xmix, player->front);
	
	std::ostringstream oss("");
	xml.print(oss);
	std::string xmlstr = oss.str();
	arch.createFileInArchive("song.xmix");
	arch.write((void*)xmlstr.c_str(), xmlstr.length());
	arch.closeFileInArchive();
	
	arch.close();
	
	setlocale(LC_NUMERIC, loc);

    return true;
}


static FLAC__StreamEncoderWriteStatus flac_stream_encoder_write_callback(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data) {
	zzub::outstream* writer=(zzub::outstream*)client_data;

    writer->write((void*)buffer, sizeof(FLAC__byte)*bytes);
	return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
	//return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
}

void flac_stream_encoder_metadata_callback(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data) {
	/*
	 * Nothing to do; if we get here, we're decoding to stdout, in
	 * which case we can't seek backwards to write new metadata.
	 */
	(void)encoder, (void)metadata, (void)client_data;
} 


struct DecodedFrame {
    void* buffer;
    size_t bytes;
};

struct DecoderInfo {
    DecoderInfo() {
        reader=0;
        totalSamples=0;
    }
    std::vector<DecodedFrame> buffers;
    size_t totalSamples;
	zzub::instream* reader;

};

//The address of the buffer to be filled is supplied, along with the number of bytes the 
// buffer can hold. The callback may choose to supply less data and modify the byte count
// but must be careful not to overflow the buffer. The callback then returns a status code
// chosen from FLAC__StreamDecoderReadStatus.
static FLAC__StreamDecoderReadStatus flac_stream_decoder_read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data) {
	DecoderInfo* info = (DecoderInfo*)client_data;

	if (info->reader->position() >= info->reader->size()-1) return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;

	int bytesRead = info->reader->read(buffer, *bytes);
	if (bytesRead!=*bytes) *bytes = bytesRead;
	return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

FLAC__StreamDecoderWriteStatus flac_stream_decoder_write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data) {
    DecoderInfo* info=(DecoderInfo*)client_data;
    
    // An array of pointers to decoded channels of data. Each pointer will point to an array of signed samples of length frame->header.blocksize. Currently, the channel order has no meaning except for stereo streams; in this case channel 0 is left and 1 is right.
    size_t numSamples=frame->header.blocksize;
    size_t channels=frame->header.channels;
    int bytesPerSample=(frame->header.bits_per_sample / 8);
    size_t bufferSize=bytesPerSample*numSamples*channels;

    void* vp;
    char* cp=new char[bufferSize];
    vp=cp;

    for (size_t i=0; i<numSamples; i++) {
        memcpy(cp, &buffer[0][i], bytesPerSample);
        cp+=bytesPerSample;
        if (channels==2) {
            memcpy(cp, &buffer[1][i], bytesPerSample);
            cp+=bytesPerSample;
        }
    }

    DecodedFrame df;
    df.bytes=bufferSize;
    df.buffer=vp;
    info->buffers.push_back(df);
    info->totalSamples+=numSamples;

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    // FLAC__STREAM_DECODER_WRITE_STATUS_ABORT 
}

void flac_stream_decoder_metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
    //MessageBox(0, "we got meta", "", MB_OK);
}

void flac_stream_decoder_error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data) {
    //MessageBox(0, "we got error", "", MB_OK);
}

void decodeFLAC(zzub::instream* reader, zzub::player& player, int wave, int level) {

    FLAC__StreamDecoder* stream=FLAC__stream_decoder_new();
    FLAC__stream_decoder_set_read_callback(stream, flac_stream_decoder_read_callback);
    FLAC__stream_decoder_set_write_callback(stream, flac_stream_decoder_write_callback);
    FLAC__stream_decoder_set_metadata_callback(stream, flac_stream_decoder_metadata_callback);
    FLAC__stream_decoder_set_error_callback(stream, flac_stream_decoder_error_callback);

    DecoderInfo decoder_info;
    decoder_info.reader = reader;
    FLAC__stream_decoder_set_client_data(stream, &decoder_info);

    // we're not intersted in meitadeita
    FLAC__stream_decoder_set_metadata_ignore_all(stream);

    FLAC__stream_decoder_init(stream);
//    FLAC__stream_decoder_process_single(stream);
    FLAC__stream_decoder_process_until_end_of_stream(stream);
    
    FLAC__stream_decoder_finish(stream);
    

//    FLAC__ChannelAssignment channel_assignment=FLAC__stream_decoder_get_channel_assignment(stream);
    int channels = FLAC__stream_decoder_get_channels(stream);
    unsigned int bps = FLAC__stream_decoder_get_bits_per_sample(stream);
    unsigned int sample_rate = FLAC__stream_decoder_get_sample_rate(stream);

    // allocate a level based on the stats retreived from the decoder stream
    zzub::wave_buffer_type waveFormat;
    switch (bps) {
        case 16:
            waveFormat = wave_buffer_type_si16;
            break;
        case 24:
            waveFormat = wave_buffer_type_si24;
            break;
        case 32:
            waveFormat = wave_buffer_type_si32;
            break;
        default:
            throw "not a supported bitsize";
    }

	player.wave_allocate_level(wave, level, decoder_info.totalSamples, channels, waveFormat);
	//bool result = info.allocate_level(level, decoder_info.totalSamples, waveFormat, channels==2);
	//assert(result); // lr: you don't want to let this one go unnoticed. either bail out or give visual cues.

	wave_info_ex& w = *player.back.wavetable.waves[wave];
	wave_level_ex& l = w.levels[level];

	char* targetBuf = (char*)l.samples;
    for (size_t i = 0; i < decoder_info.buffers.size(); i++) {
        DecodedFrame frame = decoder_info.buffers[i];
        memcpy(targetBuf, frame.buffer, frame.bytes);
        targetBuf += frame.bytes;
		delete[] (char*)frame.buffer; // discard buffer
    }

    // clean up
    FLAC__stream_decoder_delete(stream);

}

bool encodeFLAC(zzub::outstream* writer, wave_info_ex& info, int level) {

    int channels=info.get_stereo()?2:1;
	// flac is not going to encode anything that's not
	// having a standard samplerate, and since that
	// doesn't matter anyway, pick the default one.
    int sample_rate=44100; //info.getSamplesPerSec(level);
    int bps=info.get_bits_per_sample(level);
    int num_samples=info.get_sample_count(level);


    FLAC__StreamEncoder *stream=FLAC__stream_encoder_new(); 

	FLAC__stream_encoder_set_channels(stream, channels);
	FLAC__stream_encoder_set_bits_per_sample(stream, bps);
	FLAC__stream_encoder_set_sample_rate(stream, sample_rate);
	FLAC__stream_encoder_set_total_samples_estimate(stream, num_samples);
	FLAC__stream_encoder_set_write_callback(stream, flac_stream_encoder_write_callback);
	FLAC__stream_encoder_set_metadata_callback(stream, flac_stream_encoder_metadata_callback);
	FLAC__stream_encoder_set_client_data(stream, writer);

	int result = FLAC__stream_encoder_init(stream);
	// if this fails, we want it to crash hard - or else will cause dataloss
	assert(result == FLAC__STREAM_ENCODER_OK);

#define CHUNK_OF_SAMPLES 2048
    
    FLAC__int32 buffer[FLAC__MAX_CHANNELS][CHUNK_OF_SAMPLES]; 
    FLAC__int32* input_[FLAC__MAX_CHANNELS];
 
	for(int i = 0; i < FLAC__MAX_CHANNELS; i++)
		input_[i] = &(buffer[i][0]);
 
    bool done=false;
//    unsigned int ofs=0;
    
    SampleEnumerator samples(info, level, -1);

    while (!done) {
        unsigned int len=0;
        for (int i=0; i<CHUNK_OF_SAMPLES; i++) {
            buffer[0][len]=samples.getInt(0);
            if (channels>=2) buffer[1][len]=samples.getInt(1);
//            ofs++;
			//std::cout << "0: " << samples.getInt(0) << ", 1: " << samples.getInt(1) << std::endl;
            len++;

            if (!samples.next()) {
                done=true;
                break;
            }
        }

        FLAC__stream_encoder_process(stream, input_, len); 
        
    }

    FLAC__stream_encoder_finish(stream);
    FLAC__stream_encoder_delete(stream);
	return true;
}

/***

    CcmReader

***/

void CcmReader::registerNodeById(xml_node &item) {
	if (!item.attribute("id").empty()) {
		assert(getNodeById(item.attribute("id").value()).empty()); // make sure no other node has the same id
		nodes.insert(std::pair<std::string,xml_node>(item.attribute("id").value(), item));
	}
}

xml_node CcmReader::getNodeById(const std::string &id) {
	idnodemap::iterator i = nodes.find(id);
	if (i != nodes.end())
		return i->second;
	return xml_node(); // return empty node
}

bool CcmReader::for_each(xml_node& node) {
	registerNodeById(node);	
	return true;
}

bool CcmReader::loadClasses(xml_node &classes, zzub::player &player) {
	for (xml_node::iterator i = classes.begin(); i != classes.end(); ++i) {
		if (!strcmp(i->name(), "pluginclass")) {
			std::string uri = i->attribute("id").value();
			const zzub::info* loader = player.plugin_get_info(uri);
			if (!loader) { // no loader for this 
				mem_archive arc;
				// do we have some data saved?
				for (xml_node::iterator data = i->begin(); data != i->end(); ++data) {
					if (!strcmp(data->name(), "data") && (!data->attribute("src").empty())) {
						std::cout << "ccm: storing data for " << data->attribute("src").value() << std::endl;
						// store data in archive
						compressed_file_info cfi;
						if (arch.openFileInArchive(data->attribute("src").value(), &cfi)) {
							std::vector<char> &b = arc.get_buffer(data->attribute("base").value());
							b.resize(cfi.uncompressed_size);
							arch.read(&b[0], cfi.uncompressed_size);
							arch.closeFileInArchve();
						}
					}
				}
				bool found = false;
				if (arc.buffers.size()) {
					std::cout << "ccm: searching for loader for " << uri << std::endl;
					std::vector<pluginlib*>::iterator lib;
					for (lib = player.plugin_libraries.begin(); lib != player.plugin_libraries.end(); ++lib) {
						if ((*lib)->collection) {
							const zzub::info *_info = (*lib)->collection->get_info(uri.c_str(), &arc);
							if (_info) { // library could read archive
								(*lib)->register_info(_info); // register the new info
								found = true;
							}
						}
					}
					if (!player.plugin_libraries.size()) {
						std::cerr << "ccm: warning: no plugin libraries available." << std::endl;
					}
				}
				if (!found) {
					std::cout << "ccm: couldn't find loader for " << uri << std::endl;
				}
			}
			xml_node parameters = i->child("parameters");
			if (!parameters.empty()) {
				xml_node global = parameters.child("global");
				if (!global.empty()) {
				}
				xml_node track = parameters.child("track");
				if (!track.empty()) {
				}
			}
		}
	}
	return true;
}



struct ccache { // connection and node cache
	int target;
	xml_node connections;
	xml_node global;
	xml_node tracks;
	xml_node sequences;
	xml_node eventtracks;
	xml_node midi;
};

bool CcmReader::loadPlugins(xml_node plugins, zzub::player &player) {
	
	operation_copy_flags flags;
	flags.copy_plugins = true;
	flags.copy_graph = true;
	flags.copy_wavetable = true;
	player.merge_backbuffer_flags(flags);

	std::map<std::string, int> id2plugin;
	
	std::vector<ccache> conns;
	
	for (xml_node::iterator i = plugins.begin(); i != plugins.end(); ++i) {
		if (!strcmp(i->name(), "plugin")) {
			mem_archive arc;
			xml_node position;
			xml_node init;
			xml_node attribs;
			xml_node global;
			xml_node tracks;
			xml_node connections;
			xml_node sequences;
			xml_node eventtracks;
			xml_node midi;
			
			// enumerate relevant nodes
			for (xml_node::iterator j = i->begin(); j != i->end(); ++j) {
				if (!strcmp(j->name(), "init")) {
					init = *j; // store for later
					
					// enumerate init section
					for (xml_node::iterator k = j->begin(); k != j->end(); ++k) {
						if ((!strcmp(k->name(), "data")) && (!k->attribute("src").empty())) {
							// store data for later
							compressed_file_info cfi;
							if (arch.openFileInArchive(k->attribute("src").value(), &cfi)) {
								std::vector<char> &b = arc.get_buffer(k->attribute("base").value());
								b.resize(cfi.uncompressed_size);
								arch.read(&b[0], cfi.uncompressed_size);
								arch.closeFileInArchve();
							}
						} else if (!strcmp(k->name(), "attributes")) {
							attribs = *k; // store for later
						} else if (!strcmp(k->name(), "global")) {
							global = *k; // store for later
						} else if (!strcmp(k->name(), "tracks")) {
							tracks = *k; // store for later
						}
					}
				} else if (!strcmp(j->name(), "position")) {
					position = *j; // store for later
				} else if (!strcmp(j->name(), "connections")) {
					connections = *j; // store for later
				} else if (!strcmp(j->name(), "sequences")) {
					sequences = *j; // store for later
				} else if (!strcmp(j->name(), "eventtracks")) {
					eventtracks = *j; // store for later
				} else if (!strcmp(j->name(), "midi")) {
					midi = *j; // store for later
				}
			}
			
			const zzub::info *loader = player.plugin_get_info(i->attribute("ref").value());
			int plugin_id;
			if (!strcmp(i->attribute("ref").value(), "@zzub.org/master")) {
				plugin_id = 0;//player.master;
			} else if (loader) {
				std::vector<char> &b = arc.get_buffer("");

				plugin_id = player.create_plugin(b, i->attribute("name").value(), loader);

			} else {
				std::cerr << "ccm: unable to find loader for uri " << i->attribute("ref").value() << std::endl;
				plugin_id = -1;
			}

			if (plugin_id != -1 && player.back.plugins[plugin_id] != 0) {
				metaplugin& m = *player.back.plugins[plugin_id];

				id2plugin.insert(std::pair<std::string, int>(i->attribute("id").value(), plugin_id));

				if (!position.empty()) {
					m.x = position.attribute("x").as_float();
					m.y = position.attribute("y").as_float();
				}

				if (!attribs.empty()) {
					for (xml_node::iterator a = attribs.begin(); a != attribs.end(); ++a) {
						for (size_t pa = 0; pa != m.info->attributes.size(); ++pa) {
							if (!strcmp(m.info->attributes[pa]->name, a->attribute("name").value())) {
								m.plugin->attributes[pa] = a->attribute("v").as_int();
							}
						}
					}
				}

				m.plugin->attributes_changed();

				if (!tracks.empty()) {
					vector<xml_node> tracknodes;
					tracks.all_elements_by_name("track", std::back_inserter(tracknodes));
					int trackcount = (int)tracknodes.size();
					for (unsigned int a = 0; a != tracknodes.size(); ++a) {
						// if we find any track with a higher index, extend the size
						trackcount = std::max(trackcount, tracknodes[a].attribute("index").as_int());
					}

					player.plugin_set_track_count(plugin_id, trackcount);
				}

				// plugin default parameter values are read after connections are made
				ccache cc;
				cc.target = plugin_id;
				cc.connections = connections;
				cc.global = global;
				cc.tracks = tracks;
				cc.sequences = sequences;
				cc.eventtracks = eventtracks;
				cc.midi = midi;
				conns.push_back(cc);
				
			}
		}
	}
	
	// make connections
	for (std::vector<ccache>::iterator c = conns.begin(); c != conns.end(); ++c) {

		if (!c->connections.empty()) {
			for (xml_node::iterator i = c->connections.begin(); i != c->connections.end(); ++i) {
				if (!strcmp(i->name(), "input")) {
					std::map<std::string, int>::iterator iplug = id2plugin.find(i->attribute("ref").value());
					if (iplug != id2plugin.end()) {
						std::string conntype = "audio";
						if (!i->attribute("type").empty()) {
							conntype = i->attribute("type").value();
						}
						if (conntype == "audio") {
							if (player.plugin_add_input(c->target, iplug->second, connection_type_audio)) {
								int amp = double_to_amp((double)i->attribute("amplitude").as_double());
								int pan = double_to_pan((double)i->attribute("panning").as_double());
								int track = player.back.plugin_get_input_connection_count(c->target) - 1;

								player.plugin_set_parameter(c->target, 0, track, 0, amp, false, false, false);
								player.plugin_set_parameter(c->target, 0, track, 1, pan, false, false, false);
							} else
								assert(false);
						} else if (conntype == "event") {
							// restore controller associations
							player.plugin_add_input(c->target, iplug->second, connection_type_event);

							for (xml_node::iterator j = i->begin(); j != i->end(); ++j) {
								if (!strcmp(j->name(), "bindings")) {
									for (xml_node::iterator k = j->begin(); k != j->end(); ++k) {
										if (!strcmp(k->name(), "binding")) {
											event_connection_binding binding;
											long source_param_index = long(k->attribute("source_param_index").as_int());
											long target_group_index = long(k->attribute("target_group_index").as_int());
											long target_track_index = long(k->attribute("target_track_index").as_int());
											long target_param_index = long(k->attribute("target_param_index").as_int());
											
											player.plugin_add_event_connection_binding(c->target, iplug->second, 
												source_param_index, target_group_index, target_track_index, target_param_index);
										}
									}
								}
							}
						} else if (conntype == "midi") {
							assert(false);
							player.plugin_add_input(c->target, iplug->second, connection_type_midi);
							player.plugin_set_midi_connection_device(c->target, iplug->second, i->attribute("device").value());
						} else {
							assert(0);
						}
					} else {
						std::cerr << "ccm: no input " << i->attribute("ref").value() << " for connection " << i->attribute("id").value() << std::endl;
					}
					
				}
			}
		}
		
		// now that connections are set up, we can load the plugin default state values
		if (!c->global.empty()) {
			for (xml_node::iterator i = c->global.begin(); i != c->global.end(); ++i) {
				if (!strcmp(i->name(), "n")) {
					xml_node paraminfo = getNodeById(i->attribute("ref").value());
					assert(!paraminfo.empty()); // not being able to deduce the index is fatal
					if ((bool)paraminfo.attribute("state") == true) {
						
						// test if the parameter names correspond with index position
						int plugin = c->target;
						long index = long(paraminfo.attribute("index").as_int());
						const parameter* param = player.back.plugin_get_parameter_info(plugin, 1, 0, index);
						std::string name = paraminfo.attribute("name").value();
						const zzub::info* info = player.back.plugins[plugin]->info;
						
						if (index < info->global_parameters.size() && param->name == name) {
							player.plugin_set_parameter(plugin, 1, 0, index, long(i->attribute("v").as_int()), false, false, false);
						} else {
							// else search for a parameter name that matches
							for (size_t pg = 0; pg != info->global_parameters.size(); ++pg) {
								const parameter* pgp = player.back.plugin_get_parameter_info(plugin, 1, 0, pg);
								if (pgp->name == name) {
									player.plugin_set_parameter(plugin, 1, 0, pg, long(i->attribute("v").as_int()), false, false, false);
									break;
								}
							}
						}
					}
				}
			}
		}
		if (!c->tracks.empty()) {
			for (xml_node::iterator i = c->tracks.begin(); i != c->tracks.end(); ++i) {
				if (!strcmp(i->name(), "track")) {
					long t = long(i->attribute("index").as_int());
					for (xml_node::iterator j = i->begin(); j != i->end(); ++j) {
						if (!strcmp(j->name(), "n")) {
							xml_node paraminfo = getNodeById(j->attribute("ref").value());
							assert(!paraminfo.empty()); // not being able to deduce the index is fatal
							if ((bool)paraminfo.attribute("state") == true) {
								// test if the parameter names correspond with index position
								int plugin = c->target;
								long index = long(paraminfo.attribute("index").as_int());
								const parameter* param = player.back.plugin_get_parameter_info(plugin, 2, t, index);
								std::string name = paraminfo.attribute("name").value();
								const zzub::info* info = player.back.plugins[plugin]->info;

								if ((index < info->track_parameters.size()) && param->name == name) {
									player.plugin_set_parameter(plugin, 2, t, index, long(j->attribute("v").as_int()), false, false, false);
								} else {
									// else search for a parameter name that matches
									for (size_t pt = 0; pt != info->track_parameters.size(); ++pt) {
										const parameter* ptp = player.back.plugin_get_parameter_info(plugin, 2, t, pt);
										if (ptp->name == name) {
											player.plugin_set_parameter(plugin, 2, t, pt, long(j->attribute("v").as_int()), false, false, false);
											break;
										}
									}
								}

							}
						}
					}
				}
			}
		}

		player.back.process_plugin_events(c->target);
	}


		
	// we need to create patterns in the second iteration, since the master
	// might not be neccessarily the first plugin to be initialized
	double tpbfac = double(player.back.plugin_get_parameter(0, 1, 0, 2));


	for (std::vector<ccache>::iterator c = conns.begin(); c != conns.end(); ++c) {
		
		if (!c->eventtracks.empty()) {
			// load and create patterns
			for (xml_node::iterator i = c->eventtracks.begin(); i != c->eventtracks.end(); ++i) {
				if (!strcmp(i->name(), "events")) {
					int rows = int(double(i->attribute("length").as_double()) * tpbfac + 0.5);
					int plugin = c->target;
					pattern p = player.back.create_pattern(plugin, rows);
					p.name = i->attribute("name").value();
					player.plugin_add_pattern(plugin, p);
					int pattern_index = player.back.plugins[plugin]->patterns.size() - 1;
					i->append_attribute("index") = pattern_index;  // set an index attribute
					for (xml_node::iterator j = i->begin(); j != i->end(); ++j) {
						int group = 1;
						int track = 0;
						if (!strcmp(j->name(), "g")) {
							group = 1;
						} else if (!strcmp(j->name(), "c")) {
							group = 0;
						} else if (!strcmp(j->name(), "t")) {
							group = 2;
							track = (int)long(j->attribute("index").as_int());
						}


						if (group != 0) { // TODO: connections not supported yet
							for (xml_node::iterator k = j->begin(); k != j->end(); ++k) {
								if (!strcmp(k->name(), "e")) {
									int row = int(double(k->attribute("t").as_double()) * tpbfac + 0.5);
									assert(row < rows);
									xml_node paraminfo = getNodeById(k->attribute("ref").value());
									assert(!paraminfo.empty()); // not being able to deduce the index is fatal
									player.plugin_set_pattern_value(plugin, pattern_index, group, track, long(paraminfo.attribute("index").as_int()), row, long(k->attribute("v").as_int()));
								}
							}
						}

					}
				}
			}
		}

		
		if (!c->sequences.empty()) {
			// load and create sequences
			for (xml_node::iterator i = c->sequences.begin(); i != c->sequences.end(); ++i) {
				if (!strcmp(i->name(), "sequence")) {

					player.sequencer_add_track(c->target);

					int seq_track = player.back.sequencer_tracks.size() - 1;
					
					for (xml_node::iterator j = i->begin(); j != i->end(); ++j) {
						if (!strcmp(j->name(), "e")) {
							int row = int(double(j->attribute("t").as_double()) * tpbfac + 0.5);
							int value = -1;
							if (bool(j->attribute("mute"))) {
								value = sequencer_event_type_mute;
							} else if (bool(j->attribute("break"))) {
								value = sequencer_event_type_break;
							} else if (bool(j->attribute("thru"))) {
								value = sequencer_event_type_thru;
							} else {
								xml_node patternnode = getNodeById(j->attribute("ref").value());
								assert(!patternnode.empty()); // shouldn't be empty
								value = 0x10 + long(patternnode.attribute("index").as_int());
							}
							player.sequencer_set_event(seq_track, row, value);
						}
					}
				}
			}

		}


		if (!c->midi.empty()) {
			// load and set up controller bindings
			for (xml_node::iterator i = c->midi.begin(); i != c->midi.end(); ++i) {
				if (!strcmp(i->name(), "bind")) {
					xml_node refnode = getNodeById(i->attribute("ref").value());
					assert(!refnode.empty());
					
					int channel = long(i->attribute("channel").as_int());
					int controller = long(i->attribute("controller").as_int());
					
					std::string target = i->attribute("target").value();
					
					int group;
					int track = 0;
					int index = 0;
					if (target == "amp") {
						group = 0;
						index = 0;
						track = long(i->attribute("track").as_int());
					} else if (target == "pan") {
						group = 0;
						index = 1;
						track = long(i->attribute("track").as_int());
					} else if (target == "global") {
						group = 1;
						track = long(i->attribute("track").as_int());
						index = long(refnode.attribute("index").as_int());
					} else if (target == "track") {
						group = 2;
						track = long(i->attribute("track").as_int());
						index = long(refnode.attribute("index").as_int());
					} else {
						assert(0);
					}
					
					player.add_midimapping(c->target, group, track, index, channel, controller);
				}
			}
		}

	}

	player.flush_operations(0, 0, 0);

	return true;
}

bool CcmReader::loadInstruments(xml_node &instruments, zzub::player &player) {
	
    // load wave table
    // load instruments

	for (xml_node::iterator i = instruments.begin(); i != instruments.end(); ++i) {
		if (!strcmp(i->name(), "instrument")) {
			int wave_index = long(i->attribute("index").as_int());
			//wave_info_ex &info = *player.front.wavetable.waves[wave_index];
			//info.name = i->attribute("name").value();
			//info.flags = 0;
			player.wave_set_name(wave_index, i->attribute("name").value());
			
			xml_node waves = i->child("waves");
			if (!waves.empty()) {
				for (xml_node::iterator w = waves.begin(); w != waves.end(); ++w) {
					if (!strcmp(w->name(), "wave")) {
						if (arch.openFileInArchive(w->attribute("src").value())) {
							long index = long(w->attribute("index").as_int());
							decodeFLAC(&arch, player, wave_index, index);
							arch.closeFileInArchve();
							wave_info_ex &info = *player.back.wavetable.waves[wave_index];
							wave_level_ex &level = info.levels[index];

							int flags = info.flags;
							if (bool(w->attribute("loop").as_int()))
								flags |= wave_flag_loop;
							if (bool(w->attribute("pingpong").as_int()))
								flags |= wave_flag_pingpong;
							if (bool(w->attribute("envelope").as_int()))
								flags |= wave_flag_envelope;

							player.wave_set_flags(wave_index, flags);
							player.wave_set_path(wave_index, w->attribute("filename").value());
							player.wave_set_root_note(wave_index, index, midi_to_buzz_note(long(w->attribute("rootnote").as_int())));
							player.wave_set_loop_begin(wave_index, index, long(w->attribute("loopstart").as_int()));
							player.wave_set_loop_end(wave_index, index, long(w->attribute("loopend").as_int()));
							if (!w->attribute("samplerate").empty()) {
								player.wave_set_samples_per_second(wave_index, index, long(w->attribute("samplerate").as_int()));
							}

							xml_node slices = w->child("slices");
							if (!slices.empty()) {
								for (xml_node::iterator s = slices.begin(); s != slices.end(); ++s) {
									if (!strcmp(s->name(), "slice")) {
										level.slices.push_back(long(s->attribute("value").as_int()));
									}
								}
							}
						}
					}
				}
			}

			// NOTE: must set volume after (first) allocate_level (which happens in decodeFLAC)
			player.wave_set_volume(wave_index, (float)double(i->attribute("volume").as_double()));
			//info.volume = (float)double(i->attribute("volume"));
		
			vector<envelope_entry> envs;
			xml_node envelopes = i->child("envelopes");
			if (!envelopes.empty()) {
				for (xml_node::iterator e = envelopes.begin(); e != envelopes.end(); ++e) {
					if (!strcmp(e->name(), "envelope")) {
						//wave_info_ex &info = *player.back.wavetable.waves[wave_index];
						long index = long(e->attribute("index").as_int());
						if (envs.size() <= (size_t)index) {
							envs.resize(index+1);
						}
						envelope_entry &env = envs[index];
						env.disabled = false;
						xml_node adsr = i->child("adsr");
						if (!adsr.empty()) {
							env.attack = int(double(adsr.attribute("attack").as_double()) * 65535.0 + 0.5);
							env.decay = int(double(adsr.attribute("decay").as_double()) * 65535.0 + 0.5);
							env.sustain = int(double(adsr.attribute("sustain").as_double()) * 65535.0 + 0.5);
							env.release = int(double(adsr.attribute("release").as_double()) * 65535.0 + 0.5);
							env.subDivide = int(double(adsr.attribute("precision").as_double()) * 127.0 + 0.5);
						}
						xml_node points = i->child("points");
						if (!points.empty()) {
							env.points.clear();
							for (xml_node::iterator pt = points.begin(); pt != points.end(); ++pt) {
								if (!strcmp(pt->name(), "e")) {
									envelope_point evpt;
									evpt.x = int(double(pt->attribute("t").as_double()) * 65535.0 + 0.5);
									evpt.y = int(double(pt->attribute("v").as_double()) * 65535.0 + 0.5);
									evpt.flags = 0;
									
									if (bool(pt->attribute("sustain").as_int())) {
										evpt.flags |= zzub::envelope_flag_sustain;
									}
									if (bool(pt->attribute("loop").as_int())) {
										evpt.flags |= zzub::envelope_flag_loop;
									}
									env.points.push_back(evpt);
									// TODO: sort points. points might not be written down
									// in correct order
								}
							}
						}
					}
				}
			}
			player.wave_set_envelopes(wave_index, envs);

		}
	}

	return true;
}

bool CcmReader::loadSequencer(xml_node &item, zzub::player &player) {
	double tpbfac = double(player.front.plugin_get_parameter(0, 1, 0, 2));	
	
	//sequencer &seq = player.song_sequencer;
	player.front.song_loop_begin = int(double(item.attribute("loopstart").as_double()) * tpbfac + 0.5);
	player.front.song_loop_end = int(double(item.attribute("loopend").as_double()) * tpbfac + 0.5);
	player.front.song_begin = int(double(item.attribute("start").as_double()) * tpbfac + 0.5);
	player.front.song_end = int(double(item.attribute("end").as_double()) * tpbfac + 0.5);
	return true;
}

	
bool CcmReader::open(std::string fileName, zzub::player* player) {

	const char* loc = setlocale(LC_NUMERIC, "C");

	bool result = false;
	player->set_state(player_state_muted);
	
	if (arch.open(fileName)) {
		compressed_file_info cfi;
		if (arch.openFileInArchive("song.xmix", &cfi)) {
			char *xmldata = new char[cfi.uncompressed_size+1];
			xmldata[cfi.uncompressed_size] = '\0';
			arch.read(xmldata, cfi.uncompressed_size);
			arch.closeFileInArchve();
			xml_document xml;
			if (xml.parse(xmldata)) {
				xml_node xmix = xml.child("xmix");				
				if (!xmix.empty()) {
					xmix.traverse(*this); // collect all ids
					
					// load song meta information
					for (xml_node::iterator m = xmix.begin(); m != xmix.end(); ++m) {
						if (!strcmp(m->name(), "meta")) {
							if (!strcmp(m->attribute("name").value(), "comment") && !m->attribute("src").empty()) {
								if (arch.openFileInArchive(m->attribute("src").value(), &cfi)) {
									std::vector<char> infotext;
									infotext.resize(cfi.uncompressed_size+1);
									infotext[cfi.uncompressed_size] = '\0';
									arch.read(&infotext[0], cfi.uncompressed_size);
									arch.closeFileInArchve();
									player->front.song_comment = &infotext[0];
								} else {
									std::cerr << "unable to open " << m->attribute("src").value() << " for reading." << std::endl;
								}
							}
						}
					}
					
					// load parameter predefs
					xml_node classes = xmix.child("pluginclasses");
					if (classes.empty() || loadClasses(classes, *player)) {
						// load plugins (connections, parameters, patterns, sequences)
						xml_node plugins = xmix.child("plugins");
						if (plugins.empty() || loadPlugins(plugins, *player)) {
							// load instruments (waves, envelopes)
							xml_node instruments = xmix.child("instruments");
							if (instruments.empty() || loadInstruments(instruments, *player)) {
								// load song sequencer settings
								xml_node sequencer = xmix.child("transport");
								if (sequencer.empty() || loadSequencer(sequencer, *player)) {
									result = true;
								}								
							}
						}
					}
					
				} else {
					std::cerr << "ccm: no xmix node in song.xmix from " << fileName << std::endl;
				}
			} else {
				std::cerr << "ccm: error parsing song.xmix in " << fileName << std::endl;
			}
			delete[] xmldata;
		} else {
			std::cerr << "ccm: error opening song.xmix in " << fileName << std::endl;
		}
		arch.close();
	} else {
		std::cerr << "ccm: error opening " << fileName << std::endl;
	}
	
	player->flush_operations(0, 0, 0);
	player->flush_from_history();	// TODO: ccm loading doesnt support undo yet
	player->set_state(player_state_stopped);

	setlocale(LC_NUMERIC, loc);

	return result;
}


};

