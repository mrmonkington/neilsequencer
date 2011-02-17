/*
kibibu Green Milk
Exponential ADSR class
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

#include "adsr.h"
#include "green_milk.h"

// ADSR envelopes
void ADSR::trigger()
{
	rising = true;
	gate = true;
}

void ADSR::setTime(float& coeff, float time)
{
	coeff = (1.0f) / (time + 1);
	coeff = std::min(coeff, 1.0f);
	coeff = std::max(coeff, 0.000001f);
	
}

void ADSR::setAttackTime(float samp)
{	
	attack_samples = samp;
	setTime(attack_coeff, samp * scale);
}

void ADSR::setDecayTime(float samp)
{	
	decay_samples = samp;
	setTime(decay_coeff, samp * scale);
}

void ADSR::setReleaseTime(float samp)
{
	release_samples = samp;
	setTime(release_coeff, samp * scale);
}

void ADSR::setScale(float newScale)
{
	scale = newScale;	
	
	setTime(attack_coeff, attack_samples * scale);
	setTime(decay_coeff, decay_samples * scale);
	setTime(release_coeff, release_samples * scale);
}

bool ADSR::increment()
{
	// this code adapted from pseudocode by mystran
	// http://www.kvraudio.com/forum/viewtopic.php?p=2277049
	if(gate) {
		if(rising) {			
			// attack phase
			currentVal += attack_coeff * ((1.0f/0.63f) - currentVal);
			if(currentVal > 1.0f) {
				currentVal = 1.0f;
				rising = false;
			}
		} else {
			// in decay/sustain
			// decay and sustain are basically the same
			currentVal += decay_coeff * (sustain - currentVal);
		}
	} else {
		// not gate, go to release phase
		currentVal += release_coeff * (1.0f-(1.0f/0.63f) - currentVal);
		if (currentVal < 0.00001f) {
			currentVal = 0.0f;
			return false;
		}
	}

	return true;
}

void ADSR::Init()
{
	gate = false;
	rising = false;
	scale = 1.0f;

	currentVal = 0.0f;

	setAttackTime(32);
	setDecayTime(32);
	setReleaseTime(32);

}

ADSR::ADSR()
{
	Init();
}
