#ifndef LANTERNFISH_GRAINENV_HPP
#define LANTERNFISH_GRAINENV_HPP

namespace lanternfish {
  class GrainEnv {
  public:
    enum EnvStage {
      ATTACK_STAGE,
      SUSTAIN_STAGE,
      RELEASE_STAGE,
      NONE_STAGE
    };
  private:
    static const int table_size = 512;
    float attack_table[table_size], release_table[table_size];
    EnvStage stage;
    int attack_length, sustain_length, release_length, counter;
  public:
    GrainEnv();
    void set_attack_length(int samples);
    void set_sustain_length(int samples);
    void set_release_length(int samples);
    void trigger();
    void process(float *out, int n);
    bool is_off();
  };
}

#endif // LANTERNFISH_GRAINENV_HPP
