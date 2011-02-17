// raw2h by MAKK
// for the M4 project
// scans the current dir for .raw files and converts them to .h files
// the .raw files must be 2048 words
// compile with djgpp

#include <dos.h>
#include <stdio.h>

main()
{
    struct find_t f;
    int i, j, k;
    FILE *ip, *op;
    short buf[2048];

    i = 0;

     if ( !_dos_findfirst( "*.raw", 0, &f))
     {
        do
        {
            ip = fopen( f.name, "rb");
            strcpy( (char*)strchr( f.name, '.'), ".h");
            op = fopen( f.name, "w");
            fread( buf, 2, 2048, ip);
            for( j=0; j<2048; j++) {
                 if( j%20 == 0)
                     fprintf( op, "\n");
                 fprintf( op, "%i, ", buf[j]);
            }
            i++;
            fclose( ip);
            fclose( op);
        } while( !_dos_findnext(&f) );
     }


    printf( "\n%i files\n", i);
}
