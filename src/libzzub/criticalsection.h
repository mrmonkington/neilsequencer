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
// MFC syncronization replacements, the non-Windows portions are entirely untested

#include <string>
#include <assert.h>

#if !defined(_WIN32)
#include <pthread.h>
#if !defined(PTHREAD_MUTEX_RECURSIVE_NP)
#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#endif
#endif

class CCriticalSection  {
public:
	CCriticalSection() {
#if defined(_WIN32)
		InitializeCriticalSection(&m_sect); 
#else
		pthread_mutexattr_t mutexattr;   // Mutex attribute variable
		pthread_mutexattr_init(&mutexattr);
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
		pthread_mutex_init(&mutex, &mutexattr);
		pthread_mutexattr_destroy(&mutexattr);
#endif
	}
	~CCriticalSection() {
#if defined(_WIN32)
		DeleteCriticalSection(&m_sect);
#else
		pthread_mutex_destroy (&mutex);
#endif
	}


	bool Unlock() {
#if defined(_WIN32)
		LeaveCriticalSection(&m_sect); 
#else
		pthread_mutex_unlock (&mutex);
#endif
		return true;
	}

	bool Lock() {
#if defined(_WIN32)
		EnterCriticalSection(&m_sect); 
#else
		pthread_mutex_lock (&mutex);
#endif
		return true; 
	}
protected:

#if defined(_WIN32)
	CRITICAL_SECTION m_sect;
#else
	pthread_mutex_t mutex;           // Mutex
#endif

	
};

/*
CCriticalSectionLocker makes sure that a critical section is never
left locked when a scope is left. for examples see Pattern.cpp
*/
class CCriticalSectionLocker
{
protected:
	CCriticalSection* cs;

public:
	CCriticalSectionLocker(CCriticalSection& cs)
	{
		this->cs = &cs;
		this->cs->Lock();
	}
	
	~CCriticalSectionLocker()
	{
		this->cs->Unlock();
	}
};
