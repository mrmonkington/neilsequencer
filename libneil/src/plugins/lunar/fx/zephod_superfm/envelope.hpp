#ifndef ENVELOPE_H
#define ENVELOPE_H

#define ENV_ATT 1
#define ENV_DEC 2
#define ENV_SUS 3
#define ENV_REL 4
#define ENV_NONE 99
#define ENV_CLICK 5

class envelope {
public:
  envelope();
  virtual ~envelope();
  float res(void);
  void reset();
  void attack(int newv);
  void decay(int newv);
  void sustain(int newv);
  void sustainv(float newv);
  void release(int newv);
  void stop();
  void noteoff();
  int a,d,s,r;
  int envstate;
  float envvol;
  float susvol;
  float envcoef;
  int suscounter;
};

#endif // ENVELOPE_H

