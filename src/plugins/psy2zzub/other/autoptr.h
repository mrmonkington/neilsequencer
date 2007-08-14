/* autoptr.h

   Copyright (C) 2007 Frank Potulski (polac@gmx.de)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
   USA. */

#ifndef ___AUTO_PTR_H
#define ___AUTO_PTR_H

#include <memory.h>

//*****************************************************************************************************************************************************

template <class TPtr> class TAutoPtrRef;

template <class TPtr> class TAutoPtr
{
public:

	TAutoPtr(TPtr *_ptr) : ptr(_ptr)
	{
	};

	TAutoPtr(const TAutoPtr<TPtr> &other)
	{
		ptr = other.Release();
	};

	TAutoPtr(const TAutoPtrRef<TPtr> &ref)
	{
		ptr = ref.ref.Release();
	};

	~TAutoPtr() 
	{ 
		if (ptr)
		{
			delete ptr;
		}
	};	

	const TAutoPtr &operator =(const TAutoPtr &other)
	{
		if (ptr!=other.ptr)
		{
			delete ptr;

			ptr = other.Release();			
		}		

		return *this;
	};	    

	TPtr *Release(void)
	{
		TPtr *ret = ptr;

		ptr = 0;

		return ret;
	};

	void Reset(TPtr *_ptr=0)
	{
		if (_ptr!=ptr)
		{
			delete ptr;
		}

		ptr = _ptr;
	};

	inline operator bool(void) const
	{
		return (ptr!=0);
	}

	inline TPtr &operator *(void)
	{
		return *ptr;
	};

	inline const TPtr &operator *(void) const
	{
		return *ptr;
	};

	inline TPtr *operator->(void) 
	{
		return ptr;
	};

	inline const TPtr * const operator->(void) const 
	{
		return ptr; 
	};

private:	

	TAutoPtr() : ptr(0)
	{
	};

private:

	TPtr *ptr;
        
};

//*****************************************************************************************************************************************************

template <class TPtr> class TAutoPtrRef
{
public:

	friend class TAutoPtr;

	TAutoPtrRef(TAutoPtr<TPtr> &r) : ref(r)
	{
	};
	
	inline TPtr &operator *(void)
	{
		return ref;
	};

	inline const TPtr &operator *(void) const
	{
		return ref;
	};

	inline TPtr *operator->(void) 
	{
		return ref;
	};

	inline const TPtr * const operator->(void) const 
	{
		return ref;
	};

private:

	TAutoPtr<TPtr> &ref;

};

//*****************************************************************************************************************************************************

template <class TPtr> class TAutoArrayPtr
{
public:	
	
	TAutoArrayPtr(const __int32 size,const bool initZero=false) : ptr( size>0 ? new TPtr[size] : 0 ), size(size)
	{		
		int test = sizeof(TPtr);

		if ( ptr && initZero )
		{
			::memset(ptr,0,size*sizeof(TPtr));
		}
	};

	TAutoArrayPtr(const TAutoArrayPtr<TPtr> &other) : size(other.size)
	{
		ptr = other.Release();
	};	

	~TAutoArrayPtr() 
	{ 
		if (ptr)
		{
			delete[] ptr;

			ptr = 0;
		}
	};

	inline __int32 Size(void) const
	{
		return size;	
	};

	inline __int32 SizeBytes(void) const
	{
		return size * sizeof(TPtr);	
	};

	void New(const __int32 size)
	{
		if (size>0)
		{
			if (ptr)
			{
				delete[] ptr;

				ptr = 0;
			}

			this->size = 0;

			ptr = new TPtr[size];

			if (ptr)
			{
				this->size = size;
			}			
		}
	};

	const TAutoArrayPtr &operator =(const TAutoArrayPtr &other)
	{
		if (ptr!=other.ptr)
		{
			delete[] ptr;

			ptr = other.Release();

			size = other.size;
		}		

		return *this;
	};	    

	TPtr *Release(void)
	{
		TPtr *ret = ptr;

		ptr = 0;

		size = 0;

		return ret;
	};

	void Reset(TPtr *_ptr=0)
	{
		if (_ptr!=ptr)
		{
			delete[] ptr;
		}

		ptr = _ptr;

		size = 0;
	};

	template <class TPtrCast> TPtrCast *ToPtr(void)
	{
		return (TPtrCast *)ptr;
	};

	template <class TPtrCast> const TPtrCast *ToPtr(void) const
	{
		return (const TPtrCast *)ptr;
	};

	inline TPtr &operator[](const __int32 n)
	{
		return ptr[n];
	};

	inline const TPtr &operator[](const __int32 n) const
	{
		return ptr[n];
	};

	inline operator TPtr *(void)
	{
		return ptr;
	};

	inline operator const TPtr *(void) const
	{
		return ptr;
	};	

private:	

	TAutoArrayPtr() : ptr(0)
	{
	};

private:

	TPtr *ptr;

	__int32 size;
        
};

//*****************************************************************************************************************************************************

#endif