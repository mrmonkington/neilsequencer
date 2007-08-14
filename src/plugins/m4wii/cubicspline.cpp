// CUBIC SPLINE FUNCTION
//
// Arguru
//
#include "cubic.h"

Cubic::Cubic() // The resampler constructor 
{
RESOLUTION=4096;
// Initialize table...

  for (int i=0;i<RESOLUTION;i++)
  {
    float x = (float)i/(float)RESOLUTION;
    at[i] = (long)((-0.5*x*x*x+x*x-0.5*x)*(float)0xffffff);
    bt[i] = (long)((1.5*x*x*x-2.5*x*x+1)*(float)0xffffff);
    ct[i] = (long)((-1.5*x*x*x+2*x*x+0.5)*(float)0xffffff);
    dt[i] = (long)((0.5*x*x*x-0.5*x*x)*(float)0xffffff);
  }

}

// Work body		
int Cubic::Work(int yo, int y0, int y1, int y2,unsigned int res)
{
	//res=res>>22;
	//if(offset==0)yo=0;
	//if(offset+2>length)y1=0;
	//if(offset+3>length)y2=0;

	//return at[res]*yo+bt[res]*y0+ct[res]*y1+dt[res]*y2;
	//return ((((__int64)at[res]*yo) + ((__int64)bt[res]*y0) + ((__int64)ct[res]*y1) + ((__int64)dt[res]*y2))>>24);
	float f = (float)res/4096.0;
	return y0 + (int) ((float)(y1 - y0)*f);
				
}

// Despiste this CubicSpline function, only for debuggin purposes, not optimized
float CubicSpline(float oy1,float y0,float y1, float y2, float x)
{
float a=(3*(y0-y1)-oy1+y2)*0.5;
float b=2*y1+oy1-(5*y0+y2)*0.5;
float c=(y1-oy1)*0.5;
return a*x*x*x+b*x*x+c*x+y0;
}


/*
Cubic::Cubic() 
{
RESOLUTION=4096;
// Initialize table...

  for (int i=0;i<RESOLUTION;i++)
  {
    float x = (float)i/(float)RESOLUTION;
    at[i] = ((-0.5*x*x*x+x*x-0.5*x));
    bt[i] = ((1.5*x*x*x-2.5*x*x+1));
    ct[i] = ((-1.5*x*x*x+2*x*x+0.5));
    dt[i] = ((0.5*x*x*x-0.5*x*x));
  }

}

// Work body		
float Cubic::Work(float yo, float y0, float y1, float y2,unsigned __int32 res)
{
	//res=res>>22;
	//if(offset==0)yo=0;
	//if(offset+2>length)y1=0;
	//if(offset+3>length)y2=0;

	//return at[res]*yo+bt[res]*y0+ct[res]*y1+dt[res]*y2;
	return ((at[res]*yo) + (bt[res]*y0) + (ct[res]*y1) + (dt[res]*y2));
				
}
*/
