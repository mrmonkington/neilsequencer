/*
Copyright (C) 2007 Marcin Dabrowski

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
// Marcin Dabrowski
// bigyo@wp.pl
// www.patafonia.co.nr

#ifndef __LINLOG_H
#define __LINLOG_H
///////////////////////////////////////////////////////////////////////////

inline double linlog(double value, double min, double max, double slope)
{
	double delta_x, t, A, env;

	if (max>min)
	{
		delta_x = max - min;
		t = (value - min) / delta_x;
		t = 1- t ;
	}
	else
	{
		delta_x = min - max;
		t = (value - max) / delta_x;
	}

	if (slope <= 0.5)
	{
		A = 0.25/(slope*slope);
		env = (1-t) * pow(A,-t);
	}
	else // slope = (0.5 .. 1)
	{
		A = 0.25/((1-slope)*(1-slope));
		env = 1-t*pow(A,(t-1));
	}

	return env * (delta_x) + min;
}
///////////////////////////////////////////////////////////////////////////
#endif
