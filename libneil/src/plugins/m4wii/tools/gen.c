// gen by MAKK
// for the M4 project
// intended to generate a complete wavetable with morphing waveforms
// used to generate single .raw files of sine, tri, saw, square (2048 words)
// compile with djgpp

#define NUMSTEPS 32
#define DISTSTRENGTH 1+0.15


#include <dpmi.h>
#include <sys/nearptr.h>
#include <stdio.h>
#include <math.h>

short buf[2048];
int NumWaves = 0;
FILE *op;

void init320x200(void)
{
        __dpmi_regs r;
        r.x.ax = 0x13;
        __dpmi_int( 0x10, &r);
}
void initText(void)
{
        __dpmi_regs r;
        r.x.ax = 3;
        __dpmi_int( 0x10, &r);
}

void paint(void)
{
        int x, y;
        unsigned char *screen = 0xa0000+__djgpp_conventional_base;
        // ZEICHNEN
        __djgpp_nearptr_enable();
        memset( screen, 0, 320*200);
        for( x=0; x<320; x++) {
             y = buf[(int)(x*6.4)&2047]/500+100;
             screen[y*320+x] = 40;
        }
}




double dist( double a, double b)
// fr distortion
{
        double sign;
        if( a==0)
            return 0;
        sign = a / fabs(a);
        return pow( fabs(a), 1/pow(DISTSTRENGTH,b))*sign;
}


double osc( int alpha, int waveform)
// wafeform = 0 : sin
//            1 : square
//            2 : saw
//            3 : triangle
// alpha 0..2047
// -1 <= amp <= 1
{
  switch( waveform) {
  case 0: // sin
       return sin( alpha/2048.0*2*PI);
  case 1: // square
       if( alpha < 2048/2)
           return 1;
       else
           return -1;
  case 2: // saw
       return (1024-alpha)/1024.0;
  case 3: // triangle
       alpha = (alpha+512)&2047;
       if( alpha<1024)
           return alpha/512.0 - 1;
       else
           return -(alpha-1024)/512.0 + 1;

  }
}

int pulsewidth( int phase, int pw)
// returns new Phase according to pulsewidth (pw)
// 0<= pw <= NUMSTEPS
{
        int center = 1024 + (pw*700.0/NUMSTEPS);
        if( phase < center)
            return phase*1024.0/center;
        else
            return (phase - center)*1024.0/(2048.0-center) + 1024;
}

double morph( double a, double b, int mix)
// 0<= mix < NUMSTEPS
// 0 ist a
// NUMSTEP-1 ist b
{
  return (a*(NUMSTEPS-1.0-mix) + b*mix)/(NUMSTEPS-1.0);
}

void generate( int PW, int DIST, int WF1, int WF2, int MORPH)
{
  int i;
  for( i=0; i<2048; i++)
       buf[i] = dist( morph( osc( pulsewidth(i,PW), WF1),
                             osc( pulsewidth(i,PW), WF2), MORPH), DIST)*32767;
  paint();
  getch();
  for( i=0; i<2048; i++) {
       fputc( buf[i], op);
       fputc( buf[i]>>8, op);
  }
  NumWaves++;
}

main( int argc, char **argv)
{
        int i, j;
        int step;

//        if( argc != 2) {
//                printf( "\n\nUSAGE: export <outfile.raw>\n");
//                exit( 0);
//        }
        if( ( op = fopen( "!!!gen.raw", "wb")) == NULL) {
                exit( 0);
        }

        init320x200();

        generate( 0, 0, 1, 0, 0);
        initText();

        fclose( op);
        printf( "%i waves generated", NumWaves);
}
