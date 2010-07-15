#include <cstdio>
#include <cmath>

#include "m3.hpp"

M3::M3() 
{
  global_values = &gval;
  track_values = &tval;
  attributes = 0;
}

M3::~M3() 
{

}

void M3::init(zzub::archive* pi) 
{

}

void M3::destroy() 
{

}

void M3::stop() {

}

void M3::set_track_count(int tracks) 
{

}

void M3::process_events() 
{

}

bool M3::process_stereo(float **pin, float **pout, int n, int mode) 
{
  return true;
}

const char *M3::describe_value(int param, int value) 
{
  return 0;
}



