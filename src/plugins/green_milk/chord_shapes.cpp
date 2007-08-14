/*
kibibu Green Milk
Chord shapes class - contains a bunch of semi-tone offsets to get various chords
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

#include "chord_shapes.h"

char * ChordShapes::names[number] = 
{	
	"Straight", "Phat", "Sub", "SubPhat",
	"5th", "Phat5th", "Big5th", 
	"Rich", "Maj", "Min", "7th", "Min7",
	"Maj7", "MinM7", "Jimi",
	"J-Maj","J-Min","J-7th","J-m7",
	"J-Maj7","J-MinM7","J-Jimi-M", "J-Jimi-m",
	"Dim", 
	"Love", "Hate",	"Subtle","Full","Split","Splat"
};

float just_scale[12] = {
	0.0f, 1.117312853f, 2.039100017f, 3.15641287f, 3.863137139f, 4.980449991f, 6.097762844f,
		7.019550009f, 8.136862861f, 8.84358713f, 9.960899983f, 10.88268715f };

float just_minor_scale[12] = {
	0.0f, 1.117312853f, 2.039100017f, 3.15641287f, 4.273725723f, 5.195512887f, 6.31282574f,
	7.019550009f, 8.136862861f, 9.254175714f, 10.17596288f, 11.29327573f};


float ChordShapes::getOffset(int shape, int track)
{
	switch(shape)
	{
	case 0:	// straight
		return 0.0f;
		break;

	case 1: // Phat
		return (-12.0f * (track % 2)) + (0.10f / 7.0f) * (track % 8);

	case 2: // Sub (1/4 sub osc)
		return ((track % 4) == 0?-12.0f:0.0f);
		break;

	case 3: // SubPhat
		return (-12.0f * (track % 3)) + (0.10f / 7.0f) * (track % 8);
		break;

	case 4: // 7th
		return (7.0f * (track % 2));
		break;

	case 5: // Phat 7th
		switch(track % 3)
		{
		case 0: return 0.0f;
		case 1: return 7.0f;
		case 2: return -12.0f;
		}
		break;

	case 6: // Big 7th
		switch(track % 4)
		{
		case 0: return -12.0f;
		case 1: return -5.0f;
		case 2: return 0.0f;
		case 3: return 7.0f;
		} 
		break;

	case 7: // rich
		return (24.0f - (12.0f * (track % 5)));
		break;

	case 8: // Maj
		switch(track % 4)
		{
		case 0: return 0.0f;
		case 1: return 7.0f;
		case 2: return 4.0f;
		case 3: return -12.0f;
		}
		break;

	case 9: // Min
		switch(track % 4)
		{
		case 0: return 0.0f;
		case 1: return 7.0f;
		case 2: return 3.0f;
		case 3: return -12.0f;
		}
		break;

	case 10: // 7th
		switch(track % 5)
		{
		case 0: return 0.0f;
		case 1: return 7.0f;
		case 2: return 4.0f;
		case 3: return 10.0f;
		case 4: return -12.0f;
		}
		break;

	case 11: // Min7th
		switch(track % 5)
		{
		case 0: return 0.0f;
		case 1: return 7.0f;
		case 2: return 3.0f;
		case 3: return 10.0f;
		case 4: return -12.0f;
		}
		break;

	case 12: // Maj7th
		switch(track % 5)
		{
		case 0: return 0.0f;
		case 1: return 7.0f;
		case 2: return 4.0f;
		case 3: return 11.0f;
		case 4: return -12.0f;
		}
		break;

	case 13: // MinM7th
		switch(track % 5)
		{
		case 0: return 0.0f;
		case 1: return 7.0f;
		case 2: return 3.0f;
		case 3: return 11.0f;
		case 4: return -12.0f;
		}
		break;

	case 14: // Jimi
		switch(track % 5)
		{
		case 0: return -12.0f;
		case 1: return 0.0f;
		case 2: return 4.0f;
		case 3: return 10.0f;
		case 4: return 15.0f;
		}
		break;

	case 15: // J Maj
		switch(track % 4)
		{
		case 0: return 0.0f;
		case 1: return just_scale[7];
		case 2: return just_scale[4];
		case 3: return -12.0f;
		}
		break;

	case 16: // J Min
		switch(track % 4)
		{
		case 0: return 0.0f;
		case 1: return just_minor_scale[7];
		case 2: return just_minor_scale[3];
		case 3: return -12.0f;
		}
		break;

	case 17: // J 7th
		switch(track % 5)
		{
		case 0: return 0.0f;
		case 1: return just_scale[7];
		case 2: return just_scale[4];
		case 3: return just_scale[10];
		case 4: return -12.0f;
		}
		break;

	case 18: // J Min7th
		switch(track % 5)
		{
		case 0: return 0.0f;
		case 1: return just_minor_scale[7];
		case 2: return just_minor_scale[3];
		case 3: return just_minor_scale[10];
		case 4: return -12.0f;
		}
		break;

	case 19: // J Maj7th
		switch(track % 5)
		{
		case 0: return 0.0f;
		case 1: return just_scale[7];
		case 2: return just_scale[4];
		case 3: return just_scale[11];
		case 4: return -12.0f;
		}
		break;

	case 20: // J MinM7th
		switch(track % 5)
		{
		case 0: return 0.0f;
		case 1: return just_minor_scale[7];
		case 2: return just_minor_scale[3];
		case 3: return just_minor_scale[11];
		case 4: return -12.0f;
		}
		break;

	case 21: // J Jimi
		switch(track % 5)
		{
		case 0: return -12.0f;
		case 1: return 0.0f;
		case 2: return just_scale[4];
		case 3: return just_scale[10];
		case 4: return just_scale[3] + 12.0f;
		}
		break;

	case 22: // J Jimi m
		switch(track % 5)
		{
		case 0: return -12.0f;
		case 1: return 0.0f;
		case 2: return just_minor_scale[4];
		case 3: return just_minor_scale[10];
		case 4: return just_minor_scale[3] + 12.0f;
		}
		break;


	case 23: // dim
		return (-3.0f * (track % 4));
		break;

	
	case 24: // Love
		switch(track % 6)
		{
		case 0: return 0.0f + (0.10f / 16.0f) * (track);
		case 1: return 4.0f + (0.10f / 16.0f) * (track);
		case 2: return 7.0f + (0.10f / 16.0f) * (track);
		case 3: return 12.0f + (0.10f / 16.0f) * (track);
		case 4: return 14.0f + (0.10f / 16.0f) * (track);
		case 5: return 17.0f + (0.10f / 16.0f) * (track);
		}
		break;

	case 25: // Hate
		return (6.66f / 8.0f) * (track % 9) + ( 0.666f * (track % 4));
		break;

	case 26: // Subtle
		return (0.10f / 16.0f) * (track % 16);
		break;

	case 27: // full
		return (0.40f / 8.0f) * (track % 8) - 0.15f;
		break;

	case 28: // split
		return ((track % 2) * 24.0f - 12.0f) + (0.10f / 7.0f) * (track % 7);
		break;

	case 29: // splat
		return ((track % 2) * 24.0f - 12.0f) + ((0.40f / 7.0f) * (track % 7)) - 0.15f;
		break;

	}

	return 0.0f;

};
