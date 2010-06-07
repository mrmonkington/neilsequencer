#ifndef LANTERNFISH_ADSR_HPP
#define LANTERNFISH_ADSR_HPP

#include <vector>

namespace lanternfish {
  class Adsr {
  public:
    enum EnvelopeStage {
      NONE_STAGE,
      ATTACK_STAGE,
      DECAY_STAGE,
      SUSTAIN_STAGE,
      RELEASE_STAGE
    };
  private:
    int attack_time, decay_time, release_time;
    float attack_delta, decay_delta, release_delta;
    float sustain_level, power, value;
    EnvelopeStage current_stage;
    int buff_size;
  public:
    Adsr();
    ~Adsr();
    void set_attack_time(int samples);
    void set_decay_time(int samples);
    void set_sustain_level(float level);
    void set_release_time(int samples);
    void set_power(float power);
    void note_on();
    void note_off();
    void process(float *out, int n);
  };
}

#endif // LANTERNFISH_ADSR_HPP
