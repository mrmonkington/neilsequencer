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

#pragma once

#include "../minizip/unzip.h"
#include "../minizip/zip.h"

#include "pugxml.h"

class ArchiveWriter : public zzub::outstream {
    zipFile f;
    std::string currentFileInArchive;
public:
    virtual bool open(std::string fileName);
    virtual bool create(std::string fileName);
    virtual void close();

    void writeLine(std::string line);
	virtual int write(void* v, int size);

	long position() { assert(false); return 0; }
	void seek(long, int) { assert(false); }

    bool createFileInArchive(std::string fileName);
    void closeFileInArchive();
};


struct compressed_file_info {
    std::string name;
    unsigned long compressed_size;
    unsigned long uncompressed_size;
};

class ArchiveReader : public zzub::instream  {
    unzFile f;
    char lastPeekByte;
    bool hasPeeked;

    size_t lastReadOfs;
    //std::string currentFileInArchive;
    compressed_file_info currentFileInfo;
    void populateInfo(compressed_file_info* cfi, std::string fileName, unz_file_info* uzfi);

    void resetFileInArchive();
public:
    virtual bool open(std::string fileName);
    void close();

    bool findFirst(compressed_file_info* info);
    bool findNext(compressed_file_info* info);

    bool openFileInArchive(std::string fileName, compressed_file_info* info=0);
    void closeFileInArchve();

    bool eof();
    char peek();
    virtual void seek(long pos, int mode=SEEK_SET) { assert(false); }
    virtual long position() { return lastReadOfs; }

    virtual int read(void* buffer, int size);
	virtual long size() { return currentFileInfo.uncompressed_size; }
};

namespace zzub {
	using namespace pug;
	
	struct mem_archive;

class CcmWriter {
    ArchiveWriter arch;

	xml_node saveParameter(xml_node &parent, const zzub::parameter &p);
	xml_node saveClasses(xml_node &parent, zzub::player &player);
	xml_node saveClass(xml_node &parent, zzub::pluginloader &pl);
	xml_node saveHead(xml_node &parent, zzub::player &player);
	xml_node addMeta(xml_node &parent, const std::string &propname);
	xml_node saveSequencer(xml_node &parent, const std::string &id, sequencer &seq);
	xml_node saveConnection(xml_node &parent, zzub::connection &connection);
	xml_node saveEventBindings(xml_node &parent, std::vector<zzub::event_connection_binding> &bindings);
	xml_node saveEventBinding(xml_node &parent, zzub::event_connection_binding &binding);
	xml_node saveConnections(xml_node &parent, zzub::metaplugin &plugin);
	xml_node saveParameterValue(xml_node &parent, std::string &group, int track, int param, int value);
	xml_node saveArchive(xml_node &parent, const std::string&, zzub::mem_archive &arc);
	xml_node saveInit(xml_node &parent, zzub::metaplugin &plugin);
	xml_node saveAttributes(xml_node &parent, zzub::metaplugin &plugin);
	xml_node savePatterns(xml_node &parent, zzub::player &player, zzub::metaplugin &plugin);
	xml_node savePatternTrack(xml_node &parent, const std::string &colname, double fac, zzub::metaplugin &plugin, zzub::pattern &p, int group, int track);
	xml_node savePattern(xml_node &parent, zzub::player &player, zzub::metaplugin &plugin, zzub::pattern &p);
	xml_node saveSequence(xml_node &parent, double fac, zzub::player &player, zzub::metaplugin &plugin, zzub::sequence &seq);
	xml_node saveSequences(xml_node &parent, zzub::player &player, zzub::metaplugin &plugin);
	xml_node saveMidiMappings(xml_node &parent, zzub::player &player, zzub::metaplugin &plugin);
	xml_node savePlugin(xml_node &parent, zzub::player &player, zzub::metaplugin &plugin);
	xml_node savePlugins(xml_node &parent, zzub::player &player);
	xml_node saveWave(xml_node &parent, zzub::wave_info_ex &info);
	xml_node saveWaves(xml_node &parent, zzub::player &player);
	xml_node saveEnvelope(xml_node &parent, zzub::envelope_entry& env);
	xml_node saveEnvelopes(xml_node &parent, zzub::wave_info_ex &info);
public:
    bool save(std::string fileName, zzub::player* player);
};

class CcmReader : xml_tree_walker {
    ArchiveReader arch;
	
	typedef std::map<std::string, xml_node> idnodemap;
	idnodemap nodes;
	
	void registerNodeById(xml_node &item);
	xml_node getNodeById(const std::string &id);
	virtual bool for_each(xml_node&);

	bool loadClasses(xml_node &classes, zzub::player &player);
	bool loadPlugins(xml_node &plugins, zzub::player &player);
	bool loadInstruments(xml_node &instruments, zzub::player &player);
	bool loadSequencer(xml_node &seq, zzub::player &player);
public:
    bool open(std::string fileName, zzub::player* player);
};

}
