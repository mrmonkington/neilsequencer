/*
  Copyright (C) 2003-2007 Anders Ervik <calvin@countzero.no>
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

#include "tools.h"

#ifndef STATIC_BUILD

#if !defined(__GNUC__)
typedef zzub::info const *(__cdecl *GET_INFO)();
typedef zzub::plugin *(__cdecl *CREATE_MACHINE)();

#else
#ifdef cdecl
#undef cdecl
#endif
typedef zzub::info const *(__attribute__((cdecl)) *GET_INFO)();
typedef zzub::plugin *(__attribute__((cdecl)) *CREATE_MACHINE)();

#endif

#endif


namespace zzub {

  struct info;
  struct mixer;
  struct pluginloader;

  // a pluginlib maintains the generic dll handle
  struct pluginlib : pluginfactory {
    xp_modulehandle hMachine;
    std::string fileName;
    zzub::plugincollection* collection;
    std::list<const zzub::info*> loaders;
    zzub::player &player;
	
    pluginlib(const std::string& fileName, zzub::player &p, zzub::plugincollection *_collection = 0);
    ~pluginlib();
    void init_dll();
    void unload();
    virtual void register_info(const zzub::info *_info);
  };


}
