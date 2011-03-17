#include <cstdio>
#include <cmath>

#include "Epiano.hpp"

LunarEpiano::LunarEpiano() 
{
  global_values = &gval;
  track_values = &tval;
  attributes = 0;
}

LunarEpiano::~LunarEpiano() 
{

}

void LunarEpiano::init(zzub::archive* pi) 
{
 
}

void LunarEpiano::destroy() 
{

}

void LunarEpiano::stop() {

}

void LunarEpiano::set_track_count(int tracks) 
{

}

void LunarEpiano::process_events() 
{
 
}

bool LunarEpiano::process_stereo(float **pin, float **pout, int n, int mode) 
{
  return true;
}

const char *LunarEpiano::describe_value(int param, int value) 
{
  return 0;
}



