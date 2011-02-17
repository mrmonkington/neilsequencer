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

#include "lfo_waveshapes.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#if defined(_MSC_VER)
#define snprintf _snprintf
#endif

bool LfoWavebank::inited = false;

float LfoWavebank::sine[length];
float LfoWavebank::saw[length];
float LfoWavebank::square[length];
float LfoWavebank::triangle[length];
float LfoWavebank::sine_cubed[length];
float LfoWavebank::snh_1[length];
float LfoWavebank::snh_2[length];
float LfoWavebank::odd[length];
float LfoWavebank::plateau[length];

#define ST(x) ((float(x))/(12.0f))

const int LfoWavebank::arp_vals[][8] = 
{ 
	{0,7,12,7,0,7,12,7},
	{0,7,0,12,0,7,0,12},
	{0,7,0,7,0,12,0,12},
	{0,12,7,12,0,12,7,12},
	{0,12,0,7,12,7,0,7},
	{0,12,0,12,7,12,7,0}, 
	{12,0,7,12, 0,7,12,7},
	{12,0,12,0, 7,0,7,0}, // 8

	{0,-12,0,12, 0,-5,0,7},
	{0,7,-12,7, 0,12,-5,12},
	{0,7,12,-12, -5,0,7,12},
	{0,12,7,12, -5,0,-12,7},
	{0,-5,7,-5, 12,-5,7,-5},
	{0,-5,-12,12, 7,0,-5,-12},
	{12,0,-12,0, 12,0,-12,0},
	{12,7,0,-5, -12,-5,0,7},
	{12,0,7,-5, -12,0,-5,7},
	{-12,0,12,0, -12,0,12,0},
	{-12,-5,0,7, 12,7,0,-5},
	{-12,0,-5,7, 0,12,7,0}, // 12

	{0,4,7,12,7,12,7,4},
	{0,4,0,7,0,12,0,7},
	{0,12,7,4,7,12,0,4},
	{0,7,4,0,12,7,4,12},
	{0,7,4,0,12,7,4,10},
	{0,4,7,10,12,10,7,4},
	{0,12,0,12,0,4,7,12},
	{0,12,7,4, 7,12,4,7},
	{0,4,7,12,0,12,0,12},
	{0,12,0,7,0,5,0,4},
	{0,12,7,5,4,12,7,5},
	{12,10,0,4,12,10,0,7},
	{12,11,0,4,12,11,0,7},  // 13
	
	{12,7,4,0,-5,-8,-12,0},
	{12,4,7,0,-5,-12,-8,0},
	{-12,-8,-5,0,4,7,12,7},
	{-12,-5,-8,0,7,4,12,7},
	{0,-1,0,4,7,11,12,11},
	{0,-2,0,4,7,10,12,10},
	{-12,-8,-5,-2,0,4,7,10},
	{12,10,7,4,0,-2,-5,-8},
	{12,10,4,0,4,10,12,-12}, //9 

	{0,3,7,12,7,12,7,3},
	{0,3,0,7,0,12,0,7},
	{0,12,7,3,7,12,0,3},
	{0,7,3,0,12,7,3,12},
	{0,7,3,0,12,7,3,10},
	{0,3,7,10,12,10,7,3},
	{0,12,0,12,0,3,7,12},
	{0,12,7,3, 7,12,3,7},
	{0,3,7,12,0,12,0,12},
	{0,12,0,7,0,5,0,3},
	{0,12,7,5,3,12,7,5},
	{12,10,0,3,12,10,0,7},
	{12,11,0,3,12,11,0,7},  // 13

	{12,7,3,0,-5,-9,-12,0},
	{12,4,7,0,-5,-12,-9,0},
	{-12,-9,-5,0,3,7,12,7},
	{-12,-5,-9,0,7,3,12,7},
	{0,-1,0,3,7,11,12,11},
	{0,-2,0,3,7,10,12,10},
	{-12,-9,-5,-2,0,3,7,10},
	{12,10,7,3,0,-2,-5,-9},
	{12,10,3,0,3,10,12,-12}, // 9

	{3,4,0,7, 0,12,0,7},
	{3,4,0,12, 0,7,0,12},
	{3,4,0,7, -9,-8,-12,-5},
	{3,4,7,0, -9,-8,-5,-12},
	{0,12,0,12, 3,4,0,12},
	{0,0,3,4, 0,0,3,4},
	{0,0,12,0, 0,10,0,7},
	{12,0,0,13, 0,10,0,0},	
	{5,0,0,4, 0,-2,0,0}, // 9	
	
	{0,4,8,12, 8,4,0,-4},
	{-12,-8,-4,0, 4,8,12,0},
	{12,8,4,0, -4,-8,-12,0},
	{-12,-4,-8,0, -4,4,0,8},
	{12,4,8,0,4,-4,0,-8}, // 5

	{0,2,4,6, 8,6,4,2},
	{0,2,4,6, 8,10,12,-2},
	{12,10,8,6,4,2,0,-2},
	{-12,-10,-8,-6,-4,-2,0,2},
	{0,4,2,6, 4,8,6,10},
	{0,8,6,4, 2,10,8,6}, // 6

	{0,12,0,6, 7,9,0,5},
	{12,0,6,0, 7,9,0,10},	
	{12,0,-12,12, 0,9,0,6},
	{0,12,6,12,0,-12,-6,-12},
	{0,-12,-6,7, 10,6,7,-5} // 5

	// tot: 89

};

