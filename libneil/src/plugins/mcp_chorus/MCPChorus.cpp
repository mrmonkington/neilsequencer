#include <math.h>
#include <cstdio>
#include <stdint.h>
#include <cstring>

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
    _delay = gval.delay;
  }
  if (gval.freq1 != 65535) {
    _freq2 = (float)gval.freq2 / 1000.0;
  }
  if (gval.tmod1 != 65535) {
    _tmod1 = gval.tmod1;
  }
  if (gval.freq2 != 65535) {
    _freq2 = (float)gval.freq2 / 1000.0;
  }
  if (gval.tmod2 != 65535) {
    _tmod2 = gval.tmod2;
  }
}

bool MCPChorus::process_stereo(float **pin, float **pout, int n, int mode) {
    unsigned long i, k, wi, len;
    int   j;
    float *p0_l,*p0_r, *p1_l, *p1_r;
    float t, x, y,y_r;

    // sample count
    len = n;

    p0_l = pin[0];
    p0_r = pin[1];    
    p1_l = pout[0];
    p1_r = pout[1]; 
    
    // I think this is our 'state' between blocks
    wi = _wi;

    do {
        // processes in blocks (of 64?) in order to
        // minimise recomputation of some coeffs (I guess!)
        if (_gi == 0) {
            _gi = 64;

            t = 402.12f * _freq1 / _master_info->samples_per_second;
            x = _x1 - t * _y1; 
            y = _y1 + t * _x1;
            t = sqrtf (x * x + y * y);
            _x1 = x / t;
            _y1 = y / t;

            t = 402.12f * _freq2 / _master_info->samples_per_second;
            x = _x2 - t * _y2; 
            y = _y2 + t * _x2;
            t = sqrtf (x * x + y * y);
            _x2 = x / t;
            _y2 = y / t;

            x = _tmod1 * _x1 + _tmod2 * _x2;
            y = _tmod1 * _y1 + _tmod2 * _y2;

            _dr [0] = x;

            //wossiss?
            /* _dr [1] = -0.500f * x + 0.866f * y; */
            /* _dr [2] = -0.500f * x - 0.866f * y; */

            _dr[2] = -y;

            for (j = 0; j < 3; j+=2) {
                t = _delay + _dr [j];
                if (t <  0) t =  0;
                if (t > 30) t = 30;
                t *= _master_info->samples_per_second / 1000.0f;
                _dr [j] = (t - _ri [j]) / 64;
            }
        }

        // now actually process samples
        k = (_gi < len) ? _gi : len;
        _gi -= k;
        len -= k;

        while (k--) {

            wi ++;
            _line_l [wi] = *p0_l++;
            _line_r [wi] = *p0_r++;
            y = y_r = 0;

            for (j = 0; j < 3; j+=2) {
                x = wi - _ri [j];
                _ri [j] += _dr [j];
                if (x < 0) {
                    x += _size;
                }
                i = (int)(floorf (x));
                x -= i;
                if(j==0) {
                    y += (1 - x) * _line_l [i] + x * _line_l [i + 1];
                }
                if(j==2) {
                    y_r += (1 - x) * _line_r [i] + x * _line_r [i + 1];
                }
            }
            // wossiss?
            /* y *= 0.333f;
            y_r *= 0.333f; 
            y*=.667f;
            y_r*=.667f;*/

            *p1_l++  = y;
            *p1_r++  = y_r;
        }
        if (wi == _size) {
            _line_l [wi = 0] = _line_l [_size];
            _line_r [0] = _line_r [_size];
        }

    } while (len);

    // save position in block
    // this must add 64 sample lag to signal
    // which isn't much!

    _wi = wi;
    return true;
}

const char *MCPChorus::describe_value(int param, int value) {
  static char txt[20];
  switch (param) {
  case 0:
    sprintf(txt, "%d ms", value);
    break;
  case 1:
    sprintf(txt, "%0.3fHz", (float)value/1000.0);
    break;
  case 2:
    sprintf(txt, "%d ms", value);
    break;
  case 3:
    sprintf(txt, "%0.3fHz", (float)value/1000.0);
    break;
  case 4:
    sprintf(txt, "%d ms", value);
    break;
  default:
    return 0;
  }
  return txt;
}
