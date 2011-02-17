/*
Copyright (C) 2007 Marcin Dabrowski

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
#ifndef __COMPLEXFLOAT_H
#define __COMPLEXFLOAT_H


template <class T>
class complex
{
	public:
			T re, im;
			complex() {}
			complex(T x, T y): re(x), im(y) {}

};

//template <class T>
//			__forceinline complex<T> operator* (complex<T> a , complex<T> b)
//			{
//				 return   complex<T>( a.re * b.re - a.im * b.im , a.im * b.re + a.re * b.im ) ;
//			}

#endif
