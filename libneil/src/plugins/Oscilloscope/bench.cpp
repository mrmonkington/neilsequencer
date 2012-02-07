#include "Ring.hpp"

#include <cassert>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include <vector>
#include <deque>
#include <algorithm>
#include <numeric>
#include <functional>
#include <cmath>

using namespace std;

int main(int argc, char** argv) {
	// const clockid_t CLOCK_ID = CLOCK_THREAD_CPUTIME_ID;
	const clockid_t CLOCK_ID = CLOCK_REALTIME;
	// const clockid_t CLOCK_ID = CLOCK_MONOTONIC;
	timespec start, end;
	vector<long> dts;

	const int N = 1 << 10;
	const int J = 10000;



	printf("Deque\n");
	deque<float> dq;
	for(int j=0; j<J; j++) 
	{
		clock_gettime(CLOCK_ID, &start);

		assert(dq.size()==0);
		for(int i=0;i<N;i++)
			dq.push_back(1);

		assert(dq.size()==N);

		for(int i=0;i<N;i++)
			dq.pop_front();
		
		assert(dq.size()==0);
								
		clock_gettime(CLOCK_ID, &end);

		dts.push_back(end.tv_nsec - start.tv_nsec);
	}
	{
		printf("size: %d, iterations: %d\n", N, J);
		printf("max: %f\n", *max_element(dts.begin(), dts.end()) * J * 1e-9);
		printf("min: %f\n", *min_element(dts.begin(), dts.end()) * J * 1e-9);
		
		float mean = accumulate(dts.begin(), dts.end(), 0.0f) / dts.size();
	   	
	   	vector<long> zero_mean(dts);
	   	transform(zero_mean.begin(), zero_mean.end(), zero_mean.begin(), bind2nd(minus<float>(), mean) );
	  	// transform(zero_mean.begin(), zero_mean.end(), zero_mean.begin(), bind2nd(multiplies<float>(), 1e-9) );
	  	float deviation = inner_product(zero_mean.begin(), zero_mean.end(), zero_mean.begin(), 0.0f );
		deviation = sqrt( deviation / ( dts.size() - 1 ) );

		printf("mean: %f\n", mean * J * 1e-9);
		printf("std: %f\n", deviation * J * 1e-9);
	}


	dts.clear();
	usleep(.1e6);

	printf("Ring:\n");
	Ring<float, N> ring;


	// struct timeval tv;
	// gettimeofday(&tv, NULL);
	// long start = tv.tv_usec;

	// printf("readable: %d, writable: %d\n", ring.readable(), ring.writable());
	assert(ring.writable()==N);
	// printf("-------------------------------------------------------------------------\n");
	for(int j=0; j<J; j++) 
	{	
		clock_gettime(CLOCK_ID, &start);
		// printf("-- %d --\n", j);
		// printf("readable: %d, writable: %d\n", ring.readable(), ring.writable());
		assert(ring.readable()==0);
		
		for(int i=0;i<N;i++)
			ring.push(1);

		// printf("readable: %d, writable: %d\n", ring.readable(), ring.writable());
		assert(ring.readable()==N);

		
		for(int i=0;i<N;i++)
			ring.pop();

		// printf("readable: %d, writable: %d\n", ring.readable(), ring.writable());
		assert(ring.readable()==0);

		clock_gettime(CLOCK_ID, &end);

		dts.push_back(end.tv_nsec - start.tv_nsec);
	}
	// printf("-------------------------------------------------------------------------\n");
	// printf("readable: %d, writable: %d\n", ring.readable(), ring.writable());

	// gettimeofday(&tv, NULL);
	// printf("size: %d, time: %ld\n", N, tv.tv_usec - start);
	{
		printf("size: %d, iterations: %d\n", N, J);
		printf("max: %f\n", *max_element(dts.begin(), dts.end()) * J * 1e-9);
		printf("min: %f\n", *min_element(dts.begin(), dts.end()) * J * 1e-9);
		
		float mean = accumulate(dts.begin(), dts.end(), 0.0f) / dts.size();
	   	
	   	vector<long> zero_mean(dts);
	   	transform(zero_mean.begin(), zero_mean.end(), zero_mean.begin(), bind2nd(minus<float>(), mean) );
	  	// transform(zero_mean.begin(), zero_mean.end(), zero_mean.begin(), bind2nd(multiplies<float>(), 1e-9) );
	  	float deviation = inner_product(zero_mean.begin(), zero_mean.end(), zero_mean.begin(), 0.0f );
		deviation = sqrt( deviation / ( dts.size() - 1 ) );

		printf("mean: %f\n", mean * J * 1e-9);
		printf("std: %f\n", deviation * J * 1e-9);
	}
}