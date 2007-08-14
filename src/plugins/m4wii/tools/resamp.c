// resamp by MAKK
// for the M4 project
// input: .raw file any size
// output: raw file 2048 words + .h file
// the programm uses sinc curves to resample the input-raw-file
// to a 2048 words raw file in a very good quality.
// takes a while.........
// compile with djgpp


#define NUMSTEPS 32
#define DISTSTRENGTH 1+0.15


#include <dpmi.h>
#include <sys/nearptr.h>
#include <stdio.h>
#include <math.h>

int buf[2048];
short *orig;

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
             y = buf[(int)(x*6.4)&2047]/500+100;
             screen[y*320+x] = 40;
        }
}

void sinc(void)
{
  int i, j;
  double o, x;
  for( i=0; i<2048; i++) {
       x = i/2048.0*groesseIP;
       o = 0.0;
       for( j=0; j<groesseIP; j++) {
            if( j==x)
                 o += orig[j];
            else
                 o += sin(PI*(j-x)) / (PI*(j-x))  *orig[j];
       }
       buf[i] = (int) o;
  }
}

void normalize( void)
{
  int i;
  int maxamp = 0;
  for( i=0; i<2048; i++)
       if( abs(buf[i]) > maxamp)
           maxamp = abs(buf[i]);
  for( i=0; i<2048; i++)
       buf[i] = (double)buf[i]/maxamp*32767;
}

main( int argc, char **argv)
{
        int i, j;

        if( argc != 4) {
                printf( "usage: resamp <in.raw> <out.raw> <out.h>\n");
                exit( 0);
        }

        if( ( ip = fopen( argv[1], "rb")) == NULL) {
                printf( "usage: resamp <in.raw> <out.raw> <out.h>\n");
                exit( 0);
        }

        if( ( op = fopen( argv[2], "wb")) == NULL) {
                printf( "usage: resamp <in.raw> <out.raw> <out.h>\n");
                exit( 0);
        }

        if( ( op2 = fopen( argv[3], "w")) == NULL) {
                printf( "usage: resamp <in.raw> <out.raw> <out.h>\n");
                exit( 0);
        }


        fseek( ip, 0, SEEK_END);
        groesseIP = ftell( ip)/2;
        fseek( ip, 0, SEEK_SET);

        orig = (short*)malloc( groesseIP * sizeof( short));
        fread( orig, 2, groesseIP, ip);
        printf( "\n%i words read\n", groesseIP);
        sinc();

        normalize();

        init320x200();
        paint();
        getch();
        initText();

        free( orig);

        for( i=0; i<2048; i++) {
             if( i%20 == 0)
                 fprintf( op2, "\n");
             fprintf( op2, "%i, ", buf[i]);
             putc( buf[i], op);
             putc( buf[i]>>8, op);
        }

        fclose( ip);
        fclose( op);
        fclose( op2);
}
