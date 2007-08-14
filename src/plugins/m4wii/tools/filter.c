// simple lowpass filter by MAKK
// coded for the M4 project
// to avoid too high frequencies and 'unsmooth' loops
// input: 2048 words .raw file
// output: filtered 2048 words .raw file and .h file
// compile with djgpp

#define FILTERGRADE 20

#include <dpmi.h>
#include <sys/nearptr.h>
#include <stdio.h>
#include <math.h>

short buf[2048];
int buf2[2048];

FILE *ip, *op, *op2;
int groesseIP;

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
             y = buf2[(int)(x*6.4)&2047]/500+100;
             screen[y*320+x] = 40;
        }
}


void normalize( void)
{
  int i;
  int maxamp = 0;
  for( i=0; i<2048; i++)
       if( abs(buf2[i]) > maxamp)
           maxamp = abs(buf2[i]);
  for( i=0; i<2048; i++)
       buf2[i] = (double)buf2[i]/maxamp*32767;
}

void filter( void)
{
  int i, j, t;
  for( i=0; i<2048; i++) {
       t = 0;
       for( j=0; j<FILTERGRADE; j++)
            t += buf[(i+j) & 2047];
       buf2[(i+FILTERGRADE/2) & 2047] = t / FILTERGRADE;
  }
}

main( int argc, char **argv)
{
        int i, j;

        if( ( argc != 4) ||
            ( ( ip = fopen( argv[1], "rb")) == NULL) ||
            ( ( op = fopen( argv[2], "wb")) == NULL) ||
            ( ( op2 = fopen( argv[3], "w")) == NULL)) {
                printf( "usage: filter <in.raw> <out.raw> <out.h>\n");
                exit( 0);
        }

        fseek( ip, 0, SEEK_END);
        groesseIP = (ftell( ip))/2;
        fseek( ip, 0, SEEK_SET);
        if( groesseIP != 2048) {
                printf( "<in.raw> must be 2048 words\n");
                exit( 0);
        }

        fread( buf, 2, 2048, ip);

        filter();
        normalize();

        init320x200();
        paint();
        getch();
        initText();

        for( i=0; i<2048; i++) {
             if( i%20 == 0)
                 fprintf( op2, "\n");
             fprintf( op2, "%i, ", buf2[i]);
             putc( buf2[i], op);
             putc( buf2[i]>>8, op);
        }

        fclose( ip);
        fclose( op);
        fclose( op2);
}
