/*
kibibu Green Milk
Buzz synth

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

// Some definitions used in other headers

// how often to update filter envelopes
#define UPDATE_FREQUENCY		16
#define ONE_OVER_UPDATE_FREQUENCY		(1.0f/16.0f)

// number of samples per wavetable. Oversampled
#define OVERSAMPLING 4

// sample bit count
#define SAMPLE_BIT_COUNT (12)

// #define SAMPLES ((1<<SAMPLE_BIT_COUNT)/OVERSAMPLING)
#define SAMPLES ((1<<SAMPLE_BIT_COUNT))
#define ONE_OVER_SAMPLES (1.0f/SAMPLES)

// AND with this bitmask instead of using mod
#define SAMPLE_BITMASK ((1<<SAMPLE_BIT_COUNT)-1)

// PI, duh
#define PI	3.1415926535897932384626433832795f
