/*
kibibu Green Milk
EnvelopeFollower - used for gain compensation.
Not used by default, as it makes things sound a bit scratchy.
Also doesn't take sampling rate into account
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

#include "envelope_follower.h"
#include <stdlib.h>
#include <math.h>

EnvelopeFollower::EnvelopeFollower()
{
	this->currentValue = 1.0f;
}

void EnvelopeFollower::track(float * psamples, int nsamples)
{
	float newVal;
	while(nsamples--)
	{
		newVal = fabs(*psamples++);
		// newVal = __max(currentValue, newVal);
		currentValue = (currentValue * 0.99f) + (newVal * 0.01f);
	}
}
