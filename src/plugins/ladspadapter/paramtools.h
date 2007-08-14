// ladspa parameter tools
// Copyright (C) 2006 Leonard Ritter (contact@leonard-ritter.com)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#if !defined(__LADSPA_PARAMTOOLS_H__)
#define __LADSPA_PARAMTOOLS_H__

#include <math.h>
#include <algorithm>

struct ladspa_param
{
	zzub::parameter *param;
	int index;
	LADSPA_PortRangeHint hint;
	float lb;
	float ub;
	bool sr;
};

float ipol_log(float v1, float v2, float x) {
	if (x <= 0.0f)
		return v1;
	if (x >= 1.0f)
		return v2;
	if (v1 == 0.0f)
		v1 = -8; // -48dB or so
	else
		v1 = log(v1);
	v2 = log(v2);
	return exp(v1*(1-x) + v2*x);
}

void setup_ladspa_parameter(zzub::parameter *param, LADSPA_PortRangeHint hint, ladspa_param &mp) {
	LADSPA_PortRangeHintDescriptor hd = hint.HintDescriptor;

	float lb, ub;
	if (LADSPA_IS_HINT_BOUNDED_BELOW(hd))
		lb = hint.LowerBound;
	else
		lb = 0.0f;
	if (LADSPA_IS_HINT_BOUNDED_ABOVE(hd))
		ub = hint.UpperBound;
	else
		ub = 1.0f;
	mp.sr = (LADSPA_IS_HINT_SAMPLE_RATE(hd))?true:false;
	mp.lb = lb;				
	mp.ub = ub;
	mp.param = param;

	param->flags = zzub::parameter_flag_state;
	
	if (LADSPA_IS_HINT_INTEGER(hd)) {
		param->value_min = std::max(0,int(lb));
		if (int(ub) > 0xFE) {
			param->type = zzub::parameter_type_word;
			param->value_max = std::min(0xFFFE,int(ub));
			param->value_none = 0xFFFF;
		} else {
			param->type = zzub::parameter_type_byte;
			param->value_max = std::min(0xFE,int(ub));
			param->value_none = 0xFF;
		}
		param->value_default = 0;
	}
	else if (LADSPA_IS_HINT_TOGGLED(hd)) {
		param->type = zzub::parameter_type_switch;
		param->value_min = zzub::switch_value_off;
		param->value_max = zzub::switch_value_on;
		param->value_none = zzub::switch_value_none;
		param->value_default = 0;
	}
	else {
		param->type = zzub::parameter_type_word;
		param->value_min = 0;
		param->value_max = 0xFFFE;
		param->value_none = 0xFFFF;
		param->value_default = 0;
	}
	
	float d = 0.0f;
	getLADSPADefault(&hint, 44100, &d);
	if (mp.sr) {
		ub *= 44100;
		lb *= 44100;
	}
	float x = std::min(std::max((d - lb) / (ub - lb),0.0f),1.0f);
	param->value_default = (int)((float)param->value_min + (float)(param->value_max - param->value_min) * x + 0.5f);
	//printf("param %s is default %f/%f -> %i\n", param->name, d, x, (int)param->value_default);
}

float convert_ladspa_value(const ladspa_param &mp, int value, float sps) {
	zzub::parameter *param = mp.param;
	LADSPA_PortRangeHintDescriptor hd = mp.hint.HintDescriptor;
	float lb,ub;
	lb = mp.lb;
	ub = mp.ub;
	if (mp.sr)
	{
		lb *= sps;
		ub *= sps;
	}
	float x = (float)(value - param->value_min) / (float)(param->value_max - param->value_min);
	float v;
	if (LADSPA_IS_HINT_LOGARITHMIC(hd)) {
		v = ipol_log(lb,ub,x);
	} else {
		v = lb + x * (ub - lb);
	}
	if (LADSPA_IS_HINT_INTEGER(hd)) {
		v = (int)(v + 0.5f);
	} else if (LADSPA_IS_HINT_TOGGLED(hd)) {
		v = (int)(v + 0.5f);
	}
	//printf("returning %s/%f/%i as %f [%i -> %i]:[%f -> %f]\n", param->name, x, value, v, (int)param->value_min, (int)param->value_max, lb, ub);
	return v;
}

const char *describe_ladspa_value(ladspa_param &mp, int value, float sps, char *buffer) {
	zzub::parameter *param = mp.param;
	LADSPA_PortRangeHintDescriptor hd = mp.hint.HintDescriptor;
	float lb,ub;
	lb = mp.lb;
	ub = mp.ub;
	if (mp.sr)
	{
		lb *= sps;
		ub *= sps;
	}
	float x = (float)(value - param->value_min) / (float)(param->value_max - param->value_min);
	float v;
	if (LADSPA_IS_HINT_LOGARITHMIC(hd)) {
		v = ipol_log(lb,ub,x);
	} else {
		v = lb + x * (ub - lb);
	}
	if (LADSPA_IS_HINT_INTEGER(hd)) {
		sprintf(buffer, "%i", (int)(v + 0.5f));
	} else if (LADSPA_IS_HINT_TOGGLED(hd)) {
		if ((int)(v + 0.5f) == 0) {
			sprintf(buffer, "Off");
		} else {
			sprintf(buffer, "On");
		}		
	} else {
		sprintf(buffer, "%.4f", v);
	}
	return buffer;
}

#endif // __LADSPA_PARAMTOOLS_H__
