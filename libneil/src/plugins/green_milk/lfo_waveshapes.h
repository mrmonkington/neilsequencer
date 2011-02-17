/*
kibibu Green Milk
LFO shape class

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

#pragma once

class LfoWavebank
{
public:
	const static int table_bits = 8;
	const static int length = 256; 

	
	static float arp[][length];

	const static int bit_mask = 0xFF;
	const static int num_standard_banks = 9;
	const static int num_arp_banks = (89);
	const static int num_banks = (num_standard_banks + num_arp_banks);
	
	const static int arp_length = 8;	

	static float sine[length];
	static float saw[length];
	static float square[length];
	static float triangle[length];
	static float sine_cubed[length];
	static float snh_1[length];
	static float snh_2[length];
	static float odd[length];
	static float plateau[length];


	static void initialiseWavebanks();

	static float * getBank(int bank);
	static const char * getName(int bank);

private:
	static bool inited;

	static const int arp_vals[][arp_length];

	
	static float * banks[];
	const static char * names[];

};

