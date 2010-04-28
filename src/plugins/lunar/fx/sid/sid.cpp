#include "sid.h"
#include <lunar/fx.hpp>
#include <lunar/dsp.h>
#include "sidemu.h"

#define PALFRAMERATE 50
#define PALCLOCKRATE 985248
#define NTSCFRAMERATE 60
#define NTSCCLOCKRATE 1022727

unsigned char freqtbllo[] = {
  0x17,0x27,0x39,0x4b,0x5f,0x74,0x8a,0xa1,0xba,0xd4,0xf0,0x0e,
  0x2d,0x4e,0x71,0x96,0xbe,0xe8,0x14,0x43,0x74,0xa9,0xe1,0x1c,
  0x5a,0x9c,0xe2,0x2d,0x7c,0xcf,0x28,0x85,0xe8,0x52,0xc1,0x37,
  0xb4,0x39,0xc5,0x5a,0xf7,0x9e,0x4f,0x0a,0xd1,0xa3,0x82,0x6e,
  0x68,0x71,0x8a,0xb3,0xee,0x3c,0x9e,0x15,0xa2,0x46,0x04,0xdc,
  0xd0,0xe2,0x14,0x67,0xdd,0x79,0x3c,0x29,0x44,0x8d,0x08,0xb8,
  0xa1,0xc5,0x28,0xcd,0xba,0xf1,0x78,0x53,0x87,0x1a,0x10,0x71,
  0x42,0x89,0x4f,0x9b,0x74,0xe2,0xf0,0xa6,0x0e,0x33,0x20,0xff,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

unsigned char freqtblhi[] = {
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x02,
  0x02,0x02,0x02,0x02,0x02,0x02,0x03,0x03,0x03,0x03,0x03,0x04,
  0x04,0x04,0x04,0x05,0x05,0x05,0x06,0x06,0x06,0x07,0x07,0x08,
  0x08,0x09,0x09,0x0a,0x0a,0x0b,0x0c,0x0d,0x0d,0x0e,0x0f,0x10,
  0x11,0x12,0x13,0x14,0x15,0x17,0x18,0x1a,0x1b,0x1d,0x1f,0x20,
  0x22,0x24,0x27,0x29,0x2b,0x2e,0x31,0x34,0x37,0x3a,0x3e,0x41,
  0x45,0x49,0x4e,0x52,0x57,0x5c,0x62,0x68,0x6e,0x75,0x7c,0x83,
  0x8b,0x93,0x9c,0xa5,0xaf,0xb9,0xc4,0xd0,0xdd,0xea,0xf8,0xff,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

// adapted from GoatTracker
unsigned char sidorder[] = { 0x15,0x16,0x18,0x17,
0x05,0x06,0x02,0x03,0x00,0x01,0x04,
0x0c,0x0d,0x09,0x0a,0x07,0x08,0x0b,
0x13,0x14,0x10,0x11,0x0e,0x0f,0x12
};

struct voice {
    int wave;
    int freq, note;
    int attack, decay, sustain, release;
    int on;
    int filter;
    int ringmod, sync;
};

class sid : public lunar::fx<sid> {
public:
    int clockrate;
    SID emu;
    float samplerate;

    int cycles;
    unsigned char regs[29];

    // states:
    voice voices[3];
    int volume, resonance, mode;

    bool flush_regs;

    sid() {
    }

    void init() {
        clockrate = PALCLOCKRATE;
        samplerate = transport->samples_per_second;
        emu.set_sampling_parameters(clockrate, SAMPLE_FAST, samplerate);
        //emu.reset();
        //emu.set_chip_model(MOS8580);
        emu.set_chip_model(MOS6581);

        // reset states
        memset(voices, 0, sizeof(voices));
        memset(regs, 0, sizeof(regs));
        resonance = 0;
        mode = 0;
        volume = 0;
        cycles = 0;
        flush_regs = false;
    }

    void exit() {
        delete this;
    }

    void sid_write(int reg, int value) {
        //emu.clock(9);
        printf("reg %x: %x\n", reg, value);
        emu.write(reg, value);
        cycles += 9;
    }

    void process_events() {

        bool reg18_changed = false; // mode/vol
        bool reg17_changed = false;

        if (globals->volume) {
            volume = *globals->volume;
            reg18_changed = true;
        }
        if (globals->cutoff) {
            int cutoff = *globals->cutoff;
            unsigned char fclo = cutoff & 0xFF;
            unsigned char fchi = cutoff >> 8;
            regs[0x15] = fclo;
            regs[0x16] = fchi;
//          sid_write(0x15, fclo);
//          sid_write(0x16, fchi);
        }
        if (globals->resonance) {
            resonance = *globals->resonance;
            reg17_changed = true;
        }
        /*if (globals->filter) {
            filter = 1 << ((int)*globals->filter);
            reg17_changed = true;
        }*/
        if (globals->mode) {
            mode = 1 << ((int)*globals->mode);
            reg18_changed = true;
        }

        int filters = 0;
        for (int t = 0; t < track_count; t++) {
            bool reg00_changed = false; // reg 00 and 01: freq
            bool reg02_changed = false; // sustain+release
            bool reg04_changed = false; // wave + flags
            bool reg05_changed = false; // attack+decay
            bool reg06_changed = false; // attack+decay

            if (tracks[t].wave) {
                voices[t].wave = 1 << ((int)*tracks[t].wave);
                reg04_changed = true;
            }

            if (tracks[t].pw) {
                int pw = *tracks[t].pw;
                unsigned char pwlo = pw & 0xFF;
                unsigned char pwhi = pw >> 8;
                regs[0x02 + t*7] = pwlo;
                regs[0x03 + t*7] = pwhi;
                //sid_write(0x02 + t*7, pwlo);
                //sid_write(0x03 + t*7, pwhi);
            }

            if (tracks[t].filtervoice) {
                voices[t].filter = *tracks[t].filtervoice;
                reg17_changed = true;
            }

            bool no_new_note = false;
            if (tracks[t].effect) {
                int eff = *tracks[t].effect;
                if (eff == 0x30)
                    no_new_note = true;
            }

            if (voices[t].filter)
                filters |= (1 << t);


            if (tracks[t].note) {

                voices[t].note = *tracks[t].note;

                if (*tracks[t].note == 0) {
                    // stop voice
                    voices[t].on = false;
                    reg04_changed = true;       // disable voice
                } else {
                    // set up note
                    float tnote = *tracks[t].note;
                    float Fout = 440.0f * pow(2.0f, (tnote - 69.0f) / 12.0f) * samplerate / samplerate;

                    voices[t].freq = Fout;//Fout / 0.058725357f;    // Fout = (Fn * 0.058725357) Hz for PAL

                    reg18_changed = true;       // set new volume with note
                    //sid_write(0x18, 15);  // max volume
                    //sid_write(0x05 + t*7, 0xCC);  // attack/&decay
                    //sid_write(0x06 + t*7, 0xCC);  // sustain/release

                    //sid_write(0x02 + t*7, 0x2C);  // pw lo
                    //sid_write(0x03 + t*7, 0x0A);  // pw hi

                    voices[t].on = true;
                    reg00_changed = true;       // new freq
                    reg04_changed = true;       // enable voice
                }
            }

            if (tracks[t].ringmod) {
                voices[t].ringmod = *tracks[t].ringmod;
                reg04_changed;
            }
            if (tracks[t].sync) {
                voices[t].sync = *tracks[t].sync;
                reg04_changed;
            }
            if (tracks[t].attack) {
                voices[t].attack = *tracks[t].attack;
                reg05_changed = true;
            }
            if (tracks[t].decay) {
                voices[t].decay = *tracks[t].decay;
                reg05_changed = true;
            }
            if (tracks[t].sustain) {
                voices[t].sustain = *tracks[t].sustain;
                reg06_changed = true;
            }
            if (tracks[t].release) {
                voices[t].release = *tracks[t].release;
                reg06_changed = true;
            }

            // write out changed registers for this voice:
            if (reg00_changed) {
                // FREQUENCY = (REGISTER VALUE * CLOCK)/16777216 Hz
                int value = voices[t].freq * 16777216.0f / clockrate;
                unsigned char freqhi = value >> 8;
                unsigned char freqlo = value & 0xFF;

                int tlo = freqtbllo[voices[t].note];
                int thi = freqtblhi[voices[t].note];

                //printf("calculated freqs: %x %x; table freqs: %x, %x", freqhi, freqlo, thi, tlo);
                regs[0x00 + t*7] = tlo;
                regs[0x01 + t*7] = thi;
                //sid_write(0x00 + t*7, tlo);
                //sid_write(0x01 + t*7, thi);
                //sid_write(0x00 + t*7, freqlo);
                //sid_write(0x01 + t*7, freqhi);
            }
            if (reg05_changed) {
                unsigned char attackdecay = (voices[t].attack << 4) | voices[t].decay;
                regs[0x05 + t*7] = attackdecay;
                //sid_write(0x05 + t*7, attackdecay);
            }
            if (reg06_changed) {
                unsigned char sustainrelease = (voices[t].sustain << 4) | voices[t].release;
                regs[0x06 + t*7] = sustainrelease;
                //sid_write(0x06 + t*7, sustainrelease);
            }
            // reg04 = wave + enable, do this last
            if (reg04_changed) {
                unsigned char ctrl =
                    (voices[t].wave << 4) |
                    (voices[t].ringmod << 2) |
                    (voices[t].sync << 1) |
                    (voices[t].on);
                regs[0x04 + t*7] = ctrl;
                //sid_write(0x04 + t*7, ctrl);
            }
        }

        // write out global registers
        if (reg17_changed) {
            unsigned char resfilt = (resonance << 4) | filters;
            regs[0x17] = resfilt;
            //sid_write(0x17,  resfilt);
        }
        if (reg18_changed) {
            unsigned char modevol = (mode << 4) | volume;
            regs[0x18] = modevol;
            //sid_write(0x18,  modevol);
        }

        flush_regs = true;

    }

    // adapted from GoatTracker

#define NUMSIDREGS 0x19
#define SIDWRITEDELAY 9 // lda $xxxx,x 4 cycles, sta $d400,x 5 cycles
#define SIDWAVEDELAY 4 // and $xxxx,x 4 cycles extra

    unsigned char sid_getorder(unsigned char index)
    {
      //if (adparam >= 0xf000)
        //return altsidorder[index];
      //else
        return sidorder[index];
    }

    void process_stereo_goat(float *inL, float *inR, float *outL, float *outR, int n)
    //int sid_fillbuffer(short *ptr, int samples)
    {
        short buf[512];
        short* ptr = buf;
        int samples = n;

        int tdelta;
        int tdelta2;
        int result;
        int total = 0;
        int c;

        int badline = 0;//rand() % NUMSIDREGS;

        tdelta = clockrate * samples / samplerate + 4;

        for (c = 0; c < NUMSIDREGS; c++)
        {
            unsigned char o = sid_getorder(c);

            // Extra delay for loading the waveform (and mt_chngate,x)
            if ((o == 4) || (o == 11) || (o == 18))
            {
                tdelta2 = SIDWAVEDELAY;
                result = emu.clock(tdelta2, ptr, samples);
                total += result;
                ptr += result;
                samples -= result;
                tdelta -= SIDWAVEDELAY;
            }

            // Possible random badline delay once per writing
            /*if ((badline == c) && (residdelay))
            {
                tdelta2 = residdelay;
                result = sid->clock(tdelta2, ptr, samples);
                total += result;
                ptr += result;
                samples -= result;
                tdelta -= residdelay;
            }*/

            emu.write(o, regs[o]);

            tdelta2 = SIDWRITEDELAY;
            result = emu.clock(tdelta2, ptr, samples);
            total += result;
            ptr += result;
            samples -= result;
            tdelta -= SIDWRITEDELAY;
        }
        result = emu.clock(tdelta, ptr, samples);
        total += result;

        for (int i = 0; i < n; i++) {
            short s = buf[i];//emu.output();
            float fs = (float)s / 32767.0f;
            outL[i] = fs;
            outR[i] = fs;
        }
    }


    void process_stereo(float *inL, float *inR, float *outL, float *outR, int n) {

        if (flush_regs) {
            for (int i = 0; i < 29; i++) {
                emu.write(i, regs[i]);
                cycles += 9;
            }
            flush_regs = false;
        }

        cycles = 0;

        short buf[512];
        //printf("tdelta %i, n %i\n", tdelta, n);
        int samples = n;
        while (samples > 0) {
            int tdelta = clockrate * samples / 44100 + 4;// - cycles;
            int result = emu.clock(tdelta, buf, n); // en c64 går i hva, 1mhz?
            samples -= result;
            if (result < n)
                printf("result: %i, was %i\n", result, n);
        }

        for (int i = 0; i < n; i++) {
            short s = buf[i];//emu.output();
            float fs = (float)s / 32767.0f;
            outL[i] = fs;
            outR[i] = fs;
        }
    }
};

lunar_fx *new_fx() {
  return new sid();
}
