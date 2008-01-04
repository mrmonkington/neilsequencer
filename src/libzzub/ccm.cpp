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

using namespace pug;

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

    char* charBuffer=(char*)buffer;
    size_t peekSize=0;
    if (hasPeeked && size>0) {
        charBuffer[0]=lastPeekByte;
        charBuffer++;
        size--;
        lastReadOfs++;
        hasPeeked=false;
        peekSize=1;
    }

    int err=unzReadCurrentFile(f, charBuffer, size);
    if (err<0) return 0;
    err+=peekSize;

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

xml_node CcmWriter::saveHead(xml_node &parent, zzub::player &player) {
	if (strlen(player.infoText.c_str())) {
		// save song info
		if (arch.createFileInArchive("readme.txt")) {
			arch.write((void*)player.infoText.c_str(), strlen(player.infoText.c_str()));
			arch.closeFileInArchive();
			xml_node commentmeta = addMeta(parent, "comment");
			commentmeta.attribute("src") = "readme.txt";
		} else {
			std::cerr << "unable to save comment in readme.txt" << std::endl;
		}
	}
	
	return parent;
}

xml_node CcmWriter::addMeta(xml_node &parent, const std::string &propname) {
	xml_node item = parent.append_child(node_element);
	item.name("meta");
	item.attribute("name") = propname;
	return item;
}

