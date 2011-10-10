#include <cstdio>

#include "MCPChorus.hpp"

MCPChorus::MCPChorus() {
    global_values = &gval;
    track_values = 0;
    attributes = 0;
    _size = (unsigned long)(ceil (30 * _master_info->samples_per_second / 1000.0)) + 64;
    _size = (_size >> 6) << 6; 
    _line_l = new float [_size + 1];
	_line_r = new float [_size + 1];
}
MCPChorus::~MCPChorus() {
    delete[] _line_l;
    delete[] _line_r;
}

void MCPChorus::init(zzub::archive *pi) {
    unsigned int i;

    _wi = _gi = 0;
    _x1 = _x2 = 1;
    _y1 = _y2 = 0;
    memset (_line_l, 0, (_size + 1) * sizeof (float));
    memset (_line_r, 0, (_size + 1) * sizeof (float));
    for (i = 0; i < 3; i++) _ri [i] = _dr [i] = 0;

}
	
void MCPChorus::process_events() {
  if (gval.delay != 65535) {
    _delay = gval.delay
  }
  if (gval.freq1 != 65535) {
    _freq2 = (float)gval.freq2 / 1000.0 
  }
  if (gval.tmod1 != 65535) {
    _tmod1 = gval.tmod1
  }
  if (gval.freq2 != 65535) {
    _freq2 = (float)gval.freq2 / 1000.0 
  }
  if (gval.tmod2 != 65535) {
    _tmod2 = gval.tmod2
  }
}

bool MCPChorus::process_stereo(float **pin, float **pout, int n, int mode) {
    unsigned long i, k, wi;
    int   j;
    float *p0_l,*p0_r, *p1_l, *p1_r;
    float t, x, y,y_r;
    p0_l = _port [INPUT_L];
		p0_r = _port [INPUT_R];    
    p1_l = _port [OUTPUT_L];
		p1_r = _port [OUTPUT_R]; 
    
    wi = _wi;
    do
    {
      if (_gi == 0)
			{
				_gi = 64;

				t = 402.12f * _port [FREQ1][0] / _fsam;
				x = _x1 - t * _y1; 
				y = _y1 + t * _x1;
				t = sqrtf (x * x + y * y);
				_x1 = x / t;
				_y1 = y / t;

				t = 402.12f * _port [FREQ2][0] / _fsam;
				x = _x2 - t * _y2; 
				y = _y2 + t * _x2;
				t = sqrtf (x * x + y * y);
				_x2 = x / t;
				_y2 = y / t;

				x = _port [TMOD1][0] * _x1 + _port [TMOD2][0] * _x2;
				y = _port [TMOD1][0] * _y1 + _port [TMOD2][0] * _y2;
				
				_dr [0] = x;
				/* _dr [1] = -0.500f * x + 0.866f * y; */
				/* _dr [2] = -0.500f * x - 0.866f * y; */
				_dr[2] = -y;
				

				for (j = 0; j < 3; j+=2)
				{
						t = _port [DELAY][0] + _dr [j];
						if (t <  0) t =  0;
						if (t > 30) t = 30;
						t *= _fsam / 1000.0f;
						_dr [j] = (t - _ri [j]) / 64;
				}
			}

			k = (_gi < len) ? _gi : len;
			_gi -= k;
			len -= k;
 
			while (k--) {
				_line_l [++wi] = *p0_l++;
				_line_r [wi] = *p0_r++;
				y =y_r= 0;
				for (j = 0; j < 3; j+=2)
				{
					x = wi - _ri [j];
					_ri [j] += _dr [j];
					if (x < 0) x += _size;
					i = (int)(floorf (x));
					x -= i;
					if(j==0) {
						y += (1 - x) * _line_l [i] + x * _line_l [i + 1];
					}
					if(j==2) {
						y_r += (1 - x) * _line_r [i] + x * _line_r [i + 1];
					}
				}
				/* y *= 0.333f;
				y_r *= 0.333f; 
				y*=.667f;
				y_r*=.667f;*/
				if (add) {
					*p1_l++ += y * _gain;
					*p1_r++ += y_r * _gain;
				}
				else {
					*p1_l++  = y;
					*p1_r++  = y_r;
				}
			}
			if (wi == _size) {
				_line_l [wi = 0] = _line_l [_size];
					_line_r [0] = _line_r [_size];
			}
    }
    while (len);

    _wi = wi;
  return true;
}

const char *MCPChorus::describe_value(int param, int value) {
  static char txt[20];
  switch (param) {
  case 0:
    sprintf(txt, "%d", value);
    break;
  case 1:
    sprintf(txt, "%d", value);
    break;
  case 2:
    sprintf(txt, "%d", value);
    break;
  case 3:
    sprintf(txt, "%d", value);
    break;
  case 4:
    sprintf(txt, "%d", value);
    break;
  default:
    return 0;
  }
  return txt;
}
