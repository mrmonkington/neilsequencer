#ifndef UTILS_HPP
#define UTILS_HPP

static gchar* format_value_callback_fstr (GtkScale *scale, gdouble value, gpointer fmt)
{
  return g_strdup_printf ((const char*)fmt, gtk_scale_get_digits (scale), value);
}


// http://www.music.mcgill.ca/~hockman/projects/beat-to-the-run/code/waon-0.9
/* return power (amp2) of the complex number (freq(k),freq(len-k));
 * where (real,imag) = (cos(angle), sin(angle)).
 * INPUT
 *  len        :
 *  freq [len] :
 *  scale      : scale factor for amp2[]
 * OUTPUT
 *  amp2 [len/2+1] := (real^2 + imag^2) / scale
 */
void HC_to_amp2 (long len, const float * freq, float scale, float * amp2)
{
  int i;
  float rl, im;

  amp2 [0] = freq [0] * freq [0] / scale;
  for (i = 1; i < (len+1)/2; i ++)
    {
      rl = freq [i];
      im = freq [len - i];
      amp2 [i] = (rl * rl + im * im)  / scale;
    }
  if (len%2 == 0)
    {
      amp2 [len/2] = freq [len/2] * freq [len/2] / scale;
    }
}

float
hanning (int i, int nn)
{
  return ( 0.5 * (1.0 - cos (2.0*M_PI*(float)i/(float)(nn-1))) );
}

/* Reference: "Digital Filters and Signal Processing" 2nd Ed.
 * by L. B. Jackson. (1989) Kluwer Academic Publishers.
 * ISBN 0-89838-276-9
 * Sec.7.3 - Windows in Spectrum Analysis
 */
float
hamming (int i, int nn)
{
  return ( 0.54 - 0.46 * cos (2.0*M_PI*(float)i/(float)(nn-1)) );
}

float
blackman (int i, int nn)
{
  return ( 0.42 - 0.5 * cos (2.0*M_PI*(float)i/(float)(nn-1))
	  + 0.08 * cos (4.0*M_PI*(float)i/(float)(nn-1)) );
}

#endif