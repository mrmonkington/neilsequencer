/*
kibibu Green Milk
FastPow class, maps a note number to a frequency (in hz)

Copyright (C) 2007  Cameron Foale

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

class NoteLookup
{
public:
	static inline float noteNumToFreq(float num)
	{
		return (16.3516f * powf(2.0f, num/12));	// 16.3516 is C-0
	}
	
private:
};