float LfoWavebank::arp[sizeof(arp_vals)/sizeof(arp_vals[0])][length];
	

void LfoWavebank::initialiseWavebanks()
{
	if(inited) return;
	inited = true;

	int i;
	float f,inc;

	// init sines
	for(i = 0; i < LfoWavebank::length; i++)
	{
		f = i * float(M_PI) * 2.0f / LfoWavebank::length;
		f = cosf(f);
		sine[i] = (1.0f - f) * 0.5f;
		sine_cubed[i] = (1.0f - (f * f * f)) * 0.5f;
	}

	// init saw
	f = 0.0f;
	inc = 1.0f / length; 
	for(i = 0; i < LfoWavebank::length; i++)
	{
		saw[i] = f;
		f += inc;
	}

	// init square
	f = 0.0f;
	for(i = 0; i < LfoWavebank::length / 2; i++)
	{
		square[i] = 1.0f;
	}
	for(; i < LfoWavebank::length; i++)
	{
		square[i] = 0.0f;
	}

	// init triangle
	f = 0.0f;
	inc = 2.0f / length;
	for(i = 0; i < LfoWavebank::length / 2; i++)
	{
		triangle[i] = f;
		f += inc;
	}
	for(; i < LfoWavebank::length; i++)
	{
		triangle[i] = f;
		f -= inc;
	}

	// init sample and holds
	// 4 samples per value
	// LENGTH MUST BE A MULTIPLE OF 4
	srand(99);
	for(i = 0; i < LfoWavebank::length; )	// don't increment
	{
		f = float(rand()) / RAND_MAX;
		snh_1[i++] = f;
		snh_1[i++] = f;
		snh_1[i++] = f;
		snh_1[i++] = f;
	}

	srand(0x7191);
	for(i = 0; i < LfoWavebank::length; )	// don't increment
	{
		f = float(rand()) / RAND_MAX;
		snh_2[i++] = f;
		snh_2[i++] = f;
		snh_2[i++] = f;
		snh_2[i++] = f;
	}

	// init plateau. Soft square shape
	f = 0.0f; inc = 1.0f / (LfoWavebank::length / 4.0f);	// get to 1 after one quarter
	for(i = 0; i < LfoWavebank::length / 4; i++)	
	{	plateau[i] = 0.0f;	}

	for(; i < LfoWavebank::length / 2; i++)	
	{
		f += inc;
		plateau[i] = f;	}

	for(; i < (LfoWavebank::length * 3) / 4; i++)
	{		plateau[i] = 1.0f;	}

	f = 1.0f;
	inc = -inc;
	for(; i < LfoWavebank::length; i++)
	{
		f += inc;
		plateau[i] = f;
	}

	// init odd
	for(i = 0; i < (LfoWavebank::length / 8); i++)
	{	odd[i] = sine_cubed[i * 2]; }
	inc = 1.0f / ((LfoWavebank::length * 7.0f) / 8.0f);
	f = 1.0f;
	for(; i < (LfoWavebank::length); i++)
	{
		f -= inc;
		odd[i] = f;
	}

	// init arp
	// have 8 vals per arp
	int len = LfoWavebank::length / 8;
	
	// for each arp
	for(int arp_index = 0; arp_index < (sizeof(arp_vals)/sizeof(arp_vals[0])); arp_index++)
	{
		int index = 0;
		for(i = 0; i < 8; i++)
		{
			int j;
			for(j = 0; j < len; j++)
			{
				arp[arp_index][index++] = ST(arp_vals[arp_index][i]);				
			}		
		}
	}	
}

float * LfoWavebank::banks[] = {
	LfoWavebank::sine,
	LfoWavebank::saw,
	LfoWavebank::square,
	LfoWavebank::triangle,
	LfoWavebank::sine_cubed,
	LfoWavebank::snh_1,
	LfoWavebank::snh_2,
	LfoWavebank::odd,
	LfoWavebank::plateau
};

float * LfoWavebank::getBank(int bank)
{
	if(bank < 0) bank = 0;

	// check if its a sine/saw/etc
	if(bank < num_standard_banks)
	{
		return banks[bank];
	}

	// otherwise, its an arp
	bank -= num_standard_banks;
	
	if(bank < num_arp_banks)
	{
		return arp[bank];
	}
	
	// otherwise, its screwy, return first bank
	return banks[0];

}

const char * LfoWavebank::getName(int bank)
{
	static char buf[20];

	if(bank < 0) bank = 0;

	// check if its a sine/saw/etc
	if(bank < num_standard_banks)
	{
		return names[bank];
	}

	// otherwise, its an arp
	bank -= num_standard_banks;
	
	if(bank < num_arp_banks)
	{
		snprintf(buf, 20, "arp %d", bank);
		return buf;
	}
	
	// otherwise, its screwy, return duddy notice
	return "Dud!";
}

const char * LfoWavebank::names[] = {
	"Sine","Saw","Square","Triangle","Sine^3","S'n'H 1", "S'n'H 2", "Odd", "Plateau"
};
