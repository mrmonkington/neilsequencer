class Cubic
{
public:
	Cubic();

	int Work(int yo, int y0, int y1, int y2,unsigned int res);

	// Work function. Where all is cooked :]
	// yo = y[-1] [sample at x-1]
	// y0 = y[0]  [sample at x (input)]
	// y1 = y[1]  [sample at x+1]
	// y2 = y[2]  [sample at x+2]
	
	// res= distance between two neighboughs sample points [y0 and y1] 
	//      ,so [0...1.0]. You have to multiply this distance * RESOLUTION used
	//      on the  spline conversion table. [256 by default]
    // If you are using 256 is asumed you are using 8 bit decimal
	// fixed point offsets for resampling.

	// offset = sample offset [info to avoid go out of bounds on sample reading ]
	// offset = sample length [info to avoid go out of bounds on sample reading ]

private:
	int RESOLUTION; // Currently is 256, that's enough...
    long at[4096];
	long bt[4096];
	long ct[4096];
	long dt[4096];
};
