#include <cstdio>
#include <cmath>

#include "dt_blockfx.hpp"

DtBlockFX::DtBlockFX() 
{
  global_values = &gv;
  track_values = &tv;
  attributes = 0;
}

DtBlockFX::~DtBlockFX() 
{

}

void DtBlockFX::init(zzub::archive* pi) 
{

}

void DtBlockFX::destroy() 
{

}

void DtBlockFX::stop() {

}

void DtBlockFX::process_events() 
{

}

bool DtBlockFX::process_stereo(float **pin, float **pout, int n, int mode) 
{
  return true;
}

const char *DtBlockFX::describe_value(int param, int value) 
{

}



