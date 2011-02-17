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

#pragma once

class ADSR
{
public:
	bool increment();
	void setAttackTime(float samp);
	void setDecayTime(float samp);
	void setReleaseTime(float samp);
	void setScale(float multiple);
	void trigger();

	float sustain;
	
	bool gate;
	bool rising;
	
	float currentVal;

	void Init();
	ADSR();

private:

	float attack_samples;
	float decay_samples;
	float release_samples;

	float attack_coeff;
	float decay_coeff;
	float release_coeff;

	float scale;

	inline void setTime(float & coeff, float time);

};