xml_node CcmWriter::saveClasses(xml_node &parent, zzub::player &player) {
	xml_node item = parent.append_child(node_element);
	item.name("pluginclasses");
	
    std::vector<pluginloader*> distinctLoaders;
    for (size_t i=0; i<player.getMachines(); i++) {
        metaplugin* plugin=player.getMachine(i);
		if (plugin->nonSongPlugin) continue;

        std::vector<pluginloader*>::iterator p=find<vector<pluginloader*>::iterator >(distinctLoaders.begin(), distinctLoaders.end(), plugin->loader);
        if (p==distinctLoaders.end()) distinctLoaders.push_back(plugin->loader);
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
	item.name("parameter");
	
	item.attribute("id") = id_from_ptr(&p);
	item.attribute("name") = p.name;
	
	item.attribute("type") = paramtype_to_string(p.type);
	item.attribute("minvalue") = (long)p.value_min;
	item.attribute("maxvalue") = (long)p.value_max;
	item.attribute("novalue") = (long)p.value_none;
	item.attribute("defvalue") = (long)p.value_default;
	
	if (p.flags & zzub::parameter_flag_wavetable_index) {
		item.attribute("waveindex") = true;
	}
	if (p.flags & zzub::parameter_flag_state) {
		item.attribute("state") = true;
	}
	if (p.flags & zzub::parameter_flag_event_on_edit) {
		item.attribute("editevent") = true;
	}
	
	return item;
}

xml_node CcmWriter::saveClass(xml_node &parent, zzub::pluginloader &pl) {

	// we take the same format as for lunar manifests

	xml_node item = parent.append_child(node_element);
	item.name("pluginclass");
	item.attribute("id") = pl.plugin_info->uri;
	if (pl.plugin_info->flags & zzub::plugin_flag_mono_to_stereo)
		item.attribute("mono_to_stereo") = true;
	if (pl.plugin_info->flags & zzub::plugin_flag_plays_waves)
		item.attribute("plays_waves") = true;
	if (pl.plugin_info->flags & zzub::plugin_flag_uses_lib_interface)
		item.attribute("uses_lib_interface") = true;
	if (pl.plugin_info->flags & zzub::plugin_flag_uses_instruments)
		item.attribute("uses_instruments") = true;
	if (pl.plugin_info->flags & zzub::plugin_flag_does_input_mixing)
		item.attribute("does_input_mixing") = true;
	if (pl.plugin_info->flags & zzub::plugin_flag_control_plugin)
		item.attribute("control_plugin") = true;
	if (pl.plugin_info->flags & zzub::plugin_flag_auxiliary)
		item.attribute("auxiliary") = true;
	if (pl.plugin_info->flags & zzub::plugin_flag_is_root)
		item.attribute("is_root") = true;
	if (pl.plugin_info->flags & zzub::plugin_flag_has_audio_input)
		item.attribute("has_audio_input") = true;
	if (pl.plugin_info->flags & zzub::plugin_flag_has_audio_output)
		item.attribute("has_audio_output") = true;
	if (pl.plugin_info->flags & zzub::plugin_flag_has_event_input)
		item.attribute("has_event_input") = true;
	if (pl.plugin_info->flags & zzub::plugin_flag_has_event_output)
		item.attribute("has_event_output") = true;
	
	mem_archive arc;
	pl.plugin_info->store_info(&arc);
	saveArchive(item, pl.plugin_info->uri, arc);
	
	xml_node params = item.append_child(node_element);
	params.name("parameters");
	
    if (pl.plugin_info->global_parameters.size()>0) {
		xml_node global = params.append_child(node_element);
		global.name("global");
		
        for (size_t i=0; i < pl.plugin_info->global_parameters.size(); i++) {
            const zzub::parameter* p = pl.plugin_info->global_parameters[i];
			saveParameter(global, *p).attribute("index") = (long)i;
        }
    }

    if (pl.plugin_info->track_parameters.size()>0) {
		xml_node track = params.append_child(node_element);
		track.name("track");
  
        for (size_t i=0; i<pl.plugin_info->track_parameters.size(); i++) {
            const zzub::parameter* p = pl.plugin_info->track_parameters[i];
			saveParameter(track, *p).attribute("index") = (long)i;
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
	item.name("binding");
	
	item.attribute("source_param_index") = (long)binding.source_param_index;
	item.attribute("target_group_index") = (long)binding.target_group_index;
	item.attribute("target_track_index") = (long)binding.target_track_index;
	item.attribute("target_param_index") = (long)binding.target_param_index;
	
	return item;
}

xml_node CcmWriter::saveEventBindings(xml_node &parent, std::vector<zzub::event_connection_binding> &bindings) {
	xml_node item = parent.append_child(node_element);
	item.name("bindings");
	
	for (size_t i=0; i<bindings.size(); i++) {
		saveEventBinding(item, bindings[i]);
	}

	return item;
}

xml_node CcmWriter::saveConnection(xml_node &parent, zzub::connection &connection) {
	xml_node item = parent.append_child(node_element);
	item.name("input");

	item.attribute("id") = id_from_ptr(&connection);
	item.attribute("ref") = id_from_ptr(connection.plugin_in);
	item.attribute("type") = connectiontype_to_string(connection.connectionType);
	
	switch(connection.connectionType)
	{
		case zzub::connection_type_audio:
		{
			zzub::audio_connection &ac = (zzub::audio_connection &)connection;
			
			// we save normals, not exposing implementation details
			item.attribute("amplitude") = amp_to_double(ac.values.amp);
			item.attribute("panning") = pan_to_double(ac.values.pan);
		} break;
		case zzub::connection_type_event:
		{
			zzub::event_connection &ac = (zzub::event_connection &)connection;
			
			saveEventBindings(item, ac.bindings);
		} break;
		default:
			assert(0);
			break;
	}
		
	return item;
}

xml_node CcmWriter::saveConnections(xml_node &parent, zzub::metaplugin &plugin) {
	xml_node item = parent.append_child(node_element);
	item.name("connections");
	
	for (size_t i=0; i<plugin.getConnections(); i++) {
		saveConnection(item, *plugin.getConnection(i));
	}

	return item;
}

xml_node CcmWriter::saveArchive(xml_node &parent, const std::string &pathbase, zzub::mem_archive &arc) {
	zzub::mem_archive::buffermap::iterator i;
	for (i = arc.buffers.begin(); i != arc.buffers.end(); ++i) {
		if (i->second.size()) {
			xml_node data = parent.append_child(node_element);
			data.name("data");
			std::string filename;
			data.attribute("type") = "raw";
			data.attribute("base") = i->first;
			if (i->first == "") {
				filename = pathbase + "/raw";
			} else {
				filename = pathbase + "/" + i->first;
			}
			arch.createFileInArchive(filename);
			arch.write(&i->second[0], i->second.size());
			arch.closeFileInArchive();
			data.attribute("src") = filename;
		}
	}
	return xml_node();
}

xml_node CcmWriter::saveInit(xml_node &parent, zzub::metaplugin &plugin) {
	xml_node item = parent.append_child(node_element);
	item.name("init");
	
	mem_archive arc;
	plugin.machine->save(&arc);
	saveArchive(item, id_from_ptr(&plugin), arc);

	if (plugin.loader->plugin_info->global_parameters.size()) {
		xml_node global = item.append_child(node_element);
		global.name("global");
		
		for (size_t i = 0; i != plugin.loader->plugin_info->global_parameters.size(); ++i) {
			xml_node n = global.append_child(node_element);
			n.name("n");
			n.attribute("ref") = id_from_ptr(plugin.loader->plugin_info->global_parameters[i]);
			n.attribute("v") = (long)plugin.getParameter(1, 0, i);
		}
	}

	if (plugin.getTracks()) {
		xml_node tracks = item.append_child(node_element);
		tracks.name("tracks");
		
		for (int t = 0; t != plugin.getTracks(); ++t) {
			xml_node track = tracks.append_child(node_element);
			track.name("track");
			track.attribute("index") = (long)t;
			
			for (size_t i = 0; i != plugin.loader->plugin_info->track_parameters.size(); ++i) {
				xml_node n = track.append_child(node_element);
				n.name("n");
				n.attribute("ref") = id_from_ptr(plugin.loader->plugin_info->track_parameters[i]);
				n.attribute("v") = (long)plugin.getParameter(2, t, i);
			}
		}
	}
	
	
    if (plugin.hasAttributes() && plugin.getAttributes()>0)
		saveAttributes(item, plugin);
	
	return item;
}

xml_node CcmWriter::saveAttributes(xml_node &parent, zzub::metaplugin &plugin) {
	xml_node item = parent.append_child(node_element);
	item.name("attributes");
	
    // save attributes
	for (size_t i=0; i<plugin.getAttributes(); i++) {
		const attribute& attr=plugin.getAttribute(i);
		xml_node n = item.append_child(node_element);
		n.name("n");
		n.attribute("name") = attr.name;
		n.attribute("v") = (long)plugin.getAttributeValue(i);
	}

	return item;
}

xml_node CcmWriter::savePatternTrack(xml_node &parent, const std::string &colname, double fac, zzub::metaplugin &plugin, zzub::pattern &p, int group, int track) {
	xml_node item = parent.append_child(node_element);
	item.name(colname);
	if (group == 2)
		item.attribute("index") = (long)track;
	
	zzub::patterntrack& t = *p.getPatternTrack(group, track);
    for (size_t i = 0; i < t.rows; ++i) {
        for (size_t j = 0; j < t.getParams(); ++j) {
			const zzub::parameter *param = 0;
			switch(group) {
				case 0: 
				{
					param = plugin.getConnection(track)->connection_parameters[j]; 
				} break;
				case 1: param = plugin.loader->plugin_info->global_parameters[j]; break;
				case 2: param = plugin.loader->plugin_info->track_parameters[j]; break;
				default: assert(0);
			}

			int value = t.getValue(i,j);
			if (value != param->value_none) {
				xml_node e = item.append_child(node_element);
				e.name("e");
				e.attribute("t") = fac * double(i);
				if (group == 0) {
					e.attribute("ref") = id_from_ptr(plugin.getConnection(track));
					// we save normals, not exposing implementation details
					if (j == 0) {
						e.attribute("amp") = amp_to_double(value);
					} else if (j == 1) {
						e.attribute("pan") = pan_to_double(value);
					} else {
						assert(0);
					}
				} else {
					e.attribute("ref") = id_from_ptr(param);
					e.attribute("v") = (long)value;
				}
			}
        }
    }
	
	return item;
}

xml_node CcmWriter::savePattern(xml_node &parent, zzub::player &player, zzub::metaplugin &plugin, zzub::pattern &p) {
	// we're going to save pattern data as events, since storing data
	// in a table is implementation detail. also, empty patterns will
	// take less space.
	
	// event positions and track length is stored in beats, not rows. this makes it
	// easier to change the tpb count manually, and have the loader adapt all
	// patterns on loading.
	
	double tpbfac = 1.0/double(player.master->getParameter(1, 0, 2));
	
	xml_node item = parent.append_child(node_element);
	item.name("events");
	item.attribute("id") = id_from_ptr(&p);
	item.attribute("name") = p.getName();
	item.attribute("length") = double(p.rows) * tpbfac;

	// save connection columns	
	for (size_t j = 0; j != p._connections.size(); ++j) {
		savePatternTrack(item, "c", tpbfac, plugin, p, 0, j);
	}

	// save globals
	savePatternTrack(item, "g", tpbfac, plugin, p, 1, 0);

	// save tracks
	for (size_t j=0; j != p._tracks.size(); j++) {
		savePatternTrack(item, "t", tpbfac, plugin, p, 2, j);
	}

	return item;
}

xml_node CcmWriter::savePatterns(xml_node &parent, zzub::player &player, zzub::metaplugin &plugin) {
	xml_node item = parent.append_child(node_element);
	item.name("eventtracks");

    for (size_t i=0; i != plugin.getPatterns(); i++) {
		savePattern(item, player, plugin, *plugin.getPattern(i));
	}
	
	return item;
}

xml_node CcmWriter::saveSequence(xml_node &parent, double fac, zzub::player &player, zzub::metaplugin &plugin, zzub::sequence &seq) {
	xml_node item = parent.append_child(node_element);
	item.name("sequence");
	
	for (size_t i = 0; i != seq.getEvents(); ++i) {
		sequence_event &ev = *seq.getEvent(i);
		xml_node e = item.append_child(node_element);
		e.name("e");
		e.attribute("t") = fac * double(ev.pos);
		if (ev.type == sequence_event_type_mute) {
			e.attribute("mute") = true;
		} else if (ev.type == sequence_event_type_break) {
			e.attribute("break") = true;
		} else if (ev.type == sequence_event_type_pattern) {
			e.attribute("ref") = id_from_ptr(ev.value);
		} else {
			assert(0);
		}
	}

	return item;
}

xml_node CcmWriter::saveSequences(xml_node &parent, zzub::player &player, zzub::metaplugin &plugin) {
	
	// event positions and track length is stored in beats, not rows. this makes it
	// easier to change the tpb count manually, and have the loader adapt all
	// sequences on loading.
	double tpbfac = 1.0/double(player.master->getParameter(1, 0, 2));

	xml_node item = parent.append_child(node_element);
	item.name("sequences");

    for (int i = 0; i != player.getSequenceTracks(); i++) {
		zzub::sequence *seq = player.getSequenceTrack(i);
		if (seq->machine == &plugin) {
			saveSequence(item, tpbfac, player, plugin, *seq);
		}
	}
	
	return item;
}

xml_node CcmWriter::saveMidiMappings(xml_node &parent, zzub::player &player, zzub::metaplugin &plugin) {
	xml_node item = parent.append_child(node_element);
	item.name("midi");
	
	for (size_t j=0; j<player.getMidiMappings(); j++) {
		midimapping* mm=player.getMidiMapping(j);
		if (mm->machine==&plugin) {
			xml_node bind = item.append_child(node_element);
			bind.name("bind");
			
			const zzub::parameter *param = 0;
			switch(mm->group) {
				case 0: param = plugin.getConnection(mm->track)->connection_parameters[mm->column]; break;
				case 1: param = plugin.loader->plugin_info->global_parameters[mm->column]; break;
				case 2: param = plugin.loader->plugin_info->track_parameters[mm->column]; break;
				default: assert(0);
			}

			if (mm->group == 0) {
				bind.attribute("ref") = id_from_ptr(plugin.getConnection(mm->track));
				if (mm->column == 0) {
					bind.attribute("target") = "amp";
				} else if (mm->column == 1) {
					bind.attribute("target") = "pan";
				} else {
					assert(0);
				}
				bind.attribute("track") = long(mm->track);
			} else if (mm->group == 1) {
				bind.attribute("ref") = id_from_ptr(param);
				bind.attribute("target") = "global";
			} else if (mm->group == 2) {
				bind.attribute("track") = long(mm->track);
				bind.attribute("ref") = id_from_ptr(param);
				bind.attribute("target") = "track";
			} else {
				assert(0);
			}
			
			bind.attribute("channel") = long(mm->channel);
			bind.attribute("controller") = long(mm->controller);
		}
	}
	return item;
}

xml_node CcmWriter::savePlugin(xml_node &parent, zzub::player &player, zzub::metaplugin &plugin) {
	xml_node item = parent.append_child(node_element);
	item.name("plugin");
	item.attribute("id") = id_from_ptr(&plugin);
	item.attribute("name") = plugin.getName();
	item.attribute("ref") = plugin.loader->plugin_info->uri;
	
	// ui properties should be elsewhere
	xml_node position = item.append_child(node_element);
	position.name("position");
	position.attribute("x") = plugin.x;
	position.attribute("y") = plugin.y;

	if (plugin.getConnections())
		saveConnections(item, plugin);
	
	saveInit(item, plugin);
	
	saveMidiMappings(item, player, plugin);
	
	if (plugin.getPatterns())
		savePatterns(item, player, plugin);

	saveSequences(item, player, plugin);

	return item;
}

xml_node CcmWriter::savePlugins(xml_node &parent, zzub::player &player) {
	xml_node item = parent.append_child(node_element);
	item.name("plugins");

    for (size_t i=0; i<player.getMachines(); i++) {
		zzub::metaplugin* plugin = player.getMachine(i);
		if (!plugin->isNoOutput() && !plugin->nonSongPlugin) {
			savePlugin(item, player, *plugin);
		}
    }

    for (size_t i=0; i<player.getMachines(); i++) {
		zzub::metaplugin* plugin = player.getMachine(i);
		if (plugin->isNoOutput()) {
			savePlugin(item, player, *plugin);
		}
    }
	
	return item;
}

xml_node CcmWriter::saveEnvelope(xml_node &parent, zzub::envelope_entry& env) {
	xml_node item = parent.append_child(node_element);
	item.name("envelope");

	xml_node adsr = item.append_child(node_element);
	adsr.name("adsr");
	adsr.attribute("attack") = double(env.attack) / 65535.0;
	adsr.attribute("decay") = double(env.decay) / 65535.0;
	adsr.attribute("sustain") = double(env.sustain) / 65535.0;
	adsr.attribute("release") = double(env.release) / 65535.0;	
	adsr.attribute("precision") = double(env.subDivide) / 127.0;
	// no adsr flags yet?
	// please never add flags directly, instead set boolean values.
	
	xml_node points = item.append_child(node_element);
	points.name("points");
	for (size_t i = 0; i != env.points.size(); ++i) {
		envelope_point &pt = env.points[i];
		xml_node point = points.append_child(node_element);
		point.name("e");
		point.attribute("t") = double(pt.x) / 65535.0;
		point.attribute("v") = double(pt.y) / 65535.0;
		if (pt.flags & zzub::envelope_flag_sustain) {
			point.attribute("sustain") = true;
		}
		if (pt.flags & zzub::envelope_flag_loop) {
			point.attribute("loop") = true;
		}
	}
	
	return item;
}

xml_node CcmWriter::saveEnvelopes(xml_node &parent, zzub::wave_info_ex &info) {
	xml_node item = parent.append_child(node_element);
	item.name("envelopes");
	
	for (size_t i=0; i != info.envelopes.size(); ++i) {
		if (!info.envelopes[i].disabled) {
			xml_node envnode = saveEnvelope(item, info.envelopes[i]);
			envnode.attribute("index") = (long)i;
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
	item.name("instrument");
	
	item.attribute("id") = id_from_ptr(&info);
	item.attribute("name") = info.name;
	item.attribute("volume") = (double)info.volume;
	
	saveEnvelopes(item, info);
	
	xml_node waves = item.append_child(node_element);
	waves.name("waves");

	for (size_t j=0; j<info.get_levels(); j++) {
		if (info.get_sample_count(j)) {
			wave_level &level = info.levels[j];
			xml_node levelnode = waves.append_child(node_element);
			levelnode.name("wave");
			levelnode.attribute("id") = id_from_ptr(&level);
			levelnode.attribute("index") = (long)j;
			levelnode.attribute("filename") = info.fileName;
			
			// store root notes as midinote values
			levelnode.attribute("rootnote") = (long)buzz_to_midi_note(level.root_note);
			
			levelnode.attribute("loopstart") = (long)level.loop_start;
			levelnode.attribute("loopend") = (long)level.loop_end;
			
			// most flags are related to levels, not to the instrument
			// this is clearly wrong and has to be fixed once we support
			// more than one wave level in the application.
			if (info.flags & zzub::wave_flag_loop) {
				levelnode.attribute("loop") = true;
			}
			if (info.flags & zzub::wave_flag_pingpong) {
				levelnode.attribute("pingpong") = true;
			}
			if (info.flags & zzub::wave_flag_envelope) {
				levelnode.attribute("envelope") = true;
			}
			
			xml_node slices = levelnode.append_child(node_element);
			slices.name("slices");
			for (size_t k = 0; k < level.slices.size(); k++) {
				xml_node slice = slices.append_child(node_element);
				slice.name("slice");
				slice.attribute("value") = (long)level.slices[k];
			}
			
			// flac can't store anything else than default samplerates,
			// and most possibly the same goes for ogg vorbis as well
			// so save the samplerate here instead.
			levelnode.attribute("samplerate") = (long)info.get_samples_per_sec(j);
			
			// flac is going to be the standard format. maybe we're
			// going to implement ogg vorbis once the need arises.
			std::string wavename = id_from_ptr(&level) + ".flac";
			levelnode.attribute("src") = wavename;
			
			arch.createFileInArchive(wavename);
			// TODO: floats should be saved in a different format
			encodeFLAC(&arch, info, j);
			arch.closeFileInArchive();
		}
		
	}

	return item;
}

xml_node CcmWriter::saveWaves(xml_node &parent, zzub::player &player) {
	xml_node item = parent.append_child(node_element);
	item.name("instruments");
	
    for (size_t i=0; i<player.waveTable.waves.size(); i++) {

        // NOTE: getting reference, ~wave_info_ex frees the sample data!!1
        wave_info_ex& info=player.waveTable.waves[i];

        if (info.get_levels()) {
			xml_node instr = saveWave(item, info);
			instr.attribute("index") = (long)i;
		}
    }
	
	return item;
}

bool CcmWriter::save(std::string fileName, zzub::player* player) {

	if (!arch.create(fileName)) return false;

	const char* loc = setlocale(LC_NUMERIC, "C");

	xml_parser xml;
	xml.create();
	
	xml_node root = xml.document();
	xml_node xmldesc = root.append_child(node_pi);
	xmldesc.name("xml");
	xmldesc.attribute("version") = "1.0";
	xmldesc.attribute("encoding") = "utf-8";
	xml_node xmix = root.append_child(node_element);
	xmix.name("xmix");
	xmix.attribute("xmlns:xmix") = "http://www.zzub.org/ccm/xmix";
	
	// save meta information
	saveHead(xmix, *player);
	
	// save main sequencer settings
	{
		double tpbfac = 1.0/double(player->master->getParameter(1, 0, 2));	
		
		sequencer &seq = player->song_sequencer;
		xml_node item = xmix.append_child(node_element);
		item.name("transport");
		item.attribute("bpm") = double(player->master->getParameter(1, 0, 1));
		item.attribute("tpb") = double(player->master->getParameter(1, 0, 2));
		item.attribute("loopstart") = double(seq.beginOfLoop) * tpbfac;
		item.attribute("loopend") = double(seq.endOfLoop) * tpbfac;
		item.attribute("start") = double(seq.startOfSong) * tpbfac;
		item.attribute("end") = double(seq.endOfSong) * tpbfac;
	}
	
	// save classes
	saveClasses(xmix, *player);
	
	// save plugins
	savePlugins(xmix, *player);
	
	// save waves
	saveWaves(xmix, *player);
	
	std::ostringstream oss("");
	oss << xml.document();
	std::string xmlstr = oss.str();
	arch.createFileInArchive("song.xmix");
	arch.write((void*)xmlstr.c_str(), strlen(xmlstr.c_str()));
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

void decodeFLAC(zzub::instream* reader, wave_info_ex& info, int level) {

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
    int channels=FLAC__stream_decoder_get_channels(stream);
    unsigned int bps=FLAC__stream_decoder_get_bits_per_sample(stream);
    unsigned int sample_rate=FLAC__stream_decoder_get_sample_rate(stream);


    // allocate a level based on the stats retreived from the decoder stream
    zzub::wave_buffer_type waveFormat;
    switch (bps) {
        case 16:
            waveFormat=wave_buffer_type_si16;
            break;
        case 24:
            waveFormat=wave_buffer_type_si24;
            break;
        case 32:
            waveFormat=wave_buffer_type_si32;
            break;
        default:
            throw "not a supported bitsize";
    }

	bool result = info.allocate_level(level, decoder_info.totalSamples, waveFormat, channels==2);
	assert(result); // lr: you don't want to let this one go unnoticed. either bail out or give visual cues.

	char* targetBuf=(char*)info.get_sample_ptr(level);
    for (size_t i=0; i<decoder_info.buffers.size(); i++) {
        DecodedFrame frame=decoder_info.buffers[i];
        memcpy(targetBuf, frame.buffer, frame.bytes);
        targetBuf+=frame.bytes;
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
	if (item.has_attribute("id")) {
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
	for (xml_node::child_iterator i = classes.children_begin(); i != classes.children_end(); ++i) {
		if (i->has_name("pluginclass")) {
			std::string uri = i->attribute("id").value();
			pluginloader *loader = player.getMachineLoader(uri);
			if (!loader) { // no loader for this 
				mem_archive arc;
				// do we have some data saved?
				for (xml_node::child_iterator data = i->children_begin(); data != i->children_end(); ++data) {
					if ((data->has_name("data")) && (data->has_attribute("src"))) {
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
					for (lib = player.pluginLibraries.begin(); lib != player.pluginLibraries.end(); ++lib) {
						if ((*lib)->collection) {
							const zzub::info *_info = (*lib)->collection->get_info(uri.c_str(), &arc);
							if (_info) { // library could read archive
								(*lib)->register_info(_info); // register the new info
								found = true;
							}
						}
					}
					if (!player.pluginLibraries.size()) {
						std::cerr << "ccm: warning: no plugin libraries available." << std::endl;
					}
				}
				if (!found) {
					std::cout << "ccm: couldn't find loader for " << uri << std::endl;
				}
			}
			xml_node parameters = i->first_element_by_name("parameters");
			if (!parameters.empty()) {
				xml_node global = parameters.first_element_by_name("global");
				if (!global.empty()) {
				}
				xml_node track = parameters.first_element_by_name("track");
				if (!track.empty()) {
				}
			}
		}
	}
	return true;
}



struct ccache { // connection and node cache
	metaplugin *target;
	xml_node connections;
	xml_node global;
	xml_node tracks;
	xml_node sequences;
	xml_node eventtracks;
	xml_node midi;
};

bool CcmReader::loadPlugins(xml_node &plugins, zzub::player &player) {
	
	std::map<std::string, metaplugin *> id2plugin;
	
	std::vector<ccache> conns;
	
	for (xml_node::child_iterator i = plugins.children_begin(); i != plugins.children_end(); ++i) {
		if (i->has_name("plugin")) {
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
			for (xml_node::child_iterator j = i->children_begin(); j != i->children_end(); ++j) {
				if (j->has_name("init")) {
					init = *j; // store for later
					
					// enumerate init section
					for (xml_node::child_iterator k = j->children_begin(); k != j->children_end(); ++k) {
						if ((k->has_name("data")) && (k->has_attribute("src"))) {
							// store data for later
							compressed_file_info cfi;
							if (arch.openFileInArchive(k->attribute("src").value(), &cfi)) {
								std::vector<char> &b = arc.get_buffer(k->attribute("base").value());
								b.resize(cfi.uncompressed_size);
								arch.read(&b[0], cfi.uncompressed_size);
								arch.closeFileInArchve();
							}
						} else if (k->has_name("attributes")) {
							attribs = *k; // store for later
						} else if (k->has_name("global")) {
							global = *k; // store for later
						} else if (k->has_name("tracks")) {
							tracks = *k; // store for later
						}
					}
				} else if (j->has_name("position")) {
					position = *j; // store for later
				} else if (j->has_name("connections")) {
					connections = *j; // store for later
				} else if (j->has_name("sequences")) {
					sequences = *j; // store for later
				} else if (j->has_name("eventtracks")) {
					eventtracks = *j; // store for later
				} else if (j->has_name("midi")) {
					midi = *j; // store for later
				}
			}
			
			pluginloader *loader = player.getMachineLoader(i->attribute("ref").value());
			metaplugin *plugin = 0;
			if (i->attribute("ref").has_value("@zzub.org/master")) {
				plugin=player.getMaster();
			} else if (loader) {
				std::vector<char> &b = arc.get_buffer("");
        char *data = 0;
        if (b.size())
          data = &b[0];
				plugin = player.createMachine(data, b.size(), i->attribute("name").value(), loader);
				plugin->initialize(0,0,0,0,0);
			} else {
				std::cerr << "ccm: unable to find loader for uri " << i->attribute("ref").value() << std::endl;
				/*
				if (pluginNames[i]=="Master") {
					plugin=player->getMaster();
				} else {
					int type=0;
					if (pluginProps["type"]=="effect")
						type=zzub_plugin_type_effect; else
					if (pluginProps["type"]=="generator")
						type=zzub_plugin_type_generator;

					string pluginUri=pluginProps["uri"];
					// find validator
					MachineValidation* validator=0;
					for (size_t j=0; j<validators.size(); j++) {
						if (validators[j].machineName==pluginUri) {
							validator=&validators[j];
							break;
						}
					}
					if (validator) {
						//plugin=player->createDummyMachine(type, pluginNames[i], pluginUri, attributeProps.size(), validator);
						player->loadWarning+="Created dummy machine for " + pluginUri + "\n";
					} else {
						player->loadError+="Could not create dummy machine for " + pluginUri + "\n";
					}
						
				}
				*/
			}
			
			if (plugin) {
				id2plugin.insert(std::pair<std::string,metaplugin*>(i->attribute("id").value(), plugin));

				if (!position.empty()) {
					plugin->x = (double)position.attribute("x");
					plugin->y = (double)position.attribute("y");
				}

				plugin->defaultAttributes();
				
				if (!attribs.empty()) {
					for (xml_node::child_iterator a = attribs.children_begin(); a != attribs.children_end(); ++a) {
						for (size_t pa = 0; pa != plugin->getAttributes(); ++pa) {
							if (!strcmp(plugin->getAttribute(pa).name, a->attribute("name").value())) {
								plugin->setAttributeValue(pa, (int)(long)a->attribute("v"));
							}
						}
					}
				}
				
				plugin->attributesChanged();

				if (!tracks.empty()) {
					xml_node_list tracknodes;
					tracks.all_elements_by_name("track", tracknodes);
					long trackcount = tracknodes.size();
					for (unsigned int a = 0; a != tracknodes.size(); ++a) {
						// if we find any track with a higher index, extend the size
						trackcount = std::max(trackcount, (long)tracknodes[a].attribute("index"));
					}
					plugin->setTracks(trackcount);
				}

				plugin->defaultParameters();

				// plugin default parameter values are read after connections are made
				ccache cc;
				cc.target = plugin;
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
			for (xml_node::child_iterator i = c->connections.children_begin(); i != c->connections.children_end(); ++i) {
				if (i->has_name("input")) {
					std::map<std::string, metaplugin *>::iterator iplug = id2plugin.find(i->attribute("ref"));
					if (iplug != id2plugin.end()) {
						std::string conntype = "audio";
						if (i->has_attribute("type")) {
							conntype = i->attribute("type").value();
						}
						if (conntype == "audio") {
							int amp = double_to_amp((double)i->attribute("amplitude"));
							int pan = double_to_pan((double)i->attribute("panning"));
							c->target->addAudioInput(iplug->second, (unsigned short)amp, (unsigned short)pan);
						} else if (conntype == "event") {
							// TODO: restore controller associations
							event_connection *evc = c->target->addEventInput(iplug->second);
							for (xml_node::child_iterator j = i->children_begin(); j != i->children_end(); ++j) {
								if (j->has_name("bindings")) {
									for (xml_node::child_iterator k = j->children_begin(); k != j->children_end(); ++k) {
										if (k->has_name("binding")) {
											event_connection_binding binding;
											binding.source_param_index = long(k->attribute("source_param_index"));
											binding.target_group_index = long(k->attribute("target_group_index"));
											binding.target_track_index = long(k->attribute("target_track_index"));
											binding.target_param_index = long(k->attribute("target_param_index"));
											evc->bindings.push_back(binding);
										}
									}
								}
							}
						} else {
							assert(0);
						}
					} else {
						std::cerr << "ccm: no input " << i->attribute("ref").value() << " for connection " << i->attribute("id").value() << std::endl;
					}
					
				}
			}
		}
		
		// now that connections are set up, we can load the plugin default values
		if (!c->global.empty()) {
			for (xml_node::child_iterator i = c->global.children_begin(); i != c->global.children_end(); ++i) {
				if (i->has_name("n")) {
					xml_node paraminfo = getNodeById(i->attribute("ref").value());
					assert(!paraminfo.empty()); // not being able to deduce the index is fatal
					c->target->setParameter(1, 0, long(paraminfo.attribute("index")), long(i->attribute("v")), false);
				}
			}
		}
		if (!c->tracks.empty()) {
			for (xml_node::child_iterator i = c->tracks.children_begin(); i != c->tracks.children_end(); ++i) {
				if (i->has_name("track")) {
					long t = long(i->attribute("index"));
					for (xml_node::child_iterator j = i->children_begin(); j != i->children_end(); ++j) {
						if (j->has_name("n")) {
							xml_node paraminfo = getNodeById(j->attribute("ref").value());
							assert(!paraminfo.empty()); // not being able to deduce the index is fatal
							c->target->setParameter(2, t, long(paraminfo.attribute("index")), long(j->attribute("v")), false);
						}
					}
				}
			}
		}
		
		c->target->stopParameters();
		
		c->target->tick(); // works?
		//plugin->machine->process_events();  // works better
	}
		
	// we need to create patterns in the second iteration, since the master
	// might not be neccessarily the first plugin to be initialized
	double tpbfac = double(player.master->getParameter(1, 0, 2));
	
	for (std::vector<ccache>::iterator c = conns.begin(); c != conns.end(); ++c) {
		
		if (!c->eventtracks.empty()) {
			// load and create patterns
			for (xml_node::child_iterator i = c->eventtracks.children_begin(); i != c->eventtracks.children_end(); ++i) {
				if (i->has_name("events")) {
					int rows = int(double(i->attribute("length")) * tpbfac + 0.5);
					pattern *p = c->target->createPattern(rows);
					p->setName(i->attribute("name").value());
					i->attribute("index") = long(c->target->getPatternIndex(p)); // set an index attribute
					for (xml_node::child_iterator j = i->children_begin(); j != i->children_end(); ++j) {
						int group = 1;
						int track = 0;
						if (j->has_name("g")) {
							group = 1;
						} else if (j->has_name("c")) {
							group = 0;
						} else if (j->has_name("t")) {
							group = 2;
							track = (int)long(j->attribute("index"));
						}
						if (group != 0) { // TODO: connections not supported yet
							patterntrack *pt = p->getPatternTrack(group, track);
							for (xml_node::child_iterator k = j->children_begin(); k != j->children_end(); ++k) {
								if (k->has_name("e")) {
									int row = int(double(k->attribute("t")) * tpbfac + 0.5);
									assert(row < rows);
									xml_node paraminfo = getNodeById(k->attribute("ref").value());
									assert(!paraminfo.empty()); // not being able to deduce the index is fatal
									pt->setValue(row, long(paraminfo.attribute("index")), long(k->attribute("v")));
								}
							}
						}
					}
				}
			}
		}
		
		if (!c->sequences.empty()) {
			// load and create sequences
			for (xml_node::child_iterator i = c->sequences.children_begin(); i != c->sequences.children_end(); ++i) {
				if (i->has_name("sequence")) {
					sequence *seq = player.song_sequencer.createTrack(c->target);
					
					for (xml_node::child_iterator j = i->children_begin(); j != i->children_end(); ++j) {
						if (j->has_name("e")) {
							int row = int(double(j->attribute("t")) * tpbfac + 0.5);
							sequence_event_type type;
							zzub::pattern* value = 0;
							if (bool(j->attribute("mute"))) {
								type=sequence_event_type_mute;
							} else if (bool(j->attribute("break"))) {
								type=sequence_event_type_break;
							} else {
								xml_node patternnode = getNodeById(j->attribute("ref").value());
								assert(!patternnode.empty()); // shouldn't be empty
								type=sequence_event_type_pattern;
								value = c->target->getPattern(long(patternnode.attribute("index")));
							}
							seq->setEvent(row, type, value);
						}
					}
				}
			}

		}
		
		if (!c->midi.empty()) {
			// load and set up controller bindings
			for (xml_node::child_iterator i = c->midi.children_begin(); i != c->midi.children_end(); ++i) {
				if (i->has_name("bind")) {
					xml_node refnode = getNodeById(i->attribute("ref").value());
					assert(!refnode.empty());
					
					int channel = long(i->attribute("channel"));
					int controller = long(i->attribute("controller"));
					
					std::string target = i->attribute("target").value();
					
					int group;
					int track = 0;
					int index = 0;
					if (target == "amp") {
						group = 0;
						index = 0;
						track = long(i->attribute("track"));
					} else if (target == "pan") {
						group = 0;
						index = 1;
						track = long(i->attribute("track"));
					} else if (target == "global") {
						group = 1;
						track = long(i->attribute("track"));
						index = long(refnode.attribute("index"));
					} else if (target == "track") {
						group = 2;
						track = long(i->attribute("track"));
						index = long(refnode.attribute("index"));
					} else {
						assert(0);
					}
					
					player.addMidiMapping(c->target, group, track, index, channel, controller);
				}
			}
		}

	}

	return true;
}

bool CcmReader::loadInstruments(xml_node &instruments, zzub::player &player) {
	
    // load wave table
    // load instruments

	for (xml_node::child_iterator i = instruments.children_begin(); i != instruments.children_end(); ++i) {
		if (i->has_name("instrument")) {
			wave_info_ex &info = *player.getWave(long(i->attribute("index")));
			info.name = i->attribute("name").value();
			info.flags = 0;
			
			xml_node waves = i->first_element_by_name("waves");
			if (!waves.empty()) {
				for (xml_node::child_iterator w = waves.children_begin(); w != waves.children_end(); ++w) {
					if (w->has_name("wave")) {
						if (arch.openFileInArchive(w->attribute("src").value())) {
							long index = long(w->attribute("index"));
							decodeFLAC(&arch, info, index);
							arch.closeFileInArchve();
							
							if (bool(w->attribute("loop"))) {
								info.set_looping(true);
							}
							if (bool(w->attribute("pingpong"))) {
								info.set_bidir(true);
							}
							if (bool(w->attribute("envelope"))) {
								info.flags |= zzub::wave_flag_envelope;
							}
							info.fileName = w->attribute("filename").value();
							info.set_root_note(index, midi_to_buzz_note(long(w->attribute("rootnote"))));
							info.set_loop_start(index, long(w->attribute("loopstart")));
							info.set_loop_end(index, long(w->attribute("loopend")));
							if (w->has_attribute("samplerate")) {
								info.set_samples_per_sec(index, long(w->attribute("samplerate")));
							}
							wave_level &level = info.levels[index];
							xml_node slices = w->first_element_by_name("slices");
							if (!slices.empty()) {
								for (xml_node::child_iterator s = slices.children_begin(); s != slices.children_end(); ++s) {
									if (s->has_name("slice")) {
										level.slices.push_back(long(s->attribute("value")));
									}
								}
							}
						}
					}
				}
			}

			// NOTE: must set volume after (first) allocate_level (which happens in decodeFLAC)
			info.volume = double(i->attribute("volume"));
			
			xml_node envelopes = i->first_element_by_name("envelopes");
			if (!envelopes.empty()) {
				for (xml_node::child_iterator e = envelopes.children_begin(); e != envelopes.children_end(); ++e) {
					if (e->has_name("envelope")) {
						long index = long(e->attribute("index"));
						if (info.envelopes.size() <= index) {
							info.envelopes.resize(index+1);
						}
						envelope_entry &env = info.envelopes[index];
						env.disabled = false;
						xml_node adsr = i->first_element_by_name("adsr");
						if (!adsr.empty()) {
							env.attack = int(double(adsr.attribute("attack")) * 65535.0 + 0.5);
							env.decay = int(double(adsr.attribute("decay")) * 65535.0 + 0.5);
							env.sustain = int(double(adsr.attribute("sustain")) * 65535.0 + 0.5);
							env.release = int(double(adsr.attribute("release")) * 65535.0 + 0.5);
							env.subDivide = int(double(adsr.attribute("precision")) * 127.0 + 0.5);
						}
						xml_node points = i->first_element_by_name("points");
						if (!points.empty()) {
							env.points.clear();
							for (xml_node::child_iterator pt = points.children_begin(); pt != points.children_end(); ++pt) {
								if (pt->has_name("e")) {
									envelope_point evpt;
									evpt.x = int(double(pt->attribute("t")) * 65535.0 + 0.5);
									evpt.y = int(double(pt->attribute("v")) * 65535.0 + 0.5);
									evpt.flags = 0;
									
									if (bool(pt->attribute("sustain"))) {
										evpt.flags |= zzub::envelope_flag_sustain;
									}
									if (bool(pt->attribute("loop"))) {
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
		}
	}

	return true;
}

bool CcmReader::loadSequencer(xml_node &item, zzub::player &player) {
	double tpbfac = double(player.master->getParameter(1, 0, 2));	
	
	sequencer &seq = player.song_sequencer;
	seq.beginOfLoop = int(double(item.attribute("loopstart")) * tpbfac + 0.5);
	seq.endOfLoop= int(double(item.attribute("loopend")) * tpbfac + 0.5);
	seq.startOfSong = int(double(item.attribute("start")) * tpbfac + 0.5);
	seq.endOfSong = int(double(item.attribute("end")) * tpbfac + 0.5);
	return true;
}

	
bool CcmReader::open(std::string fileName, zzub::player* player) {

	const char* loc = setlocale(LC_NUMERIC, "C");

	bool result = false;
	player->setPlayerState(player_state_muted);
	
	if (arch.open(fileName)) {
		compressed_file_info cfi;
		if (arch.openFileInArchive("song.xmix", &cfi)) {
			char *xmldata = new char[cfi.uncompressed_size+1];
			xmldata[cfi.uncompressed_size] = '\0';
			arch.read(xmldata, cfi.uncompressed_size);
			arch.closeFileInArchve();
			xml_parser xml;
			if (xml.parse(xmldata)) {
				xml_node xmix = xml.document().first_element_by_name("xmix");				
				if (!xmix.empty()) {
					xmix.traverse(*this); // collect all ids
					
					// load song meta information
					for (xml_node::child_iterator m = xmix.children_begin(); m != xmix.children_end(); ++m) {
						if (m->has_name("meta")) {
							if (m->attribute("name").has_value("comment") && m->has_attribute("src")) {
								if (arch.openFileInArchive(m->attribute("src").value(), &cfi)) {
									std::vector<char> infotext;
									infotext.resize(cfi.uncompressed_size+1);
									infotext[cfi.uncompressed_size] = '\0';
									arch.read(&infotext[0], cfi.uncompressed_size);
									arch.closeFileInArchve();
									player->infoText = &infotext[0];
								} else {
									std::cerr << "unable to open " << m->attribute("src").value() << " for reading." << std::endl;
								}
							}
						}
					}
					
					// load parameter predefs
					xml_node classes = xmix.first_element_by_name("pluginclasses");
					if (classes.empty() || loadClasses(classes, *player)) {
						// load plugins (connections, parameters, patterns, sequences)
						xml_node plugins = xmix.first_element_by_name("plugins");
						if (plugins.empty() || loadPlugins(plugins, *player)) {
							// load instruments (waves, envelopes)
							xml_node instruments = xmix.first_element_by_name("instruments");
							if (instruments.empty() || loadInstruments(instruments, *player)) {
								// load song sequencer settings
								xml_node sequencer = xmix.first_element_by_name("transport");
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
	
	player->lock();
	player->playerState=player_state_stopped;
	player->resetMachines();
	player->unlock();

	setlocale(LC_NUMERIC, loc);

	return result;
}


};

