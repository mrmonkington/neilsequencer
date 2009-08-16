#include "gAHDEnv.hpp"

void gAHDEnv::Init() {
  A = H = D = false;
  atk = dec = ddec = datk = 0.0f;
  hld = 1.0;
  dcs = atk + hld;
  atkEnd = decStart = decLen = 0;

}

void gAHDEnv::SetEnvParams(float a, float q) {
  datk = 1.0f / (a * q); //to remove divs 
  atk = a * q;
  dec = a * (2 - q);
  ddec = 1.0f / dec; //to remove divs 
  hld = 1.0f - atk - dec;
  dcs = atk + hld;
}

float gAHDEnv::Envelope2(float c, int skip) {
  if (c < atk) 
    return c * datk;
  if (c > dcs) 
    return (dec - (c - dcs)) * ddec;
  if (c > 1.0f) 
    return 0.0f;
  return 1.0f;	
}

float gAHDEnv::Envelope3(float c, int skip) {
  //lastValue
  float rtn = 1.0f;
  if (c <= atk){
    //A = true;
    rtn = c / atk;
  }
  else if (c >= dcs){
    //D=true
    rtn = (dec-(c-dcs))/dec;
  }
  else{
    //H = true;
    rtn = 1.0f;
  }
  return rtn;

}
